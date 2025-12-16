#include <precomph.h>
#include <stack>
#include <queue>
#include <memory>
#include <functional>
#include <algorithm>
#include <set>

namespace fst {

static std::vector<FSTRule> rules;
static short nodeCounter = 0;

// Вспомогательные функции
FSTnode* createNode(lexan::TokenType content, const std::string& value = "", bool optional = false) {
    nodeCounter++;
    return new FSTnode(content, nodeCounter, value, optional);
}

void deleteChain(FSTnode* node) {
    if (!node) return;
    
    // Используем BFS для удаления всех узлов (так как могут быть циклы из-за alternative)
    std::queue<FSTnode*> toDelete;
    std::set<FSTnode*> visited;  // Используем set вместо unordered_set для простоты
    
    toDelete.push(node);
    visited.insert(node);
    
    while (!toDelete.empty()) {
        FSTnode* current = toDelete.front();
        toDelete.pop();
        
        if (current->next && visited.find(current->next) == visited.end()) {
            visited.insert(current->next);
            toDelete.push(current->next);
        }
        
        if (current->alternative && visited.find(current->alternative) == visited.end()) {
            visited.insert(current->alternative);
            toDelete.push(current->alternative);
        }
        
        delete current;
    }
}

void printChain(FSTnode* node, int depth) {
    if (!node) return;
    
    for (int i = 0; i < depth; i++) {
        std::cout << "  ";
    }
    
    std::cout << "[" << node->id << "] ";
    std::cout << lexan::Lexer::token_type_to_string(node->content);
    
    if (!node->value.empty()) {
        std::cout << " (\"" << node->value << "\")";
    }
    
    if (node->is_optional) {
        std::cout << " [OPTIONAL]";
    }
    
    std::cout << std::endl;
    
    if (node->next) {
        for (int i = 0; i < depth; i++) {
            std::cout << "  ";
        }
        std::cout << "  -> next:" << std::endl;
        printChain(node->next, depth + 1);
    }
    
    if (node->alternative) {
        for (int i = 0; i < depth; i++) {
            std::cout << "  ";
        }
        std::cout << "  -> alternative:" << std::endl;
        printChain(node->alternative, depth + 1);
    }
}

FSTnode* createChain(const std::vector<lexan::TokenType>& pattern,
                    const std::vector<std::string>& values) {
    if (pattern.empty()) return nullptr;
    
    FSTnode* start = nullptr;
    FSTnode* current = nullptr;
    
    for (size_t i = 0; i < pattern.size(); i++) {
        std::string value = (i < values.size()) ? values[i] : "";
        FSTnode* newNode = createNode(pattern[i], value);
        
        if (!start) {
            start = newNode;
            current = newNode;
        } else {
            current->next = newNode;
            current = newNode;
        }
    }
    
    return start;
}

// Создание специфических цепочек для языка
FSTnode* createVariableDeclChain() {
    // est type ident [= expr] ;
    std::vector<lexan::TokenType> pattern = {
        lexan::TK_EST,
        lexan::TK_INT,  // Заглушка для типа
        lexan::TK_IDENTIFIER,
        lexan::TK_ASSIGN,  // Опциональная часть
        lexan::TK_IDENTIFIER, // Выражение
        lexan::TK_SEMICOLON
    };
    
    FSTnode* chain = createChain(pattern);
    
    // Делаем присваивание и выражение опциональными
    FSTnode* assignNode = chain->next->next->next; // TK_ASSIGN
    assignNode->is_optional = true;
    assignNode->alternative = assignNode->next->next; // Переход к TK_SEMICOLON (assign->next->next)
    
    return chain;
}

FSTnode* createFunctionCallChain() {
    // ident ( [args] ) ;
    std::vector<lexan::TokenType> pattern = {
        lexan::TK_IDENTIFIER,
        lexan::TK_LPAREN,
        lexan::TK_IDENTIFIER, // Аргументы
        lexan::TK_RPAREN,
        lexan::TK_SEMICOLON
    };
    
    FSTnode* chain = createChain(pattern);
    
    // Делаем аргументы опциональными
    FSTnode* argNode = chain->next->next; // TK_IDENTIFIER (аргумент)
    argNode->is_optional = true;
    argNode->alternative = argNode->next; // Переход к TK_RPAREN
    
    return chain;
}

FSTnode* createBuiltinFunctionCallChain() {
    // builtin ( [args] ) ;
    std::vector<lexan::TokenType> pattern = {
        lexan::TK_BUILTIN_PROCLAIM, // Любой builtin
        lexan::TK_LPAREN,
        lexan::TK_IDENTIFIER, // Аргумент
        lexan::TK_RPAREN,
        lexan::TK_SEMICOLON
    };
    
    FSTnode* chain = createChain(pattern);
    
    // Делаем аргументы опциональными
    FSTnode* argNode = chain->next->next; // TK_IDENTIFIER (аргумент)
    argNode->is_optional = true;
    argNode->alternative = argNode->next; // Переход к TK_RPAREN
    
    return chain;
}

FSTnode* createAssignmentChain() {
    // ident = expr ;
    std::vector<lexan::TokenType> pattern = {
        lexan::TK_IDENTIFIER,
        lexan::TK_ASSIGN,
        lexan::TK_IDENTIFIER, // Выражение
        lexan::TK_SEMICOLON
    };
    
    return createChain(pattern);
}

FSTnode* createDoWhileChain() {
    // do { statements } while ( expr ) ;
    std::vector<lexan::TokenType> pattern = {
        lexan::TK_DO,
        lexan::TK_LBRACE,
        lexan::TK_IDENTIFIER, // Statement
        lexan::TK_RBRACE,
        lexan::TK_WHILE,
        lexan::TK_LPAREN,
        lexan::TK_IDENTIFIER, // Условие
        lexan::TK_RPAREN,
        lexan::TK_SEMICOLON
    };
    
    FSTnode* chain = createChain(pattern);
    
    // Делаем statement опциональным
    FSTnode* stmtNode = chain->next->next; // TK_IDENTIFIER (statement)
    stmtNode->is_optional = true;
    stmtNode->alternative = stmtNode->next; // Переход к TK_RBRACE
    
    return chain;
}

FSTnode* createFunctionDeclChain() {
    // type algo ident ( params ) { statements }
    std::vector<lexan::TokenType> pattern = {
        lexan::TK_INT, // Тип возвращаемого значения
        lexan::TK_ALGO,
        lexan::TK_IDENTIFIER,
        lexan::TK_LPAREN,
        lexan::TK_INT, // Параметр тип
        lexan::TK_IDENTIFIER, // Параметр имя
        lexan::TK_RPAREN,
        lexan::TK_LBRACE,
        lexan::TK_IDENTIFIER, // Statement
        lexan::TK_RBRACE
    };
    
    FSTnode* chain = createChain(pattern);
    
    // Делаем параметры опциональными
    FSTnode* paramTypeNode = chain->next->next->next->next; // TK_INT (тип параметра)
    paramTypeNode->is_optional = true;
    // Если пропускаем параметры, переходим сразу к TK_RPAREN
    paramTypeNode->alternative = paramTypeNode->next->next; // TK_RPAREN (пропускаем TK_IDENTIFIER параметра)
    
    return chain;
}

FSTnode* createProcedureDeclChain() {
    // procedure algo ident ( ) { statements }
    std::vector<lexan::TokenType> pattern = {
        lexan::TK_PROCEDURE,
        lexan::TK_ALGO,
        lexan::TK_IDENTIFIER,
        lexan::TK_LPAREN,
        lexan::TK_RPAREN,
        lexan::TK_LBRACE,
        lexan::TK_IDENTIFIER, // Statement
        lexan::TK_RBRACE
    };
    
    FSTnode* chain = createChain(pattern);
    
    return chain;
}

FSTnode* createExpressionChain() {
    // expr op expr
    std::vector<lexan::TokenType> pattern = {
        lexan::TK_IDENTIFIER,
        lexan::TK_PLUS, // Оператор
        lexan::TK_IDENTIFIER
    };
    
    FSTnode* chain = createChain(pattern);
    
    // Создаем альтернативные операторы правильно
    FSTnode* opNode = chain->next; // TK_PLUS
    FSTnode* afterOp = opNode->next; // TK_IDENTIFIER после оператора
    
    // Создаем узлы для альтернативных операторов
    FSTnode* minusNode = createNode(lexan::TK_MINUS, "-");
    minusNode->next = afterOp;
    
    FSTnode* multNode = createNode(lexan::TK_MULT, "*");
    multNode->next = afterOp;
    
    FSTnode* divNode = createNode(lexan::TK_DIV, "/");
    divNode->next = afterOp;
    
    FSTnode* powNode = createNode(lexan::TK_POW, "^");
    powNode->next = afterOp;
    
    // Связываем альтернативные операторы
    opNode->alternative = minusNode;
    minusNode->alternative = multNode;
    multNode->alternative = divNode;
    divNode->alternative = powNode;
    
    return chain;
}

FSTnode* createReturnChain() {
    // return [expr] ;
    std::vector<lexan::TokenType> pattern = {
        lexan::TK_RETURN,
        lexan::TK_IDENTIFIER, // Выражение
        lexan::TK_SEMICOLON
    };
    
    FSTnode* chain = createChain(pattern);
    
    // Делаем выражение опциональным
    FSTnode* exprNode = chain->next; // TK_IDENTIFIER
    exprNode->is_optional = true;
    exprNode->alternative = exprNode->next; // Переход к TK_SEMICOLON
    
    return chain;
}

FSTnode* createMainChain() {
    // ces { statements }
    std::vector<lexan::TokenType> pattern = {
        lexan::TK_CES,
        lexan::TK_LBRACE,
        lexan::TK_IDENTIFIER, // Statement
        lexan::TK_RBRACE
    };
    
    FSTnode* chain = createChain(pattern);
    
    return chain;
}

FSTnode* createStringAssignmentChain() {
    // ident = "string" ;
    std::vector<lexan::TokenType> pattern = {
        lexan::TK_IDENTIFIER,
        lexan::TK_ASSIGN,
        lexan::TK_STRING_LIT,
        lexan::TK_SEMICOLON
    };
    
    return createChain(pattern);
}

FSTnode* createNumberAssignmentChain() {
    // ident = number ;
    std::vector<lexan::TokenType> pattern = {
        lexan::TK_IDENTIFIER,
        lexan::TK_ASSIGN,
        lexan::TK_NUMBER,
        lexan::TK_SEMICOLON
    };
    
    return createChain(pattern);
}

// Основная функция сопоставления
bool matchPattern(FSTnode* pattern, const std::vector<lexan::Token>& tokens, 
                 size_t startPos, size_t& matchedLength) {
    if (!pattern || startPos >= tokens.size()) {
        return false;
    }
    
    // Простое сопоставление для линейных цепочек
    FSTnode* current = pattern;
    size_t pos = startPos;
    size_t minMatches = 0;
    
    while (current && pos < tokens.size()) {
        // Специальная обработка для типов
        if (current->content == lexan::TK_INT) {
            // TK_INT может представлять любой тип
            if (isTypeSpecifier(tokens[pos].type)) {
                // Тип совпал
                current = current->next;
                pos++;
                minMatches++;
                continue;
            }
        }
        
        // Специальная обработка для builtin функций
        if (current->content == lexan::TK_BUILTIN_PROCLAIM) {
            if (tokens[pos].type >= lexan::TK_BUILTIN_PROCLAIM && 
                tokens[pos].type <= lexan::TK_BUILTIN_SUM4) {
                current = current->next;
                pos++;
                minMatches++;
                continue;
            }
        }
        
        // Обычное сопоставление
        if (current->content == tokens[pos].type) {
            current = current->next;
            pos++;
            minMatches++;
        } else if (current->is_optional) {
            // Пропускаем опциональный узел
            current = current->next;
        } else if (current->alternative) {
            // Пробуем альтернативный путь
            current = current->alternative;
        } else {
            // Не совпало и нет альтернатив
            break;
        }
    }
    
    // Если прошли всю цепочку
    if (!current) {
        matchedLength = pos - startPos;
        return true;
    }
    
    // Если остались только опциональные узлы
    while (current && current->is_optional) {
        current = current->next;
    }
    
    if (!current) {
        matchedLength = pos - startPos;
        return true;
    }
    
    return false;
}

bool matchRule(const FSTRule& rule, const std::vector<lexan::Token>& tokens, 
              size_t startPos, size_t& matchedLength) {
    size_t length = 0;
    bool matched = matchPattern(rule.start, tokens, startPos, length);
    
    if (matched) {
        // Проверяем ограничения по длине
        if (rule.min_length > 0 && length < (size_t)rule.min_length) {
            return false;
        }
        if (rule.max_length > 0 && length > (size_t)rule.max_length) {
            return false;
        }
        matchedLength = length;
        return true;
    }
    
    return false;
}

std::vector<std::string> findMatchingRules(const std::vector<lexan::Token>& tokens, 
                                          size_t startPos) {
    std::vector<std::string> matches;
    
    for (const auto& rule : rules) {
        size_t matchedLength = 0;
        if (matchRule(rule, tokens, startPos, matchedLength)) {
            matches.push_back(rule.name + " (length: " + std::to_string(matchedLength) + ")");
        }
    }
    
    return matches;
}

FSTnode* findBestMatch(const std::vector<lexan::Token>& tokens, size_t startPos) {
    size_t bestLength = 0;
    FSTnode* bestMatch = nullptr;
    
    for (const auto& rule : rules) {
        size_t matchedLength = 0;
        if (matchRule(rule, tokens, startPos, matchedLength)) {
            if (matchedLength > bestLength) {
                bestLength = matchedLength;
                bestMatch = rule.start;
            }
        }
    }
    
    return bestMatch;
}

bool isVariableDeclaration(const std::vector<lexan::Token>& tokens, size_t startPos) {
    if (startPos >= tokens.size()) return false;
    
    // Проверяем, начинается ли с 'est'
    if (tokens[startPos].type != lexan::TK_EST) return false;
    
    // Проверяем наличие типа
    if (startPos + 1 >= tokens.size()) return false;
    if (!isTypeSpecifier(tokens[startPos + 1].type)) return false;
    
    // Проверяем наличие идентификатора
    if (startPos + 2 >= tokens.size()) return false;
    if (tokens[startPos + 2].type != lexan::TK_IDENTIFIER) return false;
    
    return true;
}

bool isFunctionCall(const std::vector<lexan::Token>& tokens, size_t startPos) {
    if (startPos >= tokens.size()) return false;
    
    // Может быть идентификатором или встроенной функцией
    bool isIdent = tokens[startPos].type == lexan::TK_IDENTIFIER;
    bool isBuiltin = tokens[startPos].type >= lexan::TK_BUILTIN_PROCLAIM && 
                     tokens[startPos].type <= lexan::TK_BUILTIN_SUM4;
    
    if (!isIdent && !isBuiltin) return false;
    
    // Проверяем наличие '(' после идентификатора
    if (startPos + 1 >= tokens.size()) return false;
    return tokens[startPos + 1].type == lexan::TK_LPAREN;
}

bool isAssignment(const std::vector<lexan::Token>& tokens, size_t startPos) {
    if (startPos >= tokens.size()) return false;
    
    // Должен начинаться с идентификатора
    if (tokens[startPos].type != lexan::TK_IDENTIFIER) return false;
    
    // Проверяем наличие оператора присваивания
    if (startPos + 1 >= tokens.size()) return false;
    
    lexan::TokenType nextType = tokens[startPos + 1].type;
    return nextType == lexan::TK_ASSIGN || 
           nextType == lexan::TK_PLUS_ASSIGN ||
           nextType == lexan::TK_MINUS_ASSIGN ||
           nextType == lexan::TK_MULT_ASSIGN ||
           nextType == lexan::TK_DIV_ASSIGN;
}

bool isExpression(const std::vector<lexan::Token>& tokens, size_t startPos) {
    if (startPos >= tokens.size()) return false;
    
    // Простое выражение: идентификатор или литерал
    lexan::TokenType firstType = tokens[startPos].type;
    if (firstType == lexan::TK_IDENTIFIER || 
        firstType == lexan::TK_NUMBER ||
        firstType == lexan::TK_STRING_LIT ||
        firstType == lexan::TK_CHAR_LIT ||
        firstType == lexan::TK_TRUE ||
        firstType == lexan::TK_FALSE) {
        return true;
    }
    
    // Унарный оператор
    if (firstType == lexan::TK_PLUS || firstType == lexan::TK_MINUS ||
        firstType == lexan::TK_NOT || firstType == lexan::TK_BIT_NOT) {
        if (startPos + 1 < tokens.size()) {
            return isExpression(tokens, startPos + 1);
        }
    }
    
    // Выражение в скобках
    if (firstType == lexan::TK_LPAREN) {
        // Ищем закрывающую скобку
        int parenCount = 1;
        for (size_t i = startPos + 1; i < tokens.size(); i++) {
            if (tokens[i].type == lexan::TK_LPAREN) parenCount++;
            if (tokens[i].type == lexan::TK_RPAREN) {
                parenCount--;
                if (parenCount == 0) {
                    return true;
                }
            }
        }
    }
    
    return false;
}

bool isTypeSpecifier(lexan::TokenType type) {
    return type == lexan::TK_INT || 
           type == lexan::TK_STRING ||
           type == lexan::TK_BOOL ||
           type == lexan::TK_TIME_T ||
           type == lexan::TK_UNSIGNED ||
           type == lexan::TK_SYMB;
}

void initChains() {
    std::cout << "[FST] Initializing rules..." << std::endl;
    
    // Очищаем существующие правила
    for (auto& rule : rules) {
        deleteChain(rule.start);
    }
    rules.clear();
    nodeCounter = 0;
    
    try {
        std::cout << "[FST] Creating rules..." << std::endl;
        
        // Основные правила
        rules.push_back(FSTRule("variable_declaration", createVariableDeclChain(), 3, 10));
        rules.push_back(FSTRule("function_call", createFunctionCallChain(), 4, 50));
        rules.push_back(FSTRule("builtin_function_call", createBuiltinFunctionCallChain(), 4, 50));
        rules.push_back(FSTRule("assignment", createAssignmentChain(), 4, 50));
        rules.push_back(FSTRule("do_while", createDoWhileChain(), 8, 100));
        rules.push_back(FSTRule("function_declaration", createFunctionDeclChain(), 8, 100));
        rules.push_back(FSTRule("procedure_declaration", createProcedureDeclChain(), 7, 50));
        rules.push_back(FSTRule("expression", createExpressionChain(), 3, 50));
        rules.push_back(FSTRule("return_statement", createReturnChain(), 2, 10));
        rules.push_back(FSTRule("main_block", createMainChain(), 3, 100));
        rules.push_back(FSTRule("string_assignment", createStringAssignmentChain(), 4, 10));
        rules.push_back(FSTRule("number_assignment", createNumberAssignmentChain(), 4, 10));
        rules.push_back(FSTRule("complex_expression", createExpressionChain(), 3, 50));
        
        // Специальные правила для вашего кода
        rules.push_back(FSTRule("unsigned_int_decl", createChain({
            lexan::TK_EST,
            lexan::TK_UNSIGNED,
            lexan::TK_INT,
            lexan::TK_IDENTIFIER,
            lexan::TK_SEMICOLON
        }), 5, 5));
        
        rules.push_back(FSTRule("bool_decl", createChain({
            lexan::TK_EST,
            lexan::TK_BOOL,
            lexan::TK_IDENTIFIER,
            lexan::TK_SEMICOLON
        }), 4, 4));
        
        rules.push_back(FSTRule("time_t_decl", createChain({
            lexan::TK_EST,
            lexan::TK_TIME_T,
            lexan::TK_IDENTIFIER,
            lexan::TK_SEMICOLON
        }), 4, 4));
        
        rules.push_back(FSTRule("symb_decl", createChain({
            lexan::TK_EST,
            lexan::TK_SYMB,
            lexan::TK_IDENTIFIER,
            lexan::TK_SEMICOLON
        }), 4, 4));
        
        std::cout << "[FST] Created " << rules.size() << " rules" << std::endl;
        std::cout << "[FST] Created " << nodeCounter << " nodes" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "[FST] Error during initialization: " << e.what() << std::endl;
        cleanup();
        throw;
    }
}

void printAllRules() {
    std::cout << "\n[FST] Rules:" << std::endl;
    for (size_t i = 0; i < rules.size(); i++) {
        std::cout << "\nRule " << i << ": " << rules[i].name 
                  << " (min: " << rules[i].min_length 
                  << ", max: " << (rules[i].max_length == -1 ? "unlimited" : std::to_string(rules[i].max_length)) 
                  << ")" << std::endl;
        printChain(rules[i].start, 1);
    }
}

const std::vector<FSTRule>& getAllRules() {
    return rules;
}

FSTRule* getRuleByName(const std::string& name) {
    for (auto& rule : rules) {
        if (rule.name == name) {
            return &rule;
        }
    }
    return nullptr;
}

void cleanup() {
    std::cout << "[FST] Cleaning up..." << std::endl;
    
    for (auto& rule : rules) {
        deleteChain(rule.start);
    }
    rules.clear();
    nodeCounter = 0;
    
    std::cout << "[FST] Cleanup completed" << std::endl;
}

FSTnode* createComplexFunctionDeclChain() {
    // type algo ident ( [params] ) { [statements] }
    // Параметры: type ident [, type ident]*
    std::vector<lexan::TokenType> pattern = {
        lexan::TK_INT,  // Тип возврата
        lexan::TK_ALGO,
        lexan::TK_IDENTIFIER,
        lexan::TK_LPAREN,
        lexan::TK_INT,  // Первый параметр тип
        lexan::TK_IDENTIFIER, // Первый параметр имя
        lexan::TK_RPAREN,
        lexan::TK_LBRACE,
        lexan::TK_RETURN, // Тело функции
        lexan::TK_IDENTIFIER,
        lexan::TK_PLUS,
        lexan::TK_IDENTIFIER,
        lexan::TK_SEMICOLON,
        lexan::TK_RBRACE
    };
    
    FSTnode* chain = createChain(pattern);
    
    // Делаем параметры опциональными
    FSTnode* paramTypeNode = chain->next->next->next->next; // TK_INT первого параметра
    paramTypeNode->is_optional = true;
    
    // Добавляем возможность нескольких параметров
    FSTnode* afterFirstParam = paramTypeNode->next->next; // После первого параметра
    FSTnode* moreParams = createChain({
        lexan::TK_COMMA,
        lexan::TK_INT,
        lexan::TK_IDENTIFIER
    });
    afterFirstParam->alternative = moreParams;
    moreParams->next = afterFirstParam; // Позволяет повторяться
    
    // Делаем тело функции более гибким
    FSTnode* returnStmt = chain->next->next->next->next->next->next->next; // TK_RETURN
    returnStmt->is_optional = true;
    returnStmt->alternative = chain->next->next->next->next->next->next->next->next->next->next->next; // Переход к RBRACE
    
    return chain;
}

FSTnode* createComplexFunctionCallChain() {
    // ident ( [expr [, expr]*] ) ;
    std::vector<lexan::TokenType> pattern = {
        lexan::TK_IDENTIFIER,
        lexan::TK_LPAREN,
        lexan::TK_IDENTIFIER, // Первый аргумент
        lexan::TK_RPAREN,
        lexan::TK_SEMICOLON
    };
    
    FSTnode* chain = createChain(pattern);
    
    // Делаем аргументы опциональными
    FSTnode* firstArg = chain->next->next; // TK_IDENTIFIER (первый аргумент)
    firstArg->is_optional = true;
    firstArg->alternative = firstArg->next; // Переход к TK_RPAREN
    
    // Добавляем возможность нескольких аргументов
    FSTnode* afterFirstArg = firstArg->next; // TK_RPAREN после первого аргумента
    FSTnode* moreArgs = createChain({
        lexan::TK_COMMA,
        lexan::TK_IDENTIFIER
    });
    firstArg->next = moreArgs;
    moreArgs->next = afterFirstArg;
    
    return chain;
}

FSTnode* createComplexAssignmentChain() {
    // ident = expr ;
    // Где expr может быть сложным: ident + string + string
    std::vector<lexan::TokenType> pattern = {
        lexan::TK_IDENTIFIER,
        lexan::TK_ASSIGN,
        lexan::TK_STRING_LIT, // Начало выражения
        lexan::TK_PLUS,
        lexan::TK_STRING_LIT,
        lexan::TK_SEMICOLON
    };
    
    FSTnode* chain = createChain(pattern);
    
    // Делаем выражение более гибким
    FSTnode* stringLit1 = chain->next->next; // Первая строка
    stringLit1->is_optional = true;
    stringLit1->alternative = createNode(lexan::TK_IDENTIFIER, "", true);
    stringLit1->alternative->alternative = createNode(lexan::TK_NUMBER, "", true);
    
    // Делаем остальную часть выражения опциональной
    FSTnode* plusOp = stringLit1->next; // TK_PLUS
    plusOp->is_optional = true;
    plusOp->alternative = plusOp->next->next; // Переход к TK_SEMICOLON (пропуская TK_STRING_LIT)
    
    return chain;
}

FSTnode* createComplexDoWhileChain() {
    // do { [statements] } while ( expr ) ;
    std::vector<lexan::TokenType> pattern = {
        lexan::TK_DO,
        lexan::TK_LBRACE,
        lexan::TK_IDENTIFIER,
        lexan::TK_RBRACE,
        lexan::TK_WHILE,
        lexan::TK_LPAREN,
        lexan::TK_IDENTIFIER,
        lexan::TK_RPAREN,
        lexan::TK_SEMICOLON
    };
    
    FSTnode* chain = createChain(pattern);
    
    // Делаем statements опциональными и множественными
    FSTnode* stmtNode = chain->next->next; // TK_IDENTIFIER (statement)
    stmtNode->is_optional = true;
    
    // Позволяем несколько statements
    FSTnode* moreStmts = createChain({
        lexan::TK_SEMICOLON,
        lexan::TK_IDENTIFIER
    });
    stmtNode->next = moreStmts;
    moreStmts->next = stmtNode; // Рекурсия для нескольких statements
    
    // Условие может быть сложным
    FSTnode* condition = chain->next->next->next->next->next->next->next; // TK_IDENTIFIER в условии
    condition->alternative = createChain({
        lexan::TK_IDENTIFIER,
        lexan::TK_LT,
        lexan::TK_NUMBER
    })->next;
    
    return chain;
}

} // namespace fst