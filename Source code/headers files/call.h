#include "lexer.h"

short getFlagCode (const char* arg);
short processCall (int argc, char* argv[], string input_files[], string& output_file);
bool performPreprocessing(std::string input_files[], std::string& output_filename, 
                          std::string& preprocessed_code);
bool performLexicalAnalysis(const std::string& source_code, const std::string& filename,
                           std::vector<lexan::Token>& tokens);