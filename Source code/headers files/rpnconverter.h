#ifndef RPNCONVERTER_H
#define RPNCONVERTER_H

#include "parser.h"
#include <string>
#include <vector>
#include <sstream>

namespace rpn {

class RPNConverter {
private:
    std::stringstream output;
    std::vector<std::string> tokens; // Храним токены RPN
    
    bool is_operator(const std::string& token);
    std::string convert_operator(const std::string& ast_op);
    
    void process_expression(parser::ASTNode* node);
    void process_binary_op(parser::ASTNode* node);
    void process_unary_op(parser::ASTNode* node);
    void process_function_call(parser::ASTNode* node);
    void process_identifier(parser::ASTNode* node);
    void process_literal(parser::ASTNode* node);
    
    // Вспомогательная функция для поиска выражений
    void find_and_convert_expressions(parser::ASTNode* node, 
                                     std::stringstream& result, 
                                     int depth = 0);
    
public:
    RPNConverter();
    
    std::string convert_expression(parser::ASTNode* expr_node);
    std::string convert_program(parser::ASTNode* program_node);
    
    void reset();
};

} // namespace rpn

#endif // RPNCONVERTER_H