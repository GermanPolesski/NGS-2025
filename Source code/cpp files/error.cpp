#include <precomph.h>

namespace Error {
    // Коды ошибок по категориям:
    // 0-30: ошибки вызова компилятора
    // 31-75: проблемы с выходными файлами
    // 76-100: ошибки препроцессора
    // 101-200: лексические ошибки
    // 201-300: синтаксические ошибки
    // 301-400: семантические ошибки
    // 401-500: ошибки генерации кода
    // 501-1000: прочие ошибки
    
    error_info error_list[1001] = {
        error_info(0, "Нет входных файлов в вызове"),
        error_info(1, "Нет флага или входного файла в вызове"),
        error_info(2, "Файлы не найдены"),
        error_info(3, "Некорректный флаг"),
        error_info(4, "Неизвестная ошибка при вызове компилятора"),
        error_info(5, "Входной файл не открыт корректно"),
        
        error_info(31, "Файл журнала записан некорректно"),
        
        error_info(76, "Отсутствует секция препроцессора"),
        error_info(77, "Нет такого файла для вставки"),
        error_info(78, "Неизвестная ошибка препроцессора"),
        
        error_info(101, "Неизвестная ошибка лексического анализа"),
        error_info(102, "Недопустимый символ"),
        error_info(103, "Незакрытая строковая константа"),
        error_info(104, "Незакрытая символьная константа"),
        error_info(105, "Неизвестный оператор"),
        error_info(106, "Некорректный числовой формат"),
        error_info(107, "Числовое значение вне диапазона"),
        error_info(108, "Неизвестный escape-символ"),
        
        error_info(201, "Неизвестная ошибка синтаксического анализа"),
        error_info(202, "Ожидался другой токен"),
        error_info(203, "Ожидался идентификатор"),
        error_info(204, "Ожидался тип данных"),
        error_info(205, "Ожидалось выражение"),
        error_info(206, "Ожидался оператор"),
        error_info(207, "Ожидалась закрывающая скобка"),
        error_info(208, "Ожидалась закрывающая фигурная скобка"),
        error_info(209, "Ожидалась точка с запятой"),
        error_info(210, "Ожидалась запятая"),
        error_info(211, "Ожидалось ключевое слово 'algo'"),
        error_info(212, "Ожидалось ключевое слово 'procedure'"),
        error_info(213, "Ожидалось ключевое слово 'est'"),
        error_info(214, "Ожидалось ключевое слово 'do'"),
        error_info(215, "Ожидалось ключевое слово 'while'"),
        error_info(216, "Ожидалось ключевое слово 'return'"),
        error_info(217, "Ожидалось ключевое слово 'ces'"),
        error_info(218, "Некорректная структура функции"),
        error_info(219, "Некорректная структура процедуры"),
        error_info(220, "Некорректная структура цикла do-while"),
        error_info(221, "Некорректное объявление переменной"),
        error_info(222, "Некорректное присваивание"),
        error_info(223, "Некорректный вызов функции"),
        error_info(224, "Некорректный список параметров"),
        error_info(225, "Некорректный список аргументов"),
        error_info(226, "Некорректный оператор return"),
        error_info(227, "Некорректное выражение"),
        error_info(228, "Некорректный бинарный оператор"),
        error_info(229, "Некорректный унарный оператор"),
        error_info(230, "Некорректный литерал"),
        error_info(231, "Некорректный блок кода"),
        error_info(232, "Некорректная структура программы"),
        
        error_info(301, "Неизвестная ошибка семантического анализа"),
        error_info(302, "Повторное объявление переменной"),
        error_info(303, "Повторное объявление функции"),
        error_info(304, "Необъявленный идентификатор"),
        error_info(305, "Необъявленная функция"),
        error_info(306, "Несоответствие типов"),
        error_info(307, "Некорректный тип возвращаемого значения"),
        error_info(308, "Некорректные аргументы функции"),
        error_info(309, "Неинициализированная переменная"),
        error_info(310, "Некорректное использование оператора"),
        error_info(311, "Некорректное условие цикла"),
        error_info(312, "Неиспользуемая переменная"),
        error_info(313, "Переменная скрывает глобальное объявление"),
        error_info(314, "Функция определена, но не вызвана"),
        error_info(315, "Функция вызвана, но не определена"),
        error_info(316, "Некорректный тип параметра"),
        error_info(317, "Некорректный тип выражения"),
        error_info(318, "Некорректная операция для типов"),
        error_info(319, "Return вне функции"),
        error_info(320, "Недостаточно аргументов"),
        error_info(321, "Слишком много аргументов"),
        
        error_info(401, "Неизвестная ошибка генерации кода"),
        error_info(402, "Невозможно сохранить сгенерированный код"),
        error_info(403, "Некорректная структура AST"),
        error_info(404, "Неподдерживаемая конструкция"),
        error_info(405, "Ошибка преобразования типа"),
        error_info(406, "Ошибка создания временного файла"),
        
        error_info(501, "Node.js не установлен"),
        error_info(502, "Ошибка выполнения сгенерированного кода"),
        
        error_info(998, "Недопустимый символ в кодировке Windows-1251"),
        error_info(999, "Ошибка вызвана другой ошибкой"),
        error_info(1000, "Неизвестная ошибка")
    };
    
    void initializeErrorList() {
        for (int i = 0; i <= 1000; i++) {
            if (error_list[i].id == 0) {
                error_list[i] = error_info(i, "Неизвестный код ошибки");
            }
        }
    }
    
    error_info getErrorID(unsigned short id) {
        static bool initialized = false;
        if (!initialized) {
            initializeErrorList();
            initialized = true;
        }
        
        if (id > 1000) {
            return error_list[999];
        }
        return error_list[id];
    }
    
    void ThrowConsole(unsigned short id, bool critical) {
        error_info err = getErrorID(id);
        std::cout << "\nОшибка " << err.id << ": " << err.message << "\n\n";
        if (err.line > 0) {
            std::cout << "В строке: " << err.line << ", столбец: " << err.col << "\n";
        }
        if (critical) {
            throw err.id;
        }
    }
    
    void ThrowConsole(unsigned short id, int line, int col, bool critical, const std::string& extra) {
        error_info err = getErrorID(id);
        std::cout << "\nОшибка " << err.id << ": " << err.message;
        if (!extra.empty()) {
            std::cout << " - " << extra;
        }
        std::cout << "\nВ строке: " << line << ", столбец: " << col << "\n\n";
        if (critical) {
            throw err.id;
        }
    }
    
    void ThrowConsole(unsigned short id, const lexan::Token& token, bool critical, const std::string& extra) {
        error_info err = getErrorID(id);
        std::cout << "\nОшибка " << err.id << ": " << err.message;
        if (!extra.empty()) {
            std::cout << " - " << extra;
        }
        std::cout << "\nВ строке: " << token.line << ", столбец: " << token.column;
        std::cout << " ('" << token.value << "')\n\n";
        if (critical) {
            throw err.id;
        }
    }
}