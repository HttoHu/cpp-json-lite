#include <iostream>
#include <algorithm>
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <cstring>
#include "json_parser.hpp"

// some utils functions
namespace
{
    // convert string to literal
    std::string conv_str(const std::string &str)
    {
        std::string ret;
        for (auto ch : str)
        {
            switch (ch)
            {
            case '\"':
                ret += "\\\"";
                break;
            case '\'':
                ret += "\\\'";
                break;
            case '\r':
                ret += "\\r";
                break;
            case '\n':
                ret += "\\n";
                break;
            case '\t':
                ret += "\\t";
                break;
            case '\\':
                ret += "\\\\";
                break;
            default:
                ret += ch;
                break;
            }
        }
        return ret;
    }
    // utf-8 char size
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
    // to parse a number from str+i
    long long get_number(const std::string &str, int &i)
    {
        long long v = str[i] - '0';
        i++;
        while (i < str.size() && isdigit(str[i]))
        {
            v *= 10;
            v += str[i] - '0';
            i++;
        }
        i--;
        return v;
    }
    // to parse a word or number
    std::string get_word(const std::string &str, int &i)
    {
        int sp = i;
        int len = 0;
        while (i < str.size() && isalpha(str[i]))
            i++, len++;
        i--;
        return str.substr(sp, len);
    }
}
// Lexer to scan the string and split them to tokens
namespace Lexer
{
    enum Tag
    {
        BEGIN,
        END,
        INTEGER,
        STRING,
        RAW_DATA,
        LSB,
        RSB,
        LPAR,
        RPAR,
        DOLLAR,
        COMMA,
        COLON,
        END_LINE,
        END_TAG
    };

    static std::map<Tag, std::string> &tag_to_string()
    {
        static std::map<Tag, std::string> tab{
            {BEGIN, "{"}, {END, "}"}, {LSB, "["}, {RSB, "]"}, {COMMA, ","}, {COLON, ":"}, {END_TAG, "EOF"}, {LPAR, "("}, {RPAR, ")"}, {DOLLAR, "$"}};
        return tab;
    }
    static std::map<std::string, Tag> &string_to_tag()
    {
        static std::map<std::string, Tag> tab{
            {"{", BEGIN}, {"}", END}, {"[", LSB}, {"]", RSB}, {",", COMMA}, {":", COLON}, {"(", LPAR}, {")", RPAR}, {"$", DOLLAR}};
        return tab;
    }
    static std::map<char, std::string> &escape_tab()
    {
        static std::map<char, std::string> escape_tab{
            {'\"', "\\\""},
            {'\\', "\\\\"}};
        return escape_tab;
    }

