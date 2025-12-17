#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "parser.h"
#include "lexer.h"
#include "rpnconverter.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace semantic {

// Структура для хранения информации о типе
struct TypeInfo {
    std::string name;
    bool is_builtin;
    std::string js_type;  // Соответствующий тип в JavaScript
    
    TypeInfo(const std::string& n = "", bool builtin = true, 
             const std::string& js = "any")
        : name(n), is_builtin(builtin), js_type(js) {}
    
    std::string to_string() const {
        return name + " -> " + js_type;
    }
};

// Структура для хранения информации о символе
struct SymbolInfo {
    std::string name;
    TypeInfo type;
    lexan::Token declaration_token;
    bool is_initialized;
    bool is_used;
    int scope_level;
    std::string context;  // "global", "function", "procedure", "block"
    
    SymbolInfo(const std::string& n = "", const TypeInfo& t = TypeInfo(),
               const lexan::Token& tok = lexan::Token(), 
               bool init = false, int scope = 0, 
               const std::string& ctx = "global")
        : name(n), type(t), declaration_token(tok),
          is_initialized(init), is_used(false), 
          scope_level(scope), context(ctx) {}
};

// Структура для хранения информации о функции
struct FunctionInfo {
    std::string name;
    TypeInfo return_type;
    std::vector<SymbolInfo> parameters;
    std::vector<SymbolInfo> local_vars;
    bool is_defined;
    bool is_called;
    int declaration_line;
    
    FunctionInfo(const std::string& n = "", const TypeInfo& ret = TypeInfo(),
                 int line = 0)
        : name(n), return_type(ret), is_defined(false), 
          is_called(false), declaration_line(line) {}
};

// Класс семантического анализатора
class SemanticAnalyzer {
private:
    // Таблицы символов
    std::unordered_map<std::string, SymbolInfo> global_symbols;
    std::unordered_map<std::string, FunctionInfo> functions;
    std::vector<std::unordered_map<std::string, SymbolInfo>> scope_stack;
    
    // Ошибки и предупреждения
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
    
    // Текущая информация
    std::string current_function;
    int current_scope_level;
    bool in_function_body;
    
    // Вспомогательные методы
    void enter_scope();
    void exit_scope();
    SymbolInfo* lookup_symbol(const std::string& name);
    FunctionInfo* lookup_function(const std::string& name);
    bool declare_symbol(const std::string& name, const TypeInfo& type,
                       const lexan::Token& token, const std::string& context = "");
    bool declare_function(const std::string& name, const TypeInfo& return_type,
                         const lexan::Token& token);
    
    // Методы проверки типов
    TypeInfo get_expression_type(parser::ASTNode* node);
    bool type_compatible(const TypeInfo& t1, const TypeInfo& t2, 
                        const std::string& operation = "");
    TypeInfo get_binary_op_result_type(const TypeInfo& t1, const TypeInfo& t2,
                                      const std::string& op);
    
    // Методы обхода AST
    void analyze_node(parser::ASTNode* node);
    void analyze_program(parser::ASTNode* node);
    void analyze_function_decl(parser::ASTNode* node);
    void analyze_procedure_decl(parser::ASTNode* node);
    void analyze_variable_decl(parser::ASTNode* node);
    void analyze_assignment(parser::ASTNode* node);
    void analyze_function_call(parser::ASTNode* node);
    void analyze_do_while(parser::ASTNode* node);
    void analyze_return(parser::ASTNode* node);
    void analyze_expression(parser::ASTNode* node);
    void analyze_block(parser::ASTNode* node);
    
    // Методы для встроенных функций
    bool is_builtin_function(const std::string& name);
    TypeInfo get_builtin_return_type(const std::string& name);
    bool check_builtin_arguments(const std::string& name, 
                                const std::vector<TypeInfo>& arg_types);
    
    // Утилиты
    TypeInfo token_type_to_type(lexan::TokenType type);
    void add_error(const std::string& message, const lexan::Token& token);
    void add_warning(const std::string& message, const lexan::Token& token);
    
public:
    SemanticAnalyzer();
    
    // Основной метод анализа
    bool analyze(parser::ASTNode* ast);
    
    // Получение результатов
    const std::vector<std::string>& get_errors() const { return errors; }
    const std::vector<std::string>& get_warnings() const { return warnings; }
    
    // Вывод информации
    void print_symbol_table() const;
    void print_function_table() const;
    void print_type_summary() const;
    
    // Генерация отчета
    std::string generate_report() const;
};

} // namespace semantic

#endif // SEMANTIC_H