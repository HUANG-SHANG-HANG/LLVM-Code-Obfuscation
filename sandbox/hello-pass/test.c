#include <stdio.h>

int add(int a, int b) {
    return a + b;
}

int multiply(int a, int b) {
    return a * b;
}

int main() {
    int x = add(3, 4);
    int y = multiply(x, 2);
    printf("Result: %d\n", y);
    return 0;
}
