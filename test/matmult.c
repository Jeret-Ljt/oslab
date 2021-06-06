/* matmult.c 
 *    Test program to do matrix multiplication on large arrays.
 *
 *    Intended to stress virtual memory system.
 *
 *    Ideally, we could read the matrices off of the file system,
 *	and store the result back to the file system!
 */

#include "syscall.h"

#define Dim 	3	/* sum total of the arrays doesn't fit in 
			 * physical memory 
			 */

int A[Dim][Dim] = {
	{2, 0, 0},
	{0, 2, 0},
	{0, 0, 2},
};
int C[Dim][Dim];
int
main()
{
    int i, j, k;

    for (i = 0; i < Dim; i++)		/* first initialize the matrices */
	for (j = 0; j < Dim; j++) {
	     C[i][j] = 0;
	}

    for (i = 0; i < Dim; i++)		/* then multiply them together */
	for (j = 0; j < Dim; j++)
            for (k = 0; k < Dim; k++)
		 C[i][j] += A[i][k] * A[k][j];

    Exit(C[0][0] + C[1][1] + C[2][2]);		/* and then we're done, should be 12*/
}
