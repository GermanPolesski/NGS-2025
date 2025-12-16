#ifndef PARSER_H
#define PARSER_H

#include "precomph.h"
#include "lexer.h"
#include "fst.h"

namespace parser {

struct ASTNode {
    enum class Type {
        PROGRAM,
        PROCEDURE_DECL,
        FUNCTION_DECL,
        VARIABLE_DECL,
        ASSIGNMENT,
        FUNCTION_CALL,
        DO_WHILE_LOOP,
        EXPRESSION,
        BINARY_OP,
        UNARY_OP,
        LITERAL,
        IDENTIFIER,
        BLOCK,
        PARAM_LIST,
        ARG_LIST,
        RETURN_STMT,
        IF_STMT,
        TYPE_SPECIFIER,
        NOOP
    };

    Type type;
    std::string value;
    lexan::Token token;
    std::vector<ASTNode*> children;
    ASTNode* parent;

    ASTNode(Type t, const std::string& v = "", const lexan::Token& tok = lexan::Token())
        : type(t), value(v), token(tok), parent(nullptr) {}
    
    ~ASTNode() {
        for (auto child : children) {
            delete child;
        }
    }

    void addChild(ASTNode* child) {
        if (child) {
            child->parent = this;
            children.push_back(child);
        }
    }
};

class Parser {
private:
    std::vector<lexan::Token> tokens;
    size_t current_pos;
    ASTNode* root;

    const lexan::Token& current_token() const;
    const lexan::Token& peek_token(int offset = 1) const;
    void advance();
    bool match(lexan::TokenType type);
    bool expect(lexan::TokenType type, const std::string& err_msg = "");
    bool is_at_end() const;

    ASTNode* parse_program();
    ASTNode* parse_procedure_decl();
    ASTNode* parse_function_decl();
    ASTNode* parse_ces_block();
    ASTNode* parse_statement();
    ASTNode* parse_var_decl();
    ASTNode* parse_assignment();
    ASTNode* parse_function_call();
    ASTNode* parse_do_while();
    ASTNode* parse_return();
    
    ASTNode* parse_expression();
    ASTNode* parse_equality();
    ASTNode* parse_comparison();
    ASTNode* parse_term();
    ASTNode* parse_factor();
    ASTNode* parse_unary();
    ASTNode* parse_primary();
    
    bool is_unary_operator(lexan::TokenType type) const;
    bool is_binary_operator(lexan::TokenType type) const;
    int get_operator_precedence(lexan::TokenType type) const;
    
    bool check_with_fst(ASTNode::Type node_type, const std::vector<lexan::Token>& context_tokens);
    bool validate_structure_with_fst(ASTNode* node);

public:
    Parser(const std::vector<lexan::Token>& token_list);
    ~Parser();

    bool parse();
    ASTNode* get_ast() const { return root; }
    
    void print_ast(ASTNode* node, int depth, std::ostream& out) const;
    void generate_dot_file(const std::string& filename) const;
};

bool performSyntaxAnalysis(const std::vector<lexan::Token>& tokens, 
                          const std::string& filename,
                          parser::Parser& parser);
void writeTokenLog(const std::vector<lexan::Token>& tokens, 
                   const std::string& filename, 
                   const std::string& log_filename);

}

#endif