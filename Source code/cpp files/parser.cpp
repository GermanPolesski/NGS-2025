#include "parser.h"
#include <stack>
#include <queue>
#include <sstream>

namespace parser {

Parser::Parser(const std::vector<lexan::Token>& token_list)
    : tokens(token_list), current_pos(0), root(nullptr) {
    fst::initChains();
}

Parser::~Parser() {
    delete root;
    fst::cleanup();
}

const lexan::Token& Parser::current_token() const {
    static lexan::Token eof_token(lexan::TK_EOF, "", 0, 0, 0);
    if (current_pos >= tokens.size()) return eof_token;
    return tokens[current_pos];
}

const lexan::Token& Parser::peek_token(int offset) const {
    static lexan::Token eof_token(lexan::TK_EOF, "", 0, 0, 0);
    size_t peek_pos = current_pos + offset;
    if (peek_pos >= tokens.size()) return eof_token;
    return tokens[peek_pos];
}

void Parser::advance() {
    if (current_pos < tokens.size()) {
        current_pos++;
    }
}

bool Parser::match(lexan::TokenType type) {
    if (current_token().type == type) {
        advance();
        return true;
    }
    return false;
}

bool Parser::expect(lexan::TokenType type, const std::string& err_msg) {
    if (match(type)) {
        return true;
    }
    std::cout << "\nВ строке " << current_token().line << ", столбец " << current_token().column
              << ": Ожидалось " << err_msg << ", получено '" << current_token().value << "'\n\n";
    return false;
}

bool Parser::is_at_end() const {
    return current_token().type == lexan::TK_EOF;
}

ASTNode* Parser::parse_program() {
    ASTNode* program_node = new ASTNode(ASTNode::Type::PROGRAM, "program");
    root = program_node;

    while (!is_at_end()) {
        if (current_token().type == lexan::TK_PROCEDURE) {
            ASTNode* proc = parse_procedure_decl();
            if (!proc) return nullptr;
            program_node->addChild(proc);
        }
        else if (current_token().type == lexan::TK_BOOL || 
                 current_token().type == lexan::TK_INT ||
                 current_token().type == lexan::TK_STRING ||
                 current_token().type == lexan::TK_TIME_T ||
                 current_token().type == lexan::TK_UNSIGNED ||
                 current_token().type == lexan::TK_SYMB) {
            ASTNode* func = parse_function_decl();
            if (!func) return nullptr;
            program_node->addChild(func);
        }
        else if (current_token().type == lexan::TK_CES) {
            ASTNode* ces = parse_ces_block();
            if (!ces) return nullptr;
            program_node->addChild(ces);
        }
        else {
            std::cout << "\nВ строке " << current_token().line << ", столбец " << current_token().column
                      << ": Некорректная структура программы, неожиданный токен: '" 
                      << current_token().value << "'\n\n";
            return nullptr;
        }
    }

    return program_node;
}

ASTNode* Parser::parse_procedure_decl() {
    size_t start_pos = current_pos;
    
    if (!expect(lexan::TK_PROCEDURE, "ключевое слово 'procedure'")) return nullptr;
    if (!expect(lexan::TK_ALGO, "ключевое слово 'algo'")) return nullptr;
    
    if (current_token().type != lexan::TK_IDENTIFIER) {
        std::cout << "\nВ строке " << current_token().line << ", столбец " << current_token().column
                  << ": Ожидался идентификатор после 'algo'\n\n";
        return nullptr;
    }
    
    ASTNode* proc_node = new ASTNode(ASTNode::Type::PROCEDURE_DECL, 
                                     current_token().value, current_token());
    advance();
    
    if (!expect(lexan::TK_LPAREN, "'(' после имени процедуры")) {
        delete proc_node;
        return nullptr;
    }
    
    ASTNode* params_node = new ASTNode(ASTNode::Type::PARAM_LIST, "params");
    
    if (current_token().type != lexan::TK_RPAREN) {
        do {
            if (current_token().type != lexan::TK_UNSIGNED && 
                current_token().type != lexan::TK_INT &&
                current_token().type != lexan::TK_BOOL &&
                current_token().type != lexan::TK_STRING &&
                current_token().type != lexan::TK_TIME_T &&
                current_token().type != lexan::TK_SYMB) {
                std::cout << "\nВ строке " << current_token().line << ", столбец " << current_token().column
                          << ": Ожидался спецификатор типа в параметре\n\n";
                delete params_node;
                delete proc_node;
                return nullptr;
            }
            
            ASTNode* param_type = new ASTNode(ASTNode::Type::TYPE_SPECIFIER,
                                             lexan::Lexer::token_type_to_string(current_token().type),
                                             current_token());
            advance();
            
            if (param_type->value == "UNSIGNED") {
                if (!expect(lexan::TK_INT, "'int' после 'unsigned'")) {
                    delete param_type;
                    delete params_node;
                    delete proc_node;
                    return nullptr;
                }
                param_type->value = "UNSIGNED INT";
            }
            
            if (current_token().type != lexan::TK_IDENTIFIER) {
                std::cout << "\nВ строке " << current_token().line << ", столбец " << current_token().column
                          << ": Ожидалось имя параметра\n\n";
                delete param_type;
                delete params_node;
                delete proc_node;
                return nullptr;
            }
            
            ASTNode* param_node = new ASTNode(ASTNode::Type::IDENTIFIER,
                                             current_token().value, current_token());
            param_node->addChild(param_type);
            params_node->addChild(param_node);
            advance();
            
            if (current_token().type == lexan::TK_RPAREN) break;
            if (!expect(lexan::TK_COMMA, "',' между параметрами")) {
                delete params_node;
                delete proc_node;
                return nullptr;
            }
        } while (!is_at_end());
    }
    
    proc_node->addChild(params_node);
    
    if (!expect(lexan::TK_RPAREN, "')' после списка параметров")) {
        delete proc_node;
        return nullptr;
    }
    
    if (!expect(lexan::TK_LBRACE, "'{' для начала тела процедуры")) {
        delete proc_node;
        return nullptr;
    }
    
    ASTNode* body_node = new ASTNode(ASTNode::Type::BLOCK, "procedure_body");
    
    int stmt_count = 0;
    while (current_token().type != lexan::TK_RBRACE && !is_at_end()) {
        ASTNode* stmt = parse_statement();
        if (!stmt) {
            std::cout << "\nВ строке " << current_token().line << ", столбец " << current_token().column
                      << ": Некорректный оператор в теле процедуры\n\n";
            delete body_node;
            delete proc_node;
            return nullptr;
        }
        body_node->addChild(stmt);
        stmt_count++;
    }
    
    proc_node->addChild(body_node);
    
    if (!expect(lexan::TK_RBRACE, "'}' для окончания тела процедуры")) {
        delete proc_node;
        return nullptr;
    }
    
    std::vector<lexan::Token> context(tokens.begin() + start_pos, 
                                     tokens.begin() + current_pos);
    check_with_fst(ASTNode::Type::PROCEDURE_DECL, context);
    
    return proc_node;
}

ASTNode* Parser::parse_function_decl() {
    size_t start_pos = current_pos;
    
    ASTNode* return_type = new ASTNode(ASTNode::Type::TYPE_SPECIFIER, 
                                       lexan::Lexer::token_type_to_string(current_token().type),
                                       current_token());
    advance();
    
    if (!expect(lexan::TK_ALGO, "ключевое слово 'algo'")) {
        delete return_type;
        return nullptr;
    }
    
    if (current_token().type != lexan::TK_IDENTIFIER) {
        delete return_type;
        std::cout << "\nВ строке " << current_token().line << ", столбец " << current_token().column
                  << ": Ожидался идентификатор после 'algo'\n\n";
        return nullptr;
    }
    
    ASTNode* func_node = new ASTNode(ASTNode::Type::FUNCTION_DECL, 
                                     current_token().value, current_token());
    func_node->addChild(return_type);
    advance();
    
    if (!expect(lexan::TK_LPAREN, "'(' после имени функции")) {
        delete func_node;
        return nullptr;
    }
    
    ASTNode* params_node = new ASTNode(ASTNode::Type::PARAM_LIST, "params");
    
    if (current_token().type != lexan::TK_RPAREN) {
        do {
            if (current_token().type != lexan::TK_UNSIGNED && 
                current_token().type != lexan::TK_INT &&
                current_token().type != lexan::TK_BOOL &&
                current_token().type != lexan::TK_STRING &&
                current_token().type != lexan::TK_TIME_T &&
                current_token().type != lexan::TK_SYMB) {
                delete params_node;
                delete func_node;
                std::cout << "\nВ строке " << current_token().line << ", столбец " << current_token().column
                          << ": Ожидался спецификатор типа в параметре\n\n";
                return nullptr;
            }
            
            ASTNode* param_type = new ASTNode(ASTNode::Type::TYPE_SPECIFIER,
                                             lexan::Lexer::token_type_to_string(current_token().type),
                                             current_token());
            advance();
            
            if (param_type->value == "UNSIGNED") {
                if (!expect(lexan::TK_INT, "'int' после 'unsigned'")) {
                    delete param_type;
                    delete params_node;
                    delete func_node;
                    return nullptr;
                }
                param_type->value = "UNSIGNED INT";
            }
            
            if (current_token().type != lexan::TK_IDENTIFIER) {
                delete param_type;
                delete params_node;
                delete func_node;
                std::cout << "\nВ строке " << current_token().line << ", столбец " << current_token().column
                          << ": Ожидалось имя параметра\n\n";
                return nullptr;
            }
            
            ASTNode* param_node = new ASTNode(ASTNode::Type::IDENTIFIER,
                                             current_token().value, current_token());
            param_node->addChild(param_type);
            params_node->addChild(param_node);
            advance();
            
            if (current_token().type == lexan::TK_RPAREN) break;
            if (!expect(lexan::TK_COMMA, "',' между параметрами")) {
                delete params_node;
                delete func_node;
                return nullptr;
            }
        } while (!is_at_end());
    }
    
    func_node->addChild(params_node);
    
    if (!expect(lexan::TK_RPAREN, "')' после списка параметров")) {
        delete func_node;
        return nullptr;
    }
    
    if (!expect(lexan::TK_LBRACE, "'{' для начала тела функции")) {
        delete func_node;
        return nullptr;
    }
    
    ASTNode* body_node = new ASTNode(ASTNode::Type::BLOCK, "function_body");
    
    while (current_token().type != lexan::TK_RBRACE && !is_at_end()) {
        ASTNode* stmt = parse_statement();
        if (!stmt) {
            std::cout << "\nВ строке " << current_token().line << ", столбец " << current_token().column
                      << ": Некорректный оператор в теле функции\n\n";
            delete body_node;
            delete func_node;
            return nullptr;
        }
        body_node->addChild(stmt);
    }
    
    func_node->addChild(body_node);
    
    if (!expect(lexan::TK_RBRACE, "'}' для окончания тела функции")) {
        delete func_node;
        return nullptr;
    }
    
    std::vector<lexan::Token> context(tokens.begin() + start_pos, 
                                     tokens.begin() + current_pos);
    check_with_fst(ASTNode::Type::FUNCTION_DECL, context);
    
    return func_node;
}

ASTNode* Parser::parse_ces_block() {
    size_t start_pos = current_pos;
    
    if (!expect(lexan::TK_CES, "ключевое слово 'ces'")) {
        return nullptr;
    }
    if (!expect(lexan::TK_LBRACE, "'{' после 'ces'")) return nullptr;
    
    ASTNode* ces_node = new ASTNode(ASTNode::Type::BLOCK, "ces_block");
    
    while (current_token().type != lexan::TK_RBRACE && !is_at_end()) {
        ASTNode* stmt = parse_statement();
        if (!stmt) {
            delete ces_node;
            std::cout << "\nВ строке " << current_token().line << ", столбец " << current_token().column
                      << ": Некорректный оператор в блоке ces\n\n";
            return nullptr;
        }
        ces_node->addChild(stmt);
    }
    
    if (!expect(lexan::TK_RBRACE, "'}' для окончания блока ces")) {
        delete ces_node;
        return nullptr;
    }
    
    std::vector<lexan::Token> context(tokens.begin() + start_pos, 
                                     tokens.begin() + current_pos);
    check_with_fst(ASTNode::Type::BLOCK, context);
    
    return ces_node;
}

ASTNode* Parser::parse_statement() {
    switch (current_token().type) {
        case lexan::TK_EST:
            return parse_var_decl();
        case lexan::TK_DO:
            return parse_do_while();
        case lexan::TK_RETURN:
            return parse_return();
        case lexan::TK_LBRACE: {
            ASTNode* block = new ASTNode(ASTNode::Type::BLOCK, "block");
            advance();
            
            while (current_token().type != lexan::TK_RBRACE && !is_at_end()) {
                ASTNode* stmt = parse_statement();
                if (!stmt) {
                    delete block;
                    std::cout << "\nВ строке " << current_token().line << ", столбец " << current_token().column
                              << ": Некорректный оператор в блоке\n\n";
                    return nullptr;
                }
                block->addChild(stmt);
            }
            
            if (!expect(lexan::TK_RBRACE, "'}' для окончания блока")) {
                delete block;
                return nullptr;
            }
            return block;
        }
        case lexan::TK_SEMICOLON:
            advance();
            return new ASTNode(ASTNode::Type::NOOP, ";");
        default:
            if (current_token().type == lexan::TK_IDENTIFIER || 
                (current_token().type >= lexan::TK_BUILTIN_PROCLAIM && 
                 current_token().type <= lexan::TK_BUILTIN_SUM4)) {
                
                if (peek_token().type == lexan::TK_LPAREN) {
                    ASTNode* call = parse_function_call();
                    if (!call) {
                        return nullptr;
                    }
                    
                    if (!expect(lexan::TK_SEMICOLON, "';' после вызова функции")) {
                        delete call;
                        return nullptr;
                    }
                    return call;
                } else if (peek_token().type == lexan::TK_ASSIGN ||
                          peek_token().type == lexan::TK_PLUS_ASSIGN ||
                          peek_token().type == lexan::TK_MINUS_ASSIGN ||
                          peek_token().type == lexan::TK_MULT_ASSIGN ||
                          peek_token().type == lexan::TK_DIV_ASSIGN) {
                    return parse_assignment();
                } else {
                    std::cout << "\nВ строке " << current_token().line << ", столбец " << current_token().column
                              << ": Некорректный оператор, ожидалось выражение или присваивание\n\n";
                    return nullptr;
                }
            }
            else {
                std::cout << "\nВ строке " << current_token().line << ", столбец " << current_token().column
                          << ": Некорректный оператор, неожиданный токен: '" << current_token().value << "'\n\n";
                return nullptr;
            }
    }
}

ASTNode* Parser::parse_var_decl() {
    size_t start_pos = current_pos;
    
    if (!expect(lexan::TK_EST, "ключевое слово 'est'")) return nullptr;
    
    std::string type_str;
    lexan::Token type_token = current_token();
    
    if (current_token().type == lexan::TK_UNSIGNED) {
        type_str = "UNSIGNED";
        advance();
        if (!expect(lexan::TK_INT, "'int' после 'unsigned'")) return nullptr;
        type_str += " INT";
    }
    else if (current_token().type == lexan::TK_STRING ||
             current_token().type == lexan::TK_INT ||
             current_token().type == lexan::TK_BOOL ||
             current_token().type == lexan::TK_TIME_T ||
             current_token().type == lexan::TK_SYMB) {
        type_str = lexan::Lexer::token_type_to_string(current_token().type);
        advance();
    }
    else {
        type_str = "INT";
    }
    
    ASTNode* type_node = new ASTNode(ASTNode::Type::TYPE_SPECIFIER, type_str, type_token);
    
    if (current_token().type != lexan::TK_IDENTIFIER) {
        delete type_node;
        std::cout << "\nВ строке " << current_token().line << ", столбец " << current_token().column
                  << ": Ожидался идентификатор после типа\n\n";
        return nullptr;
    }
    
    ASTNode* var_node = new ASTNode(ASTNode::Type::VARIABLE_DECL, 
                                    current_token().value, current_token());
    var_node->addChild(type_node);
    advance();
    
    if (current_token().type == lexan::TK_ASSIGN) {
        advance();
        ASTNode* init_expr = parse_expression();
        if (!init_expr) {
            delete var_node;
            std::cout << "\nВ строке " << current_token().line << ", столбец " << current_token().column
                      << ": Некорректное выражение инициализации\n\n";
            return nullptr;
        }
        var_node->addChild(init_expr);
    }
    
    if (!expect(lexan::TK_SEMICOLON, "';' после объявления переменной")) {
        delete var_node;
        return nullptr;
    }
    
    std::vector<lexan::Token> context(tokens.begin() + start_pos, 
                                     tokens.begin() + current_pos);
    check_with_fst(ASTNode::Type::VARIABLE_DECL, context);
    
    return var_node;
}

ASTNode* Parser::parse_assignment() {
    size_t start_pos = current_pos;
    
    if (current_token().type != lexan::TK_IDENTIFIER) {
        std::cout << "\nВ строке " << current_token().line << ", столбец " << current_token().column
                  << ": Ожидался идентификатор в левой части присваивания\n\n";
        return nullptr;
    }
    
    ASTNode* target = new ASTNode(ASTNode::Type::IDENTIFIER, 
                                  current_token().value, current_token());
    advance();
    
    lexan::Token op_token = current_token();
    if (!(op_token.type == lexan::TK_ASSIGN ||
          op_token.type == lexan::TK_PLUS_ASSIGN ||
          op_token.type == lexan::TK_MINUS_ASSIGN ||
          op_token.type == lexan::TK_MULT_ASSIGN ||
          op_token.type == lexan::TK_DIV_ASSIGN)) {
        delete target;
        std::cout << "\nВ строке " << current_token().line << ", столбец " << current_token().column
                  << ": Ожидался оператор присваивания\n\n";
        return nullptr;
    }
    advance();
    
    ASTNode* expr = parse_expression();
    if (!expr) {
        delete target;
        std::cout << "\nВ строке " << current_token().line << ", столбец " << current_token().column
                  << ": Некорректное выражение в правой части присваивания\n\n";
        return nullptr;
    }
    
    ASTNode* assign_node = new ASTNode(ASTNode::Type::ASSIGNMENT, 
                                       lexan::Lexer::token_type_to_string(op_token.type),
                                       op_token);
    assign_node->addChild(target);
    assign_node->addChild(expr);
    
    if (!expect(lexan::TK_SEMICOLON, "';' после присваивания")) {
        delete assign_node;
        return nullptr;
    }
    
    std::vector<lexan::Token> context(tokens.begin() + start_pos, 
                                     tokens.begin() + current_pos);
    check_with_fst(ASTNode::Type::ASSIGNMENT, context);
    
    return assign_node;
}

ASTNode* Parser::parse_function_call() {
    size_t start_pos = current_pos;
    
    lexan::Token func_token = current_token();
    std::string func_name = func_token.value;
    
    bool is_builtin = func_token.type >= lexan::TK_BUILTIN_PROCLAIM && 
                      func_token.type <= lexan::TK_BUILTIN_SUM4;
    bool is_ident = func_token.type == lexan::TK_IDENTIFIER;
    
    if (!is_builtin && !is_ident) {
        std::cout << "\nВ строке " << current_token().line << ", столбец " << current_token().column
                  << ": Некорректное начало вызова функции: " << func_name << "\n\n";
        return nullptr;
    }
    
    advance();
    
    if (!expect(lexan::TK_LPAREN, "'(' после имени функции")) {
        return nullptr;
    }
    
    ASTNode* call_node = new ASTNode(ASTNode::Type::FUNCTION_CALL, func_name, func_token);
    ASTNode* args_node = new ASTNode(ASTNode::Type::ARG_LIST, "args");
    
    if (current_token().type != lexan::TK_RPAREN) {
        do {
            ASTNode* arg = parse_expression();
            if (!arg) {
                std::cout << "\nВ строке " << current_token().line << ", столбец " << current_token().column
                          << ": Некорректный аргумент в вызове функции\n\n";
                delete args_node;
                delete call_node;
                return nullptr;
            }
            args_node->addChild(arg);
            
            if (current_token().type == lexan::TK_RPAREN) break;
            if (!expect(lexan::TK_COMMA, "',' между аргументами")) {
                delete args_node;
                delete call_node;
                return nullptr;
            }
        } while (!is_at_end());
    }
    
    call_node->addChild(args_node);
    
    if (!expect(lexan::TK_RPAREN, "')' после аргументов")) {
        delete call_node;
        return nullptr;
    }
    
    return call_node;
}

ASTNode* Parser::parse_do_while() {
    size_t start_pos = current_pos;
    
    if (!expect(lexan::TK_DO, "ключевое слово 'do'")) return nullptr;
    
    ASTNode* loop_node = new ASTNode(ASTNode::Type::DO_WHILE_LOOP, "do_while");
    
    if (current_token().type == lexan::TK_LBRACE) {
        advance();
        
        ASTNode* body = new ASTNode(ASTNode::Type::BLOCK, "loop_body");
        while (current_token().type != lexan::TK_RBRACE && !is_at_end()) {
            ASTNode* stmt = parse_statement();
            if (!stmt) {
                std::cout << "\nВ строке " << current_token().line << ", столбец " << current_token().column
                          << ": Некорректный оператор в теле цикла\n\n";
                delete body;
                delete loop_node;
                return nullptr;
            }
            body->addChild(stmt);
        }
        
        loop_node->addChild(body);
        
        if (!expect(lexan::TK_RBRACE, "'}' после тела цикла")) {
            delete loop_node;
            return nullptr;
        }
    }
    else {
        ASTNode* stmt = parse_statement();
        if (!stmt) {
            delete loop_node;
            std::cout << "\nВ строке " << current_token().line << ", столбец " << current_token().column
                      << ": Некорректный оператор в теле цикла\n\n";
            return nullptr;
        }
        loop_node->addChild(stmt);
    }
    
    if (!expect(lexan::TK_WHILE, "ключевое слово 'while' после тела цикла")) {
        delete loop_node;
        return nullptr;
    }
    
    if (!expect(lexan::TK_LPAREN, "'(' после 'while'")) {
        delete loop_node;
        return nullptr;
    }
    
    ASTNode* condition = parse_expression();
    if (!condition) {
        delete loop_node;
        std::cout << "\nВ строке " << current_token().line << ", столбец " << current_token().column
                  << ": Некорректное условие в цикле do-while\n\n";
        return nullptr;
    }
    loop_node->addChild(condition);
    
    if (!expect(lexan::TK_RPAREN, "')' после условия")) {
        delete loop_node;
        return nullptr;
    }
    
    if (!expect(lexan::TK_SEMICOLON, "';' после оператора do-while")) {
        delete loop_node;
        return nullptr;
    }
    
    std::vector<lexan::Token> context(tokens.begin() + start_pos, 
                                     tokens.begin() + current_pos);
    check_with_fst(ASTNode::Type::DO_WHILE_LOOP, context);
    
    return loop_node;
}

ASTNode* Parser::parse_return() {
    if (!expect(lexan::TK_RETURN, "ключевое слово 'return'")) return nullptr;
    
    ASTNode* return_node = new ASTNode(ASTNode::Type::RETURN_STMT, "return");
    
    if (current_token().type != lexan::TK_SEMICOLON) {
        ASTNode* expr = parse_expression();
        if (!expr) {
            delete return_node;
            std::cout << "\nВ строке " << current_token().line << ", столбец " << current_token().column
                      << ": Некорректное выражение в return\n\n";
            return nullptr;
        }
        return_node->addChild(expr);
    }
    
    if (!expect(lexan::TK_SEMICOLON, "';' после return")) {
        delete return_node;
        return nullptr;
    }
    
    return return_node;
}

bool Parser::is_unary_operator(lexan::TokenType type) const {
    switch (type) {
        case lexan::TK_PLUS:
        case lexan::TK_MINUS:
        case lexan::TK_NOT:
        case lexan::TK_BIT_NOT:
            return true;
        default:
            return false;
    }
}

bool Parser::is_binary_operator(lexan::TokenType op) const {
    switch (op) {
        case lexan::TK_PLUS: case lexan::TK_MINUS:
        case lexan::TK_MULT: case lexan::TK_DIV:
        case lexan::TK_MOD: case lexan::TK_POW:
        case lexan::TK_EQ: case lexan::TK_NE:
        case lexan::TK_LT: case lexan::TK_GT:
        case lexan::TK_LE: case lexan::TK_GE:
        case lexan::TK_AND: case lexan::TK_OR:
            return true;
        default:
            return false;
    }
}

int Parser::get_operator_precedence(lexan::TokenType op) const {
    switch (op) {
        case lexan::TK_OR: return 1;
        case lexan::TK_AND: return 2;
        case lexan::TK_EQ: case lexan::TK_NE: return 3;
        case lexan::TK_LT: case lexan::TK_GT: 
        case lexan::TK_LE: case lexan::TK_GE: return 4;
        case lexan::TK_PLUS: case lexan::TK_MINUS: return 5;
        case lexan::TK_MULT: case lexan::TK_DIV: 
        case lexan::TK_MOD: return 6;
        case lexan::TK_POW: return 7;
        default: return 0;
    }
}

ASTNode* Parser::parse_expression() {
    return parse_equality();
}

ASTNode* Parser::parse_equality() {
    ASTNode* node = parse_comparison();
    
    while (current_token().type == lexan::TK_EQ || 
           current_token().type == lexan::TK_NE) {
        lexan::Token op = current_token();
        advance();
        ASTNode* right = parse_comparison();
        
        ASTNode* bin_node = new ASTNode(ASTNode::Type::BINARY_OP, 
                                       lexan::Lexer::token_type_to_string(op.type),
                                       op);
        bin_node->addChild(node);
        bin_node->addChild(right);
        node = bin_node;
    }
    
    return node;
}

ASTNode* Parser::parse_comparison() {
    ASTNode* node = parse_term();
    
    while (current_token().type == lexan::TK_LT ||
           current_token().type == lexan::TK_GT ||
           current_token().type == lexan::TK_LE ||
           current_token().type == lexan::TK_GE) {
        lexan::Token op = current_token();
        advance();
        ASTNode* right = parse_term();
        
        ASTNode* bin_node = new ASTNode(ASTNode::Type::BINARY_OP,
                                       lexan::Lexer::token_type_to_string(op.type),
                                       op);
        bin_node->addChild(node);
        bin_node->addChild(right);
        node = bin_node;
    }
    
    return node;
}

ASTNode* Parser::parse_term() {
    ASTNode* node = parse_factor();
    
    while (current_token().type == lexan::TK_PLUS ||
           current_token().type == lexan::TK_MINUS) {
        lexan::Token op = current_token();
        advance();
        ASTNode* right = parse_factor();
        
        ASTNode* bin_node = new ASTNode(ASTNode::Type::BINARY_OP,
                                       lexan::Lexer::token_type_to_string(op.type),
                                       op);
        bin_node->addChild(node);
        bin_node->addChild(right);
        node = bin_node;
    }
    
    return node;
}

ASTNode* Parser::parse_factor() {
    ASTNode* node = parse_unary();
    
    while (current_token().type == lexan::TK_MULT ||
           current_token().type == lexan::TK_DIV ||
           current_token().type == lexan::TK_MOD ||
           current_token().type == lexan::TK_POW) {
        lexan::Token op = current_token();
        advance();
        ASTNode* right = parse_unary();
        
        ASTNode* bin_node = new ASTNode(ASTNode::Type::BINARY_OP,
                                       lexan::Lexer::token_type_to_string(op.type),
                                       op);
        bin_node->addChild(node);
        bin_node->addChild(right);
        node = bin_node;
    }
    
    return node;
}

ASTNode* Parser::parse_unary() {
    if (is_unary_operator(current_token().type)) {
        lexan::Token op = current_token();
        advance();
        ASTNode* operand = parse_unary();
        
        ASTNode* unary_node = new ASTNode(ASTNode::Type::UNARY_OP,
                                         lexan::Lexer::token_type_to_string(op.type),
                                         op);
        unary_node->addChild(operand);
        return unary_node;
    }
    
    return parse_primary();
}

ASTNode* Parser::parse_primary() {
    lexan::Token token = current_token();
    
    switch (token.type) {
        case lexan::TK_NUMBER: 
        case lexan::TK_STRING_LIT: 
        case lexan::TK_CHAR_LIT: 
        case lexan::TK_TRUE:
        case lexan::TK_FALSE: {
            ASTNode* node = new ASTNode(ASTNode::Type::LITERAL, token.value, token);
            advance();
            return node;
        }
        case lexan::TK_IDENTIFIER: {
            if (peek_token().type == lexan::TK_LPAREN) {
                size_t saved_pos = current_pos;
                
                ASTNode* call = parse_function_call();
                if (call) {
                    return call;
                } else {
                    current_pos = saved_pos;
                    ASTNode* node = new ASTNode(ASTNode::Type::IDENTIFIER, token.value, token);
                    advance();
                    return node;
                }
            } else {
                ASTNode* node = new ASTNode(ASTNode::Type::IDENTIFIER, token.value, token);
                advance();
                return node;
            }
        }
        case lexan::TK_LPAREN: {
            advance();
            ASTNode* expr = parse_expression();
            
            if (!expect(lexan::TK_RPAREN, "')' после выражения")) {
                delete expr;
                return nullptr;
            }
            return expr;
        }
        default:
            if (token.type >= lexan::TK_BUILTIN_PROCLAIM && 
                token.type <= lexan::TK_BUILTIN_SUM4) {
                if (peek_token().type == lexan::TK_LPAREN) {
                    size_t saved_pos = current_pos;
                    ASTNode* call = parse_function_call();
                    if (call) {
                        return call;
                    } else {
                        current_pos = saved_pos;
                        ASTNode* node = new ASTNode(ASTNode::Type::IDENTIFIER, token.value, token);
                        advance();
                        return node;
                    }
                } else {
                    ASTNode* node = new ASTNode(ASTNode::Type::IDENTIFIER, token.value, token);
                    advance();
                    return node;
                }
            }
            
            std::cout << "\nВ строке " << token.line << ", столбец " << token.column
                      << ": Некорректное выражение, неожиданный токен: '" << token.value << "'\n\n";
            return nullptr;
    }
}

bool Parser::check_with_fst(ASTNode::Type node_type, const std::vector<lexan::Token>& context_tokens) {
    const auto& all_rules = fst::getAllRules();
    
    for (const auto& rule : all_rules) {
        size_t matchedLength = 0;
        if (fst::matchRule(rule, context_tokens, 0, matchedLength)) {
            return true;
        }
    }
    
    return false;
}

bool Parser::validate_structure_with_fst(ASTNode* node) {
    if (!node) return true;
    
    for (auto child : node->children) {
        validate_structure_with_fst(child);
    }
    
    return true;
}

bool Parser::parse() {
    root = parse_program();
    if (!root) {
        std::cout << "\nОшибка синтаксического анализа: некорректная структура программы\n\n";
        return false;
    }
    
    return true;
}

void Parser::print_ast(ASTNode* node, int depth, std::ostream& out) const {
    if (!node) node = root;
    if (!node) return;
    
    std::string indent(depth * 2, ' ');
    
    out << indent << "[";
    
    switch (node->type) {
        case ASTNode::Type::PROGRAM: out << "PROGRAM"; break;
        case ASTNode::Type::PROCEDURE_DECL: out << "PROCEDURE " << node->value; break;
        case ASTNode::Type::FUNCTION_DECL: out << "FUNCTION " << node->value; break;
        case ASTNode::Type::VARIABLE_DECL: out << "VAR " << node->value; break;
        case ASTNode::Type::ASSIGNMENT: out << "ASSIGN " << node->value; break;
        case ASTNode::Type::FUNCTION_CALL: out << "CALL " << node->value; break;
        case ASTNode::Type::DO_WHILE_LOOP: out << "DO_WHILE"; break;
        case ASTNode::Type::BINARY_OP: out << "BIN_OP " << node->value; break;
        case ASTNode::Type::UNARY_OP: out << "UNARY_OP " << node->value; break;
        case ASTNode::Type::LITERAL: out << "LITERAL " << node->value; break;
        case ASTNode::Type::IDENTIFIER: out << "ID " << node->value; break;
        case ASTNode::Type::BLOCK: out << "BLOCK"; break;
        case ASTNode::Type::PARAM_LIST: out << "PARAMS"; break;
        case ASTNode::Type::ARG_LIST: out << "ARGS"; break;
        case ASTNode::Type::RETURN_STMT: out << "RETURN"; break;
        case ASTNode::Type::TYPE_SPECIFIER: out << "TYPE " << node->value; break;
        case ASTNode::Type::NOOP: out << "NOOP"; break;
    }
    
    if (!node->token.value.empty() && node->type != ASTNode::Type::IDENTIFIER) {
        out << " (" << node->token.value << ")";
    }
    
    out << "]" << std::endl;
    
    for (auto child : node->children) {
        print_ast(child, depth + 1, out);
    }
}

void Parser::generate_dot_file(const std::string& filename) const {
    std::ofstream dot_file(filename);
    if (!dot_file.is_open()) {
        std::cout << "\nОшибка: не удалось создать файл DOT: " << filename << "\n\n";
        return;
    }
    
    dot_file << "digraph AST {" << std::endl;
    dot_file << "    node [shape=box, style=filled, fillcolor=lightblue];" << std::endl;
    dot_file << "    edge [arrowhead=vee];" << std::endl;
    
    std::queue<std::pair<ASTNode*, int>> nodes;
    std::unordered_map<ASTNode*, int> node_ids;
    int next_id = 0;
    
    if (root) {
        nodes.push({root, next_id++});
        node_ids[root] = 0;
    }
    
    while (!nodes.empty()) {
        auto [node, id] = nodes.front();
        nodes.pop();
        
        std::string label;
        switch (node->type) {
            case ASTNode::Type::PROGRAM: label = "PROGRAM"; break;
            case ASTNode::Type::PROCEDURE_DECL: label = "PROCEDURE\\n" + node->value; break;
            case ASTNode::Type::FUNCTION_DECL: label = "FUNCTION\\n" + node->value; break;
            case ASTNode::Type::VARIABLE_DECL: label = "VAR\\n" + node->value; break;
            case ASTNode::Type::ASSIGNMENT: label = "ASSIGN\\n" + node->value; break;
            case ASTNode::Type::FUNCTION_CALL: label = "CALL\\n" + node->value; break;
            case ASTNode::Type::DO_WHILE_LOOP: label = "DO_WHILE"; break;
            case ASTNode::Type::BINARY_OP: label = "BIN_OP\\n" + node->value; break;
            case ASTNode::Type::UNARY_OP: label = "UNARY_OP\\n" + node->value; break;
            case ASTNode::Type::IDENTIFIER: label = "ID\\n" + node->value; break;
            case ASTNode::Type::LITERAL: label = "LITERAL\\n" + node->value; break;
            case ASTNode::Type::TYPE_SPECIFIER: label = "TYPE\\n" + node->value; break;
            default: label = "NODE";
        }
        
        dot_file << "    " << id << " [label=\"" << label << "\"];" << std::endl;
        
        for (auto child : node->children) {
            if (node_ids.find(child) == node_ids.end()) {
                node_ids[child] = next_id++;
                nodes.push({child, node_ids[child]});
            }
            dot_file << "    " << id << " -> " << node_ids[child] << ";" << std::endl;
        }
    }
    
    dot_file << "}" << std::endl;
    dot_file.close();
}

bool performSyntaxAnalysis(const std::vector<lexan::Token>& tokens, 
                          const std::string& filename,
                          parser::Parser& parser) {
    std::cout << "Синтаксический анализ...\n";
    
    if (!parser.parse()) {
        std::cout << "Синтаксический анализ не пройден!\n";
        return false;
    }
    
    std::cout << "Синтаксический анализ успешен!\n";
    
    std::cout << "Сохранение AST в файл...\n";
    std::stringstream ast_output;
    ast_output << "=== АБСТРАКТНОЕ СИНТАКСИЧЕСКОЕ ДЕРЕВО ===\n";
    ast_output << "Файл: " << filename << "\n";
    ast_output << "Дата: " << FileWork::getCurrentDateTime() << "\n";
    ast_output << "Обработано токенов: " << tokens.size() << "\n";
    ast_output << "==============================\n\n";
    
    parser.print_ast(nullptr, 0, ast_output);
    
    std::string ast_filename = filename + ".ast.txt";
    FileWork::WriteFile(ast_filename, ast_output.str());
    std::cout << "AST сохранен в: " << ast_filename << "\n";
    
    std::string dot_filename = filename + ".ast.dot";
    parser.generate_dot_file(dot_filename);
    std::cout << "DOT файл для визуализации: " << dot_filename << "\n";
    
    return true;
}

void writeTokenLog(const std::vector<lexan::Token>& tokens, 
                   const std::string& filename, 
                   const std::string& log_filename) {
    std::stringstream token_log;
    token_log << "=== ЖУРНАЛ ТОКЕНИЗАЦИИ ===\n";
    token_log << "Файл: " << filename << "\n";
    token_log << "Дата: " << FileWork::getCurrentDateTime() << "\n";
    token_log << "Всего токенов: " << tokens.size() << "\n";
    token_log << "=======================\n\n";
    
    size_t max_tokens_to_log = (tokens.size() > 50) ? 50 : tokens.size();
    for (size_t i = 0; i < max_tokens_to_log; i++) {
        token_log << std::setw(4) << i << ": " 
                  << std::setw(25) << lexan::Lexer::token_type_to_string(tokens[i].type)
                  << " \"" << tokens[i].value << "\""
                  << " на строке " << tokens[i].line << ":" << tokens[i].column
                  << "\n";
    }
    
    if (tokens.size() > max_tokens_to_log) {
        token_log << "... и еще " << (tokens.size() - max_tokens_to_log) << " токенов\n";
    }
    
    FileWork::WriteFile(log_filename, token_log.str());
    std::cout << "Журнал токенов сохранен в: " << log_filename << std::endl;
}
} // namespace parser