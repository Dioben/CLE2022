#!/bin/bash
mpiexec -n 2 ./a.out -f ../../01/text2.txt >logbase.txt
for i in {1..1000}
do
mpiexec -n 2 ./a.out -f ../../01/text2.txt >logdiff.txt
if cmp -s -- "logbase.txt" "logdiff.txt"; then
:
else
  echo "goddamn it"
  break
fi
done