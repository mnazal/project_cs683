# Runtime Context-Based Set Dueling for Cache Replacement

## 1. Project Overview

This project investigates whether lightweight runtime context can improve cache replacement policy selection compared with conventional Set Dueling.

The project uses **ChampSim** to evaluate cache replacement policies and study how their behavior varies across workloads. The initial focus is on LLC miss behavior and Misses Per Kilo Instructions (MPKI).

### Research Question

Can lightweight runtime context improve Set Dueling's cache replacement policy selection while keeping the mechanism simple?

## 2. Motivation

Conventional dynamic cache replacement mechanisms select between candidate policies based on their observed performance in designated sampling sets. However, the effectiveness of a replacement policy may vary across workloads and execution phases.

This project will investigate whether runtime information, initially focusing on LLC MPKI, can help characterize cache behavior and guide policy selection more effectively.

The goal is to start with an existing ChampSim implementation and develop a focused, experimentally validated extension rather than reproduce an entire context-aware learning framework.

## 3. Reference Work

The project is motivated by the following research:

- **Retrofitting Contextual Learning into Set Dueling** — MICRO 2026.
- **Context-Aware Set Dueling for Dynamic Policy Arbitration** — CAL 2025.
- **Making Cache Replacement Simple and Effective** — Qureshi and Patt, ISCA 2006, relevant background on adaptive cache replacement and set dueling.

These works provide context for investigating dynamic policy selection and the use of execution context in cache replacement.

## 4. Experimental Environment

- **Simulator:** ChampSim
- **Environment:** Ubuntu on WSL2
- **Cache level of interest:** Last-Level Cache (LLC)
- **Initial replacement policies:** LRU and DRRIP
- **Initial workload:** `400.perlbench-41B`

The initial experiments use the existing ChampSim replacement policies. The runtime-context-based extension remains planned work.

## 5. Initial Baseline Results

The following results were obtained from two ChampSim runs using `400.perlbench-41B`.

| Metric | LRU | DRRIP |
|---|---:|---:|
| IPC | 1.393 | 1.421 |
| LLC accesses | 146,418 | 146,420 |
| LLC misses | 97,436 | 77,735 |
| LLC miss rate | 66.55% | 53.09% |
| LLC MPKI | 0.390 | 0.311 |
| Average LLC miss latency | 176.5 cycles | 162.4 cycles |

Each run used 250 million warmup instructions and 250 million simulation instructions.

### Initial Observation

For this workload, DRRIP achieved higher IPC and fewer LLC misses than LRU. The result establishes an initial baseline for the project.

These results cover only one workload and do not establish that DRRIP is generally superior to LRU or that runtime context improves policy selection.

## 6. Benchmark Plan

The benchmark suite will prioritize workload families used in or relevant to the MICRO 2026 reference paper.

| Workload family | Status |
|---|---|
| SPEC CPU 2017 | Candidate traces identified |
| Google server workloads (`gtrace_v2`) | Candidate traces identified |
| Graph workloads (Ligra) | Candidate traces identified |
| AI/ML workloads | Candidate traces identified |
| Additional server workloads from the paper | Trace availability still to be verified |

The public DPC4 trace manifest has been downloaded. It lists the available traces and their compressed sizes.

Initial candidates include:

- `SPEC17/602.gcc_s-1850B.champsimtrace.xz`
- `Graph/Ligra/ligra_PageRankDelta.com-lj.ungraph.gcc_6.3.0_O3.drop_6000M.length_250M.champsimtrace.xz`
- `ai-ml/llama2.c-llama2_7b.1.champsimtrace.gz`
- `gtrace_v2/tango/tango_0000.champsim.gz`

These are candidate traces, not a finalized benchmark suite. The selected workloads will be run in ChampSim, and their LLC MPKI will be measured. The paper's focus on workloads with LLC MPKI above 1 will inform workload selection.

The DPC4 collection does not necessarily contain every server trace used in the reference paper.

## 7. Planned Methodology

1. Establish and validate the LRU and DRRIP baselines.
2. Select a manageable and diverse benchmark suite.
3. Measure LLC misses, MPKI, IPC, and other relevant performance metrics.
4. Investigate how cache behavior changes during workload execution.
5. Implement a lightweight runtime-context-based policy selection mechanism.
6. Compare the proposed mechanism against the baseline policies using identical workloads and simulation settings.
7. Analyze performance improvements, limitations, and the complexity of the proposed mechanism.

The precise context-selection mechanism will be finalized after the baseline analysis.

## 8. Current Status

- [x] Clone and build ChampSim.
- [x] Run the initial LRU experiment.
- [x] Run the initial DRRIP experiment.
- [x] Compare the initial results.
- [x] Download the public DPC4 trace manifest.
- [x] Identify candidate traces from multiple workload families.
- [ ] Download and validate the selected benchmark traces.
- [ ] Run baseline experiments on the expanded workload suite.
- [ ] Analyze runtime LLC MPKI and workload behavior.
- [ ] Design and implement the context-aware selection mechanism.
- [ ] Evaluate the proposed mechanism against the baselines.
- [ ] Prepare final results and project report.

## 9. Repository Organization

The project repository should contain the simulator configuration, any custom implementation, experiment scripts, and results. Large trace files should be kept separate from version-controlled source files.

A suggested organization is:

```text
project_cs683/
├── README.md
├── champsim_config.json
├── config.sh
├── inc/
├── src/
├── replacement/
├── results/
│   ├── baseline/
│   └── context_aware/
├── scripts/
└── docs/
```

This is a suggested organization, not a verified listing of the current repository. Existing ChampSim source directories should be preserved, and custom files should follow the repository's actual structure.

## 10. Reproducibility

For meaningful comparisons, LRU, DRRIP, and the proposed mechanism should use the same:

- Workload trace
- Warmup and simulation instruction counts
- Cache configuration
- Simulator build settings
- Reported performance metrics

Record the configuration, exact trace filename, and output log for every experiment.

Large trace files and generated binaries should not be committed to Git.

## 11. Expected Outcome

The project aims to determine whether a lightweight runtime-context signal can help identify when different cache replacement policies are effective and whether this information can improve policy selection over conventional Set Dueling.

The outcome will be based on measured simulation results. No improvement from the proposed mechanism is assumed in advance.
