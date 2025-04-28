#include "test_header.h"

/* This is a global variable */
int global_var;
int valid_global;
int x-ray;  // Invalid identifier

/* This is the main function */
int main() {
    /* Local variables */
    int local_var;
    float pi = 3.14;
    int two&four;  // Invalid identifier
    char 5brothers;  // Invalid identifier
    
    /* Print test message */
    printf("Hello, World!\n");
    
    // Calculate and print sum
    int sum = local_var + global_var;
    printf("Sum: %d\n", sum);
    
    return 0;
}