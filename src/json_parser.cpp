#include <iostream>
#include <algorithm>
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <cstring>
#include "json_parser.hpp"
namespace Lexer
{
    enum Tag
    {
        BEGIN,
        END,
        INTEGER,
        STRING,
        LSB,
        RSB,
        COMMA,
        COLON,
        END_LINE,
        END_TAG
    };

    extern std::map<Tag, std::string> tag_to_string;
    extern std::map<std::string, Tag> string_to_tag;

    class Token
    {
    public:
        Token(Tag _tag) : tag(_tag) {}
        virtual ~Token() {}
        virtual std::string to_string() const
        {
            if (tag_to_string.count(tag))
            {
                return tag_to_string[tag];
            }
            else
                throw std::runtime_error("Token::get_tag() unknown tag");
        }
        Tag get_tag() const
        {
            return tag;
        }
        virtual void print() const
        {
            std::cout << to_string();
        }

    protected:
        Tag tag;
    };

    class StringToken final : public Token
    {
    public:
        StringToken(const std::string &str) : Token(STRING), value(str) {}
        static std::string get_content(Token *tok)
        {
            return static_cast<StringToken *>(tok)->value;
        }
        std::string to_string() const override
        {
            return "<string:" + value + ">";
        }

    private:
        std::string value;
    };

    class Integer final : public Token
    {
    public:
        Integer(int64_t v) : Token(INTEGER), value(v) {}
        std::string to_string() const override
        {
            return "<integer:" + std::to_string(value) + ">";
        }
        static int64_t get_content(Token *tok)
        {
            return static_cast<Integer *>(tok)->value;
        }

    private:
        int64_t value;
    };

    class EndLine : public Token
    {
    public:
        EndLine() : Token(END_LINE) {}
        std::string to_string() const override
        {
            return "\n";
        }
    };

    class TokenStream
    {
    public:
        TokenStream() = default;

        ~TokenStream()
        {
            for (auto a : tokens)
                delete a;
        }
        void push(Token *tok) { tokens.push_back(tok); }
        Token *current()
        {
            if (cur_p >= tokens.size())
                throw std::runtime_error("TokenStream::current out of range");
            return tokens[cur_p];
        }

        void move_to_next()
        {
            cur_p++;
        }

        void match(Tag tag)
        {
            auto cur = current();
            if (cur->get_tag() == tag)
            {
                cur_p++;
                return;
            }
            throw std::runtime_error("TokenStream::match syntax error! token not matched");
        }
        Tag get_cur_tag()
        {
            return current()->get_tag();
        }
        void print()
        {
            for (auto tok : tokens)
                tok->print();
        }
        size_t size() const
        {
            return tokens.size();
        }

    private:
        std::vector<Token *> tokens;
        int cur_p = 0;
    };
    TokenStream build_token_stream(const std::string &str);
}
namespace Parser
{
    // Header
    // statement
    enum NodeType
    {
        STRING = 1,
        INT = 2,
        ARRAY = 3,
        GROUP = 4
    };
    class Node
    {
    public:
        Node(NodeType nt);
        int get_int();
        std::string get_str();
        Node *at(const std::string &str)
        {
            return operator[](str);
        }
        Node *at(size_t idx)
        {
            return operator[](idx);
        }
        Node *operator[](const std::string &str);
        Node *operator[](size_t idx);

        NodeType get_type() const { return type; }
        virtual ~Node();

    private:
        NodeType type;
    };

    class Unit : public Node
    {
    public:
        Unit(const std::string &str) : Node(STRING), is_number(false), text(str) {}
        Unit(int64_t v) : Node(INT), is_number(true), integer(v) {}
        static int64_t get_integer(Node *node);
        static std::string get_str(Node *node);
        ~Unit(){};

    private:
        bool is_number;
        std::string text;
        int64_t integer;
    };
    class Group : public Node
    {
    public:
        Group(const std::map<std::string, Node *> &tab);
        Node *operator[](const std::string &str) const;
        ~Group();
        size_t count() const;

    private:
        friend class ::JSON;
        std::map<std::string, Node *> member_table;
    };
    class Array : public Node
    {
    public:
        Array(const std::vector<Node *> &ele);
        Node *operator[](size_t idx) const;
        ~Array();
        size_t length() const;

    private:
        friend class ::JSON;
        std::vector<Node *> elements;
    };
}

namespace Lexer
{

    std::map<Tag, std::string> tag_to_string{
        {BEGIN, "{"}, {END, "}"}, {LSB, "["}, {RSB, "]"}, {COMMA, ","}, {COLON, ":"}, {END_TAG, "EOF"}

    };

