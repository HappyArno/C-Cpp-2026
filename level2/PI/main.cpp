// 下面的代码为 Linux 系统编写，在 Windows 上可能不能正常编译和运行
#include <array>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <sys/resource.h>
using namespace std;
constexpr size_t DIGITS = 10000;
constexpr size_t BASE_WIDTH = 9;
constexpr uint32_t BASE = 1'000'000'000;
constexpr size_t OUTPUT_BLOCKS = (DIGITS + BASE_WIDTH - 1) / BASE_WIDTH;
constexpr size_t GUARD_BLOCKS = 4;
constexpr size_t FRAC_BLOCKS = OUTPUT_BLOCKS + GUARD_BLOCKS;
// 使用10^9进制储存大数，便于十进制输出
struct FixedDecimal
{
    constexpr static size_t LENGTH = FRAC_BLOCKS + 1;
    // data[0]为整数部分；其余为小数部分
    array<uint32_t, LENGTH> data{}; // 零初始化
    FixedDecimal() = default;
    FixedDecimal(uint32_t integer)
    {
        data[0] = integer;
    }
    bool is_zero() const
    {
        for (size_t i = 0; i < LENGTH; i++)
            if (data[i] != 0)
                return false;
        return true;
    }
    void print() const
    {
        cout << data[0] << ".";
        for (size_t i = 1; i < OUTPUT_BLOCKS; i++)
            cout << setw(BASE_WIDTH) << setfill('0') << data[i];
        constexpr size_t LAST_DIGITS = DIGITS - (OUTPUT_BLOCKS - 1) * BASE_WIDTH;
        uint32_t divisor = 1;
        for (size_t i = LAST_DIGITS; i < BASE_WIDTH; i++)
            divisor *= 10;
        cout << setw(LAST_DIGITS) << setfill('0') << data[OUTPUT_BLOCKS] / divisor;
    }
    void add(const FixedDecimal &other)
    {
        uint64_t carry = 0;
        size_t i = LENGTH;
        while (i-- > 0)
        {
            uint64_t cur = uint64_t(data[i]) + other.data[i] + carry;
            carry = cur >= BASE;
            data[i] = cur - carry * BASE;
        }
        assert(carry == 0);
    }
    void sub(const FixedDecimal &other)
    {
        int64_t borrow = 0;
        size_t i = LENGTH;
        while (i-- > 0)
        {
            int64_t cur = int64_t(data[i]) - other.data[i] - borrow;
            borrow = cur < 0;
            data[i] = cur + borrow * BASE;
        }
        assert(borrow == 0);
    }
    void mul(uint32_t multiplier)
    {
        uint64_t carry = 0;
        size_t i = LENGTH;
        while (i-- > 0)
        {
            uint64_t cur = uint64_t(data[i]) * multiplier + carry;
            data[i] = cur % BASE;
            carry = cur / BASE;
        }
        assert(carry == 0);
    }
    void div(uint32_t divisor)
    {
        assert(divisor != 0);
        uint64_t rem = 0;
        for (size_t i = 0; i < LENGTH; i++)
        {
            uint64_t cur = rem * BASE + data[i];
            data[i] = cur / divisor;
            rem = cur % divisor;
        }
        // assert(rem == 0); // 直接丢弃
    }
};
// n*arctan(1/x) = n/x - n/(3*x^3) + n/(5*x^5) - n/(7*x^7) + ...
FixedDecimal scaled_arctan_inv(uint32_t n, uint32_t x)
{
    FixedDecimal term(n);
    term.div(x);
    FixedDecimal sum = term;
    uint32_t x2 = x * x;
    size_t i = 1;
    for (;;)
    {
        term.div(x2);
        FixedDecimal addend = term;
        addend.div(2 * i + 1);
        if (addend.is_zero())
            break;
        if (i % 2 == 1)
            sum.sub(addend);
        else
            sum.add(addend);
        i++;
    }
    return sum;
}
int main()
{
    auto start = chrono::steady_clock::now();
    // 马青(Machin)公式
    // pi = 16*arctan(1/5) - 4*arctan(1/239)
    FixedDecimal a = scaled_arctan_inv(16, 5);
    FixedDecimal b = scaled_arctan_inv(4, 239);
    a.sub(b);
    auto end = chrono::steady_clock::now();
    a.print();
    cout << endl;
    chrono::duration<double> elapsed = end - start;
    cout << "Time: " << elapsed.count() << "s" << endl;
    rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == -1)
    {
        cerr << "getrusage failed" << endl;
        return 1;
    }
    cout << "Peak memory: " << usage.ru_maxrss / 1024.0 << "MiB" << endl;
    return 0;
}