#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <vector>
#include <map>

namespace lexan {
    typedef enum {
        TK_PROCEDURE,
        TK_ALGO,           // Уже есть!
        TK_BOOL,
        TK_UNSIGNED,
        TK_INT,
        TK_STRING,
        TK_TIME_T,
        TK_CES,
        TK_EST,
        TK_DO,
        TK_WHILE,
        TK_RETURN,
        TK_SYMB,
        TK_IF,
        TK_ELSE,
        TK_BUILTIN_PROCLAIM,
        TK_BUILTIN_TO_STR,
        TK_BUILTIN_TIME_FLED,
        TK_BUILTIN_THIS_VERY_MOMENT,
        TK_BUILTIN_UNITE,
        TK_BUILTIN_SUM4,
        TK_IDENTIFIER,
        TK_NUMBER,
        TK_STRING_LIT,
        TK_CHAR_LIT,
        TK_TRUE,
        TK_FALSE,
        TK_PLUS,
        TK_MINUS,
        TK_MULT,
        TK_DIV,
        TK_MOD,
        TK_ASSIGN,
        TK_GT,
        TK_LT,
        TK_GE,
        TK_LE,
        TK_EQ,
        TK_NE,
        TK_POW,           // Уже есть!
        TK_AND,
        TK_OR,
        TK_NOT,
        TK_BIT_AND,
        TK_BIT_OR,
        TK_BIT_XOR,
        TK_BIT_NOT,
        TK_INCREMENT,
        TK_DECREMENT,
        TK_PLUS_ASSIGN,
        TK_MINUS_ASSIGN,
        TK_MULT_ASSIGN,
        TK_DIV_ASSIGN,
        TK_LPAREN,
        TK_RPAREN,
        TK_LBRACE,
        TK_RBRACE,
        TK_LBRACKET,
        TK_RBRACKET,
        TK_SEMICOLON,
        TK_COMMA,
        TK_COLON,
        TK_DOT,
        TK_EOF,
        TK_ERROR,
        TK_COMMENT
    } TokenType;

    struct Token {
        TokenType type;
        std::string value;
        int line;
        int column;
        int index;
        union {
            long int_value;
            double float_value;
        } numeric_data;
        bool is_hex;
        bool is_octal;
        bool is_float;
        bool is_signed;
        
        Token() : type(TK_ERROR), line(0), column(0), index(0), 
                 is_hex(false), is_octal(false), is_float(false), is_signed(false) {
            numeric_data.int_value = 0;
        }
        
        Token(TokenType t, const std::string& v, int l, int c, int i) 
            : type(t), value(v), line(l), column(c), index(i),
              is_hex(false), is_octal(false), is_float(false), is_signed(false) {
            numeric_data.int_value = 0;
        }
    };

    class Lexer {
    private:
        std::string source;
        std::string filename;
        size_t position;
        int line;
        int column;
        char current_char;
        std::map<std::string, TokenType> keywords;
        std::map<std::string, TokenType> builtins;
        
        void advance();
        char peek(int offset = 1) const;
        void skip_whitespace();
        void skip_comment();
        Token read_number();
        Token read_identifier();
        Token read_string();
        Token read_char();
        Token read_operator();
        bool is_alpha(char c) const;
        bool is_alpha_numeric(char c) const;
        bool is_digit(char c) const;
        bool is_hex_digit(char c) const;
        bool is_octal_digit(char c) const;
        
    public:
        Lexer(const std::string& source_code, const std::string& fname = "");
        Token get_next_token();
        std::vector<Token> tokenize();
        void reset();
        std::pair<int, int> get_position() const { return {line, column}; }
        static std::string token_type_to_string(TokenType type);
        static bool is_keyword(const std::string& word);
        static bool is_builtin(const std::string& word);
        static bool generate_token_file(const std::string& source_code, 
                                       const std::string& output_filename);
    };
}

#endif