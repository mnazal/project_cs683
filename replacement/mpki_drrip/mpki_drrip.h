#ifndef REPLACEMENT_MPKI_DRRIP_H
#define REPLACEMENT_MPKI_DRRIP_H

#include <array>
#include <vector>
#include <cstdint>

#include "cache.h"
#include "modules.h"
#include "msl/stat_methods.h"

struct mpki_drrip : public champsim::modules::replacement {
private:
  unsigned& get_rrpv(long set, long way);

public:
  static constexpr unsigned maxRRPV = 3;
  static constexpr unsigned BRRIP_MAX = 32;
  static constexpr unsigned PSEL_WIDTH = 10;

  enum class set_type { follower, brrip_leader, srrip_leader };

  long NUM_SET, NUM_WAY;

  unsigned brrip_counter=0;

  std::vector<unsigned> rrpv;
  std::vector<champsim::msl::dscounter<long, PSEL_WIDTH>> PSEL;

  mpki_drrip(CACHE* cache);

  // void initialize_replacement()
  long find_victim(uint32_t triggering_cpu, uint64_t instr_id, long set, const champsim::cache_block* current_set, champsim::address ip,
                   champsim::address full_addr, access_type type);
  void replacement_cache_fill(uint32_t triggering_cpu, long set, long way, champsim::address full_addr, champsim::address ip, champsim::address victim_addr,
                              access_type type);
  void update_replacement_state(uint32_t triggering_cpu, long set, long way, champsim::address full_addr, champsim::address ip, champsim::address victim_addr,
                                access_type type, uint8_t hit);

  // use this function to print out your own stats at the end of simulation
  void replacement_final_stats();

  void update_brrip(long set, long way);
  void update_srrip(long set, long way);


  struct phase_t {
  uint64_t last_instr = 0, misses = 0, epochs = 0, changes = 0;
  double avg = -1.0;
  int consec = 0, cooldown = 0;
  std::vector<std::pair<uint64_t, double>> mpki_trace;
};
std::vector<phase_t> ph = std::vector<phase_t>(NUM_CPUS);

static constexpr uint64_t EPOCH_INSTR     = 250000;
static constexpr double   REL_THRESH      = 0.5;
static constexpr double   MIN_ABS_MPKI    = 1.0;
static constexpr int      CONSEC_NEEDED   = 1;
static constexpr int      COOLDOWN_EPOCHS = 3;
static constexpr long     PSEL_STEP       = 0;   // 0 = hard reset

void check_epoch(uint32_t cpu);
void reset_psel(uint32_t cpu);
};

#endif
    