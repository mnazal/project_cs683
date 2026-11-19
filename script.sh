#!/bin/bash
mkdir -p results

W=10000000
S=25000000

TRACES=(
  "400.perlbench-41B.champsimtrace.xz"
  "Trace1.gz"
  "Trace2.gz"
  "ligra_PageRankDelta.com-lj.ungraph.gcc_6.3.0_O3.drop_6000M.length_250M.champsimtrace.xz"
  "llama2.c-llama2_7b.1.champsimtrace.gz"
)

for t in "${TRACES[@]}"; do
  echo "Running $t..."
  ./bin/champsim --warmup-instructions $W --simulation-instructions $S "../../traces/$t" > "results/drrip_${t}.txt" &
  ./bin/champsim_mpki_drrip --warmup-instructions $W --simulation-instructions $S "../../traces/$t" > "results/mpki_${t}.txt" &
  wait
done

echo "All done!"