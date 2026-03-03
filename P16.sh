#!/bin/bash
#SBATCH --job-name=P16
#SBATCH -N 2
#SBATCH --ntasks-per-node=16
#SBATCH --partition=cpu
#SBATCH --time=00:10:00
#SBATCH --output=P16_%j.out
#SBATCH --error=P16_%j.err

module load compiler/oneapi-2024/mpi

D1=2
D2=4
T=10
SEED=1000

for M in 262144 1048576
do
  for run in {1..5}
  do
    mpirun -np 16 ./src  $M $D1 $D2 $T $SEED
  done
done
