#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

void* thread_function(void* arg) {
    int thread_id = *(int*)arg;
    
    printf("Thread %d is running\n", thread_id);
    
    return NULL;
}

int main() {
    pthread_t threads[3];
    int thread_args[3];
    int i;
    int result;

    printf("Main thread: Starting creating threads\n");

    for (i = 0; i < 3; i++) {
        thread_args[i] = i + 1;
        
        result = pthread_create(&threads[i], NULL, thread_function, &thread_args[i]);
        
        if (result != 0) {
            perror("Thread creation failed");

            exit(EXIT_FAILURE);
        }
    }

    for (i = 0; i < 3; i++) {
        result = pthread_join(threads[i], NULL);

        if (result != 0) {
            perror("Thread join failed");
            
            exit(EXIT_FAILURE);
        }
    }

    printf("Main thread: All threads completed. Exiting.\n");

    return 0;
}
