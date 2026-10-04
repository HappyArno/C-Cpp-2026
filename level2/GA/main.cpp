// 这段代码实现了一个用于迷宫寻路的遗传算法，但是在类似deceptive maze的情况下不保证能够找到解（如maze1.txt）
#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <vector>
using namespace std;
constexpr size_t POPULATION_SIZE = 200;
constexpr size_t ELITE_COUNT = 5;
constexpr double CROSSOVER_RATE = 0.2; // 使用传统高比例0.8反而效果不好，可能是因为crossover时cut的步数一致并不代表在迷宫中位置一致，导致容易破坏优秀解
constexpr double MUTATION_RATE = 0.05; // 使用0.01反而效果不好，可能是因为需要积极变异探索
constexpr size_t NUM_GENERATIONS = 200;
struct Position
{
    int x = 0;
    int y = 0;
    Position &operator+=(const Position &other) noexcept
    {
        x += other.x;
        y += other.y;
        return *this;
    }
    friend Position operator+(Position lhs, const Position &rhs) noexcept
    {
        lhs += rhs;
        return lhs;
    }
    friend bool operator==(const Position &lhs, const Position &rhs)
    {
        return lhs.x == rhs.x && lhs.y == rhs.y;
    }
    friend bool operator!=(const Position &lhs, const Position &rhs)
    {
        return !(lhs == rhs);
    }
};
enum class Direction
{
    up = 0,
    down = 1,
    left = 2,
    right = 3,
};
Position to_offset(Direction direction)
{
    // constexpr Position offsets[4] = {
    //     {-1, 0},
    //     {1, 0},
    //     {0, -1},
    //     {0, 1},
    // };
    constexpr array<Position, 4> offsets = {{
        {-1, 0},
        {1, 0},
        {0, -1},
        {0, 1},
    }};
    return offsets[static_cast<size_t>(direction)];
}
using Chromosome = vector<Direction>;
mt19937 &get_generator()
{
    static random_device::result_type seed = (random_device())();
    static mt19937 generator(seed);
    return generator;
}
int random_integer(int min, int max)
{
    mt19937 &generator = get_generator();
    uniform_int_distribution<int> distribution(min, max);
    return distribution(generator);
}
bool random_bernoulli(double possibility)
{
    mt19937 &generator = get_generator();
    bernoulli_distribution distribution(possibility);
    return distribution(generator);
}
Direction random_direction()
{
    mt19937 &generator = get_generator();
    static uniform_int_distribution<int> distribution(0, 3);
    return static_cast<Direction>(distribution(generator));
}
Chromosome random_chromosome(size_t length)
{
    Chromosome chromosome;
    for (size_t i = 0; i < length; i++)
        chromosome.push_back(random_direction());
    return chromosome;
}
struct Population
{
    vector<Chromosome> chromosomes;
    void random(size_t size, size_t length)
    {
        for (size_t i = 0; i < size; i++)
            chromosomes.push_back(random_chromosome(length));
    }
};
struct Maze
{
    vector<string> map;
    Position start{};
    Position end{};
    size_t rows = 0;
    size_t cols = 0;
    bool is_valid(const Position &p) const
    {
        if (!(0 <= p.x && p.x < map.size() && 0 <= p.y && p.y < map[p.x].size()))
            return false;
        if (map[p.x][p.y] == '#')
            return false;
        return true;
    }
    char &at(const Position &p)
    {
        return map.at(p.x).at(p.y);
    }
};
Maze read_maze(ifstream &file)
{
    Maze maze;
    string line;
    while (getline(file, line))
    {
        if (!line.empty() && line.back() == '\r') // Fuck windows \r\n
            line.pop_back();
        for (int y = 0; y < line.size(); y++)
            if (line[y] == 'S')
                maze.start = {.x = static_cast<int>(maze.map.size()), .y = y};
            else if (line[y] == 'E')
                maze.end = {.x = static_cast<int>(maze.map.size()), .y = y};
        if (line.size() > maze.cols)
            maze.cols = line.size();
        maze.rows++;
        maze.map.push_back(line);
    }
    return maze;
}
double calculate_fitness(const Maze &maze, const Chromosome &chromosome)
{
    Position player = maze.start;
    size_t crash = 0;
    size_t steps = 0;
    bool is_win = false;
    for (const Direction direction : chromosome)
    {
        Position offset = to_offset(direction);
        Position new_player = player + offset;
        if (!maze.is_valid(new_player))
        {
            crash++;
            continue;
        }
        steps++;
        player = new_player;
        if (new_player == maze.end)
        {
            is_win = true;
            break;
        }
    }
    // if (is_win)
    // {
    //     cout << "win! ";
    //     exit(0);
    // }
    size_t distance = abs(maze.end.x - player.x) + abs(maze.end.y - player.y);
    // 这个版本找出的解几乎都会频繁回头/撞墙/绕路，这也许是因为方向路径对前面的走法有很强的依赖性，导致前期走法一旦变动（不管好坏）几乎都会导致无法到达终点，分数大幅变低，难以被选上，所以我引入了simplify机制简化路径来解决
    // 但是其实这个版本也不保证能找到解，可能是因为对于某些特定的迷宫，使用曼哈顿距离而非真实距离很容易导致陷入局部最优解，在隔着墙但最靠近的位置徘徊；直接使用真实距离应该可以解决这个问题，但是失去了遗传算法的意义
    // 或许后面可以考虑引入novelty search来解决容易陷入局部最优解的问题
    if (is_win)
        return 10000 - steps - crash * 2;
    else
        return 5000 - distance * 100 - crash;
    // 这个版本效果不理想，可能是因为在未找到终点时crash占比过高导致路径的进化目标更倾向于不撞墙
    // if (is_win)
    //     return 10000 - steps - crash * 2;
    // else
    //     return 5000 - distance * 2 - crash * 10;
    // 这个版本效果不理想，可能是因为在未找到终点时steps占比过高导致更倾向于局部最优解而非绕路的真正解
    // return 5000 + (is_win ? 5000 : 0) - 2 * distance - 10 * crash - steps;
}
vector<pair<double, Chromosome>> calculate_fitness(const Maze &maze, const Population &population)
{
    vector<pair<double, Chromosome>> fitnesses;
    for (const auto &chromosome : population.chromosomes)
        fitnesses.emplace_back(calculate_fitness(maze, chromosome), chromosome);
    sort(fitnesses.begin(), fitnesses.end(), greater<pair<double, Chromosome>>());
    return fitnesses;
}
const pair<double, Chromosome> &tournament_select(const vector<pair<double, Chromosome>> &fitnesses)
{
    const pair<double, Chromosome> *best = nullptr;
    for (size_t i = 0; i < 3; i++)
    {
        size_t index = random_integer(0, fitnesses.size() - 1);
        if (!best || fitnesses.at(index).first > best->first)
            best = &fitnesses.at(index);
    }
    return *best;
}
Chromosome crossover(const Chromosome &parent1, const Chromosome &parent2)
{
    Chromosome child = parent1;
    size_t cut = random_integer(0, parent1.size() - 1);
    for (size_t i = cut; i < child.size(); i++)
        child[i] = parent2.at(i);
    return child;
}
Chromosome mutation(const Chromosome &chromosome)
{
    Chromosome child = chromosome;
    for (auto &gene : child)
        if (random_bernoulli(MUTATION_RATE))
            gene = random_direction();
    return child;
}
Population evolve(const Maze &maze, const Population &population)
{
    vector<pair<double, Chromosome>> fitnesses = calculate_fitness(maze, population);
    Population next;
    for (size_t i = 0; i < ELITE_COUNT; i++)
        next.chromosomes.push_back(fitnesses.at(i).second);
    for (size_t i = ELITE_COUNT; i < population.chromosomes.size(); i++)
    {
        const Chromosome &parent1 = tournament_select(fitnesses).second;
        const Chromosome &parent2 = tournament_select(fitnesses).second;
        Chromosome child = random_bernoulli(CROSSOVER_RATE) ? crossover(parent1, parent2) : parent1;
        child = mutation(child);
        next.chromosomes.push_back(child);
    }
    return next;
}
void print_path(Maze maze, const Chromosome &path)
{
    Position player = maze.start;
    bool is_win = false;
    size_t steps = 0;
    size_t crash = 0;
    for (const Direction direction : path)
    {
        Position offset = to_offset(direction);
        Position new_player = player + offset;
        if (!maze.is_valid(new_player))
        {
            crash++;
            continue;
        }
        steps++;
        player = new_player;
        if (new_player == maze.end)
        {
            is_win = true;
            break;
        }
        maze.at(new_player) = '.';
    }
    cout << "is_win: " << is_win << endl;
    cout << "steps: " << steps << endl;
    cout << "crash: " << crash << endl;
    for (const string &line : maze.map)
        cout << line << endl;
}
// 去除crash和loop
Chromosome simplify(Maze maze, const Chromosome &path)
{
    Position player = maze.start;
    vector<pair<Direction, Position>> simplified;
    maze.at(player) = '.';
    for (const Direction direction : path)
    {
        // cout << static_cast<int>(direction) << endl;
        // for (const string &line : maze.map)
        //     cout << line << endl;
        // cout << endl;
        Position offset = to_offset(direction);
        Position new_player = player + offset;
        if (!maze.is_valid(new_player))
            continue;
        if (maze.at(new_player) == '.')
        {
            // 在第一步走出去第二步就退回原点的情况下，由于原点没有被simplified记录，会一直回溯到空导致.back()奔溃，只要提前判空就可以解决
            while (!simplified.empty() && simplified.back().second != new_player)
            {
                maze.at(simplified.back().second) = ' ';
                simplified.pop_back();
            }
            player = new_player;
            continue;
        }
        simplified.emplace_back(direction, new_player);
        maze.at(new_player) = '.';
        player = new_player;
        if (new_player == maze.end)
            break;
    }
    Chromosome result;
    for (const auto &p : simplified)
        result.push_back(p.first);
    return result;
}
int main()
{
    Maze maze;
    {
        ifstream file("maze.txt");
        maze = read_maze(file);
    }
    auto start_time = chrono::steady_clock::now();
    size_t length = maze.rows * maze.cols;
    Population population;
    population.random(POPULATION_SIZE, length);
    for (size_t i = 0; i < NUM_GENERATIONS; i++)
    {
        // cout << i << " " << flush;
        population = evolve(maze, population);
    }
    vector<pair<double, Chromosome>> fitnesses = calculate_fitness(maze, population);
    Chromosome answer = fitnesses[0].second;
    Chromosome simplified = simplify(maze, answer);
    auto end_time = chrono::steady_clock::now();
    auto time = chrono::duration_cast<chrono::milliseconds>(end_time - start_time);
    cout << "time: " << time.count() << " ms" << endl;
    // print_path(maze, answer);
    print_path(maze, simplified);
    return 0;
}