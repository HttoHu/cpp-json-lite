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
        GROUP = 4,
        RAW
    };
    JSON();
    JSON(const std::string &str);

    JSON(const JSON &rhs);
    JSON(JSON &&rhs);

    JSON &operator=(const JSON &rhs);
    JSON &operator=(JSON &&rhs);

    JSONTYPE get_type() const;

    int64_t& get_int()const;
    std::string& get_str()const;
    std::vector<unsigned char> &get_raw() const;

    std::map<std::string, JSON> get_map() const;
    std::vector<JSON> get_list() const;

    JSON operator[](const std::string &str);
    JSON operator[](size_t idx);

    void add_pair(const std::string &str, JSON);
    void push(JSON);

    // copy json
    JSON clone() const;
    // for map
    size_t count() const;
    // for list
    size_t length() const;
    std::string view(std::string indent = "    ") const;
    std::string to_string(std::string indent = "    ") const;
    ~JSON();

    static JSON read_from_file(const std::string &filename);
    static JSON raw(const std::vector<unsigned char> &vec);
    static JSON raw(std::vector<unsigned char> &&vec);
    static JSON val(int val);
    static JSON val(const std::string &str);
    static JSON map(const std::map<std::string, JSON> &table);
    static JSON array(const std::vector<JSON> &vec);

private:
    friend JSON raw(const std::vector<unsigned char> &vec);
    friend JSON raw(std::vector<unsigned char> &&vec);

    JSON(bool _child, Parser::Node *n) : child(_child), node(n) {}
    JSON(Parser::Node *n);
    std::string stringify_unit(std::string indent, size_t indent_cnt, bool hide_raw) const;
    mutable bool child = false;
    Parser::Node *node;
};