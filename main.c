#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "mpi.h"

/**
 * MPI Program for Iterative Data Exchange and Computation
 *
 * This program performs data exchange between a sender process (rank)
 * and two receiver processes located at distances D1 and D2 (rank + D1, rank + D2).
 *
 * Workflow:
 * 1. Initialize data with random values based on a seed.
 * 2. Loop T times:
 * a. Send data to rank + D1 (if valid). Receiver computes square.
 * b. Send data to rank + D2 (if valid). Receiver computes log.
 * c. Receive results back from D1 and D2.
 * d. Update local buffers based on received results.
 * 3. Aggregation:
 * a. Compute local maximums of the final data.
 * b. Send local maximums to Rank 0.
 * c. Rank 0 computes global maximums and prints them along with execution time.
 */

int main(int argc, char **argv) {
	// -------------------------------------------------------------------------
	// 1. MPI Initialization
	// -------------------------------------------------------------------------
	MPI_Init(&argc, &argv);

	MPI_Status status;
	int size, rank;
	MPI_Comm_size(MPI_COMM_WORLD, &size); // Get total number of processes
	MPI_Comm_rank(MPI_COMM_WORLD, &rank); // Get current process rank

	// -------------------------------------------------------------------------
	// 2. Argument Parsing and Validation
	// -------------------------------------------------------------------------
	if (argc != 6) {
		if (rank == 0)
			printf("Too Few Arguments\nUsage %s <Count> <D1> <D2> <#Iterations> <Seed>\n", argv[0]);

		MPI_Finalize();
		exit(1);
	}

	int M = atoi(argv[1]);    // Count: Number of doubles per message
	int D1 = atoi(argv[2]);   // Distance 1
	int D2 = atoi(argv[3]);   // Distance 2
	int T = atoi(argv[4]);    // Number of iterations
	int seed = atoi(argv[5]); // Random seed

	// -------------------------------------------------------------------------
	// 3. Memory Allocation
	// -------------------------------------------------------------------------
	// Allocate buffers for sending, receiving, and intermediate computations
	double *data_received = (double *)malloc(M * sizeof(double));         // Buffer for incoming data from sender
	double *data_at_D1 = (double *)malloc(M * sizeof(double));            // Buffer for computation results at D1
	double *data_at_D2 = (double *)malloc(M * sizeof(double));            // Buffer for computation results at D2
	double *data_received_D1 = (double *)malloc(M * sizeof(double));      // Buffer to store results received back from D1
	double *data_received_D2 = (double *)malloc(M * sizeof(double));      // Buffer to store results received back from D2
	double *buffer_updated_for_D1 = (double *)malloc(M * sizeof(double)); // Data to send to D1 in next iter
	double *buffer_updated_for_D2 = (double *)malloc(M * sizeof(double)); // Data to send to D2 in next iter

	// -------------------------------------------------------------------------
	// 4. Data Initialization
	// -------------------------------------------------------------------------
	// Initialize data using the provided seed.
	// Note: buffer_updated_for_D1/D2 are initialized with the starting random data.
	srand(seed);
	for (int i = 0; i < M; i++) {
		data_received[i] = (double)rand() * (rank + 1) / 10000.0;
		data_at_D1[i] = 0.0;
		data_at_D2[i] = 0.0;
		data_received_D1[i] = 0.0;
		data_received_D2[i] = 0.0;
		buffer_updated_for_D1[i] = data_received[i];
		buffer_updated_for_D2[i] = data_received[i];
	}

	// Variables for result aggregation
	double maxTime;
	double max[2] = {-INFINITY, -INFINITY}; // Stores max values for D1 (index 0) and D2 (index 1)
	double maxRecv[2];

	// -------------------------------------------------------------------------
	// 5. Main Iterative Loop (Communication & Computation)
	// -------------------------------------------------------------------------
	double sTime = MPI_Wtime(); // Start Timer

	for (int i = 0; i < T; i++) {
		
		// === Step A: Communication for D1 (Sender -> Receiver) ===
		// We use an "Odd-Even" pattern based on (rank / D1).
		// Even blocks send first, Odd blocks receive first.

		// Case 1: Even block (sender)
		if ((rank / D1) % 2 == 0 && rank + D1 < size)
			MPI_Send(buffer_updated_for_D1, M, MPI_DOUBLE, rank + D1, rank, MPI_COMM_WORLD);

		// Case 2: Odd block (receiver) - Receives from sender (rank - D1)
		else if ((rank / D1) % 2 == 1 && rank >= D1)
			MPI_Recv(data_received, M, MPI_DOUBLE, rank - D1, rank - D1, MPI_COMM_WORLD, &status);

		// Case 3: Odd block (sender) - Now it's the odd blocks' turn to send
		if ((rank / D1) % 2 == 1 && rank + D1 < size)
			MPI_Send(buffer_updated_for_D1, M, MPI_DOUBLE, rank + D1, rank, MPI_COMM_WORLD);

		// Case 4: Even block (receiver) - Now even blocks receive
		else if ((rank / D1) % 2 == 0 && rank >= D1)
			MPI_Recv(data_received, M, MPI_DOUBLE, rank - D1, rank - D1, MPI_COMM_WORLD, &status);

		// === Step B: Compute at D1 ===
		// If this process received data (acting as a D1 node), compute squares
		if (rank >= D1) {
			for (int j = 0; j < M; j++)
				data_at_D1[j] = data_received[j] * data_received[j];
		}

		// --------------------------------------------------------------------

		// === Step C: Communication for D2 (Sender -> Receiver) ===
		// Similar Odd-Even pattern logic, but using distance D2

		// Even blocks send
		if ((rank / D2) % 2 == 0 && rank + D2 < size)
			MPI_Send(buffer_updated_for_D2, M, MPI_DOUBLE, rank + D2, rank, MPI_COMM_WORLD);
		
		// Odd blocks receive
		else if ((rank / D2) % 2 == 1 && rank >= D2)
			MPI_Recv(data_received, M, MPI_DOUBLE, rank - D2, rank - D2, MPI_COMM_WORLD, &status);

		// Odd blocks send
		if ((rank / D2) % 2 == 1 && rank + D2 < size)
			MPI_Send(buffer_updated_for_D2, M, MPI_DOUBLE, rank + D2, rank, MPI_COMM_WORLD);

		// Even blocks receive
		else if ((rank / D2) % 2 == 0 && rank >= D2)
			MPI_Recv(data_received, M, MPI_DOUBLE, rank - D2, rank - D2, MPI_COMM_WORLD, &status);

		// === Step D: Compute at D2 ===
		// If this process received data (acting as a D2 node), compute natural log
		if (rank >= D2) {
			for (int j = 0; j < M; j++)
				data_at_D2[j] = log(data_received[j]);
		}

		// --------------------------------------------------------------------

		// === Step E: Return Results for D1 (Receiver -> Sender) ===
		// Sending the computed results (data_at_D1) back to the original source.
		// Logic is reversed: Receivers (high ranks) send back to Sources (low ranks).

		// Odd blocks (Receivers) send back results
		if ((rank / D1) % 2 == 1 && rank >= D1)
			MPI_Send(data_at_D1, M, MPI_DOUBLE, rank - D1, rank - D1, MPI_COMM_WORLD);

		// Even blocks (Original Senders) receive results
		else if ((rank / D1) % 2 == 0 && rank + D1 < size)
			MPI_Recv(data_received_D1, M, MPI_DOUBLE, rank + D1, rank, MPI_COMM_WORLD, &status);

		// Even blocks (Receivers) send back results
		if ((rank / D1) % 2 == 0 && rank >= D1)
			MPI_Send(data_at_D1, M, MPI_DOUBLE, rank - D1, rank - D1, MPI_COMM_WORLD);

		// Odd blocks (Original Senders) receive results
		else if ((rank / D1) % 2 == 1 && rank + D1 < size)
			MPI_Recv(data_received_D1, M, MPI_DOUBLE, rank + D1, rank, MPI_COMM_WORLD, &status);

		// --------------------------------------------------------------------

		// === Step F: Return Results for D2 (Receiver -> Sender) ===
		// Sending computed results (data_at_D2) back to source.

		// Odd blocks send
		if ((rank / D2) % 2 == 1 && rank >= D2)
			MPI_Send(data_at_D2, M, MPI_DOUBLE, rank - D2, rank - D2, MPI_COMM_WORLD);

		// Even blocks receive
		else if ((rank / D2) % 2 == 0 && rank + D2 < size)
			MPI_Recv(data_received_D2, M, MPI_DOUBLE, rank + D2, rank, MPI_COMM_WORLD, &status);

		// Even blocks send
		if ((rank / D2) % 2 == 0 && rank >= D2)
			MPI_Send(data_at_D2, M, MPI_DOUBLE, rank - D2, rank - D2, MPI_COMM_WORLD);

		// Odd blocks receive
		else if ((rank / D2) % 2 == 1 && rank + D2 < size)
			MPI_Recv(data_received_D2, M, MPI_DOUBLE, rank + D2, rank, MPI_COMM_WORLD, &status);

		// --------------------------------------------------------------------

		// === Step G: Update Buffers for Next Iteration ===
		// Update D1 Buffer
		if (rank + D1 < size) {
			for (int j = 0; j < M; j++)
				buffer_updated_for_D1[j] = (unsigned long long)data_received_D1[j] % 100000;
		}

		// Update D2 Buffer
		if (rank + D2 < size) {
			for (int j = 0; j < M; j++)
				buffer_updated_for_D2[j] = data_received_D2[j] * 100000;
		}
	} // End of T iterations

	// -------------------------------------------------------------------------
	// 6. Final Computation and Gathering
	// -------------------------------------------------------------------------

	// Compute LOCAL maximum values for D1 and D2 arrays
	if (rank + D1 < size) {
		for (int i = 0; i < M; i++)
			max[0] = buffer_updated_for_D1[i] > max[0] ? buffer_updated_for_D1[i] : max[0];
	}

	if (rank + D2 < size) {
		for (int i = 0; i < M; i++)
			max[1] = buffer_updated_for_D2[i] > max[1] ? buffer_updated_for_D2[i] : max[1];
	}

	// Gather maximum values at the root process (Rank 0)
	if (rank == 0) {
		// Receive from all valid senders (ranks that have destinations)
		for (int i = 1; i < size - D1; i++) {
			MPI_Recv(maxRecv, 2, MPI_DOUBLE, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
			max[0] = maxRecv[0] > max[0] ? maxRecv[0] : max[0];
			max[1] = maxRecv[1] > max[1] ? maxRecv[1] : max[1];
		}
	}
	else if (rank + D1 < size) {
		// Send local maximums to Rank 0
		MPI_Send(max, 2, MPI_DOUBLE, 0, rank, MPI_COMM_WORLD);
	}

	double eTime = MPI_Wtime(); // End Timer

	// -------------------------------------------------------------------------
	// 7. Time Aggregation and Output
	// -------------------------------------------------------------------------
	
	// Calculate elapsed time and find the maximum time across ALL processes
	double time = eTime - sTime;
	MPI_Reduce(&time, &maxTime, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

	// Print results: Max D1 Value, Max D2 Value, Max Time
	if (rank == 0)
		printf("%f %f %f\n", max[0], max[1], maxTime);

	// -------------------------------------------------------------------------
	// 8. Cleanup
	// -------------------------------------------------------------------------
	free(data_received);
	free(data_at_D1);
	free(data_at_D2);
	free(data_received_D1);
	free(data_received_D2);
	free(buffer_updated_for_D1);
	free(buffer_updated_for_D2);
	
	MPI_Finalize();

	return 0;
}
