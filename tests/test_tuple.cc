#include <iostream>
#include <vector>
#include <tuple>
#include "src/webserver.h"
int main() {
    // 定义一个std::vector，其中包含std::tuple元素
    std::vector<std::tuple<std::string, std::string, int>> vec;

    // 向vec中添加元素
    vec.push_back(std::make_tuple("Alice", "Smith", 25));
    vec.push_back(std::make_tuple("Bob", "Johnson", 30));

    // 遍历vec中的元素并输出
    for (const auto& tuple : vec) {
        std::cout << "First Name: " << std::get<0>(tuple)
                  << ", Last Name: " << std::get<1>(tuple)
                  << ", Age: " << std::get<2>(tuple) << std::endl;
    }
    /*
        First Name: Alice, Last Name: Smith, Age: 25
        First Name: Bob, Last Name: Johnson, Age: 30
    */
    return 0;
}
