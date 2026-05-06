// test_branch.c - 多分支测试：嵌套 if-else + switch
#include <stdio.h>

const char* classify(int score) {
    if (score >= 90) {
        return "A";
    } else if (score >= 80) {
        return "B";
    } else if (score >= 70) {
        return "C";
    } else if (score >= 60) {
        return "D";
    } else {
        return "F";
    }
}

int fibonacci(int n) {
    if (n <= 0) return 0;
    if (n == 1) return 1;

    int a = 0, b = 1;
    for (int i = 2; i <= n; ++i) {
        int tmp = a + b;
        a = b;
        b = tmp;
    }
    return b;
}

int main() {
    // 测试分类
    printf("95 -> %s\n", classify(95));     // A
    printf("85 -> %s\n", classify(85));     // B
    printf("75 -> %s\n", classify(75));     // C
    printf("65 -> %s\n", classify(65));     // D
    printf("55 -> %s\n", classify(55));     // F

    // 测试斐波那契
    printf("fib(10) = %d\n", fibonacci(10)); // 55
    printf("fib(20) = %d\n", fibonacci(20)); // 6765

    if (fibonacci(10) == 55 && fibonacci(20) == 6765) {
        printf("Test PASSED\n");
    } else {
        printf("Test FAILED\n");
        return 1;
    }
    return 0;
}
