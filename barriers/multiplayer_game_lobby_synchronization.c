#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#define NUM_PLAYERS 5

pthread_barrier_t lobby_barrier;

void* player_ready(void* arg) {
    int player_id = *(int*)arg;
    int prep_time = (rand() % 3) + 1;

    printf("Player %d will connect in %d seconds\n", player_id, prep_time);
    
    sleep(prep_time);
    
    printf("Player %d is ready\n", player_id);

    pthread_barrier_wait(&lobby_barrier);

    return NULL;
}

int main() {
    pthread_t players[NUM_PLAYERS];
    int player_ids[NUM_PLAYERS];

    srand(time(NULL));

    if (pthread_barrier_init(&lobby_barrier, NULL, NUM_PLAYERS + 1) != 0) {
        perror("Barrier init failed");

        return 1;
    }

    for (int i = 0; i < NUM_PLAYERS; i++) {
        player_ids[i] = i + 1;

        if (pthread_create(&players[i], NULL, player_ready, &player_ids[i]) != 0) {
            perror("Failed to create thread");

            return 1;
        }
    }
    
    pthread_barrier_wait(&lobby_barrier);
    
    printf("\nAll players are ready\n");

    for (int i = 0; i < NUM_PLAYERS; i++) {
        pthread_join(players[i], NULL);
    }

    pthread_barrier_destroy(&lobby_barrier);

    return 0;
}
