// stdlib.cpp
#include "stdlib.h"
#include <sstream>

namespace stdlib {

StandardLibrary::StandardLibrary() {
    initializeFunctions();
}

void StandardLibrary::initializeFunctions() {
    // 1. Функции вывода
    addFunction({
        BuiltinType::PROCLAIM,
        "proclaim",
        "console.log",
        1, -1, // Минимум 1 аргумент, максимум не ограничен
        "void",
        "Выводит значение в консоль",
        "", // Используется встроенная console.log
        false
    });
    
    // 2. Функции преобразования типов
    addFunction({
        BuiltinType::TO_STR,
        "to_str",
        "String",
        1, 1,
        "string",
        "Преобразует значение в строку",
        "",
        false
    });
    
    // 3. Функции работы со временем
    addFunction({
        BuiltinType::TIME_FLED,
        "TimeFled",
        "__timeFled",
        2, 2,
        "number",
        "Вычисляет разницу между двумя датами в формате DDMMYYYY",
        R"(
function __timeFled(startStr, endStr) {
    function parseDate(str) {
        if (str.length !== 8) return null;
        const day = parseInt(str.substring(0, 2), 10);
        const month = parseInt(str.substring(2, 4), 10) - 1;
        const year = parseInt(str.substring(4, 8), 10);
        return new Date(year, month, day);
    }
    
    const start = parseDate(startStr);
    const end = parseDate(endStr);
    
    if (!start || !end) return 0;
    return end.getTime() - start.getTime(); // разница в миллисекундах
}
        )",
        true
    });
    
    addFunction({
        BuiltinType::THIS_VERY_MOMENT,
        "ThisVeryMoment",
        "Date.now",
        0, 0,
        "number",
        "Возвращает текущее время в миллисекундах",
        "",
        false
    });
    
    // 4. Функции работы со строками
    addFunction({
        BuiltinType::UNITE,
        "unite",
        "__unite",
        2, -1, // Минимум 2 аргумента (count + хотя бы один символ)
        "string",
        "Объединяет символы в строку",
        R"(
function __unite(count, ...args) {
    if (args.length < count) {
        throw new Error("unite: not enough arguments");
    }
    let result = '';
    for (let i = 0; i < count && i < args.length; i++) {
        if (typeof args[i] !== 'string' || args[i].length !== 1) {
            throw new Error("unite: argument must be a single character");
        }
        result += args[i];
    }
    return result;
}
        )",
        true
    });
    
    // 5. Математические функции
    addFunction({
        BuiltinType::SUM4,
        "sum4",
        "__sum4",
        4, 4,
        "number",
        "Суммирует 4 числа",
        R"(
function __sum4(a, b, c, d) {
    return Number(a) + Number(b) + Number(c) + Number(d);
}
        )",
        true
    });
    
    // 6. Дополнительные математические функции
    addFunction({
        BuiltinType::MATH_FUNC,
        "abs",
        "Math.abs",
        1, 1,
        "number",
        "Абсолютное значение",
        "",
        false
    });
    
    addFunction({
        BuiltinType::MATH_FUNC,
        "sqrt",
        "Math.sqrt",
        1, 1,
        "number",
        "Квадратный корень",
        "",
        false
    });
    
    addFunction({
        BuiltinType::MATH_FUNC,
        "pow",
        "Math.pow",
        2, 2,
        "number",
        "Возведение в степень",
        "",
        false
    });
    
    // 7. Строковые функции
    addFunction({
        BuiltinType::STRING_FUNC,
        "length",
        "__strLength",
        1, 1,
        "number",
        "Длина строки",
        R"(
function __strLength(str) {
    return String(str).length;
}
        )",
        true
    });
    
    addFunction({
        BuiltinType::STRING_FUNC,
        "substring",
        "String.prototype.substring",
        2, 3,
        "string",
        "Подстрока",
        "",
        false
    });
    
    // 8. Функции проверки типов
    addFunction({
        BuiltinType::TYPE_FUNC,
        "is_number",
        "__isNumber",
        1, 1,
        "boolean",
        "Проверяет, является ли значение числом",
        R"(
function __isNumber(val) {
    return typeof val === 'number' && !isNaN(val);
}
        )",
        true
    });
    
    addFunction({
        BuiltinType::TYPE_FUNC,
        "is_string",
        "__isString",
        1, 1,
        "boolean",
        "Проверяет, является ли значение строкой",
        R"(
function __isString(val) {
    return typeof val === 'string';
}
        )",
        true
    });
    
    addFunction({
        BuiltinType::TYPE_FUNC,
        "is_boolean",
        "__isBoolean",
        1, 1,
        "boolean",
        "Проверяет, является ли значение логическим",
        R"(
function __isBoolean(val) {
    return typeof val === 'boolean';
}
        )",
        true
    });
}

