#include <stdbool.h>
#include <stdio.h>
bool is_prime(unsigned long long n)
{
    if (n < 2)
        return false;
    for (unsigned long long i = 2; i <= n / i; i++)
        if (n % i == 0)
            return false;
    return true;
}
int main()
{
    unsigned long long n;
    scanf("%llu", &n);
    if (is_prime(n))
        printf("Prime\n");
    else
        printf("Not prime\n");
    return 0;
}