/**
 *  \file producersConsumers.c (implementation file)
 *
 *  \brief Problem name: Producers / Consumers.
 *
 *  Synchronization based on monitors.
 *  Both threads and the monitor are implemented using the pthread library which enables the creation of a
 *  monitor of the Lampson / Redell type.
 *
 *  Generator thread of the intervening entities.
 *
 *  \author António Rui Borges - March 2019
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>
#include <time.h>

#include "probConst.h"
#include "fifo.h"

/** \brief producer threads return status array */
int statusProd[N];

/** \brief consumer threads return status array */
int statusCons[N];

/** \brief producer life cycle routine */
static void *producer (void *id);

/** \brief consumer life cycle routine */
static void *consumer (void *id);

/**
 *  \brief Main thread.
 *
 *  Its role is starting the simulation by generating the intervening entities threads (producers and consumers) and
 *  waiting for their termination.
 */

int main (void)
{
  pthread_t tIdProd[N],                                                        /* producers internal thread id array */
            tIdCons[N];                                                        /* consumers internal thread id array */
  unsigned int prod[N],                                             /* producers application defined thread id array */
               cons[N];                                             /* consumers application defined thread id array */
  int i;                                                                                        /* counting variable */
  int *status_p;                                                                      /* pointer to execution status */

  /* initializing the application defined thread id arrays for the producers and the consumers and the random number
     generator */

  double t0, t1;                                                                                      /* time limits */

  for (i = 0; i < N; i++)
    prod[i] = i;
  for (i = 0; i < N; i++)
    cons[i] = i;
  srandom ((unsigned int) getpid ());
  t0 = ((double) clock ()) / CLOCKS_PER_SEC;

  /* generation of intervening entities threads */

  for (i = 0; i < N; i++)
    if (pthread_create (&tIdProd[i], NULL, producer, &prod[i]) != 0)                              /* thread producer */
       { perror ("error on creating thread producer");
         exit (EXIT_FAILURE);
       }
  for (i = 0; i < N; i++)
    if (pthread_create (&tIdCons[i], NULL, consumer, &cons[i]) != 0)                             /* thread consumer */
       { perror ("error on creating thread consumer");
         exit (EXIT_FAILURE);
       }

  /* waiting for the termination of the intervening entities threads */

  printf ("\nFinal report\n");
  for (i = 0; i < N; i++)
  { if (pthread_join (tIdProd[i], (void *) &status_p) != 0)                                       /* thread producer */
       { perror ("error on waiting for thread producer");
         exit (EXIT_FAILURE);
       }
    printf ("thread producer, with id %u, has terminated: ", i);
    printf ("its status was %d\n", *status_p);
  }
  for (i = 0; i < N; i++)
  { if (pthread_join (tIdCons[i], (void *) &status_p) != 0)                                       /* thread consumer */
       { perror ("error on waiting for thread customer");
         exit (EXIT_FAILURE);
       }
    printf ("thread consumer, with id %u, has terminated: ", i);
    printf ("its status was %d\n", *status_p);
  }
  t1 = ((double) clock ()) / CLOCKS_PER_SEC;
  printf ("\nElapsed time = %.6f s\n", t1 - t0);

  exit (EXIT_SUCCESS);
}

/**
 *  \brief Function producer.
 *
 *  Its role is to simulate the life cycle of a producer.
 *
 *  \param par pointer to application defined producer identification
 */

static void *producer (void *par)
{
  unsigned int id = *((unsigned int *) par),                                                          /* producer id */
               val;                                                                                /* produced value */
  int i;                                                                                        /* counting variable */

  for (i = 0; i < M; i++)
  { val = 1000 * id + i;                                                                          /* produce a value */
    putVal (id, val);                                                                               /* store a value */
    usleep((unsigned int) floor (40.0 * random () / RAND_MAX + 1.5));                           /* do something else */
  }

  statusProd[id] = EXIT_SUCCESS;
  pthread_exit (&statusProd[id]);
}

/**
 *  \brief Function consumer.
 *
 *  Its role is to simulate the life cycle of a consumer.
 *
 *  \param par pointer to application defined consumer identification
 */

static void *consumer (void *par)
{
  unsigned int id = *((unsigned int *) par),                                                          /* consumer id */
               val;                                                                                /* produced value */
  int i;                                                                                        /* counting variable */

  for (i = 0; i < M; i++)
  { usleep((unsigned int) floor (40.0 * random () / RAND_MAX + 1.5));                           /* do something else */
    val = getVal (id);                                                                           /* retrieve a value */
    printf ("The value %u was produced by the thread P%u and consumed by the thread C%u.\n",      /* consume a value */
            val % 1000, val / 1000, id);
  }

  statusCons[id] = EXIT_SUCCESS;
  pthread_exit (&statusCons[id]);
}
