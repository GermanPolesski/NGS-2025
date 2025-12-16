#include <precomph.h>
#include "lexer.h"
#include "parser.h"
#include "semantic.h"  // Добавлен include для семантического анализа
#include "analysis.h"  // Предполагается, что у вас есть этот файл с функциями анализа
#include <filesystem>

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
            case 0: 
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
            case 1:
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
            case 2:
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
            case 3:
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
                    if (performSemanticAnalysis(parser.get_ast(), output_filename, analyzer)) {
                        return 1;
                    }
                    
                    std::cout << "\nSemantic analysis completed successfully!\n";
                    break;
                }
            case 4:
                {
                    std::cout << "=== CODE GENERATION (TODO) ===\n";
                    // TODO: Реализовать генерацию кода
                    break;
                }
            case 5:
                {
                    std::cout << "=== CODE RUNNING (TODO) ===\n";
                    // TODO: Реализовать запуск сгенерированного кода
                    break;
                }
            default:
                std::cout << "Unknown command: " << call << "\n";
                std::cout << "Available commands:\n";
                std::cout << "  0 - Preprocessing only\n";
                std::cout << "  1 - Lexical analysis + tokens\n";
                std::cout << "  2 - Syntax analysis + AST\n";
                std::cout << "  3 - Semantic analysis\n";
                std::cout << "  4 - Code generation (TODO)\n";
                std::cout << "  5 - Code running (TODO)\n";
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