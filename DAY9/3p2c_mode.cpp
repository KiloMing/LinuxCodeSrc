/*
This is a  3-producer-2-consumer example using pthreads.
This program demonstrates the use of pthreads to implement a classic producer-consumer problem with three producers and two consumers.
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

pthread_mutex_t g_mutex;    
std::queue<int> buffer; // the shared buffer based on a queue
pthread_cond_t not_empty;// Consumer waits here when buffer is empty
pthread_cond_t not_full; // Producer waits here when buffer is full

const int BUFFER_SIZE = 5; // maximum size of the shared buffer
int producer_done = 0; // keeps track of the number of producers that have finished producing
const int PRODUCE_COUNT = 3; // total number of producers
int producer_count = 5; //  Every precess will produce 10 items


//producer-consumer example using pthreads
//producer thread function
void* producer(void* arg)
{
    int id = *static_cast<int*>(arg);
    for (int i = 0; i < producer_count; ++i)
    {
        pthread_mutex_lock(&g_mutex);

        while (buffer.size() >= BUFFER_SIZE)
        {
            pthread_cond_wait(&not_full, &g_mutex);
        }

        buffer.push(i + id); // produce the data
        std::cout << "Producer " << id << " produced data: " << buffer.back() << std::endl;
        pthread_cond_signal(&not_empty);
        pthread_mutex_unlock(&g_mutex);
    }

    // indicate that this producer has finished producing
    pthread_mutex_lock(&g_mutex);
    producer_done++;
    if (producer_done == PRODUCE_COUNT)
    {
        std::cout << "Producer " << id << " finished producing." << std::endl;
        pthread_cond_signal(&not_empty);
        pthread_mutex_unlock(&g_mutex);
    }
    return nullptr;
}


//consumer thread function
void* consumer(void* arg)
{
    int id = *static_cast<int*>(arg);
    
    while (true)
    {
        pthread_mutex_lock(&g_mutex);

        while (buffer.empty() && producer_done < PRODUCE_COUNT)
        {
            pthread_cond_wait(&not_empty, &g_mutex);
        }

        if (buffer.empty() && producer_done == PRODUCE_COUNT)
        {
            pthread_mutex_unlock(&g_mutex);
            std::cout << "Consumer " << id << " exiting as all producers are done and buffer is empty." << std::endl;
            break;
        }
        int consumed_data = buffer.front();
        buffer.pop();
        std::cout << "Consumer " << id << " consumed data: " << consumed_data << std::endl;
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

    pthread_t prod_thread1, cons_thread1, prod_thread_2, cons_thread2, prod_thread_3;
    int prod_id_1 = 100;
    int prod_id_2 = 200;
    int prod_id_3 = 300;
    int cons_id_1 = 100; 
    int cons_id_2 = 200; 
    pthread_create(&prod_thread1, nullptr, producer, &prod_id_1);
    pthread_create(&prod_thread_2, nullptr, producer, &prod_id_2);
    pthread_create(&prod_thread_3, nullptr, producer, &prod_id_3);
    pthread_create(&cons_thread1, nullptr, consumer, &cons_id_1);
    pthread_create(&cons_thread2, nullptr, consumer, &cons_id_2);

    pthread_join(prod_thread1, nullptr);
    pthread_join(prod_thread_2, nullptr);
    pthread_join(prod_thread_3, nullptr);
    pthread_join(cons_thread1, nullptr);
    pthread_join(cons_thread2, nullptr);

    pthread_mutex_destroy(&g_mutex);
    pthread_cond_destroy(&not_empty);
    pthread_cond_destroy(&not_full);

    return 0;
} 