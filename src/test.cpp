#include "json_parser.hpp"
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
        RawData(const std::vector<char> &dat) : Token(RAW_DATA), data(dat) {}
        RawData(std::vector<char> &&dat) : Token(RAW_DATA), data(std::move(data)) {}
        static std::vector<char> get_raw_data(Token *tok)
        {
            return static_cast<RawData *>(tok)->data;
        }
        std::string to_string() const override
        {
            return "<raw:" + std::to_string(data.size()) + " bytes>";
        }

    private:
        std::vector<char> data;
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

    RawData *get_raw_data(const std::string &str, int &i);
}

namespace Test
{
    namespace Lex
    {
        bool test_raw_data()
        {

            std::string test1 = "(5)$12345$";
            int i = 0;
            Lexer::RawData *c = Lexer::get_raw_data(test1, i);
            auto vec = Lexer::RawData::get_raw_data(c);
            for (auto item : vec)
            {
                printf("%02X ", item);
            }
            bool flag = true;
            flag &= vec.size() == 5;
            test1 = "(5)$";
            test1.push_back(0xFF);
            for (int i = 1; i <= 4; i++)
                test1.push_back(0);
            test1 += '$';
            delete c;
            i = 0;
            c = Lexer::get_raw_data(test1, i);
            vec = Lexer::RawData::get_raw_data(c);
            for (auto item : vec)
            {
                printf("%02X ", (unsigned char)item);
            }

            return true;
        }
    }
}

int main()
{
    Test::Lex::test_raw_data();
}