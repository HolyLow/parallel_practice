#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <pthread.h>
#include "fio.h"

#define Dtype float
#define Itype long long int
#define Ltype long long int
typedef struct Task {
  Itype length;
  Dtype *a;
  Dtype *b;
  Itype row;
  Itype col;
  Itype off;
}Task;

void *_2d5p(void *arg) {
  Task task = *((Task*)arg);
  Itype i, j, k, end = task.length + task.off;
  if (end == task.row) end -= 1;
  Itype rowid = task.off;
  if (rowid == 0) {
    for (i = 0; i < task.col; ++i) {
      task.b[i] = task.a[i];
    }
    ++rowid;
  }
  Itype pre = (rowid - 1) * task.col;
  Itype cur = pre + task.col;
  Itype nxt = cur + task.col;
  for (; rowid < end; ++rowid) {
    task.b[cur] = task.a[cur];
    for (i = 1; i < task.col - 1; ++i) {
      task.b[cur + i] = (task.a[cur + i] + task.a[cur + i - 1]
                      + task.a[cur + i + 1] + task.a[pre + i]
                      + task.a[nxt + i]) / 5.0;
    }
    task.b[cur + task.col - 1] = task.a[cur + task.col - 1];
    pre = cur;
    cur = nxt;
    nxt += task.col;
  }
  if (rowid == task.row - 1) {
    for (i = 0; i < task.col; ++i) {
      task.b[cur + i] = task.a[cur + i];
    }
    ++rowid;
  }
}

int main(int argc, char *argv[]) {
  Dtype *a, *b;
  Itype n, p, m;

  // get the args
  p = atoi(argv[1]);
  n = atoi(argv[2]);
  m = atoi(argv[3]);
  Ltype size = 1 << (m + n);
  n = 1 << n;
  m = 1 << m;

  // input the arrays
  Ltype bytesize = size * sizeof(Dtype);
  a = (Dtype*)malloc(bytesize);
  b = (Dtype*)malloc(bytesize);

  Ltype returned_size = input_data((void*)a, bytesize);
  if (returned_size != bytesize) {
    printf("wrong returned input size : %lld\n", returned_size);
    return 1;
  }

  // create threads and assign tasks
  Itype i, j, k;
  int thread_num = p;
  int total_size = n;
  pthread_t *tid = (pthread_t*)malloc(thread_num * sizeof(pthread_t));
  Task *task = (Task*)malloc(thread_num * sizeof(Task));
  Itype tsize = (total_size + thread_num - 1) / thread_num;
  Itype acc = 0;
  for (i = 0; i < thread_num; ++i) {
    task[i].length = (tsize * (i + 1) <= total_size) ? tsize
                                                     : (total_size - tsize * i);
    task[i].off = acc;
    acc += task[i].length;
    task[i].a = a;
    task[i].b = b;
    task[i].row = n;
    task[i].col = m;
    pthread_create(&tid[i], NULL, _2d5p, &task[i]);
  }
  for (i = 0; i < thread_num; ++i) {
    pthread_join(tid[i], NULL);
  }

  // output the arrays
  returned_size = output_data((void*)b, bytesize);
  if (returned_size != bytesize) {
    printf("wrong returned output size : %lld\n", returned_size);
    return 1;
  }
  free(a); free(b);
  free(tid); free(task);
}
