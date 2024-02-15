#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <limits.h>

typedef long long ll;

ll min(ll a, ll b) {
    return a < b ? a : b;
}

int main(int argc, char* argv[])
{
    MPI_Init(&argc, &argv);

    int size, myRank;
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &myRank);
    
    int N;

    if (myRank == 0) {
        scanf("%d", &N);
    }

    MPI_Bcast(&N, 1, MPI_INT, 0, MPI_COMM_WORLD);

    ll adj[N][N];
    if (myRank == 0) {
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N ; j++) {
                scanf("%lld", &adj[i][j]);
                if (adj[i][j] == -1) {
                    adj[i][j]  = __LONG_LONG_MAX__;
                }
            }
        }
    }
    MPI_Bcast(adj, N*N, MPI_LONG_LONG, 0, MPI_COMM_WORLD);
    
    double startTime = MPI_Wtime();
    for (int k = 0; k < N; k++) {
        // the process responsible for the kth row will broadcast it to all other processes
        MPI_Bcast(&(adj[k][0]), N, MPI_LONG_LONG, k%size, MPI_COMM_WORLD);

        // wait for all processes to receive the broadcasts
        MPI_Barrier(MPI_COMM_WORLD);

        for (int i = myRank; i < N; i += size) {
            if (i == k) {
                continue;
            }
            for (int j = 0; j < N; j++) {
                if (j == k) continue;
                if (adj[i][k] + adj[k][j] >= 0) { // avoiding overflow
                    adj[i][j] = min(adj[i][j], adj[i][k] + adj[k][j]);
                }
            }
        }
    }
    MPI_Reduce(MPI_IN_PLACE, adj, N*N, MPI_LONG_LONG, MPI_MIN, 0, MPI_COMM_WORLD);

    double endTime = MPI_Wtime();
    if (myRank == 0) {
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++) {
                if (adj[i][j] == __LONG_LONG_MAX__) {
                    printf("-1 ");
                } 
                else {
                    printf("%lld ", adj[i][j]);
                }
            }
            printf("\n");
        }
    }

    MPI_Finalize();

    return EXIT_SUCCESS;
}