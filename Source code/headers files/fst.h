#ifndef FST_H
#define FST_H

#include "lexer.h"
#include <vector>
#include <memory>
#include <functional>

namespace fst {

struct FSTnode {
    short id;
    lexan::TokenType content;
    std::string value;  // Значение для более точного сопоставления (опционально)
    FSTnode* next;      // Следующий узел в цепочке
    FSTnode* alternative; // Альтернативный путь (например, для опциональных частей)
    bool is_optional;   // Является ли узел опциональным
    
    FSTnode(lexan::TokenType type, short num, const std::string& val = "", 
            bool optional = false) 
        : content(type), id(num), value(val), next(nullptr), 
          alternative(nullptr), is_optional(optional) {}
};

// Структура для представления правила FST
struct FSTRule {
    std::string name;
    FSTnode* start;
    int min_length;  // Минимальная длина последовательности
    int max_length;  // Максимальная длина (-1 для неограниченной)
    
    FSTRule(const std::string& n, FSTnode* s, int min = 1, int max = -1)
        : name(n), start(s), min_length(min), max_length(max) {}
};

// Основные функции
void initChains();
void cleanup();

// Функции для работы с FST
FSTnode* createChain(const std::vector<lexan::TokenType>& pattern,
                    const std::vector<std::string>& values = {});
bool matchPattern(FSTnode* pattern, const std::vector<lexan::Token>& tokens, 
                 size_t startPos, size_t& matchedLength);
bool matchRule(const FSTRule& rule, const std::vector<lexan::Token>& tokens, 
              size_t startPos, size_t& matchedLength);

// Поиск подходящих правил
std::vector<std::string> findMatchingRules(const std::vector<lexan::Token>& tokens, 
                                          size_t startPos);
FSTnode* findBestMatch(const std::vector<lexan::Token>& tokens, size_t startPos);

// Вспомогательные функции
void printChain(FSTnode* node, int depth = 0);
void printAllRules();

// Геттеры для правил
const std::vector<FSTRule>& getAllRules();
FSTRule* getRuleByName(const std::string& name);

// Функции для проверки специфических конструкций
bool isVariableDeclaration(const std::vector<lexan::Token>& tokens, size_t startPos);
bool isFunctionCall(const std::vector<lexan::Token>& tokens, size_t startPos);
bool isAssignment(const std::vector<lexan::Token>& tokens, size_t startPos);
bool isExpression(const std::vector<lexan::Token>& tokens, size_t startPos);
bool isTypeSpecifier(lexan::TokenType type);

} // namespace fst

#endif // FST_H