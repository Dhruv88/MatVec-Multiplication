#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"

int main( int argc, char *argv[] )
{
    
    // Declare Variables that will be used by all processes
    int num_proc;
    int root_rank = 0;
    int proc_rank;
    int alloted_rows;
    int *row_per_proc, *displacements;
    int m;
    int *proc_res;
    int *V;
    FILE *ptr,*log, *out;
    ptr = fopen("input.txt", "r");
    log = fopen("log.txt", "w");
    MPI_Init(&argc,&argv);
    MPI_Comm_size(MPI_COMM_WORLD, &num_proc);
    MPI_Comm_rank(MPI_COMM_WORLD, &proc_rank);
    // Get the inputs and broadcast them from root to all other processes
    if(proc_rank == root_rank)
    {
        // m=6;
        fscanf(ptr, "%d", &m);
    }
    MPI_Bcast(&m, 1, MPI_INT, 0, MPI_COMM_WORLD); 
    V = (int*)malloc(sizeof(int)*m);
    if(proc_rank == root_rank)
    {
        // V[0] = 1,V[1] = -1,V[2] = 1, V[3] = 2, V[4] = -2, V[5] = 1;
        for(int i=0;i<m;i++)
            fscanf(ptr, "%d", &V[i]);
        printf("Input Vector:\n");
        for(int i=0;i<m;i++)
            printf("%d ",V[i]);
        printf("\n");
    }
    MPI_Bcast(V, m, MPI_INT, 0, MPI_COMM_WORLD);
    // Divide the rows equally among all processes
    int rpp = m/num_proc;
    int np0 = num_proc - (m % num_proc); 	// Number of processes with rows = rpp (np0),
	int np1 = num_proc - np0;
    if(proc_rank >= np1)
    {
        alloted_rows = rpp;
    }
    else
    {
        alloted_rows = rpp+1;
    }
    // Allot the required space for compute
    int *rows_allocated = (int*)malloc(sizeof(int)*alloted_rows+1);
    int *cols_allocated = (int*)malloc(sizeof(int)*m*alloted_rows);
    int *vals_allocated = (int*)malloc(sizeof(int)*m*alloted_rows);
    // Take in the matrix input and print it and Distribute the rows using scatterv
    if(proc_rank == root_rank)
    {
        // int M[] = { 5, 8, 3, 6, 5, 1};
        // int col_index[] = { 0, 1, 2, 1, 3, 5 };
        // int row_index[] = { 0, 1, 2, 3, 4, 5, 6};
        int *row_index = (int*)malloc(sizeof(int)*(m+1));
        for(int i=0;i<m+1;i++)
            fscanf(ptr, "%d", &row_index[i]);
        int *col_index = (int*)malloc(sizeof(int)*(row_index[m]));
        for(int i=0;i<row_index[m];i++)
            fscanf(ptr, "%d", &col_index[i]);
        int *M = (int*)malloc(sizeof(int)*(row_index[m]));
        for(int i=0;i<row_index[m];i++)
            fscanf(ptr, "%d", &M[i]);
        printf("Row Index:\n");
        for(int i=0;i<m+1;i++)
            printf("%d ",row_index[i]);
        printf("\n");
        printf("Col Index:\n");
        for(int i=0;i<row_index[m];i++)
            printf("%d ",col_index[i]);
        printf("\n");
        printf("Values:\n");
        for(int i=0;i<row_index[m];i++)
            printf("%d ",M[i]);
        printf("\n");
        row_per_proc = (int*)malloc(sizeof(int)*num_proc);
        int *val_per_proc = (int*)malloc(sizeof(int)*num_proc);
        displacements = (int*)malloc(sizeof(int)*num_proc);
        int *displacements2 = (int*)malloc(sizeof(int)*num_proc);
        displacements[0] = 0;
        displacements2[0] = 0;
        for(int i=0;i<num_proc;i++)
        {
            if(i<np1)
                row_per_proc[i] = rpp+2;
            else    
                row_per_proc[i] = rpp+1;
            if(i>0)
                displacements[i] = displacements[i-1] + row_per_proc[i-1] - 1;
        }

        for(int i=0;i<num_proc;i++)
        {
            displacements2[i] = row_index[displacements[i]];
            if(i>0)
                val_per_proc[i-1] = displacements2[i] - displacements2[i-1];
            if(val_per_proc[i-1]==0) val_per_proc[i-1]=1; //avoid scatterv bug
        }

        val_per_proc[num_proc-1] = row_index[m] - displacements2[num_proc-2];
        if(val_per_proc[num_proc-1]==0) val_per_proc[num_proc-1]=1; //avoid scatterv bug
        fprintf(log, "send row:%d\n",MPI_Scatterv(row_index, row_per_proc, displacements, MPI_INT, rows_allocated, m+1, MPI_INT, root_rank, MPI_COMM_WORLD));
        fprintf(log, "send col:%d\n",MPI_Scatterv(col_index, val_per_proc, displacements2, MPI_INT, cols_allocated, m*alloted_rows, MPI_INT, root_rank, MPI_COMM_WORLD));
        fprintf(log, "send val:%d\n",MPI_Scatterv(M, val_per_proc, displacements2, MPI_INT, vals_allocated, m*alloted_rows, MPI_INT, root_rank, MPI_COMM_WORLD));
        free(displacements2);
    }
    else
    {
        // receive the rows from the root process
        fprintf(log, "rec row:%d\n",MPI_Scatterv(NULL, NULL, NULL, MPI_INT, rows_allocated, m+1, MPI_INT, root_rank, MPI_COMM_WORLD));
        if(alloted_rows>0)
        {
            fprintf(log, "rec col:%d\n",MPI_Scatterv(NULL, NULL, NULL, MPI_INT, cols_allocated, m*alloted_rows, MPI_INT, root_rank, MPI_COMM_WORLD));
            fprintf(log, "rec val:%d\n",MPI_Scatterv(NULL, NULL, NULL, MPI_INT, vals_allocated, m*alloted_rows, MPI_INT, root_rank, MPI_COMM_WORLD));
        }
    }
    fclose(ptr);
    int alloted_vals = sizeof(vals_allocated)/sizeof(vals_allocated[0]);
    // calulate the respective element of the output vector
    if(alloted_rows>0)
    {
        proc_res = (int *)calloc(alloted_rows,sizeof(int));
        int j=0;
        for(int i=1;i<alloted_rows+1;i++)
        {
            int num_vals = rows_allocated[i] - rows_allocated[i-1];
            while(num_vals)
            {
                proc_res[i-1] += vals_allocated[j]*V[cols_allocated[j]];
                j++,num_vals--;
            }
        }
    }


    // Collect the results from all processes using gatherv
    if(proc_rank==root_rank)
    {
        int *Z = calloc(m,sizeof(int));
        fprintf(log, "rec res:%d\n",MPI_Gatherv(proc_res, alloted_rows, MPI_INT, Z, row_per_proc, displacements, MPI_INT, root_rank, MPI_COMM_WORLD));
        printf("Output Vector:\n");
        for(int i=0;i<m;i++)
            printf("%d ",Z[i]);
        printf("\n");
        out = fopen("output.txt", "w");
        for(int i=0;i<m;i++)
            fprintf(out,"%d ",Z[i]);
        fprintf(out,"\n");
        fclose(out);
        free(Z);
        free(displacements);
        free(row_per_proc);
    }
    else if(alloted_rows == 0)
    {
        int zero = 0;
        fprintf(log, "send res:%d\n",MPI_Gatherv(&zero, 1, MPI_INT, NULL, NULL, NULL, MPI_INT, root_rank, MPI_COMM_WORLD));
    }
    else
    {
        fprintf(log, "send res:%d\n",MPI_Gatherv(proc_res, alloted_rows, MPI_INT, NULL, NULL, NULL, MPI_INT, root_rank, MPI_COMM_WORLD));
    }
    fprintf(log, "here1");

    // Free all the dynamic memory used
    if(alloted_rows>0)
    {
        free(V);
        free(proc_res);
        free(rows_allocated);
        free(cols_allocated);
        free(vals_allocated);
    }
    fprintf(log,"here");
    fprintf(log,"end:%d\n",MPI_Finalize());
    fclose(log);
    return 0;
}