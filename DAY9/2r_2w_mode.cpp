#include <iostream>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <vector>
#include <string>
#include <pthread.h>

pthread_rwlock_t g_rwlock;
int data = 0;

void* reader(void* arg)
{
    int id = *static_cast<int*>(arg);
    pthread_rwlock_rdlock(&g_rwlock);
    std::cout << "Reader " << id << " read data: " << data << std::endl;
    sleep(2); // Simulate reading time
    pthread_rwlock_unlock(&g_rwlock);
    return nullptr;
}

void* writer(void* arg)
{
    int id = *static_cast<int*>(arg);
    pthread_rwlock_wrlock(&g_rwlock);
    data++;
    std::cout << "Writer " << id << " wrote data: " << data << std::endl;
    sleep(2); // Simulate writing time
    pthread_rwlock_unlock(&g_rwlock);
    return nullptr;
}

int main(void)
{
    pthread_rwlock_init(&g_rwlock, nullptr);

    const int num_readers = 2;
    const int num_writers = 2;
    pthread_t readers[num_readers];
    pthread_t writers[num_writers];
    int reader_ids[num_readers];
    int writer_ids[num_writers];

    for (int i = 0; i < num_readers; ++i) {
        reader_ids[i] = i + 1;
        pthread_create(&readers[i], nullptr, reader, &reader_ids[i]);
    }

    for (int i = 0; i < num_writers; ++i) {
        writer_ids[i] = i + 1;
        pthread_create(&writers[i], nullptr, writer, &writer_ids[i]);
    }

    for (int i = 0; i < num_readers; ++i) {
        pthread_join(readers[i], nullptr);
    }

    for (int i = 0; i < num_writers; ++i) {
        pthread_join(writers[i], nullptr);
    }

    pthread_rwlock_destroy(&g_rwlock);

    return 0;
}