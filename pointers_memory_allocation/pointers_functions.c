#include <stdio.h>

void swap(int *a, int *b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}

int main() {
    int x = 100;
    int y = 200;

    printf("Before swap\n");
    printf("x = %d\n", x);
    printf("y = %d\n", y);

    swap(&x, &y);

    printf("\nAfter swap\n");
    printf("x = %d\n", x);
    printf("y = %d\n", y);

    return 0;
}
