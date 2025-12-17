#include "analysis.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include "precomph.h"

namespace fs = std::filesystem;

bool performSemanticAnalysis(parser::ASTNode* ast, 
                            const std::string& filename,
                            semantic::SemanticAnalyzer& analyzer) {
    std::cout << "Semantic analysis...\n";
    
    if (!analyzer.analyze(ast)) {
        std::cout << "Semantic analysis failed!\n";
        
        const auto& errors = analyzer.get_errors();
        for (const auto& error : errors) {
            std::cout << "  " << error << "\n";
        }
        
        // Сохраняем отчет об ошибках
        std::string error_filename = filename + ".semantic.errors.txt";
        std::ofstream error_file(error_filename);
        if (error_file.is_open()) {
            error_file << analyzer.generate_report();
            error_file.close();
            std::cout << "Error report saved to: " << error_filename << "\n";
        }
        
        return false;
    }
    
    std::cout << "Semantic analysis successful!\n";
    
    // Выводим предупреждения
    const auto& warnings = analyzer.get_warnings();
    if (!warnings.empty()) {
        std::cout << "\nWarnings:\n";
        for (const auto& warning : warnings) {
            std::cout << "  " << warning << "\n";
        }
    }
    
    // Выводим информацию о символах и функциях
    analyzer.print_symbol_table();
    analyzer.print_function_table();
    analyzer.print_type_summary();
    
    // Сохраняем полный отчет
    std::string report_filename = filename + ".semantic.report.txt";
    std::ofstream report_file(report_filename);
    if (report_file.is_open()) {
        report_file << analyzer.generate_report();
        report_file.close();
        std::cout << "\nSemantic analysis report saved to: " << report_filename << "\n";
    }
    
    return true;
}

bool performRPNConversion(parser::ASTNode* ast, 
                         const std::string& filename,
                         rpn::RPNConverter& converter) {
    std::cout << "Converting expressions to Reverse Polish Notation...\n";
    
    if (!ast) {
        std::cout << "Error: AST is null\n";
        return false;
    }
    
    // Конвертируем программу в RPN
    std::string rpn_result = converter.convert_program(ast);
    
    // Сохраняем результат
    std::string rpn_filename = filename + ".rpn.txt";
    std::ofstream rpn_file(rpn_filename);
    if (!rpn_file.is_open()) {
        std::cout << "Error: Could not create RPN output file\n";
        return false;
    }
    
    rpn_file << rpn_result;
    rpn_file.close();
    
    std::cout << "RPN conversion successful!\n";
    std::cout << "RPN output saved to: " << rpn_filename << "\n";
    
    // Выводим часть результата в консоль
    std::cout << "\n=== RPN Result (first 50 lines) ===\n";
    std::istringstream iss(rpn_result);
    std::string line;
    int line_count = 0;
    while (std::getline(iss, line) && line_count < 50) {
        std::cout << line << "\n";
        line_count++;
    }
    
    return true;
}

bool performCodeGeneration(parser::ASTNode* ast, 
                          const std::string& filename,
                          codegen::CodeGenerator& generator,
                          semantic::SemanticAnalyzer* analyzer) {
    std::cout << "Generating JavaScript code...\n";
    
    if (!ast) {
        std::cout << "Error: AST is null\n";
        return false;
    }
    
    // Generate JavaScript code
    std::string js_code = generator.generate(ast, analyzer);
    
    // Save to file
    std::string js_filename = filename + ".js";
    generator.save_to_file(js_filename);
    
    std::cout << "Code generation successful!\n";
    std::cout << "JavaScript file saved to: " << js_filename << "\n";
    
    // Show first 50 lines
    std::cout << "\n=== Generated JavaScript (first 50 lines) ===\n";
    std::istringstream iss(js_code);
    std::string line;
    int line_count = 0;
    while (std::getline(iss, line) && line_count < 50) {
        std::cout << line << "\n";
        line_count++;
    }
    
    return true;
}

bool executeGeneratedCode(const std::string& js_filename) {
    #ifdef _WIN32
        std::string command = "node \"" + js_filename + "\"";
    #else
        std::string command = "node \"" + js_filename + "\"";
    #endif
    
    std::cout << "Executing: " << command << "\n";
    std::cout << "========================================\n";
    
    int result = system(command.c_str());
    
    std::cout << "========================================\n";
    if (result == 0) {
        std::cout << "Execution completed successfully!\n";
        return true;
    } else {
        std::cout << "Execution failed with error code: " << result << "\n";
        return false;
    }
}