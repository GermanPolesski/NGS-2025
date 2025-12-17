#ifndef ANALYSIS_H
#define ANALYSIS_H

#include <string>
#include <vector>
#include "lexer.h"
#include "parser.h"
#include "semantic.h"
#include "rpnconverter.h"
#include "codegen.h"

// Функции для различных этапов анализа
bool performSemanticAnalysis(parser::ASTNode* ast, 
                            const std::string& filename,
                            semantic::SemanticAnalyzer& analyzer);

bool performRPNConversion(parser::ASTNode* ast, 
                         const std::string& filename,
                         rpn::RPNConverter& converter);

bool performCodeGeneration(parser::ASTNode* ast, 
                          const std::string& filename,
                          codegen::CodeGenerator& generator,
                          semantic::SemanticAnalyzer* analyzer = nullptr);

// Вспомогательные функции
void printHelp();
std::string readSourceCode(const std::string& filename);

#endif // ANALYSIS_H