#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <time.h>

int main(int argc, char **argv)
{
	int flag = 1;
	unsigned long int i, numb, res;
	double elapsed_time, wall_timer;

	puts("Number:");
	scanf("%ld", &numb);

	wall_timer = omp_get_wtime();
	#pragma omp parallel for private(numb, res)
	for (i = 3; i < numb; i++) {
		res = numb % i;
		if (res == 0) {
			#pragma omp critical
			flag = 0;
		}
	}

	elapsed_time = omp_get_wtime() - wall_timer;

	if (flag) {
		printf("Number %lu prime\n", numb);
	} else {
		printf("Number %lu not prime\n", numb);
	}
	printf("Elapsed time: %.15f\n", elapsed_time);

	exit(EXIT_SUCCESS);
}

