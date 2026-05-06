// test_simple.c - 最简单的测试：一个 if 分支
#include <stdio.h>

int compute(int a, int b) {
    int result = 0;
    if (a > b) {
        result = a - b;
    } else {
        result = b - a;
    }
    return result;
}

int main() {
    int x = compute(10, 3);
    printf("compute(10, 3) = %d\n", x);    // 应输出 7

    int y = compute(3, 10);
    printf("compute(3, 10) = %d\n", y);    // 应输出 7

    printf("Test PASSED\n");
    return 0;
}
