#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

#define NUM_SENSORS 5

double temperatures[NUM_SENSORS];
pthread_barrier_t collection_barrier;

void* sensor_task(void* arg) {
    int sensor_id = *(int*)arg;

    double temp = 10.0 + (rand() % 300) / 10.0;
    temperatures[sensor_id] = temp;
    
    printf("Sensor %d collected data: %.1f°C\n", sensor_id, temp);

    pthread_barrier_wait(&collection_barrier);

    return NULL;
}

int main() {
    pthread_t sensors[NUM_SENSORS];
    int sensor_ids[NUM_SENSORS];

    srand(time(NULL));

    if (pthread_barrier_init(&collection_barrier, NULL, NUM_SENSORS + 1) != 0) {
        perror("Barrier init failed");

        return 1;
    }

    for (int i = 0; i < NUM_SENSORS; i++) {
        sensor_ids[i] = i + 1;

        if (pthread_create(&sensors[i], NULL, sensor_task, &sensor_ids[i]) != 0) {
            perror("Failed to create thread");

            return 1;
        }
    }

    pthread_barrier_wait(&collection_barrier);
    
    double sum = 0.0;

    for (int i = 0; i < NUM_SENSORS; i++) {
        sum += temperatures[i];
    }
    
    double average = sum / NUM_SENSORS;
    
    printf("Average System Temperature: %.2f°C\n", average);

    for (int i = 0; i < NUM_SENSORS; i++) {
        pthread_join(sensors[i], NULL);
    }

    pthread_barrier_destroy(&collection_barrier);

    return 0;
}
