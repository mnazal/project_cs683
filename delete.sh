for p in srrip_static brrip_static; do
  cp -r replacement/drrip replacement/$p
  ( cd replacement/$p
    for f in drrip.*; do mv "$f" "${f/drrip/$p}"; done
    sed -i "s/drrip/$p/g; s/DRRIP/${p^^}/g" $p.* )
done
sed -i 's/if (PSEL\[triggering_cpu\].decide(set))/if (false)/' replacement/srrip_static/srrip_static.cc
sed -i 's/if (PSEL\[triggering_cpu\].decide(set))/if (true)/'  replacement/brrip_static/brrip_static.cc
grep -n brrip_counter replacement/brrip_static/brrip_static.h   # make sure it says "= 0"