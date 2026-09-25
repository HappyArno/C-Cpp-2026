#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#define VECTOR_DEFAULT_CAPACITY 8
typedef struct
{
    size_t size;
    size_t capacity;
    char *data;
} Vector;
void vector_init(Vector *vector)
{
    vector->size = 0;
    vector->capacity = 0;
    vector->data = NULL;
}
void vector_free(Vector *vector)
{
    free(vector->data); // NULL is OK
    vector->data = NULL;
    vector->capacity = 0;
    vector->size = 0;
}
void vector_reserve(Vector *vector, size_t capacity)
{
    if (capacity <= vector->capacity)
        return;
    char *new_data = realloc(vector->data, sizeof(char) * capacity); // When data is NULL, realloc will act as if malloc
    if (!new_data)
    {
        perror("分配内存失败");
        exit(EXIT_FAILURE);
    }
    vector->data = new_data;
    vector->capacity = capacity;
}
void vector_shrink_to_fit(Vector *vector)
{
    if (vector->size >= vector->capacity)
        return;
    if (vector->size == 0)
    {
        free(vector->data);
        vector->data = NULL;
        vector->capacity = 0;
        return;
    }
    char *new_data = realloc(vector->data, sizeof(char) * vector->size);
    if (!new_data)
    {
        perror("分配内存失败");
        exit(EXIT_FAILURE);
    }
    vector->data = new_data;
    vector->capacity = vector->size;
}
void vector_push_back(Vector *vector, char c)
{
    if (vector->size >= vector->capacity)
    {
        size_t new_capacity;
        if (vector->capacity == 0)
            new_capacity = VECTOR_DEFAULT_CAPACITY;
        else if (vector->capacity > SIZE_MAX / 2)
        {
            fprintf(stderr, "Vector容量增长时发生溢出\n");
            exit(EXIT_FAILURE);
        }
        else
            new_capacity = vector->capacity * 2;
        vector_reserve(vector, new_capacity);
    }
    vector->data[vector->size] = c;
    vector->size++;
}
Vector vector_copy(const Vector *vector)
{
    Vector new_vector;
    vector_init(&new_vector);
    if (vector->size > 0)
    {
        vector_reserve(&new_vector, vector->size);
        new_vector.size = vector->size;
        memcpy(new_vector.data, vector->data, sizeof(char) * vector->size);
    }
    // for (size_t i = 0; i < vector->size; i++)
    //     new_vector.data[i] = vector->data[i];
    return new_vector;
}
char *vector_at(Vector *vector, size_t index)
{
    if (index < vector->size)
        return &(vector->data[index]);
    else
        return NULL;
}
size_t vector_size(const Vector *vector)
{
    return vector->size;
}
size_t vector_capacity(const Vector *vector)
{
    return vector->capacity;
}
char *vector_data(Vector *vector)
{
    return vector->data;
}
typedef struct
{
    size_t rows;
    size_t cols;
    char *cells;
} Board;
typedef struct
{
    int x;
    int y;
} Player;
typedef struct
{
    Board board;
    Player player;
    size_t steps;
} Level;
#define WALL '#'
#define FLOOR ' '
#define TARGET '.'
#define BOX '$'
#define BOX_ON_TARGET '*'
#define PLAYER '@'
#define PLAYER_ON_TARGET '+'
Level read_level(FILE *fp)
{
    size_t rows = 0;
    size_t cols = 0;
    Vector cells;
    vector_init(&cells);
    Player player = {.x = 0, .y = 0};
    int c;
    do
    {
        size_t current_cols = 0;
        for (;;)
        {
            c = fgetc(fp);
            if (c == EOF || c == '\n')
                break;
            switch (c)
            {
            case WALL:
            case FLOOR:
            case TARGET:
            case BOX:
            case BOX_ON_TARGET:
                vector_push_back(&cells, c);
                break;
            case PLAYER:
                player.x = rows, player.y = current_cols;
                vector_push_back(&cells, FLOOR);
                break;
            case PLAYER_ON_TARGET:
                player.x = rows, player.y = current_cols;
                vector_push_back(&cells, TARGET);
                break;
            default:
                fprintf(stderr, "文件内包含未知的符号: %c\n", c);
                exit(EXIT_FAILURE);
            }
            current_cols++;
        }
        if (cols == 0)
        {
            if (current_cols != 0)
                cols = current_cols;
            else if (c == EOF)
            {
                fprintf(stderr, "文件内没有有效关卡\n");
                exit(EXIT_FAILURE);
            }
        }
        else if (current_cols != cols)
        {
            fprintf(stderr, "文件内关卡列数不一致\n");
            exit(EXIT_FAILURE);
        }
        rows++;
    }
    while (c != EOF);
    vector_shrink_to_fit(&cells);
    Level level = {
        .board = {.rows = rows, .cols = cols, .cells = cells.data},
        .player = player,
        .steps = 0,
    };
    return level;
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
char *access_cell(Board *board, size_t i, size_t j)
{
    return &(board->cells[i * board->cols + j]);
}
void display(Level *level)
{
    printf("\033[H\033[J");
    for (size_t i = 0; i < level->board.rows; i++)
    {
        for (size_t j = 0; j < level->board.cols; j++)
        {
            char c = *access_cell(&level->board, i, j);
            if (i == level->player.x && j == level->player.y)
            {
                if (c == FLOOR)
                    c = PLAYER;
                else if (c == TARGET)
                    c = PLAYER_ON_TARGET;
                else
                {
                    fprintf(stderr, "玩家位置非法\n");
                    exit(EXIT_FAILURE);
                }
            }
            printf("%c", c);
        }
        printf("\n");
    }
    printf("Steps: %zu\n", level->steps);
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
        set_read_mode(0, 0); // 我发现VTIME设成0也可以，这样就消除了Esc推出时0.1秒的卡顿
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
bool is_win(Level *level)
{
    for (size_t i = 0; i < level->board.rows * level->board.cols; i++)
        if (level->board.cells[i] == '$')
            return false;
    return true;
}
void run(Level *level)
{
    while (!is_win(level))
    {
        display(level);
        Direction direction;
        for (;;)
        {
            Status status = read_key(&direction);
            if (status == OK)
                break;
            else if (status == QUIT)
                exit(0);
        }
        int new_x = level->player.x + offsets[direction].dx;
        int new_y = level->player.y + offsets[direction].dy;
        if (!(0 <= new_x && new_x < level->board.rows && 0 <= new_y && new_y < level->board.cols))
            continue;
        char *new_c = access_cell(&level->board, new_x, new_y);
        if (*new_c == WALL)
            continue;
        else if (*new_c == BOX || *new_c == BOX_ON_TARGET)
        {
            int new_box_x = new_x + offsets[direction].dx;
            int new_box_y = new_y + offsets[direction].dy;
            if (!(0 <= new_box_x && new_box_x < level->board.rows && 0 <= new_box_y && new_box_y < level->board.cols))
                continue;
            char *new_box_c = access_cell(&level->board, new_box_x, new_box_y);
            if (*new_box_c == FLOOR)
                *new_box_c = BOX;
            else if (*new_box_c == TARGET)
                *new_box_c = BOX_ON_TARGET;
            else
                continue;
            if (*new_c == BOX)
                *new_c = FLOOR;
            else if (*new_c == BOX_ON_TARGET)
                *new_c = TARGET;
        }
        level->player.x = new_x, level->player.y = new_y;
        level->steps++;
    }
    display(level);
    printf("You win!\n");
}
int main(int argc, char *argv[])
{
    static const char *level_files[] = {
        "level1.txt",
        "level2.txt",
        "level3.txt",
    };
    static const size_t level_files_len = sizeof(level_files) / sizeof(char *);
    size_t index = 0;
    if (argc >= 2)
    {
        char *endptr;
        index = strtoull(argv[1], &endptr, 10);
        if (*endptr != '\0')
        {
            fprintf(stderr, "地图index非法: %s\n", argv[1]);
            exit(EXIT_FAILURE);
        }
    }
    if (index >= level_files_len)
    {
        fprintf(stderr, "地图index非法: %zu\n", index);
        exit(EXIT_FAILURE);
    }
    set_terminal_mode(true);
    atexit(cleanup);
    const char *score_file = "score.txt";
    FILE *score_fp = fopen(score_file, "w"); // 清空分数文件
    if (score_fp == NULL)
    {
        perror("打开分数文件失败");
        exit(EXIT_FAILURE);
    }
    if (fclose(score_fp) == EOF)
    {
        perror("关闭分数文件失败");
        exit(EXIT_FAILURE);
    }
    while (index < level_files_len)
    {
        FILE *fp = fopen(level_files[index], "r");
        if (!fp)
        {
            perror("打开地图文件失败");
            exit(EXIT_FAILURE);
        }
        Level level = read_level(fp);
        if (fclose(fp) == EOF)
        {
            perror("关闭地图文件失败");
            exit(EXIT_FAILURE);
        }
        run(&level);
        size_t score = level.steps;
        free(level.board.cells);
        FILE *score_fp = fopen(score_file, "a");
        if (score_fp == NULL)
        {
            perror("打开分数文件失败");
            exit(EXIT_FAILURE);
        }
        fprintf(score_fp, "%s: %zu\n", level_files[index], score);
        if (fclose(score_fp) == EOF)
        {
            perror("关闭分数文件失败");
            exit(EXIT_FAILURE);
        }
        printf("按任意键继续\n");
        getchar();
        index++;
    }
    return 0;
}