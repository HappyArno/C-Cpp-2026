#include <iostream>
#include <memory>
#include <vector>
class Animal
{
public:
    virtual void make_sound() const = 0;
    virtual ~Animal() = default;
};
class Dog : public Animal
{
public:
    void make_sound() const override
    {
        std::cout << "Woof" << std::endl;
    }
};
class Cat : public Animal
{
public:
    void make_sound() const override
    {
        std::cout << "Meow" << std::endl;
    }
};
class Bird : public Animal
{
public:
    void make_sound() const override
    {
        std::cout << "Tweet" << std::endl;
    }
};
class Wolfdog : public Dog
{
public:
    void make_sound() const override
    {
        std::cout << "Howl" << std::endl;
    }
};
class Zoo
{
    std::vector<std::unique_ptr<Animal>> animals;
public:
    template <typename... Args>
    Zoo(Args &&...args)
    {
        animals.reserve(sizeof...(Args));
        (animals.emplace_back(std::forward<Args>(args)), ...);
    }
    void push_back(std::unique_ptr<Animal> animal)
    {
        animals.push_back(std::move(animal));
    }
    template <typename T, typename... Args>
    void add_animal(Args &&...args)
    {
        animals.emplace_back(std::make_unique<T>(std::forward<Args>(args)...));
    }
    void make_sounds() const
    {
        for (auto &animal : animals)
            animal->make_sound();
    }
};
int main()
{
    Zoo zoo{std::make_unique<Dog>(), std::make_unique<Cat>()};
    zoo.add_animal<Bird>();
    zoo.push_back(std::make_unique<Wolfdog>());
    zoo.make_sounds();
    return 0;
}