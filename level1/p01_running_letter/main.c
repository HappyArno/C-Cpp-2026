// 下面的代码为 Linux 系统编写，在 Windows 上可能不能正常编译和运行
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#define FRAME_DELAY_US 20000
int get_terminal_width()
{
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == -1)
    {
        perror("获取终端大小失败");
        return -1;
    }
    return w.ws_col;
}
void print_str(const char *str, int spaces)
{
    printf("\r\033[K"); // 将光标移动到行首，并清除当前行内容
    for (int i = 0; i < spaces; i++)
        printf(" ");
    printf("%s", str);
    fflush(stdout);
    usleep(FRAME_DELAY_US);
}
int main()
{
    const char *str = "Hello UESTC!";
    int width = get_terminal_width();
    if (width == -1)
        return 1;
    int len = strlen(str);
    if (width <= len)
    {
        printf("%s\n", str);
        return 0;
    }
    int distance = width - len;
    for (;;)
    {
        // 第一个for从0打印到distance，第二个for从distance-1打印到1，正好补齐中间的点而且不会在两端停留两帧
        for (int i = 0; i <= distance; i++)
            print_str(str, i);
        for (int i = distance - 1; i >= 1; i--)
            print_str(str, i);
    }
    return 0;
}