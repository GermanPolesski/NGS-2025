#ifndef CODEGEN_H
#define CODEGEN_H

#include "parser.h"
#include "semantic.h"
#include <string>
#include <sstream>
#include <vector>
#include <stack>
#include <unordered_map>

namespace codegen {

class CodeGenerator {
private:
    std::stringstream code;
    std::vector<std::string> indent_stack;
    std::string current_indent;
    int temp_var_counter;
    std::unordered_map<std::string, std::string> type_conversions;
    semantic::SemanticAnalyzer* semantic_analyzer;
    
    void indent();
    void dedent();
    void add_line(const std::string& line);
    std::string escape_string(const std::string& str);
    std::string generate_temp_var();
    std::string get_js_type(const std::string& type);
    
    // Встроенные функции
    bool is_builtin_function(const std::string& name) {
        return name == "proclaim" || name == "to_str" || 
               name == "TimeFled" || name == "ThisVeryMoment" || 
               name == "unite" || name == "sum4";
    }
    
    std::string convert_operator(const std::string& op);
    
    // Main generation methods
    void generate_program(parser::ASTNode* node);
    void generate_function_decl(parser::ASTNode* node);
    void generate_procedure_decl(parser::ASTNode* node);
    void generate_variable_decl(parser::ASTNode* node);
    void generate_assignment(parser::ASTNode* node);
    void generate_do_while(parser::ASTNode* node);
    void generate_return(parser::ASTNode* node);
    void generate_block(parser::ASTNode* node);
    
    // Expression generation
    std::string generate_expression(parser::ASTNode* node);
    std::string generate_binary_op(parser::ASTNode* node);
    std::string generate_unary_op(parser::ASTNode* node);
    std::string generate_literal(parser::ASTNode* node);
    std::string generate_identifier(parser::ASTNode* node);
    std::string generate_function_call(parser::ASTNode* node);
    
    // Built-in functions handling
    std::string handle_builtin_function(const std::string& name, 
                                       parser::ASTNode* args_node);
    
public:
    CodeGenerator();
    
    std::string generate(parser::ASTNode* ast, semantic::SemanticAnalyzer* analyzer = nullptr);
    void save_to_file(const std::string& filename);
    
    const std::string get_code() const { return code.str(); }
};

} // namespace codegen

#endif // CODEGEN_H