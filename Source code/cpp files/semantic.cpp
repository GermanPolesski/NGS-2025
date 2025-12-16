#include "semantic.h"
#include <iostream>
#include <sstream>
#include <iomanip>

namespace semantic {

SemanticAnalyzer::SemanticAnalyzer() 
    : current_function(""), current_scope_level(0), in_function_body(false) {
    // Инициализируем стек областей видимости с глобальной областью
    scope_stack.push_back(std::unordered_map<std::string, SymbolInfo>());
}

// Вход в новую область видимости
void SemanticAnalyzer::enter_scope() {
    scope_stack.push_back(std::unordered_map<std::string, SymbolInfo>());
    current_scope_level++;
}

// Выход из текущей области видимости
void SemanticAnalyzer::exit_scope() {
    if (scope_stack.size() > 1) {
        // Проверяем неиспользуемые переменные в текущей области
        for (const auto& [name, symbol] : scope_stack.back()) {
            if (!symbol.is_used && !symbol.name.empty()) {
                std::stringstream ss;
                ss << "Unused variable '" << symbol.name << "'";
                add_warning(ss.str(), symbol.declaration_token);
            }
        }
        scope_stack.pop_back();
        current_scope_level--;
    }
}

// Поиск символа в текущей и родительских областях видимости
SymbolInfo* SemanticAnalyzer::lookup_symbol(const std::string& name) {
    // Ищем с конца (от текущей области к глобальной)
    for (int i = scope_stack.size() - 1; i >= 0; i--) {
        auto it = scope_stack[i].find(name);
        if (it != scope_stack[i].end()) {
            return &it->second;
        }
    }
    // Проверяем глобальные символы
    auto it = global_symbols.find(name);
    if (it != global_symbols.end()) {
        return &it->second;
    }
    return nullptr;
}

// Поиск функции
FunctionInfo* SemanticAnalyzer::lookup_function(const std::string& name) {
    auto it = functions.find(name);
    if (it != functions.end()) {
        return &it->second;
    }
    return nullptr;
}

// Объявление символа
bool SemanticAnalyzer::declare_symbol(const std::string& name, 
                                     const TypeInfo& type,
                                     const lexan::Token& token,
                                     const std::string& context) {
    // Проверяем, не объявлен ли уже символ в текущей области
    if (scope_stack.back().find(name) != scope_stack.back().end()) {
        std::stringstream ss;
        ss << "Redeclaration of variable '" << name << "'";
        add_error(ss.str(), token);
        return false;
    }
    
    // Проверяем глобальные символы (если не в глобальной области)
    if (current_scope_level > 0 && global_symbols.find(name) != global_symbols.end()) {
        std::stringstream ss;
        ss << "Variable '" << name << "' shadows global declaration";
        add_warning(ss.str(), token);
    }
    
    SymbolInfo symbol(name, type, token, false, current_scope_level, context);
    scope_stack.back()[name] = symbol;
    
    // Если в глобальной области, добавляем в глобальную таблицу
    if (current_scope_level == 0) {
        global_symbols[name] = symbol;
    }
    
    return true;
}

// Объявление функции
bool SemanticAnalyzer::declare_function(const std::string& name,
                                       const TypeInfo& return_type,
                                       const lexan::Token& token) {
    // Проверяем, не объявлена ли уже функция
    if (functions.find(name) != functions.end()) {
        FunctionInfo* existing = &functions[name];
        if (existing->is_defined) {
            std::stringstream ss;
            ss << "Redefinition of function '" << name << "'";
            add_error(ss.str(), token);
            return false;
        } else {
            // Обновляем информацию о функции
            existing->is_defined = true;
            existing->return_type = return_type;
            existing->declaration_line = token.line;
            return true;
        }
    }
    
    // Новая функция
    FunctionInfo func(name, return_type, token.line);
    func.is_defined = true;
    functions[name] = func;
    
    return true;
}

// Преобразование типа токена в TypeInfo
TypeInfo SemanticAnalyzer::token_type_to_type(lexan::TokenType type) {
    switch (type) {
        case lexan::TK_INT:
            return TypeInfo("int", true, "number");
        case lexan::TK_UNSIGNED:
            // Обработка unsigned int
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

// Определение типа выражения
TypeInfo SemanticAnalyzer::get_expression_type(parser::ASTNode* node) {
    if (!node) {
        std::cout << "[DEBUG] get_expression_type: null node" << std::endl;
        return TypeInfo("void", true, "undefined");
    }
    
    std::cout << "[DEBUG] get_expression_type: node type = " << static_cast<int>(node->type)
              << ", value = " << node->value << std::endl;
    
    switch (node->type) {
        case parser::ASTNode::Type::LITERAL:
            std::cout << "[DEBUG] LITERAL: token.type = " << static_cast<int>(node->token.type) 
                      << ", value = " << node->value << std::endl;
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
            std::cout << "[DEBUG] IDENTIFIER: " << node->value << std::endl;
            SymbolInfo* symbol = lookup_symbol(node->value);
            if (symbol) {
                std::cout << "[DEBUG] Found symbol: " << node->value 
                          << " with type: " << symbol->type.name << std::endl;
                symbol->is_used = true;
                return symbol->type;
            }
            // Проверяем, не функция ли это
            FunctionInfo* func = lookup_function(node->value);
            if (func) {
                std::cout << "[DEBUG] Found function: " << node->value << std::endl;
                return func->return_type;
            }
            std::stringstream ss;
            ss << "Undeclared identifier '" << node->value << "'";
            add_error(ss.str(), node->token);
            break;
        }
            
        case parser::ASTNode::Type::BINARY_OP: {
            std::cout << "[DEBUG] BINARY_OP: " << node->value << std::endl;
            TypeInfo left_type = get_expression_type(node->children[0]);
            TypeInfo right_type = get_expression_type(node->children[1]);
            
            std::cout << "[DEBUG] BINARY_OP left type: " << left_type.name 
                      << ", right type: " << right_type.name << std::endl;
            
            // Если один из типов unknown, возвращаем unknown
            if (left_type.name == "unknown" || right_type.name == "unknown") {
                return TypeInfo("unknown", false, "any");
            }
            
            TypeInfo result = get_binary_op_result_type(left_type, right_type, node->value);
            std::cout << "[DEBUG] BINARY_OP result type: " << result.name << std::endl;
            return result;
        }
            
        case parser::ASTNode::Type::UNARY_OP:
            std::cout << "[DEBUG] UNARY_OP: " << node->value << std::endl;
            return get_expression_type(node->children[0]);
            
        case parser::ASTNode::Type::FUNCTION_CALL: {
            std::cout << "[DEBUG] FUNCTION_CALL: " << node->value << std::endl;
            FunctionInfo* func = lookup_function(node->value);
            if (func) {
                std::cout << "[DEBUG] Found function in table: " << node->value << std::endl;
                func->is_called = true;
                return func->return_type;
            }
            
            // Проверяем встроенные функции
            if (is_builtin_function(node->value)) {
                std::cout << "[DEBUG] Builtin function: " << node->value << std::endl;
                TypeInfo result = get_builtin_return_type(node->value);
                std::cout << "[DEBUG] Builtin return type: " << result.name << std::endl;
                return result;
            }
            
            // Дополнительная проверка встроенных функций
            std::cout << "[DEBUG] Checking manual builtin list for: " << node->value << std::endl;
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
            
            std::stringstream ss;
            ss << "Undeclared function '" << node->value << "'";
            add_error(ss.str(), node->token);
            break;
        }
            
        default:
            std::cout << "[DEBUG] Unknown node type in get_expression_type: " 
                      << static_cast<int>(node->type) << std::endl;
            // Рекурсивно проверяем детей для сложных выражений
            for (auto child : node->children) {
                TypeInfo child_type = get_expression_type(child);
                if (child_type.name != "unknown") {
                    return child_type;
                }
            }
            break;
    }
    
    std::cout << "[DEBUG] Returning unknown type for node value: " << node->value << std::endl;
    return TypeInfo("unknown", false, "any");
}

// Проверка совместимости типов
bool SemanticAnalyzer::type_compatible(const TypeInfo& t1, const TypeInfo& t2,
                                      const std::string& operation) {
    std::cout << "[DEBUG] type_compatible: t1=" << t1.name 
              << ", t2=" << t2.name << ", op=" << operation << std::endl;
    
    // Если один из типов unknown, считаем их несовместимыми
    if (t1.name == "unknown" || t2.name == "unknown") {
        std::cout << "[DEBUG] One of types is unknown" << std::endl;
        return false;
    }
    
    // Оба типа числовые
    if ((t1.name == "int" || t1.name == "unsigned int" || t1.name == "time_t") &&
        (t2.name == "int" || t2.name == "unsigned int" || t2.name == "time_t")) {
        return true;
    }
    
    // Для строковых операций
    if (t1.name == "string" && t2.name == "string") {
        return true;
    }
    
    // Для логических операций
    if (t1.name == "bool" && t2.name == "bool") {
        return true;
    }
    
    // Для операции "PLUS" строки можно складывать с любым типом (преобразуется в строку)
    if (operation == "PLUS") {
        if (t1.name == "string" || t2.name == "string") {
            return true;
        }
    }
    
    // Для операции "ASSIGN" допускается присваивание символьного литерала строке и наоборот
    if (operation == "ASSIGN") {
        if ((t1.name == "string" && t2.name == "symb") ||
            (t1.name == "symb" && t2.name == "string")) {
            return true;
        }
    }
    
    // Во всех остальных случаях типы должны совпадать
    bool result = (t1.name == t2.name);
    std::cout << "[DEBUG] type_compatible result: " << result << std::endl;
    return result;
}

// Определение типа результата бинарной операции
TypeInfo SemanticAnalyzer::get_binary_op_result_type(const TypeInfo& t1,
                                                    const TypeInfo& t2,
                                                    const std::string& op) {
    std::cout << "[DEBUG] get_binary_op_result_type: t1=" << t1.name 
              << " (" << t1.js_type << "), t2=" << t2.name 
              << " (" << t2.js_type << "), op=" << op << std::endl;
    
    // Если один из типов unknown, возвращаем unknown
    if (t1.name == "unknown" || t2.name == "unknown") {
        return TypeInfo("unknown", false, "any");
    }
    
    // Для арифметических операций (обратите внимание на имена операторов!)
    if (op == "PLUS" || op == "MINUS" || op == "MULT" || op == "DIV" || op == "MOD" || op == "POW") {
        // Сложение строк
        if (op == "PLUS") {
            if (t1.name == "string" || t2.name == "string" || 
                t1.name == "symb" || t2.name == "symb") {
                return TypeInfo("string", true, "string");
            }
        }
        
        // Числовые операции
        if ((t1.name == "int" || t1.name == "unsigned int" || t1.name == "time_t") &&
            (t2.name == "int" || t2.name == "unsigned int" || t2.name == "time_t")) {
            return TypeInfo("int", true, "number");
        }
    }
    
    // Для операций сравнения (обратите внимание на имена операторов!)
    if (op == "EQ" || op == "NE" || op == "GT" || op == "LT" || 
        op == "GE" || op == "LE") {
        return TypeInfo("bool", true, "boolean");
    }
    
    // Для логических операций
    if (op == "AND" || op == "OR") {
        return TypeInfo("bool", true, "boolean");
    }
    
    // Для операторов присваивания
    if (op == "ASSIGN" || op == "PLUS_ASSIGN" || op == "MINUS_ASSIGN" || 
        op == "MULT_ASSIGN" || op == "DIV_ASSIGN") {
        return t1; // Тип результата - тип левой части
    }
    
    std::cout << "[DEBUG] Unknown binary operation: " << op 
              << " for types: " << t1.name << " and " << t2.name << std::endl;
    return TypeInfo("unknown", false, "any");
}

// Проверка встроенных функций
bool SemanticAnalyzer::is_builtin_function(const std::string& name) {
    bool result = name == "proclaim" || name == "to_str" || name == "TimeFled" ||
                  name == "ThisVeryMoment" || name == "unite" || name == "sum4";
    std::cout << "[DEBUG] is_builtin_function: " << name << " -> " << result << std::endl;
    return result;
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

bool SemanticAnalyzer::check_builtin_arguments(const std::string& name,
                                              const std::vector<TypeInfo>& arg_types) {
    // Простая проверка для примера
    if (name == "proclaim") return arg_types.size() >= 1;
    if (name == "to_str") return arg_types.size() == 1;
    if (name == "TimeFled") return arg_types.size() == 2;
    if (name == "ThisVeryMoment") return arg_types.size() == 0;
    if (name == "unite") return arg_types.size() >= 2;
    if (name == "sum4") return arg_types.size() == 4;
    return false;
}

// Методы добавления ошибок и предупреждений
void SemanticAnalyzer::add_error(const std::string& message, const lexan::Token& token) {
    std::stringstream ss;
    ss << "Error at line " << token.line << ", column " << token.column 
       << ": " << message << " ('" << token.value << "')";
    errors.push_back(ss.str());
}

void SemanticAnalyzer::add_warning(const std::string& message, const lexan::Token& token) {
    std::stringstream ss;
    ss << "Warning at line " << token.line << ", column " << token.column 
       << ": " << message << " ('" << token.value << "')";
    warnings.push_back(ss.str());
}

// Анализ узла AST
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
            // Обрабатываем детей для других узлов
            for (auto child : node->children) {
                analyze_node(child);
            }
            break;
    }
}

// Анализ программы
void SemanticAnalyzer::analyze_program(parser::ASTNode* node) {
    // Анализируем все дочерние узлы (объявления функций, процедур, блоков)
    for (auto child : node->children) {
        analyze_node(child);
    }
    
    // После анализа всей программы проверяем невызванные функции
    for (const auto& [name, func] : functions) {
        if (func.is_defined && !func.is_called && name != "main") {
            std::stringstream ss;
            ss << "Function '" << name << "' is defined but never called";
            warnings.push_back(ss.str());
        }
    }
}

// Анализ объявления функции
void SemanticAnalyzer::analyze_function_decl(parser::ASTNode* node) {
    if (!node || node->children.empty()) return;
    
    // Получаем информацию о типе возвращаемого значения
    TypeInfo return_type;
    if (node->children[0]->type == parser::ASTNode::Type::TYPE_SPECIFIER) {
        return_type = token_type_to_type(node->children[0]->token.type);
    }
    
    // Объявляем функцию
    current_function = node->value;
    declare_function(current_function, return_type, node->token);
    
    // Входим в область видимости функции
    in_function_body = false;
    enter_scope();
    
    // Анализируем параметры
    if (node->children.size() > 1) {
        auto params_node = node->children[1];
        if (params_node->type == parser::ASTNode::Type::PARAM_LIST) {
            for (auto param : params_node->children) {
                if (param->children.size() > 0) {
                    auto param_type_node = param->children[0];
                    TypeInfo param_type = token_type_to_type(param_type_node->token.type);
                    declare_symbol(param->value, param_type, param->token, "parameter");
                    
                    // Добавляем параметр в информацию о функции
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
    
    // Входим в тело функции
    in_function_body = true;
    
    // Анализируем тело функции
    if (node->children.size() > 2) {
        auto body_node = node->children[2];
        if (body_node->type == parser::ASTNode::Type::BLOCK) {
            analyze_block(body_node);
        }
    }
    
    // Выходим из области видимости функции
    in_function_body = false;
    exit_scope();
    current_function = "";
}

// Анализ объявления процедуры
void SemanticAnalyzer::analyze_procedure_decl(parser::ASTNode* node) {
    // Процедура - это функция, возвращающая void
    TypeInfo void_type("void", true, "undefined");
    
    current_function = node->value;
    declare_function(current_function, void_type, node->token);
    
    in_function_body = false;
    enter_scope();
    
    // Анализируем параметры (если есть)
    if (node->children.size() > 0) {
        auto params_node = node->children[0];
        if (params_node && params_node->type == parser::ASTNode::Type::PARAM_LIST) {
            for (auto param : params_node->children) {
                if (param->children.size() > 0) {
                    auto param_type_node = param->children[0];
                    TypeInfo param_type = token_type_to_type(param_type_node->token.type);
                    declare_symbol(param->value, param_type, param->token, "parameter");
                }
            }
        }
    }
    
    in_function_body = true;
    
    // Анализируем тело процедуры
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

// Анализ объявления переменной
void SemanticAnalyzer::analyze_variable_decl(parser::ASTNode* node) {
    if (!node || node->children.empty()) return;
    
    // Получаем тип переменной
    TypeInfo var_type;
    if (node->children[0]->type == parser::ASTNode::Type::TYPE_SPECIFIER) {
        var_type = token_type_to_type(node->children[0]->token.type);
        
        // Обработка unsigned int
        if (node->children[0]->value == "UNSIGNED INT") {
            var_type = TypeInfo("unsigned int", true, "number");
        }
    }
    
    std::string context = in_function_body ? "local" : "global";
    declare_symbol(node->value, var_type, node->token, context);
    
    // Проверяем инициализацию
    if (node->children.size() > 1) {
        TypeInfo expr_type = get_expression_type(node->children[1]);
        if (!type_compatible(var_type, expr_type, "=")) {
            std::stringstream ss;
            ss << "Type mismatch in initialization of '" << node->value 
               << "'. Expected " << var_type.to_string()
               << ", got " << expr_type.to_string();
            add_error(ss.str(), node->token);
        }
        
        // Помечаем переменную как инициализированную
        SymbolInfo* symbol = lookup_symbol(node->value);
        if (symbol) {
            symbol->is_initialized = true;
        }
    }
}

// Анализ присваивания
void SemanticAnalyzer::analyze_assignment(parser::ASTNode* node) {
    if (!node || node->children.size() < 2) return;
    
    std::cout << "[DEBUG] analyze_assignment: " << node->value << std::endl;
    
    // Проверяем левую часть (идентификатор)
    auto target = node->children[0];
    if (target->type != parser::ASTNode::Type::IDENTIFIER) {
        add_error("Assignment target must be an identifier", node->token);
        return;
    }
    
    std::cout << "[DEBUG] Assignment target: " << target->value << std::endl;
    
    SymbolInfo* symbol = lookup_symbol(target->value);
    if (!symbol) {
        std::stringstream ss;
        ss << "Undeclared variable '" << target->value << "' in assignment";
        add_error(ss.str(), target->token);
        return;
    }
    
    std::cout << "[DEBUG] Target type: " << symbol->type.name << std::endl;
    
    // Проверяем тип правой части
    TypeInfo expr_type = get_expression_type(node->children[1]);
    std::cout << "[DEBUG] Expression type: " << expr_type.name << std::endl;
    
    std::string op = node->value;
    if (!type_compatible(symbol->type, expr_type, op)) {
        std::stringstream ss;
        ss << "Type mismatch in assignment to '" << target->value 
           << "'. Expected " << symbol->type.to_string()
           << ", got " << expr_type.to_string();
        add_error(ss.str(), node->token);
        return; // Добавлен return для предотвращения дальнейшей обработки
    }
    
    // Помечаем переменную как инициализированную
    symbol->is_initialized = true;
    symbol->is_used = true;
}

// Анализ вызова функции
void SemanticAnalyzer::analyze_function_call(parser::ASTNode* node) {
    if (!node) return;
    
    // Проверяем, объявлена ли функция
    FunctionInfo* func = lookup_function(node->value);
    bool is_builtin = is_builtin_function(node->value);
    
    if (!func && !is_builtin) {
        std::stringstream ss;
        ss << "Undeclared function '" << node->value << "'";
        add_error(ss.str(), node->token);
        return;
    }
    
    if (func) {
        func->is_called = true;
    }
    
    // Проверяем аргументы
    if (node->children.size() > 0) {
        auto args_node = node->children[0];
        if (args_node->type == parser::ASTNode::Type::ARG_LIST) {
            // Для встроенных функций выполняем простую проверку
            if (is_builtin) {
                std::vector<TypeInfo> arg_types;
                for (auto arg : args_node->children) {
                    arg_types.push_back(get_expression_type(arg));
                }
                if (!check_builtin_arguments(node->value, arg_types)) {
                    std::stringstream ss;
                    ss << "Invalid arguments for builtin function '" << node->value << "'";
                    add_error(ss.str(), node->token);
                }
            }
        }
    }
}

// Анализ цикла do-while
void SemanticAnalyzer::analyze_do_while(parser::ASTNode* node) {
    if (!node || node->children.size() < 2) return;
    
    // Входим в область видимости цикла
    enter_scope();
    
    // Анализируем тело цикла
    if (node->children[0]->type == parser::ASTNode::Type::BLOCK) {
        analyze_block(node->children[0]);
    } else {
        analyze_node(node->children[0]);
    }
    
    // Проверяем условие
    TypeInfo cond_type = get_expression_type(node->children[1]);
    if (cond_type.name != "bool" && cond_type.name != "int") {
        std::stringstream ss;
        ss << "Condition in do-while must be boolean or numeric, got " 
           << cond_type.to_string();
        add_warning(ss.str(), node->token);
    }
    
    // Выходим из области видимости цикла
    exit_scope();
}

// Анализ оператора return
void SemanticAnalyzer::analyze_return(parser::ASTNode* node) {
    if (!current_function.empty()) {
        FunctionInfo* func = lookup_function(current_function);
        if (func) {
            if (node->children.empty()) {
                // return без значения
                if (func->return_type.name != "void") {
                    std::stringstream ss;
                    ss << "Function '" << current_function 
                       << "' must return a value of type " 
                       << func->return_type.to_string();
                    add_error(ss.str(), node->token);
                }
            } else {
                // return с выражением
                TypeInfo return_type = get_expression_type(node->children[0]);
                if (!type_compatible(func->return_type, return_type)) {
                    std::stringstream ss;
                    ss << "Return type mismatch in function '" << current_function
                       << "'. Expected " << func->return_type.to_string()
                       << ", got " << return_type.to_string();
                    add_error(ss.str(), node->token);
                }
            }
        }
    } else {
        add_error("return statement outside of function", node->token);
    }
}

// Анализ выражения
void SemanticAnalyzer::analyze_expression(parser::ASTNode* node) {
    // Для выражений просто получаем тип (проверки уже выполнены в get_expression_type)
    get_expression_type(node);
}

// Анализ блока
void SemanticAnalyzer::analyze_block(parser::ASTNode* node) {
    enter_scope();
    
    for (auto child : node->children) {
        analyze_node(child);
    }
    
    exit_scope();
}

// Основной метод анализа
bool SemanticAnalyzer::analyze(parser::ASTNode* ast) {
    if (!ast) {
        errors.push_back("AST is null");
        return false;
    }
    
    // Сбрасываем состояние
    errors.clear();
    warnings.clear();
    global_symbols.clear();
    functions.clear();
    scope_stack.clear();
    scope_stack.push_back(std::unordered_map<std::string, SymbolInfo>());
    current_scope_level = 0;
    current_function = "";
    in_function_body = false;
    
    // Выполняем анализ
    analyze_node(ast);
    
    // Проверяем неинициализированные переменные
    for (const auto& [name, symbol] : global_symbols) {
        if (!symbol.is_initialized) {
            std::stringstream ss;
            ss << "Global variable '" << name << "' may be uninitialized";
            warnings.push_back(ss.str());
        }
    }
    
    return errors.empty();
}

// Вывод таблицы символов
void SemanticAnalyzer::print_symbol_table() const {
    std::cout << "\n=== SYMBOL TABLE ===\n";
    
    if (global_symbols.empty()) {
        std::cout << "No global symbols\n";
    } else {
        std::cout << "Global symbols (" << global_symbols.size() << "):\n";
        std::cout << std::left << std::setw(20) << "Name" 
                  << std::setw(25) << "Type" 
                  << std::setw(12) << "Initialized"
                  << std::setw(12) << "Used"
                  << std::setw(15) << "Context\n";
        std::cout << std::string(84, '-') << "\n";
        
        for (const auto& [name, symbol] : global_symbols) {
            std::cout << std::left << std::setw(20) << name
                      << std::setw(25) << symbol.type.to_string()
                      << std::setw(12) << (symbol.is_initialized ? "yes" : "no")
                      << std::setw(12) << (symbol.is_used ? "yes" : "no")
                      << std::setw(15) << symbol.context << "\n";
        }
    }
}

// Вывод таблицы функций
void SemanticAnalyzer::print_function_table() const {
    std::cout << "\n=== FUNCTION TABLE ===\n";
    
    if (functions.empty()) {
        std::cout << "No functions\n";
    } else {
        std::cout << "Functions (" << functions.size() << "):\n";
        std::cout << std::left << std::setw(20) << "Name" 
                  << std::setw(25) << "Return Type" 
                  << std::setw(12) << "Defined"
                  << std::setw(12) << "Called"
                  << std::setw(12) << "Params"
                  << "Line\n";
        std::cout << std::string(81, '-') << "\n";
        
        for (const auto& [name, func] : functions) {
            std::cout << std::left << std::setw(20) << name
                      << std::setw(25) << func.return_type.to_string()
                      << std::setw(12) << (func.is_defined ? "yes" : "no")
                      << std::setw(12) << (func.is_called ? "yes" : "no")
                      << std::setw(12) << func.parameters.size()
                      << func.declaration_line << "\n";
        }
    }
}

// Вывод сводки по типам
void SemanticAnalyzer::print_type_summary() const {
    std::cout << "\n=== TYPE SUMMARY ===\n";
    
    std::unordered_map<std::string, int> type_counts;
    for (const auto& [name, symbol] : global_symbols) {
        type_counts[symbol.type.name]++;
    }
    
    for (const auto& [type, count] : type_counts) {
        std::cout << type << ": " << count << " variables\n";
    }
}

// Генерация отчета
std::string SemanticAnalyzer::generate_report() const {
    std::stringstream report;
    
    report << "=== SEMANTIC ANALYSIS REPORT ===\n";
    report << "Errors: " << errors.size() << "\n";
    report << "Warnings: " << warnings.size() << "\n\n";
    
    if (!errors.empty()) {
        report << "ERRORS:\n";
        for (const auto& error : errors) {
            report << "  " << error << "\n";
        }
        report << "\n";
    }
    
    if (!warnings.empty()) {
        report << "WARNINGS:\n";
        for (const auto& warning : warnings) {
            report << "  " << warning << "\n";
        }
        report << "\n";
    }
    
    report << "STATISTICS:\n";
    report << "  Global symbols: " << global_symbols.size() << "\n";
    report << "  Functions: " << functions.size() << "\n";
    
    int used_vars = 0;
    int initialized_vars = 0;
    for (const auto& [name, symbol] : global_symbols) {
        if (symbol.is_used) used_vars++;
        if (symbol.is_initialized) initialized_vars++;
    }
    
    report << "  Used variables: " << used_vars << "/" << global_symbols.size() << "\n";
    report << "  Initialized variables: " << initialized_vars << "/" << global_symbols.size() << "\n";
    
    int called_funcs = 0;
    for (const auto& [name, func] : functions) {
        if (func.is_called) called_funcs++;
    }
    
    report << "  Called functions: " << called_funcs << "/" << functions.size() << "\n";
    
    return report.str();
}
} // namespace semantic