#include "mpki_drrip.h"

#include <algorithm>
#include <cassert>
#include <random>
#include <utility>

#include "champsim.h"
#include "instruction_counter.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>

mpki_drrip::mpki_drrip(CACHE* cache)
    : replacement(cache), NUM_SET(cache->NUM_SET), NUM_WAY(cache->NUM_WAY), rrpv(static_cast<std::size_t>(NUM_SET * NUM_WAY)),
      PSEL(NUM_CPUS, champsim::msl::dscounter<long, PSEL_WIDTH>(champsim::msl::get_sample_rate(NUM_SET)))
{

  if (auto v = std::getenv("MPKI_REL"))  rel_thresh = std::atof(v);
if (auto v = std::getenv("MPKI_NEED")) consec_needed = std::atoi(v);
if (auto v = std::getenv("MPKI_STEP")) psel_step = std::atol(v);
}

unsigned& mpki_drrip::get_rrpv(long set, long way) { return rrpv.at(static_cast<std::size_t>(set * NUM_WAY + way)); }

void mpki_drrip::update_brrip(long set, long way)
{
  get_rrpv(set, way) = maxRRPV;

  brrip_counter++;
  if (brrip_counter == BRRIP_MAX) {
    brrip_counter = 0;
    get_rrpv(set, way) = maxRRPV - 1;
  }
}

void mpki_drrip::update_srrip(long set, long way) { get_rrpv(set, way) = maxRRPV - 1; }

// MPKI-change detector: runs once per EPOCH_INSTR retired instructions (per CPU)
// void mpki_drrip::reset_psel(uint32_t cpu) { PSEL[cpu].reset_to(PSEL[cpu].mid() - 1, PSEL_STEP); }
void mpki_drrip::reset_psel(uint32_t cpu) { PSEL[cpu].reset_to(PSEL[cpu].mid() - 1, psel_step); }

void mpki_drrip::check_epoch(uint32_t cpu)
{
  // auto& p = ph[cpu];
  // uint64_t now = champsim::retired_instr[cpu];
  // if (now < p.last_instr) { p.last_instr = now; p.misses = 0; return; } // counter was reset
  // uint64_t d = now - p.last_instr;
  // if (d < EPOCH_INSTR)
  //   return;

  // double mpki = 1000.0 * static_cast<double>(p.misses) / static_cast<double>(d);
  // p.mpki_trace.emplace_back(p.epochs + 1, mpki);
  // if (p.avg < 0)
  //   p.avg = mpki;

  // bool changed = std::fabs(mpki - p.avg) > std::max(MIN_ABS_MPKI, REL_THRESH * p.avg);
  // p.consec = changed ? p.consec + 1 : 0;

  // bool fired = false;
  // if (p.cooldown > 0) {
  //   p.cooldown--;
  // } else if (p.consec >= CONSEC_NEEDED) {
  //   reset_psel(cpu);
  //   p.cooldown = COOLDOWN_EPOCHS;
  //   p.consec = 0;
  //   p.changes++;
  //   fired = true;
  // }

  // if (fired)
  //   p.avg = mpki;                          // new regime becomes the baseline
  // else if (!changed || p.cooldown > 0)
  //   p.avg += 0.25 * (mpki - p.avg);        // don't let a pending change drag the baseline

  // p.misses = 0;
  // p.last_instr = now;
  // p.epochs++;


    auto& p = ph[cpu];
  uint64_t now = champsim::retired_instr[cpu];
  if (now < p.last_instr) { p.last_instr = now; p.misses = 0; return; } // counter was reset
  uint64_t d = now - p.last_instr;
  if (d < EPOCH_INSTR)
    return;

  double mpki = 1000.0 * static_cast<double>(p.misses) / static_cast<double>(d);
  p.mpki_trace.emplace_back(p.epochs + 1, mpki);
  if (p.avg < 0)
    p.avg = mpki;

  bool changed = std::fabs(mpki - p.avg) > std::max(MIN_ABS_MPKI, rel_thresh * p.avg);
  p.consec = changed ? p.consec + 1 : 0;

  bool fired = false;
  if (p.cooldown > 0) {
    p.cooldown--;
  } else if (p.consec >= consec_needed) {
    reset_psel(cpu);
    p.cooldown = COOLDOWN_EPOCHS;
    p.consec = 0;
    p.changes++;
    fired = true;
  }

  if (fired)
    p.avg = mpki;                          // new regime becomes the baseline
  else if (!changed || p.cooldown > 0)
    p.avg += 0.25 * (mpki - p.avg);        // don't let a pending change drag the baseline

  p.misses = 0;
  p.last_instr = now;
  p.epochs++;
}

// called on every cache hit and cache fill
void mpki_drrip::update_replacement_state(uint32_t triggering_cpu, long set, long way, champsim::address full_addr, champsim::address ip,
                                          champsim::address victim_addr, access_type type, uint8_t hit)
{
  if (hit) {
    if (access_type{type} == access_type::WRITE) {
      get_rrpv(set, way) = maxRRPV - 1;
      return;
    }
    get_rrpv(set, way) = 0;
  }
}

void mpki_drrip::replacement_cache_fill(uint32_t triggering_cpu, long set, long way, champsim::address full_addr, champsim::address ip,
                                        champsim::address victim_addr, access_type type)
{
  if (access_type{type} == access_type::WRITE) {
    get_rrpv(set, way) = maxRRPV - 1;
    return;
  }

  check_epoch(triggering_cpu); // close the previous epoch first
  if (type == access_type::LOAD || type == access_type::RFO)
    ph[triggering_cpu].misses++; // demand misses only

  // plain DRRIP decision, no override
  if (PSEL[triggering_cpu].decide(set))
    update_brrip(set, way);
  else
    update_srrip(set, way);

  PSEL[triggering_cpu].update_bad(set);
}

long mpki_drrip::find_victim(uint32_t triggering_cpu, uint64_t instr_id, long set, const champsim::cache_block* current_set, champsim::address ip,
                             champsim::address full_addr, access_type type)
{
  auto begin = std::next(std::begin(rrpv), set * NUM_WAY);
  auto end = std::next(begin, NUM_WAY);

  auto victim = std::max_element(begin, end);
  if (auto rrpv_update = maxRRPV - *victim; rrpv_update != 0)
    for (auto it = begin; it != end; ++it)
      *it += rrpv_update;

  assert(begin <= victim);
  assert(victim < end);
  return std::distance(begin, victim);
}

void mpki_drrip::replacement_final_stats()
{
  for (std::size_t c = 0; c < ph.size(); ++c)
    if (ph[c].epochs)
      std::printf("CPU%zu MPKI phase changes: %lu / %lu epochs (final avg MPKI %.2f)\n", c, ph[c].changes, ph[c].epochs, ph[c].avg);
}