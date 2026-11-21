#!/bin/bash
cd "$(dirname "$0")"          # run from ~/cs683/Project/project_cs683
mkdir -p results
T=../../traces
MAIN="605.mcf_s-1644B.champsimtrace.xz 620.omnetpp_s-874B.champsimtrace.xz Trace1.gz Trace2.gz"
HOLD="$(cd $T && ls ligra_PageRankDelta* 2>/dev/null | head -1) $(cd $T && ls llama2* 2>/dev/null | head -1)"

for b in drrip srrip_static brrip_static reuse_drrip mpki_drrip; do
  [ -x bin/champsim_$b ] || { echo "MISSING bin/champsim_$b"; exit 1; }
done

CORES=$(nproc); P=4
[ "$CORES" -le 4 ] && P=$(( CORES>1 ? CORES-1 : 1 ))
MEM=$(free -g | awk '/Mem:/{print $7}')
[ "$MEM" -lt 8 ] && P=2

P=4
echo "parallel jobs: $P"

cat > run1.sh <<'EOF'
#!/bin/bash
IFS='|' read -r bin out trace envs <<< "$1"
grep -q "ChampSim completed" "results/$out" 2>/dev/null && exit 0   # skip finished runs (resumable)
env $envs ./bin/champsim_$bin --warmup-instructions 10000000 --simulation-instructions 25000000 ../../traces/$trace > "results/$out" 2>&1
EOF
chmod +x run1.sh

JOBS=jobs.txt; : > $JOBS
# A: reuse_drrip threshold sweep (main traces)
for hi in 0.8 0.9 0.95; do for t in $MAIN; do
  echo "reuse_drrip|reuse_hi${hi}_$t.txt|$t|REUSE_HI=$hi" >> $JOBS
done; done
# B: held-out traces, all policies
for t in $HOLD; do
  for b in drrip srrip_static brrip_static; do echo "$b|${b}_$t.txt|$t|" >> $JOBS; done
  for hi in 0.7 0.9; do echo "reuse_drrip|reuse_hi${hi}_$t.txt|$t|REUSE_HI=$hi" >> $JOBS; done
done
# C: mpki_drrip (Design 1) sweep: REL NEED STEP
for cfg in "0.35 1 0" "0.5 1 0" "0.5 2 0" "0.25 1 100"; do
  set -- $cfg
  for t in $MAIN; do
    echo "mpki_drrip|mpki_$1_$2_$3_$t.txt|$t|MPKI_REL=$1 MPKI_NEED=$2 MPKI_STEP=$3" >> $JOBS
  done
done

echo "jobs: $(wc -l < $JOBS)"
xargs -d '\n' -P $P -I{} ./run1.sh '{}' < $JOBS
echo DONE