    std::map<std::string, Tag> string_to_tag{
        {"{", BEGIN}, {"}", END}, {"[", LSB}, {"]", RSB}, {",", COMMA}, {":", COLON}};
    // number or string
    TokenStream build_token_stream(const std::string &str)
    {
        TokenStream token_stream;
        for (int i = 0; i < str.size(); i++)
        {
            char ch = str[i];
            if (isdigit(ch))
            {
                long long v = ch - '0';
                i++;
                while (i < str.size() && isdigit(str[i]))
                {
                    v *= 10;
                    v += str[i] - '0';
                    i++;
                }
                token_stream.push(new Integer(v));
                i--;
                continue;
            }

            if (ch == '\"')
            {
                std::string v;
                i++;
                while (i < str.size() && str[i] != '\"')
                {
                    if (str[i] == '\\')
                    {
                        if (i + 1 >= str.size())
                            throw std::runtime_error("build_token_stream: invalid string");
                        i++;
                        switch (str[i])
                        {
                        case 'r':
                            v += '\r';
                            break;
                        case 'n':
                            v += '\n';
                            break;
                        case 't':
                            v += '\t';
                            break;
                        case '\\':
                        case '\"':
                        case '\'':
                            v += str[i];
                            break;
                        default:
                            throw std::runtime_error("build_token_stream: invalid string");
                        }
                    }
                    else
                        v += str[i];
                    i++;
                }
                token_stream.push(new StringToken(v));
                continue;
            }

            switch (ch)
            {
            case '[':
            case ']':
            case '{':
            case '}':
            case ':':
            case ',':
                token_stream.push(new Token(string_to_tag[std::string(1, ch)]));
                break;
            case '\r':
            case '\n':
                token_stream.push(new EndLine());
                break;
            default:
                break;
            }
        }
        token_stream.push(new Token(END_TAG));
        return token_stream;
    }
}
namespace Parser
{
    // Node
    Node::Node(NodeType nt) : type(nt) {}
    int Node::get_int()
    {
        if (type == INT)
        {
            return Unit::get_integer(this);
        }
        else
            throw std::runtime_error("type not matched");
    }
    std::string Node::get_str()
    {
        if (type == STRING)
        {
            return Unit::get_str(this);
        }
        else
            throw std::runtime_error("type not matched");
    }
    Node *Node::operator[](const std::string &str)
    {
        if (type != GROUP)
        {
            throw std::runtime_error("type not matched, expected an array!");
        }
        return static_cast<Group *>(this)->operator[](str);
    }
    Node *Node::operator[](size_t idx)
    {
        if (type != ARRAY)
            throw std::runtime_error("type not matched, expected an array!");
        return static_cast<Array *>(this)->operator[](idx);
    }
    Node::~Node() {}
    // Unit
    int64_t Unit::get_integer(Node *node)
    {
        return static_cast<Unit *>(node)->integer;
    }
    std::string Unit::get_str(Node *node)
    {
        return static_cast<Unit *>(node)->text;
    }
    // Array
    Array::Array(const std::vector<Node *> &ele) : Node(ARRAY), elements(ele) {}
    Node *Array::operator[](size_t idx) const
    {
        if (idx >= elements.size())
            throw std::runtime_error("Array out of range!");
        return elements[idx];
    }
    Array::~Array()
    {
        for (auto e : elements)
            delete e;
    }
    size_t Array::length() const
    {
        return elements.size();
    }
    //Group
    Group::Group(const std::map<std::string, Node *> &tab) : Node(GROUP), member_table(tab) {}
    Node *Group::operator[](const std::string &str) const
    {
        auto it = member_table.find(str);
        if (it == member_table.end())
        {
            throw std::runtime_error("key " + str + " not found");
        }
        return it->second;
    }
    Group::~Group()
    {
        for (auto it : member_table)
            delete it.second;
    }
    size_t Group::count() const
    {
        return member_table.size();
    }

    Node *parse_unit(Lexer::TokenStream &token_stream);
    Array *parse_array(Lexer::TokenStream &ts)
    {
        ts.match(Lexer::LSB);
        if (ts.get_cur_tag() == Lexer::RSB)
        {
            ts.match(Lexer::RSB);
            return new Array({});
        }
        std::vector<Node *> vec;
        while (true)
        {
            vec.push_back(parse_unit(ts));
            if (ts.get_cur_tag() != Lexer::COMMA)
                break;
            ts.match(Lexer::COMMA);
        }
        ts.match(Lexer::RSB);
        return new Array(vec);
    }
    Group *parse_group(Lexer::TokenStream &ts)
    {
        ts.match(Lexer::BEGIN);
        if (ts.get_cur_tag() == Lexer::END)
        {
            ts.match(Lexer::END);
            return new Group({});
        }
        std::map<std::string, Node *> table;
        while (true)
        {
            auto variable_name = ts.current();
            ts.match(Lexer::STRING);
            ts.match(Lexer::COLON);
            table.insert({Lexer::StringToken::get_content(variable_name), parse_unit(ts)});
            if (ts.get_cur_tag() != Lexer::COMMA)
                break;
            ts.match(Lexer::COMMA);
        }
        ts.match(Lexer::END);
        return new Group(table);
    }
    Node *parse_unit(Lexer::TokenStream &ts)
    {
        switch (ts.get_cur_tag())
        {
        case Lexer::INTEGER:
        {
            auto v = Lexer::Integer::get_content(ts.current());
            ts.match(Lexer::INTEGER);

            return (Node *)(new Unit(v));
        }
        case Lexer::STRING:
        {
            auto front_part = ts.current();
            ts.match(Lexer::STRING);
            return (Node *)(new Unit(Lexer::StringToken::get_content(front_part)));
        }
        case Lexer::LSB:
        {
            return (Node *)(parse_array(ts));
        }
        case Lexer::BEGIN:
        {
            return (Node *)(parse_group(ts));
        }
        default:
            throw std::runtime_error(ts.current()->to_string() + " json-syntax error");
            break;
        }
    }
}

