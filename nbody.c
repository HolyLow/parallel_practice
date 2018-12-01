#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h> 
#include <pthread.h>

#include "fio.h"

// #define G 0.56
typedef struct Task {
  int tid;
  int thread_num;
  int64_t size;
  int32_t T;
  double *pbd;
  double *pfc;
  pthread_barrier_t *barrier;
}Task;

void *nbody(void *arg) {
  Task task = *((Task*)arg);
  int32_t tid = task.tid;
  int thread_num = task.thread_num;
  int64_t size = task.size;
  int32_t T = task.T;
  double *pbd = task.pbd;
  double *pfc = task.pfc;
  pthread_barrier_t *barrier = task.barrier;
  // parameters for stage 1 and stage 3, divide task into thread_num blocks
  int64_t host_seg = (size + thread_num - 1) / thread_num;
  int64_t host_off = host_seg * tid;
  int64_t host_end = (host_seg + host_off <= size) ? (host_seg + host_off) : size;

  // parameters for stage 2, divide task into 2*thread_num blocks
  int32_t stage2_seg_num = 2 * thread_num;
  int64_t master_seg = (size + stage2_seg_num - 1) / stage2_seg_num;
  // int32_t round = stage2_seg_num - 1;

  int32_t t = 0;
  int64_t i, j;
  while (t < T) {
    // stage 1
    for (i = host_off; i < host_end; ++i) {
      int64_t bi = (i<<2);              
      int64_t fi = bi - i; 
      for (j = i + 1; j < host_end; ++j) {
        int64_t bj = (j<<2);
        int64_t fj = bj - j;
        
        double dx = pbd[bi+1] - pbd[bj+1];
        double dy = pbd[bi+2] - pbd[bj+2];
        double dz = pbd[bi+3] - pbd[bj+3];
        double sq = dx*dx + dy*dy + dz*dz;
        double dist = sqrt(sq);
        double fac = G * pbd[bi] * pbd[bj] / ( dist * sq );
        double fx = fac * dx;
        double fy = fac * dy;
        double fz = fac * dz;
        
        pfc[fi] -= fx;
        pfc[fi+1] -= fy;
        pfc[fi+2] -= fz;
        pfc[fj] += fx;
        pfc[fj+1] += fy;
        pfc[fj+2] += fz;
      }
    }
    // synchronize all the threads in the end of stage 1
    pthread_barrier_wait(barrier);

    // stage 2
    for (int32_t round = 1; round < thread_num; ++round) {
      // decide the thread's master block and left&right slave blocks
      int32_t master_id;
      int32_t step = round * 2;
      if (step != 2 && stage2_seg_num % step == 0) {
        int32_t step_num = stage2_seg_num / step;
        int32_t circle_num = thread_num / step_num;
        int32_t circle_id = tid / circle_num;
        int32_t circle_off = tid % circle_num;
        master_id = (circle_id + circle_off * step) % stage2_seg_num;
      }
      else {
        master_id = (step * tid) % stage2_seg_num;
      }
      int32_t lslave_id = (master_id - round + stage2_seg_num) % stage2_seg_num;
      int32_t rslave_id = (master_id + round) % stage2_seg_num;

      // int64_t host_seg = (size + thread_num - 1) / thread_num;
      int64_t master_off = master_seg * master_id;
      int64_t master_end = (master_seg + master_off <= size) 
                         ? (master_seg + master_off) : size;
      int64_t lslave_off = master_seg * lslave_id;
      int64_t lslave_end = (master_seg + lslave_off <= size) 
                         ? (master_seg + lslave_off) : size;
      int64_t rslave_off = master_seg * rslave_id;
      int64_t rslave_end = (master_seg + rslave_off <= size) 
                         ? (master_seg + rslave_off) : size;
      for (i = master_off; i < master_end; ++i) {
        int64_t bi = (i<<2);              
        int64_t fi = bi - i; 
        for (j = lslave_off; j < lslave_end; ++j) {
          int64_t bj = (j<<2);
          int64_t fj = bj - j;
          
          double dx = pbd[bi+1] - pbd[bj+1];
          double dy = pbd[bi+2] - pbd[bj+2];
          double dz = pbd[bi+3] - pbd[bj+3];
          double sq = dx*dx + dy*dy + dz*dz;
          double dist = sqrt(sq);
          double fac = G * pbd[bi] * pbd[bj] / ( dist * sq );
          double fx = fac * dx;
          double fy = fac * dy;
          double fz = fac * dz;
          
          pfc[fi] -= fx;
          pfc[fi+1] -= fy;
          pfc[fi+2] -= fz;
          pfc[fj] += fx;
          pfc[fj+1] += fy;
          pfc[fj+2] += fz;
        }
      }
      // synchronize all the threads in each round, after lslave
      pthread_barrier_wait(barrier);

      for (i = master_off; i < master_end; ++i) {
        int64_t bi = (i<<2);              
        int64_t fi = bi - i; 
        for (j = rslave_off; j < rslave_end; ++j) {
          int64_t bj = (j<<2);
          int64_t fj = bj - j;
          
          double dx = pbd[bi+1] - pbd[bj+1];
          double dy = pbd[bi+2] - pbd[bj+2];
          double dz = pbd[bi+3] - pbd[bj+3];
          double sq = dx*dx + dy*dy + dz*dz;
          double dist = sqrt(sq);
          double fac = G * pbd[bi] * pbd[bj] / ( dist * sq );
          double fx = fac * dx;
          double fy = fac * dy;
          double fz = fac * dz;
          
          pfc[fi] -= fx;
          pfc[fi+1] -= fy;
          pfc[fi+2] -= fz;
          pfc[fj] += fx;
          pfc[fj+1] += fy;
          pfc[fj+2] += fz;
        }
      }
      // synchronize all the threads in each round, after rslave
      pthread_barrier_wait(barrier);
    }

    // stage 3, update the host block
    for (i = host_off; i < host_end; ++i) { 
      int64_t bi = (i<<2);              
      int64_t fi = bi - i;            
      pbd[bi+1] = pbd[bi+1] + pfc[fi] / pbd[bi]; 
			
      pfc[fi] = 0;
      pbd[bi+2] = pbd[bi+2] + pfc[fi+1] / pbd[bi];
      pfc[fi+1] = 0;
      pbd[bi+3] = pbd[bi+3] + pfc[fi+2] / pbd[bi];
      pfc[fi+2] = 0;
    }
    ++t;
  }
  
}
                                   
