#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <pthread.h>

pthread_mutex_t mutex;
void* work(void *arg)
{
    pthread_mutex_lock(&mutex);
    std::cout << "work" << std::endl;
    pthread_mutex_unlock(&mutex);

    return nullptr; //don't forget the return
}

int main(void)
{
    //create the procecss
    pthread_t t1;
    pthread_t t2;

    pthread_mutex_init(&mutex, nullptr);

    pthread_create(&t1, nullptr, work, nullptr);
    pthread_create(&t2, nullptr, work, nullptr);

    pthread_join(t1, nullptr);
    pthread_join(t2, nullptr);

    std::cout << "*****end*****" << std::endl;

    pthread_mutex_destroy(&mutex);
}