//              ===== JSON defination ======
JSON::JSON(const std::string &str) : child(false)
{
    auto ts = Lexer::build_token_stream(str);
    node = Parser::parse_unit(ts);
}
JSON::JSON(Parser::Node *n) : child(true), node(n) {}
JSON::JSON(const JSON &rhs) : child(true), node(rhs.node) {}
JSON &JSON::operator=(const JSON &rhs)
{
    child = true;
    node = rhs.node;
    return *this;
}

JSON::JSONTYPE JSON::get_type() const
{
    return (JSONTYPE)node->get_type();
}
int64_t JSON::get_int() const
{
    return node->get_int();
}
std::string JSON::get_str() const
{
    return node->get_str();
}

std::map<std::string, JSON> JSON::get_map() const
{
    if (node->get_type() != Parser::GROUP)
        throw std::runtime_error("JSON::get_keys(): expected a GROUP");
    std::map<std::string, JSON> ret;
    auto &tmp = static_cast<Parser::Group *>(node)->member_table;
    for (auto val : tmp)
    {
        ret.insert({val.first, JSON(val.second)});
    }
    return ret;
}
std::vector<JSON> JSON::get_list() const
{
    if (node->get_type() != Parser::ARRAY)
        throw std::runtime_error("JSON::get_list(): expected an array");
    auto &tmp = static_cast<Parser::Array *>(node)->elements;
    std::vector<JSON> ret;
    for (auto val : tmp)
    {
        ret.push_back(val);
    }
    return ret;
}

JSON JSON::operator[](const std::string &str)
{
    return JSON(node->operator[](str));
}
JSON JSON::operator[](size_t idx)
{
    return JSON(node->operator[](idx));
}
size_t JSON::count() const
{
    if (node->get_type() == Parser::GROUP)
        return static_cast<Parser::Group *>(node)->count();
    return 0;
}
size_t JSON::length() const
{
    if (node->get_type() == Parser::ARRAY)
        return static_cast<Parser::Array *>(node)->length();
    return 0;
}
namespace
{
    std::string conv_str(const std::string &str)
    {
        std::string ret;
        for (auto ch : str)
        {
            switch (ch)
            {
            case '\r':
                ret += "\\r";
            case '\n':
                ret += "\\n";
                break;
            case '\t':
                ret += "\\t";
                break;
            case '\\':
                ret += "\\";
                break;
            default:
                ret += ch;
                break;
            }
        }
        return ret;
    }

}

std::string JSON::stringify_unit(std::string indent, size_t indent_cnt) const
{
    if (get_type() == JSON::INT)
        return std::to_string(get_int());
    else if (get_type() == JSON::STRING)
        return "\"" + conv_str(get_str()) + "\"";

    std::string indent_prefix;
    indent_prefix.reserve(indent_cnt * indent.size());
    for (int i = 1; i <= indent_cnt; i++)
        indent_prefix += indent;

    if (get_type() == JSON::ARRAY)
    {
        std::string ret = "[\n";
        auto &vec = static_cast<Parser::Array *>(node)->elements;
        for (size_t i = 0; i < vec.size(); i++)
        {
            ret += indent_prefix + indent + JSON(vec[i]).stringify_unit(indent, indent_cnt + 1);
            if (i != vec.size() - 1)
            {
                ret += ",";
            }
            ret += "\n";
        }
        ret += indent_prefix + "]";
        return ret;
    }
    if (get_type() == JSON::GROUP)
    {
        std::string ret = "{\n";
        auto &mp = static_cast<Parser::Group *>(node)->member_table;
        size_t idx = 0;
        for (auto pair : mp)
        {
            ret += indent_prefix + indent + "\"" + pair.first + "\": " + JSON(pair.second).stringify_unit(indent, indent_cnt + 1);
            if (++idx != mp.size())
                ret += ",\n";
            else
                ret += "\n";
        }
        ret += indent_prefix + "}";
        return ret;
    }
    return "null";
}

std::string JSON::to_string(std::string indent) const
{
    return stringify_unit(indent, 0);
}
JSON::~JSON()
{
    if (!child)
        delete node;
}
//             end ===== JSON defination ======