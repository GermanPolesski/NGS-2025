#include "rpnconverter.h"
#include <iostream>

namespace rpn {

RPNConverter::RPNConverter() {
    reset();
}

void RPNConverter::reset() {
    output.str("");
    tokens.clear();
}

bool RPNConverter::is_operator(const std::string& token) {
    return token == "+" || token == "-" || token == "*" || token == "/" || 
           token == "%" || token == "^" || token == "==" || token == "!=" ||
           token == "<" || token == ">" || token == "<=" || token == ">=" ||
           token == "&&" || token == "||";
}

// Для RPN нам не нужен стек операторов - порядок определяется позицией
void RPNConverter::process_expression(parser::ASTNode* node) {
    if (!node) return;
    
    switch (node->type) {
        case parser::ASTNode::Type::BINARY_OP:
            process_binary_op(node);
            break;
        case parser::ASTNode::Type::UNARY_OP:
            process_unary_op(node);
            break;
        case parser::ASTNode::Type::FUNCTION_CALL:
            process_function_call(node);
            break;
        case parser::ASTNode::Type::IDENTIFIER:
            process_identifier(node);
            break;
        case parser::ASTNode::Type::LITERAL:
            process_literal(node);
            break;
        default:
            // Рекурсивно обрабатываем детей
            for (auto child : node->children) {
                process_expression(child);
            }
            break;
    }
}

void RPNConverter::process_binary_op(parser::ASTNode* node) {
    // Сначала левый операнд
    if (node->children.size() >= 1) {
        process_expression(node->children[0]);
    }
    
    // Затем правый операнд
    if (node->children.size() >= 2) {
        process_expression(node->children[1]);
    }
    
    // Оператор (после операндов - это и есть postfix)
    std::string op = convert_operator(node->value);
    tokens.push_back(op);
}

void RPNConverter::process_unary_op(parser::ASTNode* node) {
    // Операнд
    if (node->children.size() >= 1) {
        process_expression(node->children[0]);
    }
    
    // Унарный оператор (после операнда - постфиксная запись)
    std::string op = convert_operator(node->value);
    tokens.push_back(op);
}

void RPNConverter::process_function_call(parser::ASTNode* node) {
    // Сначала обрабатываем все аргументы (если есть)
    if (node->children.size() > 0) {
        auto args_node = node->children[0];
        if (args_node->type == parser::ASTNode::Type::ARG_LIST) {
            for (auto arg : args_node->children) {
                process_expression(arg);
            }
        }
    }
    
    // Затем имя функции (после аргументов - постфиксная запись)
    // Для встроенных функций используем их реальные имена
    std::string func_name = node->value;
    tokens.push_back(func_name);
}

void RPNConverter::process_identifier(parser::ASTNode* node) {
    tokens.push_back(node->value);
}

void RPNConverter::process_literal(parser::ASTNode* node) {
    std::string value = node->value;
    
    // Для строковых литералов добавляем кавычки
    if (node->token.type == lexan::TK_STRING_LIT) {
        value = "\"" + value + "\"";
    } else if (node->token.type == lexan::TK_CHAR_LIT) {
        value = "'" + value + "'";
    } else if (node->token.type == lexan::TK_TRUE) {
        value = "true";
    } else if (node->token.type == lexan::TK_FALSE) {
        value = "false";
    }
    
    tokens.push_back(value);
}

std::string RPNConverter::convert_operator(const std::string& ast_op) {
    // Преобразуем AST имена операторов в математические символы
    if (ast_op == "PLUS") return "+";
    else if (ast_op == "MINUS") return "-";
    else if (ast_op == "MULT") return "*";
    else if (ast_op == "DIV") return "/";
    else if (ast_op == "MOD") return "%";
    else if (ast_op == "POW") return "^";
    else if (ast_op == "EQ") return "==";
    else if (ast_op == "NE") return "!=";
    else if (ast_op == "LT") return "<";
    else if (ast_op == "GT") return ">";
    else if (ast_op == "LE") return "<=";
    else if (ast_op == "GE") return ">=";
    else if (ast_op == "AND") return "&&";
    else if (ast_op == "OR") return "||";
    else if (ast_op == "NOT") return "!";
    else if (ast_op == "BIT_NOT") return "~";
    
    return ast_op; // Если оператор не найден, возвращаем как есть
}

std::string RPNConverter::convert_expression(parser::ASTNode* expr_node) {
    reset();
    process_expression(expr_node);
    
    // Собираем все токены в строку, разделяя пробелами
    std::stringstream result;
    for (size_t i = 0; i < tokens.size(); i++) {
        if (i > 0) result << " ";
        result << tokens[i];
    }
    
    return result.str();
}

void RPNConverter::find_and_convert_expressions(parser::ASTNode* node, 
                                               std::stringstream& result, 
                                               int depth) {
    if (!node) return;
    
    // Проверяем, является ли узел выражением
    if (node->type == parser::ASTNode::Type::BINARY_OP ||
        node->type == parser::ASTNode::Type::UNARY_OP ||
        node->type == parser::ASTNode::Type::FUNCTION_CALL) {
        
        // Добавляем отступ
        for (int i = 0; i < depth; i++) result << "  ";
        
        // Преобразуем выражение в чистую RPN
        std::string rpn = convert_expression(node);
        
        // Добавляем информацию о строке
        result << "Line " << node->token.line << ": " << rpn << "\n";
    }
    
    // Рекурсивно обрабатываем детей
    for (auto child : node->children) {
        find_and_convert_expressions(child, result, depth + 1);
    }
}

std::string RPNConverter::convert_program(parser::ASTNode* program_node) {
    if (!program_node) return "";
    
    std::stringstream result;
    result << "=== PURE REVERSE POLISH NOTATION (POSTFIX) ===\n\n";
    
    // Обходим все узлы программы
    for (auto child : program_node->children) {
        if (child->type == parser::ASTNode::Type::FUNCTION_DECL ||
            child->type == parser::ASTNode::Type::PROCEDURE_DECL) {
            
            result << "Function: " << child->value << "\n";
            
            // Ищем выражения в теле функции
            for (auto func_child : child->children) {
                if (func_child->type == parser::ASTNode::Type::BLOCK) {
                    find_and_convert_expressions(func_child, result);
                }
            }
            result << "\n";
        } else if (child->type == parser::ASTNode::Type::BLOCK && 
                   child->value == "ces_block") {
            result << "Main block:\n";
            find_and_convert_expressions(child, result);
            result << "\n";
        }
    }
    
    return result.str();
}

} // namespace rpn