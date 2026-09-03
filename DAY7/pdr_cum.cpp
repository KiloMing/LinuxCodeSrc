#include <iostream>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <vector>
#include <string>
#include <pthread.h>
//This file contains an example of the producer-consumer problem using pthreads

//https://chatgpt.com/s/m_6aa002316924819181700f0968e3ee14 : this photo can explin the producer-consumer problem using pthreads 

// global mutex and condition variable for synchronizing producer and consumer threads
pthread_mutex_t g_mutex;
pthread_cond_t not_empty;// ()condition variable to signal that the buffer is not empty
pthread_cond_t not_full; // condition variable to signal that the buffer is not full

bool full = false;  
// the shared buffer is only a single ite
// the shared buffer item
int data;   

//producer-consumer example using pthreads
//producer thread function
void* producer(void* arg)
{
    //producer thread code here
    pthread_mutex_lock(&g_mutex);
    //produce an item and add it to the shared buffer
    while(full) {
        pthread_cond_wait(&not_full, &g_mutex);
    }
    full = true;
    pthread_cond_signal(&not_empty);
    pthread_mutex_unlock(&g_mutex);
    return nullptr;
}


//consumer thread function
void* consumer(void* arg)
{
    //consumer thread code here
    pthread_mutex_lock(&g_mutex);
    while (!full) {
        pthread_cond_wait(&not_empty, &g_mutex);
    }
    //consume the item from the shared buffer
    full = false;
    pthread_cond_signal(&not_full);
    pthread_mutex_unlock(&g_mutex);
    return nullptr;
}
int main(void)
{
    pthread_mutex_init(&g_mutex, nullptr);
    pthread_cond_init(&not_empty, nullptr);
    pthread_cond_init(&not_full, nullptr);

    pthread_t prod_thread, cons_thread;
    pthread_create(&prod_thread, nullptr, producer, nullptr);
    pthread_create(&cons_thread, nullptr, consumer, nullptr);

    pthread_join(prod_thread, nullptr);
    pthread_join(cons_thread, nullptr);


    pthread_mutex_destroy(&g_mutex);
    pthread_cond_destroy(&not_empty);
    pthread_cond_destroy(&not_full);

    return 0;
}