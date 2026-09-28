#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define N 10
#define INF 99999

void generateMatrix(int n, int* graph) {
  for (int i = 0; i < n * n; i++)
    graph[i] = INF;
  for (int i = 0; i < n; i++)
    graph[i * n + i] = 0;
  for (int i = 0; i < n; i++) {
    for (int j = i + 1; j < n; j++) {
      if (rand() % 10 < 9) {
        int weight = (rand() % 20) + 1;
        if (rand() % 2 == 0) {
          graph[i * n + j] = weight;
        } else {
          graph[j * n + i] = weight;
        }
      }
    }
  }
}

int main() {
  int commsize, rank;
  MPI_Init(NULL, NULL);
  MPI_Comm_size(MPI_COMM_WORLD, &commsize);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  int block_size = N / commsize;
  int* local_matrix = malloc(block_size * N * sizeof(int));
  int* k_row = malloc(N * sizeof(int));

  double start_time = MPI_Wtime();
  if (rank == 0) {
    int* matrix = malloc(N * N * sizeof(int));
    srand(time(NULL));
    generateMatrix(N, matrix);

    MPI_Scatter(matrix, block_size * N, MPI_INT, local_matrix, block_size * N,
                MPI_INT, 0, MPI_COMM_WORLD);
    free(matrix);
  } else {
    MPI_Scatter(NULL, block_size * N, MPI_INT, local_matrix, block_size * N,
                MPI_INT, 0, MPI_COMM_WORLD);
  }

  for (int k = 0; k < N; k++) {
    int root = k / block_size;
    if (rank == root) {
      int local_k = k % block_size;
      for (int j = 0; j < N; j++) {
        k_row[j] = local_matrix[local_k * N + j];
      }
    }
    MPI_Bcast(k_row, N, MPI_INT, root, MPI_COMM_WORLD);

    for (int i = 0; i < block_size; i++) {
      for (int j = 0; j < N; j++) {
        if (local_matrix[i * N + j] > local_matrix[i * N + k] + k_row[j]) {
          local_matrix[i * N + j] = local_matrix[i * N + k] + k_row[j];
        }
      }
    }
  }

  double end_time = MPI_Wtime();
  double result_time = end_time - start_time;
  double max_time;
  MPI_Reduce(&result_time, &max_time, 1, MPI_DOUBLE, MPI_MAX, 0,
             MPI_COMM_WORLD);

  if (rank == 0) {
    int* result = malloc(N * N * sizeof(int));
    MPI_Gather(local_matrix, block_size * N, MPI_INT, result, block_size * N,
               MPI_INT, 0, MPI_COMM_WORLD);
    free(result);
  } else {
    MPI_Gather(local_matrix, block_size * N, MPI_INT, NULL, block_size * N,
               MPI_INT, 0, MPI_COMM_WORLD);
  }


  if (rank == 0) {
    printf("Время работы алгоритма Флойда при n=%d и p=%d: %.4f сек\n",
           N, commsize, max_time);
  }

  free(local_matrix);
  free(k_row);
  MPI_Finalize();
  return 0;
}