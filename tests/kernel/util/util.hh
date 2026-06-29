#ifndef UTILS_HH
#define UTILS_HH

#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <string>

template <typename T>
void print_vector(const std::vector<T> &v)
{
    for (auto x : v)
        std::cout << x << ' ';
    std::cout << '\n';
}

template <typename T>
void read_array_from_file(const std::string &path, std::vector<T> &data)
{
    std::ifstream file(path);
    data.clear();

    std::string line;
    while (std::getline(file, line))
    {
        std::istringstream ss(line);
        T val;
        ss >> val;
        data.push_back(val);
    }
}

#endif // UTILS_HH
