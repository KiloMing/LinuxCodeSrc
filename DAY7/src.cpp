#include <iostream>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <vector>
#include <string>
#include <pthread.h>

int data = 0;
pthread_rwlock_t rwlock;

void* read(void* arg)
{
    int id = *static_cast<int*>(arg);
    std::cout << "Read " << id << " rdlock" << std::endl;

    pthread_rwlock_rdlock(&rwlock);

    std::cout << "READ: "<< id << " "  << data << std::endl;
    sleep(2);

    std::cout << "READ " << id << " unlock" << std::endl;
    pthread_rwlock_unlock(&rwlock);

    return nullptr;     //attention
}

void* write(void*)
{
    std::cout << "write lock" << std::endl;

    pthread_rwlock_wrlock(&rwlock);

    data++;

    sleep(2);
    std::cout << "write unlock " << std::endl;

    pthread_rwlock_unlock(&rwlock);

    return nullptr;     //attention 
}

int main(void)
{
    pthread_rwlock_init(&rwlock, nullptr);

    pthread_t tid1;
    pthread_t tid2;
    pthread_t tid3;
    pthread_t tid4;

    int id1 = 1;
    int id2 = 2;

    pthread_create(&tid1, nullptr, read, &id1);
    pthread_create(&tid2, nullptr, read, &id2);
    pthread_create(&tid3, nullptr, write, nullptr);

    pthread_join(tid1, nullptr);
    pthread_join(tid2, nullptr);
    pthread_join(tid3, nullptr);
    pthread_join(tid4, nullptr);
    
    pthread_rwlock_destroy(&rwlock);
}

