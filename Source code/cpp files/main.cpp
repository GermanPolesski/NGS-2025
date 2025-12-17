#include <precomph.h>
#include "lexer.h"
#include "parser.h"
#include "semantic.h"
#include "analysis.h"
#include "rpnconverter.h"
#include "codegen.h"
#include <filesystem>
#include <chrono>
#include <cstdlib>
#include <sys/stat.h>
#include <unistd.h>  // Для getpid()

namespace fs = std::filesystem;

int main(int argc, char* argv[]) {
    try {
        std::string input_files[10];
        std::string output_filename;

        short call = processCall(argc, argv, input_files, output_filename);
        if (call == -1) {
            return -1;
        }

        for (int i = 0; i < 10; i++) {
            if (input_files[i].empty()) {
                break;
            }
            if (!isWindows1251(FileWork::ReadFile(input_files[i]), output_filename)) {
                Error::ThrowConsole(998);
                std::cout << "Compilation Terminated\n";
                return -1;
            }
        }

        switch(call) {
            case 0: // -prep
                {
                    std::cout << "=== PREPROCESSING ONLY ===\n";
                    short prep_result = Preprocess(input_files, output_filename);
                    if (prep_result != 0) {
                        std::cout << "Preprocessing failed with error code: " << prep_result << std::endl;
                        return 1;
                    }
                    std::cout << "Preprocessed code saved to: " << output_filename << "\n";
                    break; 
                }   
            case 1: // -lex
                {
                    std::cout << "=== LEXICAL ANALYSIS ===\n";
                    std::string source_code;
                    if (!performPreprocessing(input_files, output_filename, source_code)) {
                        return 1;
                    }
                    
                    std::vector<lexan::Token> tokens;
                    if (!performLexicalAnalysis(source_code, output_filename, tokens)) {
                        return 1;
                    }
                    
                    std::string token_log_filename = output_filename + ".tokens.log";
                    parser::writeTokenLog(tokens, output_filename, token_log_filename);
                    
                    std::string token_filename = output_filename + ".tokens.txt";
                    if (lexan::Lexer::generate_token_file(source_code, token_filename)) {
                        std::cout << "Standard token file saved to: " << token_filename << "\n";
                    }
                    
                    std::cout << "\nLexical analysis completed successfully!\n";
                    break;
                }
            case 2: // -syn
                {
                    std::cout << "=== SYNTAX ANALYSIS ===\n";
                    std::string source_code;
                    if (!performPreprocessing(input_files, output_filename, source_code)) {
                        return 1;
                    }
                    
                    std::vector<lexan::Token> tokens;
                    if (!performLexicalAnalysis(source_code, output_filename, tokens)) {
                        return 1;
                    }
                    
                    std::string token_log_filename = output_filename + ".tokens.log";
                    parser::writeTokenLog(tokens, output_filename, token_log_filename);
                    
                    parser::Parser parser(tokens);
                    if (!parser::performSyntaxAnalysis(tokens, output_filename, parser)) {
                        return 1;
                    }
                    
                    std::cout << "\nSyntax analysis completed successfully!\n";
                    std::cout << "AST files generated:\n";
                    std::cout << "  - " << output_filename << ".ast.txt (text representation)\n";
                    std::cout << "  - " << output_filename << ".ast.dot (Graphviz DOT format)\n";
                    break;
                }
            case 3: // -sem
                {
                    std::cout << "=== SEMANTIC ANALYSIS ===\n";
                    std::string source_code;
                    if (!performPreprocessing(input_files, output_filename, source_code)) {
                        return 1;
                    }
                    
                    std::vector<lexan::Token> tokens;
                    if (!performLexicalAnalysis(source_code, output_filename, tokens)) {
                        return 1;
                    }
                    
                    std::string token_log_filename = output_filename + ".tokens.log";
                    parser::writeTokenLog(tokens, output_filename, token_log_filename);
                    
                    // Синтаксический анализ
                    parser::Parser parser(tokens);
                    if (!parser.parse()) {
                        std::cout << "Parsing failed! Cannot perform semantic analysis.\n";
                        return 1;
                    }
                    
                    // Семантический анализ
                    semantic::SemanticAnalyzer analyzer;
                    if (!performSemanticAnalysis(parser.get_ast(), output_filename, analyzer)) {
                        return 1;
                    }
                    
                    std::cout << "\nSemantic analysis completed successfully!\n";
                    break;
                }
            case 4: // -pol (польская нотация)
                {
                    std::cout << "=== REVERSE POLISH NOTATION CONVERSION ===\n";
                    std::string source_code;
                    if (!performPreprocessing(input_files, output_filename, source_code)) {
                        return 1;
                    }
                    
                    std::vector<lexan::Token> tokens;
                    if (!performLexicalAnalysis(source_code, output_filename, tokens)) {
                        return 1;
                    }
                    
                    std::string token_log_filename = output_filename + ".tokens.log";
                    parser::writeTokenLog(tokens, output_filename, token_log_filename);
                    
                    // Синтаксический анализ
                    parser::Parser parser(tokens);
                    if (!parser.parse()) {
                        std::cout << "Parsing failed! Cannot perform RPN conversion.\n";
                        return 1;
                    }
                    
                    // RPN конверсия
                    rpn::RPNConverter converter;
                    if (!performRPNConversion(parser.get_ast(), output_filename, converter)) {
                        return 1;
                    }
                    
                    std::cout << "\nRPN conversion completed successfully!\n";
                    break;
                }
            case 5: // -tran (трансляция в JS)
                {
                    std::cout << "=== CODE GENERATION ===\n";
                    std::string source_code;
                    if (!performPreprocessing(input_files, output_filename, source_code)) {
                        return 1;
                    }
                    
                    std::vector<lexan::Token> tokens;
                    if (!performLexicalAnalysis(source_code, output_filename, tokens)) {
                        return 1;
                    }
                    
                    std::string token_log_filename = output_filename + ".tokens.log";
                    parser::writeTokenLog(tokens, output_filename, token_log_filename);
                    
                    // Синтаксический анализ
                    parser::Parser parser(tokens);
                    if (!parser.parse()) {
                        std::cout << "Parsing failed! Cannot generate code.\n";
                        return 1;
                    }
                    
                    // Семантический анализ
                    semantic::SemanticAnalyzer analyzer;
                    if (!analyzer.analyze(parser.get_ast())) {
                        std::cout << "Semantic analysis failed! Code generation may produce incorrect results.\n";
                    }
                    
                    // Генерация кода
                    codegen::CodeGenerator generator;
                    std::string js_code = generator.generate(parser.get_ast(), &analyzer);
                    
                    // Сохранение кода в файл
                    std::string js_filename = output_filename + ".js";
                    
                    std::ofstream js_file(js_filename);
                    if (!js_file.is_open()) {
                        std::cout << "Error: Could not create JavaScript file\n";
                        return 1;
                    }
                    
                    js_file << js_code;
                    js_file.close();
                    
                    std::cout << "Code generation successful!\n";
                    std::cout << "JavaScript code saved to: " << js_filename << "\n";
                    
                    // Показать часть сгенерированного кода
                    std::cout << "\n=== Generated JavaScript (first 50 lines) ===\n";
                    std::istringstream iss(js_code);
                    std::string line;
                    int line_count = 0;
                    while (std::getline(iss, line) && line_count < 50) {
                        std::cout << line << "\n";
                        line_count++;
                    }
                    break;
                }
            case 6: // -run (запуск сгенерированного кода)
                {
                    std::cout << "=== CODE EXECUTION ===\n";
                    
                    // Сначала проверяем, установлен ли Node.js
                    std::cout << "Checking Node.js installation...\n";
                    int node_check = system("which node > /dev/null 2>&1");
                    if (node_check != 0) {
                        std::cout << "Error: Node.js is not installed!\n";
                        std::cout << "Install Node.js with:\n";
                        std::cout << "  sudo apt update\n";
                        std::cout << "  sudo apt install nodejs npm\n";
                        return 1;
                    }
                    
                    std::string source_code;
                    if (!performPreprocessing(input_files, output_filename, source_code)) {
                        return 1;
                    }
                    
                    std::vector<lexan::Token> tokens;
                    if (!performLexicalAnalysis(source_code, output_filename, tokens)) {
                        return 1;
                    }
                    
                    // Синтаксический анализ
                    parser::Parser parser(tokens);
                    if (!parser.parse()) {
                        std::cout << "Parsing failed! Cannot generate code for execution.\n";
                        return 1;
                    }
                    
                    // Генерация кода
                    codegen::CodeGenerator generator;
                    std::string js_code = generator.generate(parser.get_ast());
                    
                    // Создаем временный файл
                    std::string temp_js_filename = "/tmp/mycompiler_" + std::to_string(getpid()) + ".js";
                    
                    std::ofstream temp_file(temp_js_filename);
                    if (!temp_file.is_open()) {
                        std::cout << "Error: Could not create temporary file\n";
                        return 1;
                    }
                    
                    temp_file << js_code;
                    temp_file.close();
                    
                    // Запуск сгенерированного JavaScript кода
                    std::cout << "\nExecuting generated JavaScript code...\n";
                    std::cout << "========================================\n";
                    
                    // Собираем команду для выполнения
                    std::string command = "node \"" + temp_js_filename + "\"";
                    
                    // Выполняем команду
                    int result = system(command.c_str());
                    
                    std::cout << "========================================\n";
                    
                    // Удаляем временный файл
                    std::remove(temp_js_filename.c_str());
                    
                    if (result == 0) {
                        std::cout << "Execution completed successfully!\n";
                    } else {
                        std::cout << "Execution failed with exit code: " << result << "\n";
                    }
                    break;
                }
            default:
                std::cout << "Unknown command: " << call << "\n";
                std::cout << "Available commands:\n";
                std::cout << "  -prep  : Preprocessing only\n";
                std::cout << "  -lex   : Lexical analysis + tokens\n";
                std::cout << "  -syn   : Syntax analysis + AST\n";
                std::cout << "  -sem   : Semantic analysis\n";
                std::cout << "  -pol   : Reverse Polish Notation conversion\n";
                std::cout << "  -tran  : Code generation to JavaScript\n";
                std::cout << "  -run   : Execute generated JavaScript code\n";
                return 1;
        }
    }
    catch (const char* e) {
        std::cout << "Error: " << e << '\n';
    }
    catch (const std::exception& e) {
        std::cout << "Standard exception: " << e.what() << '\n';
    }
    catch (...) {
        std::cout << "Unknown error occurred\n";
    }

    return 0;
}