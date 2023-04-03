# Matrix vector implementation where matrix is in CSR format

## Code structure
The code is written in main.c
The input.txt file is for providing input
The output.txt file stores the output
The log.txt file stores some intermediate execution logs

## Input format:
The first line is m the matrix dimension
The second line should have m space separated components of the input vector
The third line should have row index for matrix in CSR format
The fourth line should have the col index for matrix in CSR format
The fiveth line should have corresponding the non-zero values
The input in lines 2-5 should be space separated

## Output:
All m components of output vector space separated

## Code compile and Run:
`mpicc main.c`
`mpirun -np <number_of_processes> ./a.out`
eg: `mpirun -np 6 ./a.out`

The input provided is once printed in console and also the output

All the necessary comments have been provided in the code