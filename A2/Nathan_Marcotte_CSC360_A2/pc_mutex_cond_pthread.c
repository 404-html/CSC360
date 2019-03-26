/* Nathan Marcotte
 * CSC 360 Spring 2019
 * V00876934
 * pc_mutex_cond_pthread.c
 */

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <pthread.h>
#define MAX_ITEMS 10
const int NUM_ITERATIONS = 200;
const int NUM_CONSUMERS  = 2;
const int NUM_PRODUCERS  = 2;

int producer_wait_count;     // # of times producer had to wait
int consumer_wait_count;     // # of times consumer had to wait
int histogram [MAX_ITEMS+1]; // histogram [i] == # of times list stored i items


pthread_mutex_t mutex; 
pthread_cond_t max;
pthread_cond_t none;

int items = 0;

void* producer (void* v) {
  for (int i=0; i<NUM_ITERATIONS; i++) {
   
    pthread_mutex_lock(&mutex);
     
      while(items>=MAX_ITEMS){
        producer_wait_count++;
        pthread_cond_wait(&none,&mutex);
      }

    items++;
    histogram[items]++;
    assert(items<=MAX_ITEMS);
    
    pthread_cond_signal(&max);
    pthread_mutex_unlock(&mutex);
  }
  return NULL;
}

void* consumer (void* v) {
  for (int i=0; i<NUM_ITERATIONS; i++) {
       
      pthread_mutex_lock(&mutex);
      
      while(items<=0){
        consumer_wait_count++;
        pthread_cond_wait(&max,&mutex);
      }
   
    
    items--;
    histogram[items]++;
    assert(items>=0);
   
     pthread_cond_signal(&none);
     pthread_mutex_unlock(&mutex);
  }
  return NULL;
}



int main (int argc, char** argv) {
  pthread_t t[4];
  pthread_mutex_init(&mutex, NULL);
  pthread_cond_init(&max,NULL);
  pthread_cond_init(&none,NULL);
  // TODO: Create Threads and Join

for(int i = 0;i<NUM_PRODUCERS;i++){
  pthread_create(&t[i],NULL,&producer,NULL);
}

for(int i = NUM_PRODUCERS;i<NUM_PRODUCERS+NUM_CONSUMERS;i++){
  pthread_create(&t[i],NULL,&consumer,NULL);
}
  
for(int i = 0;i<NUM_PRODUCERS+NUM_CONSUMERS;i++){
  pthread_join(t[i],NULL);
}
 
  
pthread_mutex_destroy(&mutex);
pthread_cond_destroy(&max);
pthread_cond_destroy(&none);

  printf ("producer_wait_count=%d\nconsumer_wait_count=%d\n", producer_wait_count, consumer_wait_count);
  printf ("items value histogram:\n");
  int sum=0;
  for (int i = 0; i <= MAX_ITEMS; i++) {
    printf ("  items=%d, %d times\n", i, histogram [i]);
    sum += histogram [i];
  }
  assert (sum == sizeof (t) / sizeof (pthread_t) * NUM_ITERATIONS);
}

