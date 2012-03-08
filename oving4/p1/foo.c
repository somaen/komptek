#include <stdio.h>
#include <stdlib.h>

int foo(int N) {
	int sum = 0;
	for (int i = 1; i < N; i++) {
		if (i % 3 == 0 || i % 5 == 0) {
			sum += 1;
		}
	}
	return sum;
}

int main(int argc, char ** argv) {
	int N = atoi(argv[1]);
	int ans = foo(N);
	printf("Sum is %d.\n", ans);

	return 0;
}

