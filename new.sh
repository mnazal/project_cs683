for t in 605.mcf_s-1644B.champsimtrace.xz 620.omnetpp_s-874B.champsimtrace.xz Trace1.gz Trace2.gz; do
  ./bin/champsim --warmup-instructions 10000000 --simulation-instructions 25000000 ../../traces/$t > drrip_$t.txt &
done; wait
