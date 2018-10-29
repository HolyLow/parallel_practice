#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <pthread.h>
#include "fio.h"

typedef struct Task {
  long long int length;
  long long int *a;
  long long int *b;
  long long int *acc_b;
  long long int *f;
  long long int *acc_f;
}Task;

int compare_long(const void* a, const void* b)
{
  long long int arg1 = *(const long long int*)a;
  long long int arg2 = *(const long long int*)b;
  if (arg1 < arg2) return -1;
  if (arg1 > arg2) return 1;
  return 0;
}

void *concat(void *arg) {
  Task task = *((Task*)arg);
  long long int i;
  long long int pos_a, array_length, pos_f, length_f, array_id;
  for (i = 0; i < task.length; ++i) {
    pos_a = task.acc_b[i];
    array_length = task.b[2 * i + 1];
    array_id = task.b[2 * i];
    qsort((void*)&task.a[pos_a], array_length,
          sizeof(long long int), compare_long);
    pos_f = task.acc_f[array_id];
    memcpy((void*)&task.f[pos_f], (const void*)&task.a[pos_a],
        array_length * sizeof(long long int));
  }
}

int main(int argc, char *argv[]) {
  long long int *a, *b;
  int n, p, m;

  // get the args
  p = atoi(argv[1]);
  n = atoi(argv[2]);
  m = atoi(argv[3]);
  m = m + 1;
  long long int fnum = (1 << (m - 1));

  // input the arrays
  long long int sa = (1 << n) * sizeof(long long int);
  long long int sb = (1 << m) * sizeof(long long int);
  long long int bytesize = sa + sb;
  a = (long long int*)malloc(sa);
  b = (long long int*)malloc(sb);

  long long int returned_size = input_data((void*)a, sa, (void*)b, sb);
  if (returned_size != bytesize) {
    printf("wrong returned input size : %lld\n", returned_size);
    return 1;
  }

  long long int *f = (long long int*)malloc(fnum * sizeof(long long int));
  long long int *acc_b = (long long int*)malloc(fnum * sizeof(long long int));
  long long int *acc_f = (long long int*)malloc(fnum * sizeof(long long int));
  long long int *val_f = (long long int*)malloc(sa);

  // reform the arrays
  long long int i, j, k, acc;
  acc = 0;
  for (i = 0; i < fnum; ++i) {
    f[b[2 * i]] = b[2 * i + 1];
    acc_b[i] = acc;
    acc += b[2 * i + 1];
  }
  acc = 0;
  for (i = 0; i < fnum; ++i) {
    acc_f[i] = acc;
    acc += f[i];
  }

  // create threads and assign tasks
  int thread_num = p;
  pthread_t *tid = (pthread_t*)malloc(thread_num * sizeof(pthread_t));
  Task *task = (Task*)malloc(thread_num * sizeof(Task));
  int tsize = (fnum + thread_num - 1) / thread_num;
  for (i = 0; i < thread_num; ++i) {
    task[i].length = (tsize * (i + 1) <= fnum) ? tsize : (fnum - tsize * i);
    task[i].a = a;
    task[i].b = &b[tsize * i * 2];
    task[i].acc_b = &acc_b[tsize * i];
    task[i].f = val_f;
    task[i].acc_f = acc_f;
    pthread_create(&tid[i], NULL, concat, &task[i]);
  }
  for (i = 0; i < thread_num; ++i) {
    pthread_join(tid[i], NULL);
  }

  // output the arrays
  returned_size = output_data((void*)val_f, sa);
  if (returned_size != sa) {
    printf("wrong returned output size : %lld\n", returned_size);
    return 1;
  }
  free(a); free(b); free(acc_b); free(f); free(acc_f); free(val_f);
  free(tid); free(task);
}
