#include <stdio.h>
#include <stdlib.h>

int main() {
    int *p_int = (int*)malloc(sizeof(int));

    if (p_int == NULL) {
        printf("Memory allocation failed!\n");

        return 1;
    }

    *p_int = 42;

    printf("Value: %d, Address: %p\n", *p_int, (void*)p_int);
    
    int *p_arr = (int*)malloc(5 * sizeof(int));

    if (p_arr == NULL) {
        printf("Memory allocation failed!\n");

        free(p_int);
        
        p_int = NULL;

        return 1;
    }

    for (int i = 0; i < 5; i++) {
        *(p_arr + i) = (i + 1) * 10;

        printf("Index %d: %d\n", i, *(p_arr + i));
    }

    free(p_int);
    free(p_arr);
    
    p_int = NULL;
    p_arr = NULL;

    return 0;
}
