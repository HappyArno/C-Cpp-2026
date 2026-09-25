#include <stdio.h>
static const size_t DISK_COUNT = 5; // 题目的意思真的是64吗？根本算不完，所以改成一个较小的数 qwq
void hanoi(size_t disk, char src, char aux, char dst)
{
    if (disk == 0)
        return;
    hanoi(disk - 1, src, dst, aux);
    printf("%c -> %c\n", src, dst);
    hanoi(disk - 1, aux, src, dst);
}
int main()
{
    hanoi(DISK_COUNT, 'A', 'B', 'C');
    return 0;
}