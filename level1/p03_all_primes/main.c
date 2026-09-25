// 埃拉托斯特尼筛法
// 下面的代码为 Linux 系统编写，在 Windows 上可能不能正常编译和运行
#include <stdbool.h>
#include <stdio.h>
#include <time.h>
#define SIZE 1000
int main()
{
    bool arr[SIZE + 1];
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i <= SIZE; i++)
        arr[i] = true;
    arr[0] = false;
    arr[1] = false;
    // 虽然这里可以使用i+=2和j+=2*i跳过一半的计算，但是在该规模下我认为不应引入这个优化导致破坏算法实现的简洁和可读性
    for (int i = 2; i * i <= SIZE; i++)
        if (arr[i])
            for (int j = i * i; j <= SIZE; j += i)
                arr[j] = false;
    clock_gettime(CLOCK_MONOTONIC, &end);
    printf("2~1000以内的所有素数:\n");
    for (int i = 2; i <= SIZE; i++)
        if (arr[i])
            printf("%d ", i);
    printf("\n");
    long long elapsed_ns = (end.tv_sec - start.tv_sec) * 1000000000LL + (end.tv_nsec - start.tv_nsec);
    printf("计算时间: %lld ns\n", elapsed_ns);
    return 0;
}