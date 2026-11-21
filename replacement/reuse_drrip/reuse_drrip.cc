#include "reuse_drrip.h"

#include <algorithm>
#include <cassert>
#include <random>
#include <utility>

#include "champsim.h"

//ADDED FOR REUSE DRRIP
#include <cstdio>
#include "instruction_counter.h"
#include <cstdint>

reuse_drrip::reuse_drrip(CACHE* cache)
    : replacement(cache), NUM_SET(cache->NUM_SET), NUM_WAY(cache->NUM_WAY), rrpv(static_cast<std::size_t>(NUM_SET * NUM_WAY)),
      PSEL(NUM_CPUS, champsim::msl::dscounter<long, PSEL_WIDTH>(champsim::msl::get_sample_rate(NUM_SET))),
      state(static_cast<std::size_t>(NUM_SET * NUM_WAY), 0),
      cat_sampler(champsim::msl::get_sample_rate(NUM_SET))
{
}

//ADDED FOR REUSE DRRIP
uint8_t& reuse_drrip::get_state(long set, long way) { return state.at(static_cast<std::size_t>(set * NUM_WAY + way)); }

// category 1 = SRRIP leader sets
bool reuse_drrip::in_measure_set(long set) const { return cat_sampler.get_sample_category(set) == 1; }

// followers choose BRRIP when PSEL >= mid, so BRRIP is the high side
void reuse_drrip::bias_psel(uint32_t cpu, bool toward_brrip)
{
  constexpr long OFFSET = 1L << (PSEL_WIDTH - 3);
  PSEL[cpu].reset_to(PSEL[cpu].mid() + (toward_brrip ? OFFSET : -OFFSET), PSEL_STEP);
}

void reuse_drrip::check_epoch(uint32_t cpu)
{
  auto& p = ph[cpu];
  uint64_t now = champsim::retired_instr[cpu];
  if (now < p.last_instr) { p.last_instr = now; return; }   // counter was reset
  if (now - p.last_instr < EPOCH_INSTR) return;             // epoch not over yet
  p.last_instr = now;
  p.epochs++;
  if (p.evictions < MIN_EVICTIONS) return;                  // too few samples, keep accumulating

  double r = static_cast<double>(p.dead) / static_cast<double>(p.evictions);
  p.avg_r = (p.avg_r < 0) ? r : p.avg_r + EMA_ALPHA * (r - p.avg_r);

  if (p.avg_r > HI_THRESH)      { bias_psel(cpu, true);  p.forced_brrip++; }
  else if (p.avg_r < LO_THRESH) { bias_psel(cpu, false); p.forced_srrip++; }

  p.evictions = p.dead = 0;
}

unsigned& reuse_drrip::get_rrpv(long set, long way) { return rrpv.at(static_cast<std::size_t>(set * NUM_WAY + way)); }

void reuse_drrip::update_brrip(long set, long way)
{
  get_rrpv(set, way) = maxRRPV;

  brrip_counter++;
  if (brrip_counter == BRRIP_MAX) {
    brrip_counter = 0;
    get_rrpv(set, way) = maxRRPV - 1;
  }
}

void reuse_drrip::update_srrip(long set, long way) { get_rrpv(set, way) = maxRRPV - 1; }

// called on every cache hit and cache fill
void reuse_drrip::update_replacement_state(uint32_t triggering_cpu, long set, long way, champsim::address full_addr, champsim::address ip,
                                     champsim::address victim_addr, access_type type, uint8_t hit)
{

  // cache hit
  if (hit) {
    // do not update replacement state for writebacks
    if (access_type{type} == access_type::WRITE) {
      get_rrpv(set, way) = maxRRPV - 1;
      //experiment
        //get_state(set, way) = LINE_REUSED;  

      return;
    }
    get_rrpv(set, way) = 0; // for cache hit, DRRIP always promotes a cache line to the MRU position
    if (type == access_type::LOAD || type == access_type::RFO)
    {get_state(set, way) = LINE_REUSED;}
  }
}

void reuse_drrip::replacement_cache_fill(uint32_t triggering_cpu, long set, long way, champsim::address full_addr, champsim::address ip,
                                   champsim::address victim_addr, access_type type)
{


  // added for reuse_drrip
  uint8_t& st = get_state(set, way);
  if (st != LINE_INVALID && in_measure_set(set)) {
    ph[triggering_cpu].evictions++;
    if (st == LINE_FILLED) ph[triggering_cpu].dead++;     // evicted without a demand hit
   }
    st = LINE_FILLED;   
  // do not update replacement state for writebacks
  if (access_type{type} == access_type::WRITE) {
    get_rrpv(set, way) = maxRRPV - 1;
    return;
  }

  //added for reuse_drrip
  check_epoch(triggering_cpu); 
  if (PSEL[triggering_cpu].decide(set)) {
    update_brrip(set, way);
  } else {
    update_srrip(set, way);
  }
  // cache miss, update bad
  PSEL[triggering_cpu].update_bad(set);
}

// find replacement victim
long reuse_drrip::find_victim(uint32_t triggering_cpu, uint64_t instr_id, long set, const champsim::cache_block* current_set, champsim::address ip,
                        champsim::address full_addr, access_type type)
{
  // look for the maxRRPV line
  auto begin = std::next(std::begin(rrpv), set * NUM_WAY);
  auto end = std::next(begin, NUM_WAY);

  auto victim = std::max_element(begin, end);
  if (auto rrpv_update = maxRRPV - *victim; rrpv_update != 0)
    for (auto it = begin; it != end; ++it)
      *it += rrpv_update;

  assert(begin <= victim);
  assert(victim < end);
  return std::distance(begin, victim); // cast protected by assertions
}

void reuse_drrip::replacement_final_stats()
{
  for (std::size_t c = 0; c < ph.size(); ++c)
    if (ph[c].epochs)
      std::printf("CPU%zu reuse signal: epochs=%lu forced_brrip=%lu forced_srrip=%lu final R=%.3f\n",
                  c, ph[c].epochs, ph[c].forced_brrip, ph[c].forced_srrip, ph[c].avg_r);
}