/*
参考文献：
[跳跃列表](https://zh.wikipedia.org/wiki/跳跃列表)
[跳表](https://oi-wiki.org/ds/skiplist/)
[SkipList的原理与实现](https://zhuanlan.zhihu.com/p/33674267)
[SkipListPro](https://github.com/HiWong/SkipListPro)
*/
// 实现一个升序的跳表
#include <iostream>
#include <memory>
#include <random>
constexpr size_t MAX_LEVEL = 16;
constexpr double PROBABILITY = 0.25;
// 随机层数，范围为[1, MAX_LEVEL]
size_t random_level()
{
    thread_local std::random_device::result_type seed = (std::random_device())();
    thread_local std::mt19937 generator(seed);
    thread_local std::bernoulli_distribution distribution(PROBABILITY);
    size_t level = 1;
    while (distribution(generator) && level < MAX_LEVEL)
        level++;
    return level;
}
template <typename K, typename V>
struct Node
{
    K key;
    V value;
    size_t level; // 层数，范围[1, MAX_LEVEL]
    std::shared_ptr<Node> forward[MAX_LEVEL];
    Node() = default;
    Node(size_t l) : key(), value(), level(l) {}
    Node(const K &k, const V &v, size_t l) : key(k), value(v), level(l) {}
};
template <typename K, typename V>
struct SkipList
{
    // Node中的forward数组会使用默认构造函数生成空的shared_ptr，符合这里指向空的语义，所以直接初始化即可
    std::shared_ptr<Node<K, V>> dummy_head = std::make_shared<Node<K, V>>(MAX_LEVEL);
    SkipList(const SkipList &) = delete;
    SkipList &operator=(const SkipList &) = delete;
    SkipList() = default;
    V *find(const K &key)
    {
        std::shared_ptr<Node<K, V>> node = dummy_head;
        size_t level = MAX_LEVEL;
        while (level-- > 0)
            while (node->forward[level] && node->forward[level]->key < key)
                node = node->forward[level];
        node = node->forward[0];
        if (node && node->key == key)
            return &node->value;
        return nullptr;
    }
    void insert(const K &key, const V &value)
    {
        std::shared_ptr<Node<K, V>> update[MAX_LEVEL];
        std::shared_ptr<Node<K, V>> node = dummy_head;
        size_t level = MAX_LEVEL;
        while (level-- > 0)
        {
            while (node->forward[level] && node->forward[level]->key < key)
                node = node->forward[level];
            update[level] = node;
        }
        node = node->forward[0];
        if (node && node->key == key)
        {
            node->value = value;
            return;
        }
        level = random_level();
        std::shared_ptr<Node<K, V>> new_node = std::make_shared<Node<K, V>>(key, value, level);
        for (size_t i = 0; i < level; i++)
        {
            new_node->forward[i] = update[i]->forward[i];
            update[i]->forward[i] = new_node;
        }
    }
    void erase(const K &key)
    {
        std::shared_ptr<Node<K, V>> update[MAX_LEVEL];
        std::shared_ptr<Node<K, V>> node = dummy_head;
        size_t level = MAX_LEVEL;
        while (level-- > 0)
        {
            while (node->forward[level] && node->forward[level]->key < key)
                node = node->forward[level];
            update[level] = node;
        }
        node = node->forward[0];
        if (!(node && node->key == key))
            return;
        for (size_t i = 0; i < node->level; i++)
            update[i]->forward[i] = node->forward[i];
    }
    void print() const
    {
        for (std::shared_ptr<Node<K, V>> node = dummy_head->forward[0]; node; node = node->forward[0])
            std::cout << node->key << ": " << node->value << std::endl;
    }
};
int main()
{
    SkipList<int, int> list;
    list.insert(1, 11);
    list.insert(2, 22);
    list.insert(3, 33);
    const auto find_key = [&list](const auto &key) {
        auto *value = list.find(key);
        std::cout << key << ": ";
        if (value)
            std::cout << *value;
        else
            std::cout << "Not found";
        std::cout << std::endl;
    };
    find_key(1), find_key(2), find_key(3);
    std::cout << std::endl;
    list.erase(1);
    find_key(1), find_key(2), find_key(3);
    std::cout << std::endl;
    list.print();
    return 0;
}