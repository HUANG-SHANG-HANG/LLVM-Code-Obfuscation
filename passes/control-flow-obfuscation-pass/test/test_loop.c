// test_loop.c - 循环测试：for 循环 + 累加
#include <stdio.h>

int sum_range(int n) {
    int sum = 0;
    for (int i = 1; i <= n; ++i) {
        sum += i;
    }
    return sum;
}

int factorial(int n) {
    int result = 1;
    for (int i = 2; i <= n; ++i) {
        result *= i;
    }
    return result;
}

int main() {
    int s = sum_range(100);
    printf("sum(1..100) = %d\n", s);       // 应输出 5050

    int f = factorial(10);
    printf("10! = %d\n", f);               // 应输出 3628800

    if (s == 5050 && f == 3628800) {
        printf("Test PASSED\n");
    } else {
        printf("Test FAILED\n");
        return 1;
    }
    return 0;
}
