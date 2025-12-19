#include "semantic.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <set>

namespace semantic {

SemanticAnalyzer::SemanticAnalyzer() 
    : current_function(""), current_scope_level(0), in_function_body(false),
      has_errors(false), has_warnings(false) {
    scope_stack.push_back(std::unordered_map<std::string, SymbolInfo>());
}

void SemanticAnalyzer::enter_scope() {
    scope_stack.push_back(std::unordered_map<std::string, SymbolInfo>());
    current_scope_level++;
}

void SemanticAnalyzer::exit_scope() {
    if (scope_stack.size() > 1) {
        for (const auto& [name, symbol] : scope_stack.back()) {
            if (!symbol.is_used && !symbol.name.empty()) {
                std::cout << "\nВ строке " << symbol.declaration_token.line 
                          << ", столбец " << symbol.declaration_token.column
                          << ": Неиспользуемая переменная '" << symbol.name << "'\n\n";
                has_warnings = true;
            }
        }
        scope_stack.pop_back();
        current_scope_level--;
    }
}

SymbolInfo* SemanticAnalyzer::lookup_symbol(const std::string& name) {
    for (int i = scope_stack.size() - 1; i >= 0; i--) {
        auto it = scope_stack[i].find(name);
        if (it != scope_stack[i].end()) {
            return &it->second;
        }
    }
    auto it = global_symbols.find(name);
    if (it != global_symbols.end()) {
        return &it->second;
    }
    return nullptr;
}

FunctionInfo* SemanticAnalyzer::lookup_function(const std::string& name) {
    auto it = functions.find(name);
    if (it != functions.end()) {
        return &it->second;
    }
    return nullptr;
}

bool SemanticAnalyzer::declare_symbol(const std::string& name, 
                                     const TypeInfo& type,
                                     const lexan::Token& token,
                                     const std::string& context) {
    if (scope_stack.back().find(name) != scope_stack.back().end()) {
        std::cout << "\nОшибка " << 302 << ": Повторное объявление переменной '" << name << "'";
        std::cout << "\nВ строке " << token.line << ", столбец " << token.column << "\n\n";
        has_errors = true;
        return false;
    }
    
    if (current_scope_level > 0 && global_symbols.find(name) != global_symbols.end()) {
        std::cout << "\nВ строке " << token.line << ", столбец " << token.column
                  << ": Переменная '" << name << "' скрывает глобальное объявление\n\n";
        has_warnings = true;
    }
    
    SymbolInfo symbol(name, type, token, false, current_scope_level, context);
    scope_stack.back()[name] = symbol;
    
    if (current_scope_level == 0) {
        global_symbols[name] = symbol;
    }
    
    return true;
}

bool SemanticAnalyzer::declare_function(const std::string& name,
                                       const TypeInfo& return_type,
                                       const lexan::Token& token) {
    if (functions.find(name) != functions.end()) {
        FunctionInfo* existing = &functions[name];
        if (existing->is_defined) {
            std::cout << "\nОшибка " << 303 << ": Повторное определение функции '" << name << "'";
            std::cout << "\nВ строке " << token.line << ", столбец " << token.column << "\n\n";
            has_errors = true;
            return false;
        } else {
            existing->is_defined = true;
            existing->return_type = return_type;
            existing->declaration_line = token.line;
            return true;
        }
    }
    
    FunctionInfo func(name, return_type, token.line);
    func.is_defined = true;
    functions[name] = func;
    
    return true;
}

TypeInfo SemanticAnalyzer::token_type_to_type(lexan::TokenType type) {
    switch (type) {
        case lexan::TK_INT:
            return TypeInfo("int", true, "number");
        case lexan::TK_UNSIGNED:
            return TypeInfo("unsigned int", true, "number");
        case lexan::TK_BOOL:
            return TypeInfo("bool", true, "boolean");
        case lexan::TK_STRING:
            return TypeInfo("string", true, "string");
        case lexan::TK_TIME_T:
            return TypeInfo("time_t", true, "number");
        case lexan::TK_SYMB:
            return TypeInfo("symb", true, "string");
        case lexan::TK_STRING_LIT:
            return TypeInfo("string", true, "string");
        case lexan::TK_CHAR_LIT:
            return TypeInfo("symb", true, "string");
        default:
            return TypeInfo("unknown", false, "any");
    }
}

TypeInfo SemanticAnalyzer::get_expression_type(parser::ASTNode* node) {
    if (!node) {
        return TypeInfo("void", true, "undefined");
    }
    
    switch (node->type) {
        case parser::ASTNode::Type::LITERAL:
            if (node->token.type == lexan::TK_NUMBER) {
                return TypeInfo("int", true, "number");
            } else if (node->token.type == lexan::TK_STRING_LIT) {
                return TypeInfo("string", true, "string");
            } else if (node->token.type == lexan::TK_CHAR_LIT) {
                return TypeInfo("symb", true, "string");
            } else if (node->token.type == lexan::TK_TRUE || 
                      node->token.type == lexan::TK_FALSE) {
                return TypeInfo("bool", true, "boolean");
            }
            break;
            
        case parser::ASTNode::Type::IDENTIFIER: {
            SymbolInfo* symbol = lookup_symbol(node->value);
            if (symbol) {
                symbol->is_used = true;
                return symbol->type;
            }
            FunctionInfo* func = lookup_function(node->value);
            if (func) {
                return func->return_type;
            }
            std::cout << "\nОшибка " << 304 << ": Необъявленный идентификатор '" << node->value << "'";
            std::cout << "\nВ строке " << node->token.line << ", столбец " << node->token.column << "\n\n";
            has_errors = true;
            break;
        }
            
        case parser::ASTNode::Type::BINARY_OP: {
            TypeInfo left_type = get_expression_type(node->children[0]);
            TypeInfo right_type = get_expression_type(node->children[1]);
            
            if (left_type.name == "unknown" || right_type.name == "unknown") {
                return TypeInfo("unknown", false, "any");
            }
            
            TypeInfo result = get_binary_op_result_type(left_type, right_type, node->value);
            
            if (result.name == "unknown") {
                std::cout << "\nОшибка " << 318 << ": Некорректная операция для типов '"
                          << left_type.name << "' и '" << right_type.name << "' с оператором '" << node->value << "'";
                std::cout << "\nВ строке " << node->token.line << ", столбец " << node->token.column << "\n\n";
                has_errors = true;
            }
            
            return result;
        }
            
        case parser::ASTNode::Type::UNARY_OP:
            return get_expression_type(node->children[0]);
            
        case parser::ASTNode::Type::FUNCTION_CALL: {
            FunctionInfo* func = lookup_function(node->value);
            if (func) {
                func->is_called = true;
                return func->return_type;
            }
            
            if (is_builtin_function(node->value)) {
                return get_builtin_return_type(node->value);
            }
            
            if (node->value == "to_str") {
                return TypeInfo("string", true, "string");
            } else if (node->value == "ThisVeryMoment") {
                return TypeInfo("time_t", true, "number");
            } else if (node->value == "TimeFled") {
                return TypeInfo("int", true, "number");
            } else if (node->value == "unite") {
                return TypeInfo("string", true, "string");
            } else if (node->value == "sum4") {
                return TypeInfo("int", true, "number");
            } else if (node->value == "proclaim") {
                return TypeInfo("void", true, "undefined");
            }
            
            std::cout << "\nОшибка " << 305 << ": Необъявленная функция '" << node->value << "'";
            std::cout << "\nВ строке " << node->token.line << ", столбец " << node->token.column << "\n\n";
            has_errors = true;
            break;
        }
            
        default:
            for (auto child : node->children) {
                TypeInfo child_type = get_expression_type(child);
                if (child_type.name != "unknown") {
                    return child_type;
                }
            }
            break;
    }
    
    return TypeInfo("unknown", false, "any");
}

bool SemanticAnalyzer::type_compatible(const TypeInfo& t1, const TypeInfo& t2,
                                      const std::string& operation) {
    if (t1.name == "unknown" || t2.name == "unknown") {
        return false;
    }
    
    if ((t1.name == "int" || t1.name == "unsigned int" || t1.name == "time_t") &&
        (t2.name == "int" || t2.name == "unsigned int" || t2.name == "time_t")) {
        return true;
    }
    
    if (t1.name == "string" && t2.name == "string") {
        return true;
    }
    
    if (t1.name == "bool" && t2.name == "bool") {
        return true;
    }
    
    if (operation == "PLUS") {
        if (t1.name == "string" || t2.name == "string") {
            return true;
        }
    }
    
    if (operation == "ASSIGN") {
        if ((t1.name == "string" && t2.name == "symb") ||
            (t1.name == "symb" && t2.name == "string")) {
            return true;
        }
    }
    
    return (t1.name == t2.name);
}

TypeInfo SemanticAnalyzer::get_binary_op_result_type(const TypeInfo& t1,
                                                    const TypeInfo& t2,
                                                    const std::string& op) {
    if (t1.name == "unknown" || t2.name == "unknown") {
        return TypeInfo("unknown", false, "any");
    }
    
    if (op == "PLUS" || op == "MINUS" || op == "MULT" || op == "DIV" || op == "MOD" || op == "POW") {
        if (op == "PLUS") {
            if (t1.name == "string" || t2.name == "string" || 
                t1.name == "symb" || t2.name == "symb") {
                return TypeInfo("string", true, "string");
            }
        }
        
        if ((t1.name == "int" || t1.name == "unsigned int" || t1.name == "time_t") &&
            (t2.name == "int" || t2.name == "unsigned int" || t2.name == "time_t")) {
            return TypeInfo("int", true, "number");
        }
    }
    
    if (op == "EQ" || op == "NE" || op == "GT" || op == "LT" || 
        op == "GE" || op == "LE") {
        return TypeInfo("bool", true, "boolean");
    }
    
    if (op == "AND" || op == "OR") {
        return TypeInfo("bool", true, "boolean");
    }
    
    if (op == "ASSIGN" || op == "PLUS_ASSIGN" || op == "MINUS_ASSIGN" || 
        op == "MULT_ASSIGN" || op == "DIV_ASSIGN") {
        return t1;
    }
    
    return TypeInfo("unknown", false, "any");
}

bool SemanticAnalyzer::is_builtin_function(const std::string& name) {
    static const std::set<std::string> builtins = {
        "proclaim", "to_str", "TimeFled", 
        "ThisVeryMoment", "unite", "sum4"
    };
    return builtins.find(name) != builtins.end();
}

bool SemanticAnalyzer::check_builtin_arguments(const std::string& name,
                                              const std::vector<TypeInfo>& arg_types) {
    if (name == "proclaim") return arg_types.size() >= 1;
    if (name == "to_str") return arg_types.size() == 1;
    if (name == "TimeFled") return arg_types.size() == 2;
    if (name == "ThisVeryMoment") return arg_types.size() == 0;
    if (name == "unite") return arg_types.size() >= 2;
    if (name == "sum4") return arg_types.size() == 4;
    return false;
}

TypeInfo SemanticAnalyzer::get_builtin_return_type(const std::string& name) {
    if (name == "proclaim") return TypeInfo("void", true, "undefined");
    if (name == "to_str") return TypeInfo("string", true, "string");
    if (name == "TimeFled") return TypeInfo("int", true, "number");
    if (name == "ThisVeryMoment") return TypeInfo("time_t", true, "number");
    if (name == "unite") return TypeInfo("string", true, "string");
    if (name == "sum4") return TypeInfo("int", true, "number");
    return TypeInfo("unknown", false, "any");
}

void SemanticAnalyzer::analyze_node(parser::ASTNode* node) {
    if (!node) return;
    
    switch (node->type) {
        case parser::ASTNode::Type::PROGRAM:
            analyze_program(node);
            break;
        case parser::ASTNode::Type::FUNCTION_DECL:
            analyze_function_decl(node);
            break;
        case parser::ASTNode::Type::PROCEDURE_DECL:
            analyze_procedure_decl(node);
            break;
        case parser::ASTNode::Type::VARIABLE_DECL:
            analyze_variable_decl(node);
            break;
        case parser::ASTNode::Type::ASSIGNMENT:
            analyze_assignment(node);
            break;
        case parser::ASTNode::Type::FUNCTION_CALL:
            analyze_function_call(node);
            break;
        case parser::ASTNode::Type::DO_WHILE_LOOP:
            analyze_do_while(node);
            break;
        case parser::ASTNode::Type::RETURN_STMT:
            analyze_return(node);
            break;
        case parser::ASTNode::Type::BLOCK:
            analyze_block(node);
            break;
        case parser::ASTNode::Type::BINARY_OP:
        case parser::ASTNode::Type::UNARY_OP:
        case parser::ASTNode::Type::LITERAL:
        case parser::ASTNode::Type::IDENTIFIER:
            analyze_expression(node);
            break;
        default:
            for (auto child : node->children) {
                analyze_node(child);
            }
            break;
    }
}

void SemanticAnalyzer::analyze_program(parser::ASTNode* node) {
    for (auto child : node->children) {
        analyze_node(child);
    }
    
    for (const auto& [name, func] : functions) {
        if (func.is_defined && !func.is_called && name != "main") {
            std::cout << "\nПредупреждение: Функция '" << name << "' определена, но нигде не вызывается\n\n";
            has_warnings = true;
        }
    }
}

void SemanticAnalyzer::analyze_function_decl(parser::ASTNode* node) {
    if (!node || node->children.empty()) return;
    
    TypeInfo return_type;
    if (node->children[0]->type == parser::ASTNode::Type::TYPE_SPECIFIER) {
        return_type = token_type_to_type(node->children[0]->token.type);
    }
    
    current_function = node->value;
    if (!declare_function(current_function, return_type, node->token)) {
        has_errors = true;
        return;
    }
    
    in_function_body = false;
    enter_scope();
    
    if (node->children.size() > 1) {
        auto params_node = node->children[1];
        if (params_node->type == parser::ASTNode::Type::PARAM_LIST) {
            for (auto param : params_node->children) {
                if (param->children.size() > 0) {
                    auto param_type_node = param->children[0];
                    TypeInfo param_type = token_type_to_type(param_type_node->token.type);
                    if (!declare_symbol(param->value, param_type, param->token, "parameter")) {
                        has_errors = true;
                    }
                    
                    FunctionInfo* func = lookup_function(current_function);
                    if (func) {
                        SymbolInfo param_symbol(param->value, param_type, 
                                              param->token, true, 1, "parameter");
                        func->parameters.push_back(param_symbol);
                    }
                }
            }
        }
    }
    
    in_function_body = true;
    
    if (node->children.size() > 2) {
        auto body_node = node->children[2];
        if (body_node->type == parser::ASTNode::Type::BLOCK) {
            analyze_block(body_node);
        }
    }
    
    in_function_body = false;
    exit_scope();
    current_function = "";
}

void SemanticAnalyzer::analyze_procedure_decl(parser::ASTNode* node) {
    TypeInfo void_type("void", true, "undefined");
    
    current_function = node->value;
    if (!declare_function(current_function, void_type, node->token)) {
        has_errors = true;
        return;
    }
    
    in_function_body = false;
    enter_scope();
    
    if (node->children.size() > 0) {
        auto params_node = node->children[0];
        if (params_node && params_node->type == parser::ASTNode::Type::PARAM_LIST) {
            for (auto param : params_node->children) {
                if (param->children.size() > 0) {
                    auto param_type_node = param->children[0];
                    TypeInfo param_type = token_type_to_type(param_type_node->token.type);
                    if (!declare_symbol(param->value, param_type, param->token, "parameter")) {
                        has_errors = true;
                    }
                }
            }
        }
    }
    
    in_function_body = true;
    
    if (node->children.size() > 1) {
        auto body_node = node->children[1];
        if (body_node->type == parser::ASTNode::Type::BLOCK) {
            analyze_block(body_node);
        }
    }
    
    in_function_body = false;
    exit_scope();
    current_function = "";
}

void SemanticAnalyzer::analyze_variable_decl(parser::ASTNode* node) {
    if (!node || node->children.empty()) return;
    
    TypeInfo var_type;
    if (node->children[0]->type == parser::ASTNode::Type::TYPE_SPECIFIER) {
        var_type = token_type_to_type(node->children[0]->token.type);
        
        if (node->children[0]->value == "UNSIGNED INT") {
            var_type = TypeInfo("unsigned int", true, "number");
        }
    }
    
    std::string context = in_function_body ? "local" : "global";
    if (!declare_symbol(node->value, var_type, node->token, context)) {
        has_errors = true;
        return;
    }
    
    if (node->children.size() > 1) {
        TypeInfo expr_type = get_expression_type(node->children[1]);
        if (!type_compatible(var_type, expr_type, "=")) {
            std::cout << "\nОшибка " << 306 << ": Несоответствие типов при инициализации '" << node->value 
                      << "'. Ожидается " << var_type.to_string()
                      << ", получено " << expr_type.to_string();
            std::cout << "\nВ строке " << node->token.line << ", столбец " << node->token.column << "\n\n";
            has_errors = true;
        }
        
        SymbolInfo* symbol = lookup_symbol(node->value);
        if (symbol) {
            symbol->is_initialized = true;
        }
    }
}

void SemanticAnalyzer::analyze_assignment(parser::ASTNode* node) {
    if (!node || node->children.size() < 2) return;
    
    auto target = node->children[0];
    if (target->type != parser::ASTNode::Type::IDENTIFIER) {
        std::cout << "\nОшибка " << 310 << ": Цель присваивания должна быть идентификатором";
        std::cout << "\nВ строке " << node->token.line << ", столбец " << node->token.column << "\n\n";
        has_errors = true;
        return;
    }
    
    SymbolInfo* symbol = lookup_symbol(target->value);
    if (!symbol) {
        std::cout << "\nОшибка " << 304 << ": Необъявленная переменная '" << target->value << "' в присваивании";
        std::cout << "\nВ строке " << target->token.line << ", столбец " << target->token.column << "\n\n";
        has_errors = true;
        return;
    }
    
    TypeInfo expr_type = get_expression_type(node->children[1]);
    
    std::string op = node->value;
    if (!type_compatible(symbol->type, expr_type, op)) {
        std::cout << "\nОшибка " << 306 << ": Несоответствие типов в присваивании '" << target->value 
                  << "'. Ожидается " << symbol->type.to_string()
                  << ", получено " << expr_type.to_string();
        std::cout << "\nВ строке " << node->token.line << ", столбец " << node->token.column << "\n\n";
        has_errors = true;
        return;
    }
    
    symbol->is_initialized = true;
    symbol->is_used = true;
}

void SemanticAnalyzer::analyze_function_call(parser::ASTNode* node) {
    if (!node) return;
    
    FunctionInfo* func = lookup_function(node->value);
    bool is_builtin = is_builtin_function(node->value);
    
    if (!func && !is_builtin) {
        std::cout << "\nОшибка " << 305 << ": Необъявленная функция '" << node->value << "'";
        std::cout << "\nВ строке " << node->token.line << ", столбец " << node->token.column << "\n\n";
        has_errors = true;
        return;
    }
    
    if (func) {
        func->is_called = true;
    }
    
    if (node->children.size() > 0) {
        auto args_node = node->children[0];
        if (args_node->type == parser::ASTNode::Type::ARG_LIST) {
            if (is_builtin) {
                std::vector<TypeInfo> arg_types;
                for (auto arg : args_node->children) {
                    arg_types.push_back(get_expression_type(arg));
                }
                if (!check_builtin_arguments(node->value, arg_types)) {
                    std::cout << "\nОшибка " << 308 << ": Некорректные аргументы для встроенной функции '" << node->value << "'";
                    std::cout << "\nВ строке " << node->token.line << ", столбец " << node->token.column << "\n\n";
                    has_errors = true;
                }
            }
        }
    }
}

void SemanticAnalyzer::analyze_do_while(parser::ASTNode* node) {
    if (!node || node->children.size() < 2) return;
    
    enter_scope();
    
    if (node->children[0]->type == parser::ASTNode::Type::BLOCK) {
        analyze_block(node->children[0]);
    } else {
        analyze_node(node->children[0]);
    }
    
    TypeInfo cond_type = get_expression_type(node->children[1]);
    if (cond_type.name != "bool" && cond_type.name != "int") {
        std::cout << "\nПредупреждение: Условие в do-while должно быть булевым или числовым, получено " 
                  << cond_type.to_string();
        std::cout << "\nВ строке " << node->token.line << ", столбец " << node->token.column << "\n\n";
        has_warnings = true;
    }
    
    exit_scope();
}

void SemanticAnalyzer::analyze_return(parser::ASTNode* node) {
    if (!current_function.empty()) {
        FunctionInfo* func = lookup_function(current_function);
        if (func) {
            if (node->children.empty()) {
                if (func->return_type.name != "void") {
                    std::cout << "\nОшибка " << 307 << ": Функция '" << current_function 
                              << "' должна возвращать значение типа " 
                              << func->return_type.to_string();
                    std::cout << "\nВ строке " << node->token.line << ", столбец " << node->token.column << "\n\n";
                    has_errors = true;
                }
            } else {
                TypeInfo return_type = get_expression_type(node->children[0]);
                if (!type_compatible(func->return_type, return_type)) {
                    std::cout << "\nОшибка " << 306 << ": Несоответствие типа возвращаемого значения в функции '" << current_function
                              << "'. Ожидается " << func->return_type.to_string()
                              << ", получено " << return_type.to_string();
                    std::cout << "\nВ строке " << node->token.line << ", столбец " << node->token.column << "\n\n";
                    has_errors = true;
                }
            }
        }
    } else {
        std::cout << "\nОшибка " << 319 << ": Оператор return вне функции";
        std::cout << "\nВ строке " << node->token.line << ", столбец " << node->token.column << "\n\n";
        has_errors = true;
    }
}

void SemanticAnalyzer::analyze_expression(parser::ASTNode* node) {
    get_expression_type(node);
}

void SemanticAnalyzer::analyze_block(parser::ASTNode* node) {
    enter_scope();
    
    for (auto child : node->children) {
        analyze_node(child);
    }
    
    exit_scope();
}

bool SemanticAnalyzer::analyze(parser::ASTNode* ast) {
    if (!ast) {
        std::cout << "\nОшибка " << 301 << ": AST равен null\n\n";
        has_errors = true;
        return false;
    }
    
    global_symbols.clear();
    functions.clear();
    scope_stack.clear();
    scope_stack.push_back(std::unordered_map<std::string, SymbolInfo>());
    current_scope_level = 0;
    current_function = "";
    in_function_body = false;
    has_errors = false;
    has_warnings = false;
    
    analyze_node(ast);
    
    for (const auto& [name, symbol] : global_symbols) {
        if (!symbol.is_initialized) {
            std::cout << "\nПредупреждение: Глобальная переменная '" << name << "' может быть неинициализированной\n\n";
            has_warnings = true;
        }
    }
    
    return !has_errors;
}

void SemanticAnalyzer::print_symbol_table() const {
    std::cout << "\n=== ТАБЛИЦА СИМВОЛОВ ===\n";
    
    if (global_symbols.empty()) {
        std::cout << "Нет глобальных символов\n";
    } else {
        std::cout << "Глобальные символы (" << global_symbols.size() << "):\n";
        std::cout << std::left << std::setw(20) << "Имя" 
                  << std::setw(25) << "Тип" 
                  << std::setw(12) << "Инициализир."
                  << std::setw(12) << "Используется"
                  << std::setw(15) << "Контекст\n";
        std::cout << std::string(84, '-') << "\n";
        
        for (const auto& [name, symbol] : global_symbols) {
            std::cout << std::left << std::setw(20) << name
                      << std::setw(25) << symbol.type.to_string()
                      << std::setw(12) << (symbol.is_initialized ? "да" : "нет")
                      << std::setw(12) << (symbol.is_used ? "да" : "нет")
                      << std::setw(15) << symbol.context << "\n";
        }
    }
}

void SemanticAnalyzer::print_function_table() const {
    std::cout << "\n=== ТАБЛИЦА ФУНКЦИЙ ===\n";
    
    if (functions.empty()) {
        std::cout << "Нет функций\n";
    } else {
        std::cout << "Функции (" << functions.size() << "):\n";
        std::cout << std::left << std::setw(20) << "Имя" 
                  << std::setw(25) << "Тип возврата" 
                  << std::setw(12) << "Определена"
                  << std::setw(12) << "Вызывалась"
                  << std::setw(12) << "Параметры"
                  << "Строка\n";
        std::cout << std::string(81, '-') << "\n";
        
        for (const auto& [name, func] : functions) {
            std::cout << std::left << std::setw(20) << name
                      << std::setw(25) << func.return_type.to_string()
                      << std::setw(12) << (func.is_defined ? "да" : "нет")
                      << std::setw(12) << (func.is_called ? "да" : "нет")
                      << std::setw(12) << func.parameters.size()
                      << func.declaration_line << "\n";
        }
    }
}

void SemanticAnalyzer::print_type_summary() const {
    std::cout << "\n=== СВОДКА ПО ТИПАМ ===\n";
    
    std::unordered_map<std::string, int> type_counts;
    for (const auto& [name, symbol] : global_symbols) {
        type_counts[symbol.type.name]++;
    }
    
    for (const auto& [type, count] : type_counts) {
        std::cout << type << ": " << count << " переменных\n";
    }
}

std::string SemanticAnalyzer::generate_report() const {
    std::stringstream report;
    
    report << "=== ОТЧЕТ СЕМАНТИЧЕСКОГО АНАЛИЗА ===\n";
    report << "Статус: " << (has_errors ? "С ОШИБКАМИ" : (has_warnings ? "С ПРЕДУПРЕЖДЕНИЯМИ" : "УСПЕШНО")) << "\n";
    report << "Функций: " << functions.size() << "\n";
    report << "Символов: " << global_symbols.size() << "\n\n";
    
    int used_vars = 0;
    int initialized_vars = 0;
    for (const auto& [name, symbol] : global_symbols) {
        if (symbol.is_used) used_vars++;
        if (symbol.is_initialized) initialized_vars++;
    }
    
    report << "Использовано переменных: " << used_vars << "/" << global_symbols.size() << "\n";
    report << "Инициализировано переменных: " << initialized_vars << "/" << global_symbols.size() << "\n";
    
    int called_funcs = 0;
    for (const auto& [name, func] : functions) {
        if (func.is_called) called_funcs++;
    }
    
    report << "Вызвано функций: " << called_funcs << "/" << functions.size() << "\n";
    
    return report.str();
}

bool SemanticAnalyzer::has_errors_occurred() const {
    return has_errors;
}

bool SemanticAnalyzer::has_warnings_occurred() const {
    return has_warnings;
}

} // namespace semantic