int main(int argc, char** argv) {     
  int64_t i, j;

  int32_t thread_num = atoi(argv[1]);
  int64_t size = atoll(argv[2]);
  int32_t T = atoi(argv[3]);

  size = ((int64_t)1<<size);
  double *pbd = (double*)malloc(4*size*sizeof(double));
  input_data(pbd, 4*size*sizeof(double));
   
  double *pfc = (double*)malloc(3*size*sizeof(double));
  memset(pfc, 0, 3*size*sizeof(double));

  pthread_t *tid = (pthread_t*)malloc(thread_num * sizeof(pthread_t));
  Task *task = (Task*)malloc(thread_num * sizeof(Task));

  pthread_barrier_t barrier;
  pthread_barrier_init(&barrier, NULL, thread_num);
  for (i = 0; i < thread_num; ++i) {
    task[i].tid = i;
    task[i].thread_num = thread_num;
    task[i].pbd = pbd;
    task[i].pfc = pfc;
    task[i].T = T;
    task[i].size = size;
    task[i].barrier = &barrier;
    pthread_create(&tid[i], NULL, nbody, &task[i]);
  }
  for (i = 0; i < thread_num; ++i) {
    pthread_join(tid[i], NULL);
  }
  
  output_data(pbd, 4*size*sizeof(double));
  free(pbd);
  free(pfc);
  
  return EXIT_SUCCESS;
}
