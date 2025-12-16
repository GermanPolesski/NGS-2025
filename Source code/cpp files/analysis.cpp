#include "analysis.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <iomanip>
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