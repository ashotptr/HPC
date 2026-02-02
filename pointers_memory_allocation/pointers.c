#include <stdio.h>

int main() {
    int number = 10;

    int *ptr = &number;

    printf("Initial value of number: %d\n", number);

    printf("Address of number: %p\n", (void*)&number);
    printf("Address of number: %p\n", (void*)ptr);

    *ptr = 20;

    printf("New value of number: %d\n", number);

    return 0;
}
