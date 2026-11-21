# for f in results/*.txt; do
#   ipc=$(grep -m1 "CPU 0 cumulative IPC" "$f" | awk '{print $5}')
#   ld=$(grep "cpu0->LLC LOAD" "$f" | awk '{print $8}')
#   rfo=$(grep "cpu0->LLC RFO" "$f" | awk '{print $8}')
#   echo "$f  IPC=$ipc  demand_misses=$((ld+rfo))"
# done


for f in results/*.txt; do
  ipc=$(grep -m1 'CPU 0 cumulative IPC' $f | awk '{print $5}')
  ld=$(grep 'cpu0->LLC LOAD' $f | awk '{print $8}'); rfo=$(grep 'cpu0->LLC RFO' $f | awk '{print $8}')
  ok=$(grep -q 'ChampSim completed' $f || echo INCOMPLETE)
  echo "$(basename $f .txt) IPC=$ipc DM=$((ld+rfo)) $ok | $(tail -1 $f | cut -c1-80)"
done | sort > summary.txt