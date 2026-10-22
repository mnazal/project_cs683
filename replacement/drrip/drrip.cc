#include "drrip.h"

#include <algorithm>
#include <cassert>
#include <random>
#include <utility>
#include<iostream>

#include "champsim.h"

drrip::drrip(CACHE* cache)
    : replacement(cache), NUM_SET(cache->NUM_SET), NUM_WAY(cache->NUM_WAY), rrpv(static_cast<std::size_t>(NUM_SET * NUM_WAY)),
      PSEL(NUM_CPUS, champsim::msl::dscounter<long, PSEL_WIDTH>(champsim::msl::get_sample_rate(NUM_SET)))
{
}

unsigned& drrip::get_rrpv(long set, long way) { return rrpv.at(static_cast<std::size_t>(set * NUM_WAY + way)); }

void drrip::update_brrip(long set, long way)
{
  get_rrpv(set, way) = maxRRPV;

  brrip_counter++;
  if (brrip_counter == BRRIP_MAX) {
    brrip_counter = 0;
    get_rrpv(set, way) = maxRRPV - 1;
  }
}

void drrip::update_srrip(long set, long way) { get_rrpv(set, way) = maxRRPV - 1; }

// called on every cache hit and cache fill
void drrip::update_replacement_state(uint32_t triggering_cpu, long set, long way, champsim::address full_addr, champsim::address ip,
                                     champsim::address victim_addr, access_type type, uint8_t hit)
{

  // cache hit
  if (hit) {
    // do not update replacement state for writebacks
    if (access_type{type} == access_type::WRITE) {
      get_rrpv(set, way) = maxRRPV - 1;
      return;
    }
    get_rrpv(set, way) = 0; // for cache hit, DRRIP always promotes a cache line to the MRU position
  }
}

void drrip::replacement_cache_fill(uint32_t triggering_cpu, long set, long way, champsim::address full_addr, champsim::address ip,
                                   champsim::address victim_addr, access_type type)
{
  // do not update replacement state for writebacks
  if (access_type{type} == access_type::WRITE) {
    get_rrpv(set, way) = maxRRPV - 1;
    return;
  }
  if (PSEL[triggering_cpu].decide(set)) {
    update_brrip(set, way);
  } else {
    update_srrip(set, way);
  }
  // cache miss, update bad
  const auto psel_before = PSEL[triggering_cpu].value();
  PSEL[triggering_cpu].update_bad(set);
  const auto psel_after = PSEL[triggering_cpu].value();
  constexpr long psel_threshold = 1L << (PSEL_WIDTH - 1);
  if ((psel_before < psel_threshold && psel_after >= psel_threshold) || (psel_before >= psel_threshold && psel_after < psel_threshold)) {
    std::cout << "[DRRIP-PSEL] cache=" << intern_->NAME << " next_follower_policy=" << (psel_after >= psel_threshold ? "BRRIP" : "SRRIP") << '\n';
  }

  psel_log_fill_count++;
  if (psel_log_fill_count % PSEL_LOG_INTERVAL == 0) {
    psel_log_sample++;
    std::cout << "[DRRIP-PSEL] cache=" << intern_->NAME  << " PSEL value=" << PSEL[0].value() << '\n';
  }
}

// find replacement victim
long drrip::find_victim(uint32_t triggering_cpu, uint64_t instr_id, long set, const champsim::cache_block* current_set, champsim::address ip,
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
