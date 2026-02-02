#include <stdio.h>

int main() {
    int arr[5] = {10, 20, 30, 40, 50};
    int *ptr = arr;

    printf("Original array\n");
    for (int i = 0; i < 5; i++) {
        printf("Element %d : %d : %p\n", i, *(ptr + i), (void*)(ptr + i));
    }
    
    for (int i = 0; i < 5; i++) {
        *(ptr + i) += 5; 
    }

    printf("\nModified Array\n");
    for (int i = 0; i < 5; i++) {
        printf("Element %d : %d : %d\n", i, arr[i], *(ptr + i));
    }

    return 0;
}
