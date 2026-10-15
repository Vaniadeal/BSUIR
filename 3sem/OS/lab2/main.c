#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#define CHAIRS 5

sem_t customers;
sem_t barber;
pthread_mutex_t mutex;

int waiting = 0;

void get_haircut(int customer_id) {
    printf("[Клиент %d] Стрижется...\n", customer_id);
    printf("[Клиент %d] Постригся и ушел.\n", customer_id);
}

void* barber_thread(void* arg) {
    while (1) {
        sem_wait(&customers);
        
        pthread_mutex_lock(&mutex);
        waiting--;
        sem_post(&barber);
        pthread_mutex_unlock(&mutex);
        
        printf("[Парикмахер] Стригу клиента.\n");
        sleep(2);
    }
    return NULL;
}

void* customer_thread(void* arg) {
    int id = *(int*)arg;
    
    pthread_mutex_lock(&mutex);
    
    if (waiting < CHAIRS) {
        waiting++;
        sem_post(&customers);
        pthread_mutex_unlock(&mutex);
        
        sem_wait(&barber); 
        
        get_haircut(id);
    } else {
        pthread_mutex_unlock(&mutex);
        printf("[Клиент %d] В приемной нет мест. Ухожу.\n", id);
    }
    
    return NULL;
}

int main() {
    pthread_t barber_tid;
    pthread_t customer_tids[10];
    int customer_ids[10];

    sem_init(&customers, 0, 0);
    sem_init(&barber, 0, 0);
    pthread_mutex_init(&mutex, NULL);

    printf("--- Парикмахерская открыта! (Стульев в приемной: %d) ---\n", CHAIRS);

    pthread_create(&barber_tid, NULL, barber_thread, NULL);

    for (int i = 0; i < 10; i++) {
        customer_ids[i] = i + 1;
        pthread_create(&customer_tids[i], NULL, customer_thread, &customer_ids[i]);
        usleep(500000);
    }

    for (int i = 0; i < 10; i++) {
        pthread_join(customer_tids[i], NULL);
    }

    printf("--- Все клиенты обслужены или ушли. Завершение работы. ---\n");
    
    exit(0); 
}