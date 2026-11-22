#!/bin/bash
IFS='|' read -r bin out trace envs <<< "$1"
grep -q "ChampSim completed" "results/$out" 2>/dev/null && exit 0  
env $envs ./bin/champsim_$bin --warmup-instructions 10000000 --simulation-instructions 25000000 ../../traces/$trace > "results/$out" 2>&1
