#include <future>
#include <iostream>
#include <string>
#include <functional>
 
void func(std::string str)
{
    if (str.compare("Hello World 1") == 0)
    {
        char pause = getchar(); // 按回车显示
    }
    std::cout << str << std::endl;
}
 
void Print(std::function< void(std::string)> Functional)
{
    auto a = std::async(std::launch::async, Functional, "Hello World 1");
    auto b = std::async(std::launch::async, Functional, "Hello World 2");
    auto c = std::async(std::launch::async, Functional, "Hello World 3");
}
 
int main()
{
    Print(func);
    return 0;
}