void StandardLibrary::addFunction(const BuiltinFunction& func) {
    functions[func.name] = func;
}

bool StandardLibrary::isBuiltin(const std::string& name) const {
    return functions.find(name) != functions.end();
}

const BuiltinFunction* StandardLibrary::getFunction(const std::string& name) const {
    auto it = functions.find(name);
    return it != functions.end() ? &it->second : nullptr;
}

const std::unordered_map<std::string, BuiltinFunction>& StandardLibrary::getAllFunctions() const {
    return functions;
}

std::string StandardLibrary::generateJSImports() const {
    std::stringstream imports;
    
    // Собираем все функции, требующие импорта
    std::vector<std::string> funcs_to_import;
    for (const auto& [name, func] : functions) {
        if (func.requires_import && !func.js_implementation.empty()) {
            funcs_to_import.push_back(func.js_name);
        }
    }
    
    if (!funcs_to_import.empty()) {
        imports << "// === Standard Library Functions ===\n";
        for (const auto& func_name : funcs_to_import) {
            auto it = std::find_if(functions.begin(), functions.end(),
                [&func_name](const auto& p) { return p.second.js_name == func_name; });
            if (it != functions.end()) {
                imports << it->second.js_implementation << "\n\n";
            }
        }
        imports << "// ==================================\n\n";
    }
    
    return imports.str();
}

std::string StandardLibrary::generateJSPolyfills() const {
    std::stringstream polyfills;
    
    polyfills << R"(
// === JavaScript Polyfills for compatibility ===

// Polyfill для String.prototype.padStart (если не поддерживается)
if (!String.prototype.padStart) {
    String.prototype.padStart = function padStart(targetLength, padString) {
        targetLength = targetLength >> 0;
        padString = String(padString || ' ');
        if (this.length > targetLength) {
            return String(this);
        } else {
            targetLength = targetLength - this.length;
            if (targetLength > padString.length) {
                padString += padString.repeat(targetLength / padString.length);
            }
            return padString.slice(0, targetLength) + String(this);
        }
    };
}

// Функция форматирования даты
function __formatDate(date) {
    const day = String(date.getDate()).padStart(2, '0');
    const month = String(date.getMonth() + 1).padStart(2, '0');
    const year = date.getFullYear();
    return `${day}.${month}.${year}`;
}

// Функция безопасного парсинга чисел
function __safeParseInt(str, defaultValue = 0) {
    const num = parseInt(str, 10);
    return isNaN(num) ? defaultValue : num;
}

// Функция глубокого копирования (для массивов и объектов)
function __deepCopy(obj) {
    if (obj === null || typeof obj !== 'object') return obj;
    if (obj instanceof Array) return obj.map(__deepCopy);
    const copy = {};
    for (const key in obj) {
        if (obj.hasOwnProperty(key)) {
            copy[key] = __deepCopy(obj[key]);
        }
    }
    return copy;
}

// ================================================

)";
    
    return polyfills.str();
}

bool StandardLibrary::validateArguments(const std::string& func_name, int arg_count) const {
    auto func = getFunction(func_name);
    if (!func) return false;
    
    if (func->max_args == -1) {
        // Переменное число аргументов
        return arg_count >= func->min_args;
    } else {
        // Фиксированное число аргументов
        return arg_count >= func->min_args && arg_count <= func->max_args;
    }
}

std::string StandardLibrary::getJSEquivalent(const std::string& func_name) const {
    auto func = getFunction(func_name);
    if (!func) return func_name; // Если не нашли, возвращаем исходное имя
    
    return func->js_name;
}

} // namespace stdlib