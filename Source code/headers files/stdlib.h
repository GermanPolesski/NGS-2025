// stdlib.h
#ifndef STDLIB_H
#define STDLIB_H

#include <string>
#include <vector>
#include <unordered_map>
#include "parser.h"

namespace stdlib {

// Типы стандартных функций
enum class BuiltinType {
    PROCLAIM,      // Вывод в консоль
    TO_STR,        // Преобразование в строку
    TIME_FLED,     // Разница между датами
    THIS_VERY_MOMENT, // Текущее время
    UNITE,         // Объединение символов
    SUM4,          // Сумма 4 чисел
    MATH_FUNC,     // Математические функции
    STRING_FUNC,   // Строковые функции
    ARRAY_FUNC,    // Функции массивов
    TYPE_FUNC      // Функции проверки типов
};

// Информация о стандартной функции
struct BuiltinFunction {
    BuiltinType type;
    std::string name;
    std::string js_name;         // Имя в JavaScript
    int min_args;               // Минимальное число аргументов
    int max_args;               // Максимальное число аргументов (-1 для переменного)
    std::string return_type;    // Тип возвращаемого значения
    std::string description;    // Описание
    std::string js_implementation; // Реализация на JS (опционально)
    bool requires_import;       // Требует ли импорта библиотеки
};

class StandardLibrary {
private:
    std::unordered_map<std::string, BuiltinFunction> functions;
    
    void initializeFunctions();
    void addFunction(const BuiltinFunction& func);
    
public:
    StandardLibrary();
    
    // Проверка существования функции
    bool isBuiltin(const std::string& name) const;
    
    // Получение информации о функции
    const BuiltinFunction* getFunction(const std::string& name) const;
    
    // Получение всех функций
    const std::unordered_map<std::string, BuiltinFunction>& getAllFunctions() const;
    
    // Генерация кода импорта для JavaScript
    std::string generateJSImports() const;
    
    // Генерация полифилов для JavaScript (если нужны)
    std::string generateJSPolyfills() const;
    
    // Проверка аргументов функции
    bool validateArguments(const std::string& func_name, 
                          int arg_count) const;
    
    // Получение JavaScript эквивалента
    std::string getJSEquivalent(const std::string& func_name) const;
};

} // namespace stdlib

#endif // STDLIB_H