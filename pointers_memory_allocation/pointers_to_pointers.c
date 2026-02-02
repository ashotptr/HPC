#include <stdio.h>

int main() {
    int num = 123;
    int *ptr = &num;
    int **ptr_to_ptr = &ptr;

    printf("Value of num: %d\n", num);
    printf("Address of num: %p\n", (void*)&num);
    printf("Address stored in ptr: %p\n", (void*)ptr);
    printf("Address stored in ptr_to_ptr: %p\n", (void*)ptr_to_ptr);    
    printf("Value of num: %d\n", *ptr);
    printf("Value of num: %d\n", **ptr_to_ptr);

    return 0;
}
