#include <iostream>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#define NUM_GHOSTS 3

sem_t keySemaphore;
sem_t permitSemaphore;

void* GhostThread(void* arg) {
    int ghostNum = *((int*)arg);
    delete (int*)arg;

    while (true) {
        printf("Ghost %d is attempting to acquire key and permit\n", ghostNum);
        sleep(1);
        
        sem_wait(&keySemaphore);
        printf("Ghost %d acquired a key\n", ghostNum);
        sleep(1);
        
        sem_wait(&permitSemaphore);
        printf("Ghost %d acquired a permit\n", ghostNum);
        sleep(1);
        
        printf("Ghost %d acquired both key and permit\n", ghostNum);
        sleep(1);

        printf("Ghost %d leaves the house\n", ghostNum);
        sleep(1);

        sem_post(&keySemaphore);
        printf("Ghost %d released a key\n", ghostNum);
        sleep(1);
        
        sem_post(&permitSemaphore);
        printf("Ghost %d released a permit\n", ghostNum);
        sleep(1);

    }

    return nullptr;
}

int main() {
    
    sem_init(&keySemaphore, 0, 2); // Two keys available
    sem_init(&permitSemaphore, 0, 1); // One permit available

    pthread_t ghostThreads[NUM_GHOSTS];
    for (int i = 0; i < NUM_GHOSTS; ++i) {
        int* ghostNum = new int(i + 1);
        pthread_create(&ghostThreads[i], nullptr, GhostThread, (void*)ghostNum);
        printf("Thread %d created\n", *ghostNum);
    }

    // Join threads (not necessary for this example since threads run indefinitely)
    for (int i = 0; i < NUM_GHOSTS; ++i) {
        pthread_join(ghostThreads[i], nullptr);
    }

    sem_destroy(&keySemaphore);
    sem_destroy(&permitSemaphore);

    return 0;
}
