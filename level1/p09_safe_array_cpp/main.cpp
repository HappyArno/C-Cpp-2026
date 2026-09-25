#include <algorithm>
#include <initializer_list>
#include <iostream>
#include <stdexcept>
template <typename T, size_t N>
class SafeArray
{
    T array[N]{};
public:
    constexpr static size_t size = N;
    using value_type = T;
    SafeArray() = default;
    SafeArray(std::initializer_list<T> list)
    {
        std::copy_n(list.begin(), std::min(list.size(), N), array);
    }
    T &at(size_t index)
    {
        if (index >= N)
            throw std::out_of_range("index out of range");
        return array[index];
    }
    const T &at(size_t index) const
    {
        if (index >= N)
            throw std::out_of_range("index out of range");
        return array[index];
    }
    // 为了保证这个类是安全的，这里的行为和std::array不同，operator[]也会进行检查
    T &operator[](size_t index)
    {
        return at(index);
    }
    const T &operator[](size_t index) const
    {
        return at(index);
    }
};
int main()
{
    SafeArray<int, 5> array{1, 2, 3, 4, 5};
    for (size_t i = 0; i < array.size; i++)
        std::cout << array[i] << " ";
    std::cout << std::endl;
    return 0;
}