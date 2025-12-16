#include <precomph.h>

namespace Error {
    //Errors 0-30: incorrect call to compiler
    //Errors 31-75: problems with output file
    //Errors 76-100: incorrect using of preprocessor
    //Errors 101-200: lexical analysis errors
    //Errors 201-300: syntax analysis errors
    //Errors 301-400: Semantical analysis problems
    //Errors 401-500: Object code generation problems
    //Errors 501-1000: Other errors
    error_info error_list[1001] = {
        error_info(0, "No input files in call"),
        error_info(1, "No flag or input file in call"),
        error_info(2, "No files found"),
        error_info(3, "Incorrect flag"),
        error_info(4, "Unknown error with call to compiler"),
        error_info(5, "Input file wasn't opened correctly"),
        error_info(31, "Log file was written incorrectly"),
        error_info(76, "There is no preprocessor section"),
        error_info(77, "There is no such file to insert"),
        error_info(78, "Unknown error with preprocessor occurred"),
        error_info(101, "Unknown error occurred during Lexical analysis"),
        error_info(998, "Unallowed symbol"),
        error_info(999, "Error caused by an error"),
        error_info(1000, "Unknown error")
    };
    
    // Инициализируем остальные элементы как пустые ошибки
    void initializeErrorList() {
        for (int i = 0; i <= 1000; i++) {
            // Пропускаем уже инициализированные
            if (i == 0 || i == 1 || i == 2 || i == 3 || i == 4 || i == 5 ||
                i == 31 || i == 76 || i == 77 || i == 78 || i == 101 ||
                i == 998 || i == 999 || i == 1000) {
                continue;
            }
            error_list[i] = error_info(i, "Unknown error code");
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
    
    void ThrowConsole(unsigned short id) {
        error_info err = getErrorID(id);
        std::cout << "\nError " << err.id << ": " << err.message << "\n\n";
        if (err.line > 0) {
            std::cout << "At line: " << err.line << ", column: " << err.col << "\n";
        }
    }
}