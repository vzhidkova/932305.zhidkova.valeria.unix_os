#include <stdio.h>
#include <pthread.h>
#include <unistd.h>


pthread_cond_t cond1 = PTHREAD_COND_INITIALIZER;
pthread_mutex_t lock  = PTHREAD_MUTEX_INITIALIZER;

int ready = 0;     
int running = 1;   

void* provider(void* arg) {
    int i = 0;

    while (i < 5) {          
        sleep(1);            

        pthread_mutex_lock(&lock);

        while (ready == 1) {
            pthread_cond_wait(&cond1, &lock);
        }

        i++;
        ready = 1;
        printf("Provider sent event with data: %d\n", i);

        pthread_cond_signal(&cond1);

        pthread_mutex_unlock(&lock);
    }

    pthread_mutex_lock(&lock);
    running = 0;              
    pthread_cond_broadcast(&cond1); 
    pthread_mutex_unlock(&lock);

    return NULL;
}


void* consumer(void* arg) {
    int i = 0;

    while (1) {
        pthread_mutex_lock(&lock);

        while (ready == 0 && running == 1) {
            pthread_cond_wait(&cond1, &lock);
        }

        if (running == 0 && ready == 0) {
            pthread_mutex_unlock(&lock);
            break;
        }

        i++;
        ready = 0;
        printf("Consumer got event with data: %d\n", i);

        pthread_cond_signal(&cond1);

        pthread_mutex_unlock(&lock);
    }

    return NULL;
}


int main() {
    pthread_t prov, cons;

    pthread_create(&prov, NULL, provider, NULL);
    pthread_create(&cons, NULL, consumer, NULL);

    pthread_join(prov, NULL);
    pthread_join(cons, NULL);

    pthread_mutex_destroy(&lock);
    pthread_cond_destroy(&cond1);

    return 0;
}