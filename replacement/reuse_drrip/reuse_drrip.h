#ifndef REPLACEMENT_REUSE_DRRIP_H
#define REPLACEMENT_REUSE_DRRIP_H

#include <array>
#include <cstdint>
#include <vector>

#include "cache.h"
#include "modules.h"
#include "msl/stat_methods.h"

struct reuse_drrip : public champsim::modules::replacement {
private:
  unsigned& get_rrpv(long set, long way);

public:
  static constexpr unsigned maxRRPV = 3;
  static constexpr unsigned BRRIP_MAX = 32;
  static constexpr unsigned PSEL_WIDTH = 10;
  // Added for reuse_drrip
  static constexpr uint64_t EPOCH_INSTR = 250000;
  static constexpr uint64_t MIN_EVICTIONS = 256;
  static constexpr double HI_THRESH = 0.9;
  static constexpr double EMA_ALPHA = 0.25;
  static constexpr long BIAS_OFF = 128;
  struct phase_t {
    uint64_t last_instr = 0, evictions = 0, dead = 0, epochs = 0, forced_brrip = 0;
    double avg_r = -1.0;
  };
  std::vector<phase_t> ph = std::vector<phase_t>(NUM_CPUS);

  enum class set_type { follower, brrip_leader, srrip_leader };

  long NUM_SET, NUM_WAY;

  unsigned brrip_counter = 0;

  std::vector<unsigned> rrpv;
  std::vector<champsim::msl::dscounter<long, PSEL_WIDTH>> PSEL;

  // Added for reuse_drrip
  enum line_state : uint8_t { LINE_INVALID = 0, LINE_FILLED = 1, LINE_REUSED = 2 };
  std::vector<uint8_t> state;                   // one byte per cache line
  champsim::msl::categorizer<long> cat_sampler; // same set -> category mapping that PSEL uses

  uint8_t& get_state(long set, long way);
  bool in_measure_set(long set) const;

  reuse_drrip(CACHE* cache);

  // void initialize_replacement()
  long find_victim(uint32_t triggering_cpu, uint64_t instr_id, long set, const champsim::cache_block* current_set, champsim::address ip,
                   champsim::address full_addr, access_type type);
  void replacement_cache_fill(uint32_t triggering_cpu, long set, long way, champsim::address full_addr, champsim::address ip, champsim::address victim_addr,
                              access_type type);
  void update_replacement_state(uint32_t triggering_cpu, long set, long way, champsim::address full_addr, champsim::address ip, champsim::address victim_addr,
                                access_type type, uint8_t hit);

  // use this function to print out your own stats at the end of simulation
  void replacement_final_stats();

  // Added for reuse_drrip
  void bias_psel(uint32_t cpu);
  void check_epoch(uint32_t cpu);

  void update_brrip(long set, long way);
  void update_srrip(long set, long way);
};

#endif
