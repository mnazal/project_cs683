#!/bin/bash
mkdir -p results/run11

W=10000000
S=25000000

TRACES=(
  "Trace1.gz"
  "Trace2.gz"
  "620.omnetpp_s-874B.champsimtrace.xz"
  "605.mcf_s-1644B.champsimtrace.xz"
)

for t in "${TRACES[@]}"; do
  echo "Running $t..."
  ./bin/champsim_mpki_drrip --warmup-instructions $W --simulation-instructions $S "../../traces/$t" > "results/run11/mpki_${t}.txt" &
  wait
done

echo "All done!"