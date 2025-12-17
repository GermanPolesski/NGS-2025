#include "codegen.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <sys/stat.h>
#include <unistd.h>  // Для getpid()

namespace codegen {

CodeGenerator::CodeGenerator() : temp_var_counter(0), semantic_analyzer(nullptr) {
    // Initialize type conversions
    type_conversions["int"] = "Number";
    type_conversions["unsigned int"] = "Number";
    type_conversions["bool"] = "Boolean";
    type_conversions["string"] = "String";
    type_conversions["time_t"] = "Number";
    type_conversions["symb"] = "String";
}

void CodeGenerator::indent() {
    current_indent += "    ";
    indent_stack.push_back("    ");
}

void CodeGenerator::dedent() {
    if (!indent_stack.empty()) {
        indent_stack.pop_back();
        current_indent.clear();
        for (const auto& indent : indent_stack) {
            current_indent += indent;
        }
    }
}

void CodeGenerator::add_line(const std::string& line) {
    code << current_indent << line << "\n";
}

std::string CodeGenerator::escape_string(const std::string& str) {
    std::string result;
    for (char c : str) {
        switch (c) {
            case '\\': result += "\\\\"; break;
            case '\"': result += "\\\""; break;
            case '\'': result += "\\'"; break;
            case '\n': result += "\\n"; break;
            case '\t': result += "\\t"; break;
            case '\r': result += "\\r"; break;
            default: result += c; break;
        }
    }
    return result;
}

std::string CodeGenerator::generate_temp_var() {
    return "__temp" + std::to_string(temp_var_counter++);
}

std::string CodeGenerator::get_js_type(const std::string& type) {
    auto it = type_conversions.find(type);
    if (it != type_conversions.end()) {
        return it->second;
    }
    return "Object";
}

std::string CodeGenerator::generate(parser::ASTNode* ast, semantic::SemanticAnalyzer* analyzer) {
    this->semantic_analyzer = analyzer;
    code.str("");
    current_indent = "";
    indent_stack.clear();
    temp_var_counter = 0;
    
    // Добавляем JavaScript header
    code << "// Generated JavaScript code from custom language\n";
    code << "// Date: " << __DATE__ << " " << __TIME__ << "\n\n";
    
    // Добавляем хелпер-функции для встроенных операций
    code << "// Helper functions for built-in operations\n";
    
    // Исправляем __timeFled для возврата секунд
    code << "function __timeFled(startStr, endStr) {\n";
    code << "    function parseDate(str) {\n";
    code << "        if (typeof str !== 'string' || str.length !== 8) return new Date();\n";
    code << "        const day = parseInt(str.substring(0, 2), 10);\n";
    code << "        const month = parseInt(str.substring(2, 4), 10) - 1;\n";
    code << "        const year = parseInt(str.substring(4, 8), 10);\n";
    code << "        return new Date(year, month, day);\n";
    code << "    }\n";
    code << "    const start = parseDate(startStr);\n";
    code << "    const end = parseDate(endStr);\n";
    code << "    return Math.floor((end.getTime() - start.getTime()) / 1000); // Возвращаем секунды\n";
    code << "}\n\n";
    
    // Исправляем __unite для работы с любым количеством аргументов
    code << "function __unite(count, ...args) {\n";
    code << "    let result = '';\n";
    code << "    const limit = Math.min(count, args.length);\n";
    code << "    for (let i = 0; i < limit; i++) {\n";
    code << "        if (args[i] !== undefined && args[i] !== null) {\n";
    code << "            result += String(args[i]);\n";
    code << "        }\n";
    code << "    }\n";
    code << "    return result;\n";
    code << "}\n\n";
    
    code << "function __sum4(a, b, c, d) {\n";
    code << "    return Number(a) + Number(b) + Number(c) + Number(d);\n";
    code << "}\n\n";
    
    // Генерируем код программы
    generate_program(ast);
    
    return code.str();
}

void CodeGenerator::generate_program(parser::ASTNode* node) {
    if (!node) return;
    
    // Generate all functions and procedures first
    for (auto child : node->children) {
        if (child->type == parser::ASTNode::Type::FUNCTION_DECL) {
            generate_function_decl(child);
        } else if (child->type == parser::ASTNode::Type::PROCEDURE_DECL) {
            generate_procedure_decl(child);
        }
    }
    
    // Generate main execution block (ces block)
    for (auto child : node->children) {
        if (child->type == parser::ASTNode::Type::BLOCK && child->value == "ces_block") {
            code << "\n// Main execution block\n";
            code << "(function() {\n";
            indent();
            generate_block(child);
            dedent();
            code << "})();\n";
        }
    }
}

void CodeGenerator::generate_function_decl(parser::ASTNode* node) {
    if (!node || node->children.empty()) return;
    
    std::string func_name = node->value;
    
    // Генерируем сигнатуру функции
    code << "function " << func_name << "(";
    
    // Добавляем параметры
    if (node->children.size() > 1) {
        auto params_node = node->children[1];
        if (params_node->type == parser::ASTNode::Type::PARAM_LIST) {
            for (size_t i = 0; i < params_node->children.size(); i++) {
                if (i > 0) code << ", ";
                code << params_node->children[i]->value;
            }
        }
    }
    
    code << ") {\n";
    indent();
    
    // Исправляем: генерируем тело функции правильно
    if (node->children.size() > 2) {
        auto body_node = node->children[2];
        // Проверяем, что это действительно блок
        if (body_node->type == parser::ASTNode::Type::BLOCK) {
            // Генерируем содержимое блока
            for (auto child : body_node->children) {
                // Обрабатываем разные типы операторов в теле функции
                if (child->type == parser::ASTNode::Type::VARIABLE_DECL) {
                    generate_variable_decl(child);
                } else if (child->type == parser::ASTNode::Type::ASSIGNMENT) {
                    generate_assignment(child);
                } else if (child->type == parser::ASTNode::Type::FUNCTION_CALL) {
                    std::string call = generate_function_call(child);
                    add_line(call + ";");
                } else if (child->type == parser::ASTNode::Type::RETURN_STMT) {
                    generate_return(child);
                } else if (child->type == parser::ASTNode::Type::DO_WHILE_LOOP) {
                    generate_do_while(child);
                } else if (child->type == parser::ASTNode::Type::BLOCK) {
                    generate_block(child);
                } else {
                    // Попытка сгенерировать как выражение
                    std::string expr = generate_expression(child);
                    if (!expr.empty()) {
                        add_line(expr + ";");
                    }
                }
            }
        } else {
            // Если тело не блок, пытаемся сгенерировать как есть
            generate_expression(body_node);
        }
    }
    
    // НЕ добавляем автоматический return в конце функции
    // Функция должна возвращать значение только через явный return
    
    dedent();
    code << "}\n\n";
}

void CodeGenerator::generate_procedure_decl(parser::ASTNode* node) {
    if (!node) return;
    
    std::string proc_name = node->value;
    std::cout << "[DEBUG] Generating procedure: " << proc_name << std::endl;
    
    // Проверим структуру узла процедуры
    std::cout << "[DEBUG] Procedure has " << node->children.size() << " children\n";
    for (size_t i = 0; i < node->children.size(); i++) {
        std::cout << "[DEBUG] Child " << i << ": type = " 
                  << static_cast<int>(node->children[i]->type) 
                  << ", value = " << node->children[i]->value << std::endl;
    }
    
    // Генерируем сигнатуру процедуры (функция без возвращаемого значения в JS)
    code << "function " << proc_name << "(";
    
    // Добавляем параметры, если есть
    if (node->children.size() > 0) {
        auto params_node = node->children[0];
        if (params_node && params_node->type == parser::ASTNode::Type::PARAM_LIST) {
            for (size_t i = 0; i < params_node->children.size(); i++) {
                if (i > 0) code << ", ";
                code << params_node->children[i]->value;
            }
        }
    }
    
    code << ") {\n";
    indent();
    
    // Генерируем тело процедуры
    if (node->children.size() > 1) {
        auto body_node = node->children[1];
        if (body_node->type == parser::ASTNode::Type::BLOCK) {
            generate_block(body_node);
        }
    }
    
    dedent();
    code << "}\n\n";
}

void CodeGenerator::generate_variable_decl(parser::ASTNode* node) {
    if (!node || node->children.empty()) return;
    
    std::string var_name = node->value;
    std::string type_hint = "";
    
    // Get type information if available
    if (node->children[0]->type == parser::ASTNode::Type::TYPE_SPECIFIER) {
        std::string type = node->children[0]->value;
        if (type == "UNSIGNED INT") {
            type_hint = "/* unsigned int */ ";
        } else {
            type_hint = "/* " + type + " */ ";
        }
    }
    
    // Generate variable declaration
    if (node->children.size() > 1) {
        // With initialization
        std::string init_value = generate_expression(node->children[1]);
        add_line("let " + type_hint + var_name + " = " + init_value + ";");
    } else {
        // Without initialization
        add_line("let " + type_hint + var_name + ";");
    }
}

void CodeGenerator::generate_assignment(parser::ASTNode* node) {
    if (!node || node->children.size() < 2) return;
    
    std::string target = generate_expression(node->children[0]);
    std::string value = generate_expression(node->children[1]);
    std::string op = convert_operator(node->value);
    
    add_line(target + " " + op + " " + value + ";");
}

std::string CodeGenerator::generate_expression(parser::ASTNode* node) {
    if (!node) return "";
    
    switch (node->type) {
        case parser::ASTNode::Type::BINARY_OP:
            return generate_binary_op(node);
        case parser::ASTNode::Type::UNARY_OP:
            return generate_unary_op(node);
        case parser::ASTNode::Type::LITERAL:
            return generate_literal(node);
        case parser::ASTNode::Type::IDENTIFIER:
            return generate_identifier(node);
        case parser::ASTNode::Type::FUNCTION_CALL:
            return generate_function_call(node);
        default:
            return "/* unknown expression */";
    }
}

std::string CodeGenerator::generate_binary_op(parser::ASTNode* node) {
    if (!node || node->children.size() < 2) return "";
    
    std::string left = generate_expression(node->children[0]);
    std::string right = generate_expression(node->children[1]);
    std::string op = convert_operator(node->value);
    
    // Для оператора возведения в степень используем Math.pow
    if (node->value == "POW") {
        return "Math.pow(" + left + ", " + right + ")";
    }
    
    // Для остальных операторов
    return "(" + left + " " + op + " " + right + ")";
}

std::string CodeGenerator::convert_operator(const std::string& op) {
    if (op == "PLUS") return "+";
    else if (op == "MINUS") return "-";
    else if (op == "MULT") return "*";
    else if (op == "DIV") return "/";
    else if (op == "MOD") return "%";
    else if (op == "POW") return "^";
    else if (op == "EQ") return "===";
    else if (op == "NE") return "!==";
    else if (op == "LT") return "<";
    else if (op == "GT") return ">";
    else if (op == "LE") return "<=";
    else if (op == "GE") return ">=";
    else if (op == "AND") return "&&";
    else if (op == "OR") return "||";
    else if (op == "ASSIGN") return "=";
    else if (op == "PLUS_ASSIGN") return "+=";
    else if (op == "MINUS_ASSIGN") return "-=";
    else if (op == "MULT_ASSIGN") return "*=";
    else if (op == "DIV_ASSIGN") return "/=";
    return op;
}

std::string CodeGenerator::generate_unary_op(parser::ASTNode* node) {
    if (!node || node->children.empty()) return "";
    
    std::string operand = generate_expression(node->children[0]);
    std::string op = convert_operator(node->value);
    
    if (op == "-") {
        return "-" + operand;
    } else if (op == "!") {
        return "!" + operand;
    } else if (op == "~") {
        return "~" + operand;
    }
    
    return op + operand;
}

std::string CodeGenerator::generate_literal(parser::ASTNode* node) {
    if (!node) return "";
    
    if (node->token.type == lexan::TK_STRING_LIT) {
        return "\"" + escape_string(node->value) + "\"";
    } else if (node->token.type == lexan::TK_CHAR_LIT) {
        return "\"" + escape_string(node->value) + "\"";
    } else if (node->token.type == lexan::TK_NUMBER) {
        // Обрабатываем шестнадцатеричные числа
        if (node->token.is_hex) {
            std::string hex_str = node->value;
            // Удаляем префикс 0x если есть
            if (hex_str.find("0x") == 0 || hex_str.find("0X") == 0) {
                hex_str = hex_str.substr(2);
            }
            // Конвертируем шестнадцатеричную строку в число
            return "parseInt('" + hex_str + "', 16)";
        } 
        // Обрабатываем восьмеричные числа
        else if (node->token.is_octal) {
            std::string oct_str = node->value;
            // Удаляем префикс 0
            if (oct_str[0] == '0') {
                oct_str = oct_str.substr(1);
            }
            return "parseInt('" + oct_str + "', 8)";
        } 
        // Обычные числа
        else {
            return node->value;
        }
    } else if (node->token.type == lexan::TK_TRUE) {
        return "true";
    } else if (node->token.type == lexan::TK_FALSE) {
        return "false";
    }
    
    return node->value;
}

std::string CodeGenerator::generate_identifier(parser::ASTNode* node) {
    return node->value;
}

std::string CodeGenerator::generate_function_call(parser::ASTNode* node) {
    if (!node) return "";
    
    std::string func_name = node->value;
    
    // Обработка встроенных функций
    if (is_builtin_function(func_name)) {
        return handle_builtin_function(func_name, 
            node->children.empty() ? nullptr : node->children[0]);
    }
    
    // Для пользовательских функций проверяем, что у них есть правильное количество аргументов
    std::string call = func_name + "(";
    
    if (!node->children.empty()) {
        auto args_node = node->children[0];
        if (args_node->type == parser::ASTNode::Type::ARG_LIST) {
            for (size_t i = 0; i < args_node->children.size(); i++) {
                if (i > 0) call += ", ";
                call += generate_expression(args_node->children[i]);
            }
        }
    }
    
    call += ")";
    return call;
}

std::string CodeGenerator::handle_builtin_function(const std::string& name, 
                                                  parser::ASTNode* args_node) {
    if (name == "proclaim") {
        std::string args = "";
        if (args_node && args_node->type == parser::ASTNode::Type::ARG_LIST) {
            for (size_t i = 0; i < args_node->children.size(); i++) {
                if (i > 0) args += ", ";
                args += generate_expression(args_node->children[i]);
            }
        }
        return "console.log(" + args + ")";
    }
    else if (name == "to_str") {
        if (args_node && !args_node->children.empty()) {
            return "String(" + generate_expression(args_node->children[0]) + ")";
        }
        return "String()";
    }
    else if (name == "ThisVeryMoment") {
        return "Math.floor(Date.now() / 1000)"; // Возвращаем секунды
    }
    else if (name == "TimeFled") {
        if (args_node && args_node->children.size() >= 2) {
            std::string start = generate_expression(args_node->children[0]);
            std::string end = generate_expression(args_node->children[1]);
            return "__timeFled(" + start + ", " + end + ")";
        }
        return "0";
    }
    else if (name == "unite") {
        if (args_node && !args_node->children.empty()) {
            // Первый аргумент - количество символов
            if (args_node->children.size() >= 2) {
                std::string count = generate_expression(args_node->children[0]);
                std::string args = "";
                
                for (size_t i = 1; i < args_node->children.size(); i++) {
                    if (i > 1) args += ", ";
                    args += generate_expression(args_node->children[i]);
                }
                
                return "__unite(" + count + ", " + args + ")";
            }
        }
        return "''";
    }
    else if (name == "sum4") {
        if (args_node && args_node->children.size() >= 4) {
            std::string a = generate_expression(args_node->children[0]);
            std::string b = generate_expression(args_node->children[1]);
            std::string c = generate_expression(args_node->children[2]);
            std::string d = generate_expression(args_node->children[3]);
            return "__sum4(" + a + ", " + b + ", " + c + ", " + d + ")";
        }
        return "0";
    }
    
    return name + "()";
}

void CodeGenerator::generate_do_while(parser::ASTNode* node) {
    if (!node || node->children.size() < 2) return;
    
    add_line("do {");
    indent();
    
    // Генерируем тело цикла
    if (node->children[0]->type == parser::ASTNode::Type::BLOCK) {
        generate_block(node->children[0]);
    } else {
        // Одиночный statement
        std::string stmt = generate_expression(node->children[0]);
        if (!stmt.empty()) {
            add_line(stmt + ";");
        }
    }
    
    dedent();
    
    // Генерируем условие
    std::string condition = generate_expression(node->children[1]);
    add_line("} while (" + condition + ");");
}

void CodeGenerator::generate_return(parser::ASTNode* node) {
    if (!node) {
        add_line("return;");
        return;
    }
    
    if (node->children.empty()) {
        add_line("return;");
    } else {
        std::string value = generate_expression(node->children[0]);
        add_line("return " + value + ";");
    }
}

void CodeGenerator::generate_block(parser::ASTNode* node) {
    if (!node) return;
    
    for (auto child : node->children) {
        switch (child->type) {
            case parser::ASTNode::Type::VARIABLE_DECL:
                generate_variable_decl(child);
                break;
            case parser::ASTNode::Type::ASSIGNMENT:
                generate_assignment(child);
                break;
            case parser::ASTNode::Type::FUNCTION_CALL: {
                std::string call = generate_function_call(child);
                if (!call.empty()) {
                    add_line(call + ";");
                }
                break;
            }
            case parser::ASTNode::Type::DO_WHILE_LOOP:
                generate_do_while(child);
                break;
            case parser::ASTNode::Type::RETURN_STMT:
                generate_return(child);
                break;
            case parser::ASTNode::Type::BLOCK:
                generate_block(child);
                break;
            default:
                // Пробуем сгенерировать как выражение
                std::string expr = generate_expression(child);
                if (!expr.empty()) {
                    add_line(expr + ";");
                }
                break;
        }
    }
}

void CodeGenerator::save_to_file(const std::string& filename) {
    std::ofstream file(filename);
    if (file.is_open()) {
        file << code.str();
        file.close();
    } else {
        std::cerr << "Error: Could not save generated code to " << filename << std::endl;
    }
}

} // namespace codegen