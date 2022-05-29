#!/bin/bash
for i in {1..100}
do
if mpiexec -n 4 ./a.out -f ../../02/mat128_32.bin>log.txt
then
:
else
break
fi
done