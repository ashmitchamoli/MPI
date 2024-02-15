#include <iostream>
#include <fstream>
#include <mpi.h>
#include <vector>
#include <set>
#include <algorithm>
#include <limits>

using namespace std;

typedef long long ll;

int main(int argc, char* argv[]) {
    int rank, size;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank); // get current process id
    MPI_Comm_size(MPI_COMM_WORLD, &size); // get number of processes

    int N, M, T;
    if (rank == 0) {
        cin >> N >> M >> T;
    }
    MPI_Bcast(&N, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&M, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&T, 1, MPI_INT, 0, MPI_COMM_WORLD);
    
    int board1[N][M]; // write into this at even iterations
    int board2[N][M]; // write into this at odd iterations

    if (rank == 0) {
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < M; j++) {
                cin >> board1[i][j];
                board2[i][j] = board1[i][j];
            }
        }
    }

    MPI_Bcast(board1, N*M, MPI_INT, 0, MPI_COMM_WORLD);

    // measure the time taken to run the algorithm
    double startTime = MPI_Wtime();

    // start and end rows of current process
    int rowsPerProc = N / size;
    int startRow, endRow;
    if (rowsPerProc == 0) {
        startRow = rank;
        endRow = rank+1;
        if (rank >= N) {
            startRow = N;
            endRow = N;
        }
    }
    else {
        startRow = rank * rowsPerProc;
        if (rank == size - 1) {
            endRow = N;
        }
        else {
            endRow = startRow + rowsPerProc;
        }
    }

    for (int t = 0; t < T; t++) {
        int (*readBoard)[M];
        int (*writeBoard)[M];

        if (t % 2) {
            readBoard = board2;
            writeBoard = board1;
        }
        else {
            readBoard = board1;
            writeBoard = board2;
        }

        // idea 1: not all processes need info about all the rows.
        // only need the boundary rows of the board
        // idea 2: can allocate contiguous rows for each process, that way boundary sharing between processes is minimized.

        // the current process requires the latest value of the row above and the row below ti
        // the current process needs to send the latest value of its first and last row to the processes above and below it

        if (startRow < N) { // the current process has something to compute
            if (rank > 0) {
                // send the first row to the process above
                MPI_Send(&(readBoard[startRow][0]), M, MPI_INT, rank-1, 0, MPI_COMM_WORLD);
                // recieve the last row from the process above
                MPI_Recv(&(readBoard[startRow-1][0]), M, MPI_INT, rank-1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }
            if (endRow < N && rank+1 < size) {
                // send the last row to the process below
                MPI_Send(&(readBoard[endRow-1][0]), M, MPI_INT, rank+1, 0, MPI_COMM_WORLD);
                // recieve the first row from the process below
                MPI_Recv(&(readBoard[endRow][0]), M, MPI_INT, rank+1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }
        }
        // gather all the rows of the board computed in the precvious iteration
        // for (int i = 0; i < N; i++) {
        //     MPI_Bcast(&(readBoard[i][0]), M, MPI_INT, i%size, MPI_COMM_WORLD);
        // }

        // wait till all communication is done
        MPI_Barrier(MPI_COMM_WORLD);

        for (int i = startRow; i < N && i < endRow; i++) {
            for (int j = 0; j < M; j++) {
                int aliveNeighbours = 0;

                for (int x = -1; x < 2; x++) {
                    if (i + x < 0 || i + x > N-1) continue;
                    for (int y = -1; y < 2; y++) {
                        if ((x == 0 && y == 0) || j + y < 0 || j + y > M-1) continue;
                        aliveNeighbours += readBoard[i+x][j+y];
                    } 
                }

                if (readBoard[i][j] == 1) {
                    if (aliveNeighbours < 2 || aliveNeighbours > 3)
                        writeBoard[i][j] = 0;
                    else
                        writeBoard[i][j] = 1;
                } 
                else {
                    if (aliveNeighbours == 3)
                        writeBoard[i][j] = 1;
                    else
                        writeBoard[i][j] = 0;
                }
            }
        }
    }

    int (*board)[M];
    if (T % 2) {
        board = board2;
    }
    else {
        board = board1;
    }

    for (int i = 0; i < N; i++) {
        if(i < startRow || i >= endRow) {
            // board[i][j] = INTMAX_MAX;
            for (int j = 0; j < M; j++) {
                board[i][j] = INT32_MAX;
            }
        }
    }
    MPI_Reduce(MPI_IN_PLACE, &(board[0][0]), N*M, MPI_INT, MPI_MIN, 0, MPI_COMM_WORLD);
    MPI_Barrier(MPI_COMM_WORLD);

    double endTime = MPI_Wtime();
    // print the final board
    if (rank == 0) {
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < M; j++) {
                cout << board[i][j] << " ";
            }
            cout << endl;
        }
    }

    MPI_Finalize();
    return 0;
}