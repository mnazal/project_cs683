# Runtime-signal-driven set dueling for DRRIP

**Overview:** Two enhancements to DRRIP's set-dueling counter (PSEL) in ChampSim's last-level cache.

Course project, CS683 Advanced Computer Architecture, IIT Bombay (Autumn 2026).

| Design | Module | Idea |
|---|---|---|
| 1 | `mpki_drrip` | When demand MPKI changes sharply between 250k-instruction interval, reset PSEL to just below the midpoint (i.e. restart the duel). |
| 2 | `reuse_drrip` | Track the share of LLC lines evicted without a demand hit (dead-block ratio R) in the SRRIP leader sets. If smoothed R > 0.9, push PSEL toward BRRIP. |


---

## Experimental Setup & Configuration

* **Simulator:** ChampSim (1 core)
* **LLC Topology:** 2048 sets $\times$ 16 ways
* **L2 Cache:** $1024 \times 8$
* **Dueling Configuration:** PSEL (10 bits), with 64 SRRIP + 64 BRRIP leader sets.




## Download dependencies

ChampSim uses [vcpkg](https://vcpkg.io) to manage its dependencies. In this repository, vcpkg is included as a submodule. You can download the dependencies with
```
git submodule update --init
vcpkg/bootstrap-vcpkg.sh
vcpkg/vcpkg install
```
## Configuration File Setup
To set the replacement policy in your configuration file, update the `"replacement"` field under the `"LLC"` object:

> **Design 1 (`mpki_drrip`):**
> ```json
> {
>     "LLC": {
>         "replacement": "mpki_drrip"
>     }
> }
> ```

> **Design 2 (`reuse_drrip`):**
> ```json
> {
>     "LLC": {
>         "replacement": "reuse_drrip"
>     }
> }
> ```
## Build and Run 

```bash
./config.sh champsim_config.json && make -j   
./bin/champsim --warmup-instructions 10000000 --simulation-instructions 25000000 <trace>
```
- `mpki_drrip`: epoch detector (`check_epoch`, `reset_psel`), demand-miss counting in the fill path, stats line.
- `reuse_drrip`: per-line state (invalid/filled/reused), eviction accounting before the writeback early-return,
  `check_epoch`, `bias_psel`, stats line.



## Hardware cost (estimate)
**mpki_drrip**: about 54 bits of registers, shifts and compares only. 

**reuse_drrip**: 1 reused bit per LLC line (4 KB) plus about 54 bits.
Neither touches the critical path.

## Traces used
https://drive.google.com/drive/folders/10cJoTfv8WRjTOkBvxiickZO9SDUmJLS7?usp=sharing



## Acknowledgements
Built on ChampSim and the RRIP/DRRIP policies (Jaleel et al., ISCA 2010). Course: CS683, IIT Bombay.
<!-- Add an AI-assistance disclosure here if your course requires one. -->
