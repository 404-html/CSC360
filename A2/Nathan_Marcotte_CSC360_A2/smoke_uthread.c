/* Nathan Marcotte
 * CSC 360 Spring 2019
 * V00876934
 * smoke_uthread.c
 */

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include "uthread.h"
#include "uthread_mutex_cond.h"

#define NUM_ITERATIONS 1000

#ifdef VERBOSE
#define VERBOSE_PRINT(S, ...) printf (S, ##__VA_ARGS__);
#else
#define VERBOSE_PRINT(S, ...) ;
#endif

uthread_cond_t bac_mat;
uthread_cond_t pap_mat;
uthread_cond_t bac_pap;
int sum = 0;

struct Agent {
  uthread_mutex_t mutex;
  uthread_cond_t  match;
  uthread_cond_t  paper;
  uthread_cond_t  tobacco;
  uthread_cond_t  smoke;
};

struct Agent* createAgent() {
  struct Agent* agent = malloc (sizeof (struct Agent));
  agent->mutex   = uthread_mutex_create();
  agent->paper   = uthread_cond_create (agent->mutex);
  agent->match   = uthread_cond_create (agent->mutex);
  agent->tobacco = uthread_cond_create (agent->mutex);
  agent->smoke   = uthread_cond_create (agent->mutex);
  return agent;
}

//
// TODO
// You will probably need to add some procedures and struct etc.
//

/**
 * You might find these declarations helpful.
 *   Note that Resource enum had values 1, 2 and 4 so you can combine resources;
 *   e.g., having a MATCH and PAPER is the value MATCH | PAPER == 1 | 2 == 3
 */
enum Resource            {    MATCH = 1, PAPER = 2,   TOBACCO = 4};
char* resource_name [] = {"", "match",   "paper", "", "tobacco"};

int signal_count [5];  // # of times resource signalled
int smoke_count  [5];  // # of times smoker with resource smoked

/**
 * This is the agent procedure.  It is complete and you shouldn't change it in
 * any material way.  You can re-write it if you like, but be sure that all it does
 * is choose 2 random reasources, signal their condition variables, and then wait
 * wait for a smoker to smoke.
 */
void* agent (void* av) {
  struct Agent* a = av;
  static const int choices[]         = {MATCH|PAPER, MATCH|TOBACCO, PAPER|TOBACCO};
  static const int matching_smoker[] = {TOBACCO,     PAPER,         MATCH};
  
  uthread_mutex_lock (a->mutex);
    for (int i = 0; i < NUM_ITERATIONS; i++) {
      int r = random() % 3;
      signal_count [matching_smoker [r]] ++;
      int c = choices [r];
      if (c & MATCH) {
        VERBOSE_PRINT ("match available\n");
        uthread_cond_signal (a->match);
      }
      if (c & PAPER) {
        VERBOSE_PRINT ("paper available\n");
        uthread_cond_signal (a->paper);
      }
      if (c & TOBACCO) {
        VERBOSE_PRINT ("tobacco available\n");
        uthread_cond_signal (a->tobacco);
      }
      VERBOSE_PRINT ("agent is waiting for smoker to smoke\n");
      uthread_cond_wait (a->smoke);
    }
  uthread_mutex_unlock (a->mutex);
  return NULL;
}

void* get_smoker(int value){
  switch(value){
    case TOBACCO + PAPER:
      VERBOSE_PRINT("Get Match");
      uthread_cond_signal(bac_pap);
      sum = 0;
      break;
  
    case TOBACCO + MATCH:
      VERBOSE_PRINT("Get Paper");
      uthread_cond_signal(bac_mat);
      sum = 0;
      break;
  
    case MATCH + PAPER:
      VERBOSE_PRINT("Get Tobacco");
      uthread_cond_signal(pap_mat);
      sum = 0;
      break;

    default :
      break;
  }
}

void* handle_bac(void* agent){ //listens to tobacco requests
  struct Agent* ag = agent;
  
  uthread_mutex_lock(ag->mutex);

    while(1){
      uthread_cond_wait(ag->tobacco);
      sum+=TOBACCO;
      get_smoker(sum);
    }

  uthread_mutex_unlock(ag->mutex);

}
void* handle_pap(void* agent){ //listens to paper requests
    struct Agent* ag = agent;
  
  uthread_mutex_lock(ag->mutex);

    while(1){
      uthread_cond_wait(ag->paper);
      sum+=PAPER;
      get_smoker(sum);
    }

  uthread_mutex_unlock(ag->mutex);
}
void* handle_mat(void* agent){ //listens to match requests
    struct Agent* ag = agent;
  
  uthread_mutex_lock(ag->mutex);

    while(1){
      uthread_cond_wait(ag->match);
      sum+=MATCH;
      get_smoker(sum);
    }

  uthread_mutex_unlock(ag->mutex);
}

void* smoke_bac(void* agent){
  struct Agent* ag = agent;
  uthread_mutex_lock(ag->mutex);

    while(1){
      uthread_cond_wait(pap_mat);
      VERBOSE_PRINT("Tobacco Smoking");
      uthread_cond_signal(ag->smoke);
      smoke_count[TOBACCO]++;
    }

  uthread_mutex_unlock(ag->mutex);
}
void* smoke_pap(void* agent){
  struct Agent* ag = agent;
  uthread_mutex_lock(ag->mutex);

    while(1){
      uthread_cond_wait(bac_mat);
      VERBOSE_PRINT("Paper Smoking");
      uthread_cond_signal(ag->smoke);
      smoke_count[PAPER]++;
    }

  uthread_mutex_unlock(ag->mutex);
}
void* smoke_mat(void* agent){
  struct Agent* ag = agent;
  uthread_mutex_lock(ag->mutex);

    while(1){
      uthread_cond_wait(bac_pap);
      VERBOSE_PRINT("Match Smoking");
      uthread_cond_signal(ag->smoke);
      smoke_count[MATCH]++;
    }

  uthread_mutex_unlock(ag->mutex);
}

int main (int argc, char** argv) {
  uthread_init (7);
  struct Agent*  a = createAgent();
  
  bac_mat = uthread_cond_create(a->mutex);
  pap_mat = uthread_cond_create(a->mutex);
  bac_pap = uthread_cond_create(a->mutex);
  // TODO

  uthread_create(handle_bac,a);
  uthread_create(handle_pap,a);
  uthread_create(handle_mat,a);
  uthread_create(smoke_bac,a);
  uthread_create(smoke_pap,a);
  uthread_create(smoke_mat,a);
  

  uthread_join (uthread_create (agent, a), 0);
  assert (signal_count [MATCH]   == smoke_count [MATCH]);
  assert (signal_count [PAPER]   == smoke_count [PAPER]);
  assert (signal_count [TOBACCO] == smoke_count [TOBACCO]);
  assert (smoke_count [MATCH] + smoke_count [PAPER] + smoke_count [TOBACCO] == NUM_ITERATIONS);
  printf ("Smoke counts: %d matches, %d paper, %d tobacco\n",
          smoke_count [MATCH], smoke_count [PAPER], smoke_count [TOBACCO]);
}