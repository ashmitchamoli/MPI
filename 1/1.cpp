#include <iostream>
#include <mpi.h>
#include <vector>
#include <set>
#include <algorithm>

using namespace std;

typedef unsigned long long ull;

vector<int> getPerm(int rank, int N, vector<ull> &factorial) { // get the rank-th permutation of N numbers. rank starts from 0. 0 <= rank < N!
    vector<int> permutation(N);

    vector<int> items(N); // list of items in ascending order
    for (int i = 0; i < N; i++) {
        items[i] = i;
    }

    for (int i = N - 1; i >= 0; i--) {
        int index = rank / factorial[i];       // calculating the factorial representation of rank
        permutation[N - 1 - i] = items[index]; // set the corresponding index in the permutation
        items.erase(items.begin() + index);    // remove the index from the list of available items
        rank %= factorial[i];                  // update rank
    }

    return permutation;
}

bool isValid(vector<int> &permutation) {
    // We need to check only the diagonals because we are only considering solutions where each row and column has exactly one queen
    for (int i = 0; i < permutation.size(); i++) {
        for (int j = i + 1; j < permutation.size(); j++) {
            if (abs(i - j) == abs(permutation[i] - permutation[j])) { // if two queens are on the same diagonal
                return false;
            }
        }
    }
    return true;
}

int main(int argc, char *argv[]) {
    int rank, size;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank); // get current process id
    MPI_Comm_size(MPI_COMM_WORLD, &size); // get number of processes

    // take input for N in process 0 and broadcast it to all other processes
    int N;
    if (rank == 0) {
        cin >> N;
    }
    MPI_Bcast(&N, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // a permutation of 0...N-1 represents the state of the board such that the i-th queen is in the i-th row and the permutation[i]-th column.
    // this ensure that each row and column pair have exactly one queen.

    double startTime = MPI_Wtime();
    vector<ull> factorial(N + 1, 1);
    for (int i = 1; i <= N; i++) {
        factorial[i] = factorial[i - 1] * i;
    }

    // calculate the number of permutations current process will handle
    ull numPerms = factorial[N] / size;
    ull start, end;

    if(numPerms > 0) {
        // rank of the first permutation current process will handle
        start = rank * numPerms;

        // rank of the last permutation current process will handle
        if (rank == size - 1) {
            end = factorial[N];
        }
        else {
            end = start + numPerms;
        }
    }
    else {
        start = rank;
        end = rank+1;
        if (rank >= factorial[N]) {
            start = factorial[N]-1;
            end = factorial[N]-1;
        }
    }

    // calculate the number of solutions
    ull count = 0;
    // the first permutation current process will handle
    auto startPerm = getPerm(start, N, factorial);
    for (ull i = start; i < end; i++) {
        count += isValid(startPerm);
        next_permutation(startPerm.begin(), startPerm.end());
    }

    // reduce the number of solutions to process 0
    ull total;
    MPI_Reduce(&count, &total, 1, MPI_UNSIGNED_LONG_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
    if (rank == 0) {
        cout << total << endl;
    }

    double endTime = MPI_Wtime();
    // if (rank == 0) {
    //     cout << "Time taken: " << endTime - startTime << " seconds" << endl;
    // }

    MPI_Finalize();
    return 0;
}