#include <stdio.h>

int main() {
    const char *arr[] = {
        "ab",
        "de",
        "fg",
        "hi"
    };

    for (int i = 0; i < 4; i++) {
        printf("Index %d: %s\n", i, *(arr + i));
    }

    *(arr + 1) = "jk";
    
    for (int i = 0; i < 4; i++) {
        printf("Index %d: %s\n", i, *(arr + i));
    }

    return 0;
}
