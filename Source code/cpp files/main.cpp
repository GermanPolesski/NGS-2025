#include <precomph.h>
#include "lexer.h"
#include "parser.h"
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
                    if (!performSyntaxAnalysis(tokens, output_filename, parser)) {
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
                    std::cout << "=== SEMANTIC ANALYSIS (TODO) ===\n";
                    break;
                }
            case 4:
                {
                    std::cout << "=== CODE GENERATION (TODO) ===\n";
                    break;
                }
            case 5:
            {
                std::cout << "=== CODE RUNNING (TODO) ===\n";
                break;
            }
            default:
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