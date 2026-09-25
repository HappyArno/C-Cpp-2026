// 下面的代码为 Linux 系统编写，在 Windows 上可能不能正常编译和运行
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#define ROWS 10
#define COLS 10
typedef struct
{
    bool visited;
    bool wall_up;
    bool wall_down;
    bool wall_left;
    bool wall_right;
} Cell;
void init_cells(int rows, int cols, Cell cells[rows][cols])
{
    for (int i = 0; i < rows; i++)
        for (int j = 0; j < cols; j++)
        {
            cells[i][j] = (Cell){
                .visited = false,
                .wall_up = true,
                .wall_down = true,
                .wall_left = true,
                .wall_right = true
            };
        }
}
#define DIRECTION_COUNT 4
typedef enum
{
    UP,
    DOWN,
    LEFT,
    RIGHT,
} Direction;
typedef struct
{
    int dx;
    int dy;
} Offset;
static const Offset offsets[DIRECTION_COUNT] = {
    [UP] = {-1, 0},
    [DOWN] = {1, 0},
    [LEFT] = {0, -1},
    [RIGHT] = {0, 1},
};
static const Direction directions[] = {UP, DOWN, LEFT, RIGHT};
// Fisher-Yates shuffle
void shuffle(Direction directions[], size_t size)
{
    for (size_t i = size - 1; i > 0; i--)
    {
        size_t random = rand() % (i + 1);
        Direction temp = directions[i];
        directions[i] = directions[random];
        directions[random] = temp;
    }
}
void dig_wall(Cell *current, Cell *next, Direction direction)
{
    switch (direction)
    {
    case UP:
        current->wall_up = false;
        next->wall_down = false;
        break;
    case DOWN:
        current->wall_down = false;
        next->wall_up = false;
        break;
    case LEFT:
        current->wall_left = false;
        next->wall_right = false;
        break;
    case RIGHT:
        current->wall_right = false;
        next->wall_left = false;
        break;
    }
}
// DFS生成迷宫
void dfs(int rows, int cols, Cell cells[rows][cols], int x, int y)
{
    cells[x][y].visited = true;
    Direction shuffled[DIRECTION_COUNT];
    memcpy(shuffled, directions, sizeof(shuffled));
    shuffle(shuffled, DIRECTION_COUNT);
    for (size_t i = 0; i < DIRECTION_COUNT; i++)
    {
        int new_x = x + offsets[shuffled[i]].dx;
        int new_y = y + offsets[shuffled[i]].dy;
        if (!(0 <= new_x && new_x < rows && 0 <= new_y && new_y < cols))
            continue;
        if (cells[new_x][new_y].visited)
            continue;
        dig_wall(&cells[x][y], &cells[new_x][new_y], shuffled[i]);
        dfs(rows, cols, cells, new_x, new_y);
    }
}
void generate_map(int rows, int cols, Cell cells[rows][cols], char map[rows * 2 + 1][cols * 2 + 1])
{
    memset(map, '#', (rows * 2 + 1) * (cols * 2 + 1));
    for (int i = 0; i < rows; i++)
        for (int j = 0; j < cols; j++)
        {
            int map_x = 2 * i + 1;
            int map_y = 2 * j + 1;
            map[map_x][map_y] = ' ';
            // 由于每条通路由相邻两个Cell共享，因此理论上这里只需要处理两个垂直的方向即可
            if (cells[i][j].wall_up == false)
                map[map_x - 1][map_y] = ' ';
            if (cells[i][j].wall_down == false)
                map[map_x + 1][map_y] = ' ';
            if (cells[i][j].wall_left == false)
                map[map_x][map_y - 1] = ' ';
            if (cells[i][j].wall_right == false)
                map[map_x][map_y + 1] = ' ';
        }
    map[rows * 2 - 1][cols * 2 - 1] = 'E';
}
typedef struct
{
    int x;
    int y;
} Player;
void display(int rows, int cols, char map[rows * 2 + 1][cols * 2 + 1], Player player)
{
    printf("\033[H\033[J");
    for (int i = 0; i < rows * 2 + 1; i++)
    {
        for (int j = 0; j < cols * 2 + 1; j++)
        {
            if (i == player.x && j == player.y)
                printf("P");
            else
                printf("%c", map[i][j]);
        }
        printf("\n");
    }
}
void set_terminal_mode(bool enable)
{
    static struct termios old_term;
    static bool raw_mode_active = false;
    if (enable && !raw_mode_active)
    {
        if (tcgetattr(STDIN_FILENO, &old_term) == -1)
        {
            perror("获取终端控制属性失败");
            exit(EXIT_FAILURE);
        }
        struct termios term = old_term;
        term.c_lflag &= ~(ICANON | ECHO);
        term.c_cc[VMIN] = 1;
        term.c_cc[VTIME] = 0;
        if (tcsetattr(STDIN_FILENO, TCSANOW, &term) == -1)
        {
            perror("设置终端控制属性失败");
            exit(EXIT_FAILURE);
        }
        raw_mode_active = true;
    }
    else if (!enable && raw_mode_active)
    {
        if (tcsetattr(STDIN_FILENO, TCSANOW, &old_term) == -1)
        {
            perror("设置终端控制属性失败");
            exit(EXIT_FAILURE);
        }
        raw_mode_active = false;
    }
}
void cleanup()
{
    set_terminal_mode(false);
}
void set_read_mode(int vmin, int vtime)
{
    struct termios term;
    tcgetattr(STDIN_FILENO, &term);
    term.c_cc[VMIN] = vmin;
    term.c_cc[VTIME] = vtime;
    tcsetattr(STDIN_FILENO, TCSANOW, &term);
}
typedef enum
{
    OK,
    QUIT,
    NONE,
} Status;
Status read_key(Direction *direction)
{
    char c;
    if (read(STDIN_FILENO, &c, 1) <= 0)
        return NONE;
    if (c == '\033')
    {
        char seq[2];
        // set_read_mode(0, 1);
        set_read_mode(0, 0); // 我发现VTIME设成0也可以，这样就消除了按Esc退出时0.1秒的卡顿
        int n = read(STDIN_FILENO, seq, 2);
        set_read_mode(1, 0);
        if (!(n == 2 && seq[0] == '['))
            return QUIT;
        switch (seq[1])
        {
        case 'A':
            *direction = UP;
            return OK;
        case 'B':
            *direction = DOWN;
            return OK;
        case 'C':
            *direction = RIGHT;
            return OK;
        case 'D':
            *direction = LEFT;
            return OK;
        }
    }
    else
    {
        switch (c)
        {
        case 'w':
        case 'W':
            *direction = UP;
            return OK;
        case 's':
        case 'S':
            *direction = DOWN;
            return OK;
        case 'a':
        case 'A':
            *direction = LEFT;
            return OK;
        case 'd':
        case 'D':
            *direction = RIGHT;
            return OK;
        case 'q':
            return QUIT;
        }
    }
    return NONE;
}
void run(int rows, int cols, char map[rows * 2 + 1][cols * 2 + 1])
{
    set_terminal_mode(true);
    atexit(cleanup);
    Player player = {1, 1};
    for (;;)
    {
        display(rows, cols, map, player);
        Direction direction;
        for (;;)
        {
            Status status = read_key(&direction);
            if (status == OK)
                break;
            else if (status == QUIT)
                exit(0);
        }
        int new_x = player.x + offsets[direction].dx;
        int new_y = player.y + offsets[direction].dy;
        if (!(0 <= new_x && new_x < rows * 2 + 1 && 0 <= new_y && new_y < cols * 2 + 1))
            continue;
        if (map[new_x][new_y] == '#')
            continue;
        player.x = new_x;
        player.y = new_y;
        if (map[new_x][new_y] == 'E')
        {
            display(rows, cols, map, player);
            printf("You win!\n");
            break;
        }
    }
}
int main()
{
    srand(time(NULL));
    Cell cells[ROWS][COLS];
    init_cells(ROWS, COLS, cells);
    dfs(ROWS, COLS, cells, 0, 0);
    char map[ROWS * 2 + 1][COLS * 2 + 1];
    generate_map(ROWS, COLS, cells, map);
    run(ROWS, COLS, map);
    return 0;
}