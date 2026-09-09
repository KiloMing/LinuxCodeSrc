
/*
This file demonstrates a simple producer-consumer problem using pthreads in C++ with buffers.
And this process has two producer threads and one consumer thread.
The shared buffer is implemented using a queue, and the synchronization between threads is handled using mutexes and condition variables.
As the same time, the producers and consumer must coordinate to avoid race conditions and ensure proper synchronization.
ATTENTION: Two producer use the same buffer. 
In the process, there will 20 data items being produced and consumed. Every producer will produce 10 data items.
*/
#include <iostream>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <vector>
#include <string>
#include <pthread.h>
#include <queue>

//This file demonstrates a simple producer-consumer problem using pthreads in C++ with buffers.
//And this buffer is based on a queue.
//DAY7/thePdrCumMode.png : this photo can explin the producer-consumer problem using pthreads 

// global mutex and condition variable for synchronizing producer and consumer threads
pthread_mutex_t g_mutex;    
std::queue<int> buffer; // the shared buffer based on a queue
pthread_cond_t not_empty;// Consumer waits here when buffer is empty
pthread_cond_t not_full; // Producer waits here when buffer is full

bool full = false;  
// the shared buffer is only a single ite
// the shared buffer item
int data;   

//producer-consumer example using pthreads
//producer thread function
void* producer(void* arg)
{
    // get the producer thread id
    int id = *static_cast<int*>(arg);
    for(int i = 0; i < 10; i++) {
    //producer thread code here
    pthread_mutex_lock(&g_mutex);
    //produce an item and add it to the shared buffer
    while(buffer.size() >= 5) {
        pthread_cond_wait(&not_full, &g_mutex);
    }
    buffer.push(i+id); // produce the data
    pthread_cond_signal(&not_empty);
    pthread_mutex_unlock(&g_mutex);
    }
    return nullptr;
}


//consumer thread function
void* consumer(void* arg)
{
    //consumer thread code here
    for(int i = 0; i < 20; i++) {
        pthread_mutex_lock(&g_mutex);
        while (buffer.empty()) {
            pthread_cond_wait(&not_empty, &g_mutex);
        }
        //consume the item from the shared buffer
        int consumed_data = buffer.front();
        buffer.pop();
        std::cout << "Consumed data: " << consumed_data << std::endl;
        pthread_cond_signal(&not_full);
        pthread_mutex_unlock(&g_mutex);
    }
    return nullptr;
}
int main(void)
{
    pthread_mutex_init(&g_mutex, nullptr);
    pthread_cond_init(&not_empty, nullptr);
    pthread_cond_init(&not_full, nullptr);

    pthread_t prod_thread, cons_thread;
    int prod_id_1 = 1;
    int prod_id_2 = 2;
    pthread_create(&prod_thread, nullptr, producer, &prod_id_1);
    pthread_t prod_thread_2;
    pthread_create(&prod_thread_2, nullptr, producer, &prod_id_2);
    pthread_create(&cons_thread, nullptr, consumer, nullptr);

    pthread_join(prod_thread, nullptr);
    pthread_join(prod_thread_2, nullptr);
    pthread_join(cons_thread, nullptr);


    pthread_mutex_destroy(&g_mutex);
    pthread_cond_destroy(&not_empty);
    pthread_cond_destroy(&not_full);

    return 0;
}