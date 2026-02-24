#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#define NUM_PLAYERS 4
#define NUM_ROUNDS 5

int rolls[NUM_PLAYERS];
int wins[NUM_PLAYERS] = {0};

pthread_barrier_t roll_barrier;
pthread_barrier_t next_round_barrier;

void* play_round(void* arg) {
    int player_id = *(int*)arg;
    
    for (int r = 0; r < NUM_ROUNDS; r++) {
        rolls[player_id] = (rand() % 6) + 1;

        printf("Player %d rolled %d\n", player_id, rolls[player_id]);
        
        pthread_barrier_wait(&roll_barrier);
        
        pthread_barrier_wait(&next_round_barrier);
    }
    
    return NULL;
}

int main() {
    pthread_t players[NUM_PLAYERS];
    int player_ids[NUM_PLAYERS];

    srand(time(NULL));
    
    int total_threads = NUM_PLAYERS + 1;

    pthread_barrier_init(&roll_barrier, NULL, total_threads);
    pthread_barrier_init(&next_round_barrier, NULL, total_threads);
    
    for (int i = 0; i < NUM_PLAYERS; i++) {
        player_ids[i] = i;

        pthread_create(&players[i], NULL, play_round, &player_ids[i]);
    }
    
    for (int current_round = 1; current_round <= NUM_ROUNDS; current_round++) {
        pthread_barrier_wait(&roll_barrier);
        
        int max_roll = 0;
        int round_winner = -1;
        
        for (int i = 0; i < NUM_PLAYERS; i++) {
            if (rolls[i] > max_roll) {
                max_roll = rolls[i];
                round_winner = i;
            }
        }
        
        wins[round_winner]++;

        printf("Round %d winner is player %d with a roll of %d\n\n", current_round, round_winner, max_roll);
               
        pthread_barrier_wait(&next_round_barrier);
    }
    
    for (int i = 0; i < NUM_PLAYERS; i++) {
        pthread_join(players[i], NULL);
    }
    
    int max_wins = 0;
    int overall_winner = -1;
    
    for (int i = 0; i < NUM_PLAYERS; i++) {
        printf("Player %d: %d wins\n", i, wins[i]);

        if (wins[i] > max_wins) {
            max_wins = wins[i];
            overall_winner = i;
        }
    }
    
    printf("\nWinner: Player %d with %d wins\n", overall_winner, max_wins);
    
    pthread_barrier_destroy(&roll_barrier);
    pthread_barrier_destroy(&next_round_barrier);
    
    return 0;
}
