#include <mpi.h>
#include <iostream>
#include <cstdlib>
#include <string.h>
using namespace std;

#define ROOT_0 0
#define ROOT_1 1
#define ROOT_2 2

// function to concatenate strings
string string_topo(int no_procs, int* v) {
    string s;
    for (int i = 0; i < no_procs - 1; ++i) {
        s += std::to_string(v[i]) + ",";
    }
    s += std::to_string(v[no_procs - 1]) + " ";

    return s;
}

int main (int argc, char *argv[])
{
    int  numtasks, rank, len, N, inter_rank;
    char hostname[MPI_MAX_PROCESSOR_NAME];
    int no_procs0, no_procs1, no_procs2;
    int* v0;
    int* v1;
    int* v2;
    int* v;
    char* str;
    string s;
    int error = atoi(argv[2]);

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numtasks); // Total number of processes.
    MPI_Comm_rank(MPI_COMM_WORLD,&rank); // The current process ID / Rank.

    // Coordinator 0
    if (rank == ROOT_0) {
        int val, len, dim, len0, len1, len2;
        int plus_size;
        
        N = atoi(argv[1]);

        FILE* f1 = fopen("cluster0.txt", "rt"); // open file to read
        fscanf(f1, "%d", &no_procs0);
        v0 = (int*) malloc(no_procs0 * sizeof(int));
        for (int i = 0; i < no_procs0; ++i) {
            fscanf(f1, "%d", &val);
            v0[i] = val;
        }

        // Send info to coord 2
        MPI_Send(&no_procs0, 1, MPI_INT, ROOT_2, 0, MPI_COMM_WORLD);
        MPI_Send(v0, no_procs0, MPI_INT, ROOT_2, 0, MPI_COMM_WORLD);
        printf("M(%d,%d)\n", rank, ROOT_2);

        // Receive info from coord 2
        MPI_Recv(&no_procs2, 1, MPI_INT, ROOT_2, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        v2 = (int*) malloc(no_procs2 * sizeof(int));
        MPI_Recv(v2, no_procs2, MPI_INT, ROOT_2, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        // Receive from coord 2 info about coord 1
        MPI_Recv(&no_procs1, 1, MPI_INT, ROOT_2, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        v1 = (int*) malloc(no_procs1 * sizeof(int));
        MPI_Recv(v1, no_procs1, MPI_INT, ROOT_2, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        // build topology
        s += std::to_string(ROOT_0) + ":" + string_topo(no_procs0, v0);
        s += std::to_string(ROOT_1) + ":" + string_topo(no_procs1, v1);
        s += std::to_string(ROOT_2) + ":" + string_topo(no_procs2, v2);

        str = (char*)s.c_str();
        len = s.length();

        // print topology
        printf("%d -> %s\n", rank, str);

        // Send to workers
        for (int i = 0; i < no_procs0; ++i) {
            // coord rank
            MPI_Send(&rank, 1, MPI_INT, v0[i], 0, MPI_COMM_WORLD);
            // topology as string
            MPI_Send(&len, 1, MPI_INT, v0[i], 0, MPI_COMM_WORLD);
            MPI_Send(str, len + 1, MPI_CHAR, v0[i], 0, MPI_COMM_WORLD);

            // print message info
            printf("M(%d,%d)\n", rank, v0[i]);
        }

        // Task 2

        // initialize array
        v = (int*) malloc(N * sizeof(int));
        for (int k = 0; k < N; ++k) {
            v[k] = k;
        }

        // set dimension of array to be processed by a single worker
        dim = N / (no_procs0 + no_procs1 + no_procs2);

        // length for every cluster to process
        len0 = no_procs0 * dim;
        len1 = no_procs1 * dim;
        len2 = N - len0 - len1;

        // Send the part of array for coord 1 through coord 2
        MPI_Send(&len1, 1, MPI_INT, ROOT_2, 0, MPI_COMM_WORLD);
        MPI_Send(v + len0, len1, MPI_INT, ROOT_2, 0, MPI_COMM_WORLD);
        printf("M(%d,%d)\n", rank, ROOT_2);

        // Send the part of array for coord 2
        MPI_Send(&len2, 1, MPI_INT, ROOT_2, 0, MPI_COMM_WORLD);
        MPI_Send(v + len0 + len1, len2, MPI_INT, ROOT_2, 0, MPI_COMM_WORLD);
        printf("M(%d,%d)\n", rank, ROOT_2);

        plus_size = 0; // offset to know where to read from

        // Send arrays to workers
        for (int i = 0; i < no_procs0; ++i) {
            if (dim < len0 - (no_procs0 - 1)* dim) {
                dim = len0 - (no_procs0 - 1)* dim;
            }

            MPI_Send(&dim, 1, MPI_INT, v0[i], 0, MPI_COMM_WORLD);
            MPI_Send(v + plus_size, dim, MPI_INT, v0[i], 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, v0[i]);

            // receive array after processing
            MPI_Recv(v + plus_size, dim, MPI_INT, v0[i], 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            plus_size += dim;
        }

        // Receive part processed by coord 1 from coord 2
        MPI_Recv(v + len0, len1, MPI_INT, ROOT_2, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        // Receive part processed by coord 2
        MPI_Recv(v + len0 + len1, len2, MPI_INT, ROOT_2, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        
        // Print result
        printf("Rezultat: ");
        for (int i = 0; i < N; ++i) {
            printf("%d ", v[i]);
        }
        cout << endl;

        fclose(f1); // Close file

        // Free allocated memory
        free(v);
        free(v0);
        free(v1);
        free(v2);
    }
    // coordinator 1 
    else if (rank == ROOT_1) {
        int val, v_len, dim, plus_size;

        FILE* f2 = fopen("cluster1.txt", "rt"); // open file to read
        fscanf(f2, "%d", &no_procs1);
        v1 = (int*) malloc(no_procs1 * sizeof(int));
        for (int i = 0; i < no_procs1; ++i) {
            fscanf(f2, "%d", &val);
            v1[i] = val;
        }

        // Send info to coord 2
        MPI_Send(&no_procs1, 1, MPI_INT, ROOT_2, 0, MPI_COMM_WORLD);
        MPI_Send(v1, no_procs1, MPI_INT, ROOT_2, 0, MPI_COMM_WORLD);
        printf("M(%d,%d)\n", rank, ROOT_2);

        // Receive info from coord 2
        MPI_Recv(&no_procs2, 1, MPI_INT, ROOT_2, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        v2 = (int*) malloc(no_procs2 * sizeof(int));
        MPI_Recv(v2, no_procs2, MPI_INT, ROOT_2, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        // Receive from coord 2 info about coord 0
        MPI_Recv(&no_procs0, 1, MPI_INT, ROOT_2, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        v0 = (int*) malloc(no_procs0 * sizeof(int));
        MPI_Recv(v0, no_procs0, MPI_INT, ROOT_2, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        // build topology
        s += std::to_string(ROOT_0) + ":" + string_topo(no_procs0, v0);
        s += std::to_string(ROOT_1) + ":" + string_topo(no_procs1, v1);
        s += std::to_string(ROOT_2) + ":" + string_topo(no_procs2, v2);

        str = (char*)s.c_str();
        int len = s.length();

        // print topology
        printf("%d -> %s\n", rank, str);

        // Send to workers
        for (int i = 0; i < no_procs1; ++i) {
            // coord rank
            MPI_Send(&rank, 1, MPI_INT, v1[i], 0, MPI_COMM_WORLD);
            // topology as string
            MPI_Send(&len, 1, MPI_INT, v1[i], 0, MPI_COMM_WORLD);
            MPI_Send(str, len + 1, MPI_CHAR, v1[i], 0, MPI_COMM_WORLD);

            // print message info
            printf("M(%d,%d)\n", rank, v1[i]);
        }
        
        // Task 2

        // Receives from coord 2 the part of array for coord 1
        MPI_Recv(&v_len, 1, MPI_INT, ROOT_2, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        v = (int*) malloc(v_len * sizeof(int));
        MPI_Recv(v, v_len, MPI_INT, ROOT_2, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        // set dimension of array to be processed by workers
        dim = v_len / no_procs1;

        plus_size = 0; // offset to know where to read from

        // Send arrays to workers
        for (int i = 0; i < no_procs1; ++i) {
            if (dim < v_len - (no_procs1 - 1)* dim) {
                dim = v_len - (no_procs1 - 1)* dim;
            }

            MPI_Send(&dim, 1, MPI_INT, v1[i], 0, MPI_COMM_WORLD);
            MPI_Send(v + plus_size, dim, MPI_INT, v1[i], 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, v1[i]);

            // Receive array after processing
            MPI_Recv(v + plus_size, dim, MPI_INT, v1[i], 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            plus_size += dim;
        }

        // Send part of array processed by cluster 1 to coord 2
        MPI_Send(&v_len, 1, MPI_INT, ROOT_2, 0, MPI_COMM_WORLD);
        MPI_Send(v, v_len, MPI_INT, ROOT_2, 0, MPI_COMM_WORLD);
        printf("M(%d,%d)\n", rank, ROOT_2);

        fclose(f2); // close file
 
        // Free allocated memory
        free(v);
        free(v0);
        free(v1);
        free(v2);
    }
    // coordinator 2
    else if(rank == ROOT_2) {
        int val, v_len, len1, dim, plus_size;
        int* v_part1;

        FILE* f3 = fopen("cluster2.txt", "rt"); // open file to read
        fscanf(f3, "%d", &no_procs2);
        v2 = (int*) malloc(no_procs2 * sizeof(int));
        for (int i = 0; i < no_procs2; ++i) {
            fscanf(f3, "%d", &val);
            v2[i] = val;
        }

        // Send info to coord 0
        MPI_Send(&no_procs2, 1, MPI_INT, ROOT_0, 0, MPI_COMM_WORLD);
        MPI_Send(v2, no_procs2, MPI_INT, ROOT_0, 0, MPI_COMM_WORLD);
        printf("M(%d,%d)\n", rank, ROOT_0);

        // Send info to coord 1
        MPI_Send(&no_procs2, 1, MPI_INT, ROOT_1, 0, MPI_COMM_WORLD);
        MPI_Send(v2, no_procs2, MPI_INT, ROOT_1, 0, MPI_COMM_WORLD);
        printf("M(%d,%d)\n", rank, ROOT_1);

        // Receive from coord 0 info about it
        MPI_Recv(&no_procs0, 1, MPI_INT, ROOT_0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        v0 = (int*) malloc(no_procs0 * sizeof(int));
        MPI_Recv(v0, no_procs0, MPI_INT, ROOT_0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        // Receive from coord 1 info about it
        MPI_Recv(&no_procs1, 1, MPI_INT, ROOT_1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        v1 = (int*) malloc(no_procs1 * sizeof(int));
        MPI_Recv(v1, no_procs1, MPI_INT, ROOT_1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        // Coord 2 as intermediate
        // Send to coord 1 info about coord 0
        MPI_Send(&no_procs0, 1, MPI_INT, ROOT_1, 0, MPI_COMM_WORLD);
        MPI_Send(v0, no_procs0, MPI_INT, ROOT_1, 0, MPI_COMM_WORLD);
        printf("M(%d,%d)\n", rank, ROOT_1);

        // Send to coord 0 info about coord 1
        MPI_Send(&no_procs1, 1, MPI_INT, ROOT_0, 0, MPI_COMM_WORLD);
        MPI_Send(v1, no_procs1, MPI_INT, ROOT_0, 0, MPI_COMM_WORLD);
        printf("M(%d,%d)\n", rank, ROOT_0);

        // BUild topology
        s += std::to_string(ROOT_0) + ":" + string_topo(no_procs0, v0);
        s += std::to_string(ROOT_1) + ":" + string_topo(no_procs1, v1);
        s += std::to_string(ROOT_2) + ":" + string_topo(no_procs2, v2);

        str = (char*)s.c_str();
        int len = s.length();

        // Print topology
        printf("%d -> %s\n", rank, str);

        // Send to workers
        for (int i = 0; i < no_procs2; ++i) {
            // coord rank
            MPI_Send(&rank, 1, MPI_INT, v2[i], 0, MPI_COMM_WORLD);

            // topology as string
            MPI_Send(&len, 1, MPI_INT, v2[i], 0, MPI_COMM_WORLD);
            MPI_Send(str, len + 1, MPI_CHAR, v2[i], 0, MPI_COMM_WORLD);

            // print message
            printf("M(%d,%d)\n", rank, v2[i]);
        }

        // Receive part of array to be processed by coord 1
        MPI_Recv(&len1, 1, MPI_INT, ROOT_0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        v_part1 = (int*) malloc(len1 * sizeof(int));
        MPI_Recv(v_part1, len1, MPI_INT, ROOT_0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        // Send to coord 1 his part
        MPI_Send(&len1, 1, MPI_INT, ROOT_1, 0, MPI_COMM_WORLD);
        MPI_Send(v_part1, len1, MPI_INT, ROOT_1, 0, MPI_COMM_WORLD);

        // Receive my part of array
        MPI_Recv(&v_len, 1, MPI_INT, ROOT_0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        v = (int*) malloc(v_len * sizeof(int));
        MPI_Recv(v, v_len, MPI_INT, ROOT_0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        // set dimension of array to be processed by workers
        dim = v_len / no_procs2;

        plus_size = 0;  // offset to read from

        // Send arrays to workers
        for (int i = 0; i < no_procs2; ++i) {
            if (dim < v_len - (no_procs2 - 1)* dim) {
                dim = v_len - (no_procs2 - 1)* dim;
            }

            MPI_Send(&dim, 1, MPI_INT, v2[i], 0, MPI_COMM_WORLD);
            MPI_Send(v + plus_size, dim, MPI_INT, v2[i], 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, v2[i]);

            // receive processed array
            MPI_Recv(v + plus_size, dim, MPI_INT, v2[i], 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            plus_size += dim;
        }

        // Receive processed array from cluster 1 
        MPI_Recv(&len1, 1, MPI_INT, ROOT_1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        v_part1 = (int*) malloc(len1 * sizeof(int));
        MPI_Recv(v_part1, len1, MPI_INT, ROOT_1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        // Send to coord 0 the part of array processed by cluster 1
        MPI_Send(v_part1, len1, MPI_INT, ROOT_0, 0, MPI_COMM_WORLD);
        printf("M(%d,%d)\n", rank, ROOT_0);

        // Send to coord 0 the part processed by me
        MPI_Send(v, v_len, MPI_INT, ROOT_0, 0, MPI_COMM_WORLD);
        printf("M(%d,%d)\n", rank, ROOT_0);

        fclose(f3); // close file

        // Free allocated memory
        free(v);
        free(v_part1);
        free(v0);
        free(v1);
        free(v2);
    }
    // Worker
    else {
        int coord_rank;
        int len, dim;

        // Receive rank of the coordinator
        MPI_Recv(&coord_rank, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        
        // Receive topology from coordinator
        MPI_Recv(&len, 1, MPI_INT, coord_rank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        str = (char*) malloc((len + 1) * sizeof(char));
        MPI_Recv(str, len + 1, MPI_CHAR, coord_rank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        printf("%d -> %s\n", rank, str);

        // Receive array to change
        MPI_Recv(&dim, 1, MPI_INT, coord_rank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        v = (int*) malloc(dim * sizeof(int));
        MPI_Recv(v, dim, MPI_INT, coord_rank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        // Iterate through "my part" and double the values
        for (int i = 0; i < dim; ++i) {
            v[i] *= 2;
        }

        // Send the updated array to the coordinator
        MPI_Send(v, dim, MPI_INT, coord_rank, 0, MPI_COMM_WORLD);
        printf("M(%d,%d)\n", rank, coord_rank);

        // free memory
        free(str);
        free(v);
    }

    MPI_Finalize();
    return 0;
}
