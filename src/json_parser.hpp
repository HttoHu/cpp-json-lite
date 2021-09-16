#pragma once
#include <iostream>
#include <map>
#include <vector>
#include <set>
namespace Parser
{
    class Node;
}
class JSON
{
public:
    enum JSONTYPE
    {
        STRING = 1,
        INT = 2,
        ARRAY = 3,
        GROUP = 4
    };


    JSON(const std::string &str);

    JSON(const JSON &rhs);
    JSON &operator=(const JSON &rhs);
    JSONTYPE get_type() const;

    int64_t get_int() const;
    std::string get_str() const;
    std::map<std::string, JSON> get_map() const;
    std::vector<JSON> get_list() const;

    JSON operator[](const std::string &str);
    JSON operator[](size_t idx);

    // for map
    size_t count() const;
    // for list
    size_t length() const;

    std::string to_string(std::string indent = "    ") const;

    ~JSON();

private:
    JSON(Parser::Node *n);
    std::string stringify_unit(std::string indent, size_t indent_cnt) const;
    bool child = false;
    Parser::Node *node;
};