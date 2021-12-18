#include "json_parser.hpp"
#include <fstream>
#include <string>
#include <iostream>
#include <bitset>
using namespace std;
int get_char_size(unsigned char ch)
{
    unsigned char header = ch >> 4;
    int len = 0;
    if ((header >> 3 & 1) == 0)
        return 1;
    for (int k = 3; k >= 1; k--)
    {
        if (header >> k & 1)
            len++;
    }
    return len;
}
int main()
{
    JSON js = JSON::read_from_file("config.json");
    std::cout << js.view() << std::endl;
    return 0;
}