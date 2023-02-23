#include <assert.h>
#include <getopt.h>
#include <math.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>

#include <shmem.h>

#include "wctimer.h"

//#define DEBUG
#ifdef DEBUG
#define dbg_printf(...) dbg_printf_real(__VA_ARGS__)
#else
#define dbg_printf(...)
#endif
#define dprintf(...) dbg_printf_real(__VA_ARGS__)

#define N 10000
#define G 6.67e-11
#define TIMESTEP 0.25
#define NSTEPS 10

// types
struct body_s {
  double x;
  double y;
  double z;
  double dx;
  double dy;
  double dz;
  double mass;
};
typedef struct body_s body_t;

struct global_s {
  int rank;
  int nproc;                  // # of parallel processes
  int n;                      // N = number of bodies in simulation
  int nsteps;                 // # of timesteps to run simulation
};
typedef struct global_s global_t;


/*
 *  global data structure
 */
global_t g;


/*
 *  prototypes
 */
int    dbg_printf_real(const char *format, ...);
int    eprintf(const char *format, ...);
void   print_body(body_t *b);
static inline double dist(double dx, double dy, double dz);

/**
 * dist - computes distance beteween two bodies
 *  @param dx x-coordinate difference
 *  @param dy y-coordinate difference
 *  @param dz z-coordinate difference
 *  @returns distance magnitude
 */
static inline double dist(double dx, double dy, double dz) {
  return sqrt((dx*dx) + (dy*dy) + (dz*dz));;
}


/**
 *  dbg_printf - debug printing wrapper, only enabled if DEBUG defined
 *    @param format printf-like format string
 *    @param ... arguments to printf format string
 *    @return number of bytes written to stderr
 */
int dbg_printf_real(const char *format, ...) {
  va_list ap;
  int ret, len;
  char buf[1024], obuf[1024];

  va_start(ap, format);
  ret = vsprintf(buf, format, ap);
  va_end(ap);
  len = sprintf(obuf, "%4d: %s", g.rank, buf);
  write(STDOUT_FILENO, obuf, len);
  return ret;
}


/**
 *  eprintf - error printing wrapper
 *    @param format printf-like format string
 *    @param ... arguments to printf format string
 *    @return number of bytes written to stderr
 */
int eprintf(const char *format, ...) {
  va_list ap;
  int ret;
  char buf[1024];

  if (g.rank == 0) {
    va_start(ap, format);
    ret = vsprintf(buf, format, ap);
    va_end(ap);
    write(STDOUT_FILENO, buf, ret);
    return ret;
  }
  else
    return 0;

}



/**
 * print_body - prints the contents of a body
 *  @param b - body to print
 */
void print_body(body_t *b) {
  dprintf("x: %7.3f y: %7.3f z: %7.3f dx: %7.3f dy: %7.3f dz: %7.3f\n",
      b->x, b->y, b->z, b->dx, b->dy, b->dz);
}




int main(int argc, char **argv) {
  char c;
  wc_timer_t ttimer, itimer;

  memset(&g, 0, sizeof(g));    // zero out global data structure
  shmem_init();                // initialize OpenSHMEM

  g.nproc = shmem_n_pes();
  g.rank  = shmem_my_pe();

  wc_tsc_calibrate();

  g.n      = N;
  g.nsteps = NSTEPS;

  while ((c = getopt(argc, argv, "hn:t:")) != -1) {
    switch (c) {
      case 'h':
        eprintf("usage: lab3 [-n #bodies] [-t #timesteps]\n");
        shmem_finalize();
        exit(0);
        break;
      case 'n':
        g.n = atoi(optarg);
        break;
      case 't':
        g.nsteps = atoi(optarg);
        break;
    }
  }

  eprintf("beginning N-body simulation of %d bodies with %d processes over %d timesteps\n", g.n, g.nproc, g.nsteps);

  // fired up, ready to go
  WC_INIT_TIMER(ttimer);
  WC_START_TIMER(ttimer);

  for (int i=0; i<g.nproc; i++) {
    if (i == g.rank) {
      dprintf("hello from thread %d of %d\n", g.rank, g.nproc);
    }
    shmem_barrier_all();
  }

  shmem_barrier_all();
  WC_STOP_TIMER(ttimer);
  eprintf("execution time: %7.4f ms\n", WC_READ_TIMER_MSEC(ttimer));

  shmem_finalize(); // finalize OpenSHMEM
}
