#include "json_parser.hpp"
#include <fstream>
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

    TokenStream *build_token_stream(const std::string &str);
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
        Node(NodeType nt);
        int get_int();
        std::string get_str();
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
    Node *parse_unit(Lexer::TokenStream &token_stream);
}

int main()
{
    std::string str = "(5)$";
    str.push_back(0xFF);
    str.push_back(0);
    str+="GGG$";
    JSON js(str);
    std::cout << js.to_string() << std::endl;
    return 0;
}