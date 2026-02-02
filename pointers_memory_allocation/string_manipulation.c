#include <stdio.h>
#include <string.h>

int str_length(char *str) {
    char *ptr = str;

    while (*ptr != '\0') {
        ptr++;
    }

    return ptr - str; 
}

int main() {
    const char *message = "Ashot"; 
    const char *ptr = message;

    while (*ptr != '\0') {
        printf("%c ", *ptr);

        ptr++;
    }

    printf("\n");

    char buffer[100];

    printf("Enter a string: ");
    
    if (fgets(buffer, sizeof(buffer), stdin) != NULL) {
        char *newline_ptr = strchr(buffer, '\n');
        
        if (newline_ptr != NULL) {
            *newline_ptr = '\0';
        }

        int len = str_length(buffer);

        printf("String: \"%s\"\n", buffer);
        printf("Calculated Length: %d\n", len);
    }

    return 0;
}
