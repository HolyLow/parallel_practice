#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <pthread.h>
#include "fio.h"

typedef struct Task {
	int length;
	float *a;
	float *b;
	float alpha;
	float *c;
}Task;


void *saxpy(void *arg) {
	Task task = *((Task*)arg);
	int i;
	for (i = 0; i < task.length; ++i) {
		task.c[i] = task.a[i] * task.alpha + task.b[i];
	}
}

int main(int argc, char *argv[]) {
	float *a, *b;
	int n, p;
	float alpha;

	// get the args
	p = atoi(argv[1]);
	n = atoi(argv[2]);
	alpha = atof(argv[3]);

	// input the arrays
	int size = (1 << n);
	int bytesize = size * sizeof(float); 
	a = (float*)malloc(bytesize);
	b = (float*)malloc(bytesize);

	int returned_size = input_data((void*)a, bytesize, (void*)b, bytesize);
	if (returned_size != bytesize * 2) {
		printf("wrong returned input size : %d\n", returned_size);
		return 1;
	}

	// create threads and assign tasks
	int thread_num = p;
	int i, j, k, begin, end;
	pthread_t *tid = (pthread_t*)malloc(thread_num * sizeof(pthread_t));
	Task *task = (Task*)malloc(thread_num * sizeof(Task));
	int tsize = (size + thread_num - 1) / thread_num;
	for (i = 0; i < thread_num; ++i) {
		begin = tsize * i;
		task[i].a = &a[begin];
		task[i].b = &b[begin];
		task[i].c = &b[begin];
		task[i].alpha = alpha;
		end = tsize * (i + 1);
		task[i].length = (end <= size) ? tsize : (size - begin);
		pthread_create(&tid[i], NULL, saxpy, &task[i]);
	}
	for (i = 0; i < thread_num; ++i) {
		pthread_join(tid[i], NULL);
	}

	// output the arrays
	returned_size = output_data((void*)b, bytesize);
	if (returned_size != bytesize) {
		printf("wrong returned output size : %d\n", returned_size);
		return 1;
	}
	free(a); free(b); free(tid); free(task);
}
