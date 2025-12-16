#ifndef ANALYSIS_H
#define ANALYSIS_H

#include <string>
#include <vector>
#include "lexer.h"
#include "parser.h"
#include "semantic.h"

bool performSemanticAnalysis(parser::ASTNode* ast, 
                            const std::string& filename,
                            semantic::SemanticAnalyzer& analyzer);

#endif // ANALYSIS_H