    class Token
    {
    public:
        Token(Tag _tag) : tag(_tag) {}
        virtual ~Token() {}
        virtual std::string to_string() const
        {
            if (tag_to_string().count(tag))
            {
                return tag_to_string()[tag];
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

    class RawData : public Token
    {
    public:
        RawData(const std::vector<unsigned char> &dat) : Token(RAW_DATA), data(dat) {}
        RawData(std::vector<unsigned char> &&dat) : Token(RAW_DATA), data(std::move(dat)) {}
        static std::vector<unsigned char> get_raw_data(Token *tok)
        {
            return static_cast<RawData *>(tok)->data;
        }
        std::string to_string() const override
        {
            return "<raw:" + std::to_string(data.size()) + " bytes>";
        }

    private:
        std::vector<unsigned char> data;
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
            while (cur_p < tokens.size() && tokens[cur_p]->get_tag() == Tag::END_LINE)
                cur_p++;
            return tokens[cur_p];
        }

        void move_to_next()
        {
            cur_p++;
        }

        void match(Tag tag)
        {
            while (get_cur_tag() == Tag::END_LINE)
                cur_p++;
            if (get_cur_tag() == tag)
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
    
    // cpp json lite supports insert raw binary data to the json
    RawData *get_raw_data(const std::string &str, int &i)
    {
        // skip (
        i++;
        int len = 0;
        while (i + len < str.size() && str[i + len] != ')')
            len++;
        if (i + len >= str.size())
            throw std::runtime_error("invalid raw_data format! use (length)$raw_content$ to define a raw data");

        int sz = std::stoi(str.substr(i, len));
        // skip )
        i += len + 1;
        if (str[i] != '$')
            throw std::runtime_error("invalid raw_data format expected a $!");
        // skip $
        i++;
        if (i + sz >= str.size())
            throw std::runtime_error("invalid raw_data format may be loss right $? ");
        // raw_data;
        std::string tmp_str = str.substr(i, sz);

        std::vector<unsigned char> vec(tmp_str.begin(), tmp_str.end());
        // skip raw_data
        i += sz;
        if (i >= str.size() || str[i] != '$')
            throw std::runtime_error("invalid raw_data format may be loss right $!");
        return new RawData(std::move(vec));
    }

    TokenStream *build_token_stream(const std::string &str)
    {
        TokenStream *token_stream = new TokenStream();
        for (int i = 0; i < str.size(); i++)
        {
            char ch = str[i];
            if (isdigit(ch))
            {
                token_stream->push(new Integer(get_number(str, i)));
                continue;
            }
            else if (ch == '(')
            {
                token_stream->push(get_raw_data(str, i));
                continue;
            }
            if (ch == '\"')
            {
                std::string v;
                i++;
                while (i < str.size() && str[i] != '\"')
                {
                    // process UTF8;
                    int len = get_char_size(str[i]);
                    if (len == 1)
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
                    else
                    {
                        for (int k = 0; k < len; k++, i++)
                        {
                            v += str[i];

                            if (i >= str.size())
                            {
                                throw std::runtime_error("Lexer Error: invalid UTF8 string\n");
                            }
                        }
                    }
                }
                token_stream->push(new StringToken(v));
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
                token_stream->push(new Token(string_to_tag()[std::string(1, ch)]));
                break;
            case '\r':
            case '\n':
                token_stream->push(new EndLine());
                break;
            default:
                if (isalpha(ch))
                {
                    std::string word = get_word(str, i);
                    if (word == "null" || word == "false")
                    {
                        token_stream->push(new Integer(0));
                    }
                    else if (word == "true")
                    {
                        token_stream->push(new Integer(1));
                    }
                    else
                    {
                        throw std::runtime_error("unexcepted word: " + word);
                    }
                }
                break;
            }
        }
        token_stream->push(new Token(END_TAG));
        return token_stream;
    }

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
        GROUP = 4,
        RAW = 5
    };
    class Node
    {
    public:
        Node(NodeType nt) : type(nt) {}
        int64_t &get_int();
        std::string &get_str();
        std::vector<unsigned char> &get_raw();
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
        static int64_t &get_integer(Node *node);
        static std::string &get_str(Node *node);
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
    // extend json. (length)$raw_data$
    class Bytes : public Node
    {
    public:
        Bytes(const std::vector<unsigned char> &tmp) : Node(RAW), data(tmp) {}
        Bytes(std::vector<unsigned char> &&tmp) : Node(RAW), data(std::move(tmp)) {}
        size_t raw_length() const
        {
            return data.size();
        }
        static std::vector<unsigned char> &get_bytes(Node *node)
        {
            return static_cast<Bytes *>(node)->data;
        }

    private:
        std::vector<unsigned char> data;
    };
}

namespace Parser
{
    int64_t &Node::get_int()
    {
        if (type == INT)
        {
            return Unit::get_integer(this);
        }
        else
            throw std::runtime_error("type not matched");
    }
    std::vector<unsigned char> &Node::get_raw()
    {
        if (type == RAW)
        {
            return Bytes::get_bytes(this);
        }
        else
        {
            throw std::runtime_error("type not matched excepted a bytes");
        }
    }
    std::string &Node::get_str()
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
    int64_t &Unit::get_integer(Node *node)
    {
        return static_cast<Unit *>(node)->integer;
    }
    std::string &Unit::get_str(Node *node)
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
    // Group
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
        case Lexer::RAW_DATA:
        {
            std::vector<unsigned char> &&v = Lexer::RawData::get_raw_data(ts.current());
            ts.match(Lexer::RAW_DATA);

            return (Node *)(new Bytes(std::move(v)));
        }
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

//              ===== JSON implementation ======
// constructor
JSON::JSON() : JSON("{}")
{
}
JSON::JSON(const std::string &str) : child(false)
{
    auto ts = Lexer::build_token_stream(str);
    node = Parser::parse_unit(*ts);

    delete ts;
}
JSON::JSON(Parser::Node *n) : child(true), node(n) {}
JSON::JSON(const JSON &rhs) : child(rhs.child), node(rhs.node)
{
    rhs.child = true;
}
JSON::JSON(JSON &&rhs) : child(rhs.child), node(rhs.node)
{
    rhs.child = true;
}

JSON &JSON::operator=(const JSON &rhs)
{
    child = rhs.child;
    rhs.child = true;
    node = rhs.node;
    return *this;
}
JSON &JSON::operator=(JSON &&rhs)
{
    child = rhs.child;
    rhs.child = true;
    node = rhs.node;
    return *this;
}

JSON::JSONTYPE JSON::get_type() const
{
    return (JSONTYPE)node->get_type();
}
int64_t &JSON::get_int() const
{
    return node->get_int();
}
std::string &JSON::get_str() const
{
    return node->get_str();
}
std::vector<unsigned char> &JSON::get_raw() const
{
    return node->get_raw();
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

void JSON::add_pair(const std::string &str, JSON json)
{
    json.child = true;
    if (get_type() != JSON::GROUP)
        throw std::runtime_error("JSON::add_pair type not matched expected a map");

    static_cast<Parser::Group *>(node)->member_table.insert({str, json.node});
}
void JSON::push(JSON json)
{
    json.child = true;
    if (get_type() != JSON::ARRAY)
        throw std::runtime_error("JSON::push type not matched expected an array");

    static_cast<Parser::Array *>(node)->elements.push_back(json.node);
}

JSON JSON::clone() const
{
    return JSON(to_string());
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

std::string JSON::stringify_unit(std::string indent, size_t indent_cnt, bool hide_raw) const
{
    if (get_type() == JSON::INT)
        return std::to_string(get_int());
    else if (get_type() == JSON::STRING)
        return "\"" + conv_str(get_str()) + "\"";
    else if (get_type() == JSON::RAW)
    {
        auto cur = static_cast<Parser::Bytes *>(node);
        if (hide_raw)
            return "(raw-data:" + std::to_string(cur->raw_length()) + " Bytes)";
        return "(" + std::to_string(cur->raw_length()) + ")$" + std::string(cur->get_raw().begin(), cur->get_raw().end()) + "$";
    }
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
            ret += indent_prefix + indent + JSON(vec[i]).stringify_unit(indent, indent_cnt + 1, hide_raw);
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
            ret += indent_prefix + indent + "\"" + pair.first + "\": " + JSON(pair.second).stringify_unit(indent, indent_cnt + 1, hide_raw);
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
    return stringify_unit(indent, 0, true);
}

std::string JSON::view(std::string indent) const
{
    return stringify_unit(indent, 0, true);
}

JSON::~JSON()
{
    if (!child)
        delete node;
}

JSON JSON::raw(const std::vector<unsigned char> &vec)
{
    JSON ret;
    //
    ret.child = false;
    ret.node = new Parser::Bytes(vec);

    return ret;
}
JSON JSON::raw(std::vector<unsigned char> &&vec)
{
    JSON ret;
    //
    ret.child = false;
    ret.node = new Parser::Bytes(std::move(vec));

    return ret;
}
// build json
JSON JSON::val(int val)
{
    return JSON(std::to_string(val));
}
JSON JSON::val(const std::string &str)
{
    std::string nstr;
    for (auto ch : str)
    {
        if (Lexer::escape_tab().count(ch))
            nstr += Lexer::escape_tab()[ch];
        else
            nstr += ch;
    }
    return JSON("\"" + nstr + "\"");
}

JSON JSON::array(const std::vector<JSON> &vec)
{
    // vector<JSON> -> vector<Node*>
    std::vector<Parser::Node *> tmp;
    for (auto item : vec)
    {
        item.child = true;
        tmp.push_back(item.node);
    }
    Parser::Array *node = new Parser::Array(tmp);
    return JSON(false, node);
}

JSON JSON::map(const std::map<std::string, JSON> &table)
{
    // vector<JSON> -> vector<Node*>
    std::map<std::string, Parser::Node *> tmp;
    for (auto item : table)
    {
        item.second.child = true;
        tmp.insert({item.first, item.second.node});
    }
    Parser::Group *node = new Parser::Group(tmp);
    return JSON(false, node);
}

JSON JSON::read_from_file(const std::string &filename)
{
    std::ifstream ifs(filename, std::ios::in | std::ios::binary);
    if (!ifs)
        throw std::runtime_error("open file " + filename + " failed\n");
    char *file_content;
    ifs.seekg(0, std::ios::end);
    size_t file_length = ifs.tellg();
    ifs.seekg(0, std::ios::beg);

    file_content = new char[file_length];

    ifs.read(file_content, file_length);
    ifs.close();

    std::string str(file_content, file_content + file_length);
    delete[] file_content;
    return JSON(str);
}
//             end ===== JSON defination ======