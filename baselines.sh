for b in srrip_static brrip_static reuse_drrip; do
  for t in 605.mcf_s-1644B.champsimtrace.xz 620.omnetpp_s-874B.champsimtrace.xz Trace1.gz Trace2.gz; do
    echo "$b $t"
  done
done | xargs -P 4 -L1 sh -c './bin/champsim_$0 --warmup-instructions 10000000 --simulation-instructions 25000000 ../../traces/$1 > results/$0_$1.txt'