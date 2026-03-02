# MPI Parallel Communication with Distance-Based Rank Interaction, Data Exchange and Communication Analysis
### IIT Kanpur CS633: Parallel Computing

<br> 

This project implements a parallel MPI program that studies the performance of blocking point-to-point communication between processes separated by fixed distances. Each process exchanges data with neighbors at distances D1 and ​D2, over multiple iterations, performs computation on received data, and returns the results to the sender.

The implementation uses an alternating odd–even communication scheme to safely coordinate blocking MPI_Send and MPI_Recv operations and avoid serialized communication patterns. The project evaluates how communication overhead affects scalability by measuring execution time for different process counts and message sizes on an HPC cluster.

The repository includes source code, SLURM job scripts, timing data, plotting scripts, and experimental results used for performance analysis.
