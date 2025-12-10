#include <precomph.h>

bool isIdentStart(char c) { return isalpha(c) || c == '_'; }
bool isIdentChar(char c)  { return isalnum(c) || c == '_'; }
bool isNumber(char c)     { return isdigit(c); }

void AddIdentifier(unordered_map<string, Identifier>& tbl,
                  const string& name,
                  const string& type,
                  const string& scope,
                  const string& source,
                  size_t pos) {
    if (tbl.find(name) != tbl.end()) return;

    Identifier id;
    id.name = name;
    id.type = type;
    id.value = "-";
    id.scope = scope;
    id.firstRow = FileWork::FindRow(source, pos);
    id.firstCol = FileWork::FindCol(source, pos);
    tbl[name] = id;
}

short LexAnalysis(const string& prep_filename, const string& output_filename)
{
    // Start logging
    string log_content = "========Lexer log========\n";
    log_content.append("Lexer started for file: " + prep_filename + "\n");
    
    string source = FileWork::ReadFile(prep_filename);
    log_content.append("Source file read successfully\n");
    
    // Remove preprocessor section
    size_t p = source.find("[preprocessor section end]");
    if (p != string::npos) {
        source.erase(0, p + 26);
        log_content.append("Preprocessor section removed\n");
    }
    
    // Remove lines with [] markers
    {
        istringstream iss(source);
        ostringstream oss;
        string line;
        int removed_count = 0;
        while (getline(iss, line)) {
            if (line.find("[") == string::npos && line.find("]") == string::npos) {
                oss << line << "\n";
            } else {
                removed_count++;
            }
        }
        source = oss.str();
        log_content.append("Removed " + to_string(removed_count) + " lines with [] markers\n");
    }
    
    // Remove leading/trailing whitespace from lines
    {
        istringstream iss(source);
        ostringstream oss;
        string line;
        while (getline(iss, line)) {
            size_t start = line.find_first_not_of(" \t");
            if (start != string::npos) {
                line = line.substr(start);
            }
            size_t end = line.find_last_not_of(" \t");
            if (end != string::npos) {
                line = line.substr(0, end + 1);
            }
            if (!line.empty()) {
                oss << line << "\n";
            }
        }
        source = oss.str();
    }
    
    log_content.append("Source cleaned and prepared for lexing\n");
    
    // Output data
    string lex;
    unordered_map<string, Identifier> ids;
    
    int line_num = 1;
    lex = "1.\t";
    
    // Parser states
    bool inFunctionDecl = false;
    bool inParams = false;
    bool insupfunc = false;
    bool afterType = false;
    bool afterAlgo = false;
    bool afterEst = false;
    bool inProclaim = false;
    
    string currentType = "";
    string currentFunction = "";
    string scope = "global";
    
    log_content.append("Starting token scanning...\n");
    
    // supfunc scanning loop
    for (size_t i = 0; i < source.size();) {
        char c = source[i];
        
        // Skip whitespace and tabs
        if (c == ' ' || c == '\t') {
            i++;
            continue;
        }
        
        // New line
        if (c == '\n') {
            line_num++;
            lex += "\n" + to_string(line_num) + ".\t";
            i++;
            afterEst = false;
            afterType = false;
            continue;
        }
        
        // Identifiers and keywords
        if (isIdentStart(c)) {
            size_t start = i;
            while (i < source.size() && isIdentChar(source[i])) i++;
            string word = source.substr(start, i - start);
            
            // Data types
            if (word == "int" || word == "char" || word == "string" || word == "time_t" || word == "procedure") {
                lex += "t";
                currentType = word;
                afterType = true;
                continue;
            }
            
            // algo keyword
            if (word == "algo") {
                lex += "a";
                afterAlgo = true;
                continue;
            }
            
            // est keyword
            if (word == "est") {
                lex += "e";
                afterEst = true;
                continue;
            }
            
            // return keyword
            if (word == "return") {
                lex += "r";
                continue;
            }
            
            // ces (supfunc function)
            if (word == "ces") {
                lex += "c";
                insupfunc = true;
                scope = "supfunc";
                continue;
            }
            
            // proclaim function
            if (word == "proclaim") {
                lex += "i";
                inProclaim = true;
                AddIdentifier(ids, word, "function", scope, source, start);
                continue;
            }
            
            // Function name after algo
            if (afterAlgo) {
                lex += "i";
                currentFunction = word;
                scope = word;
                AddIdentifier(ids, word, currentType, scope, source, start);
                afterAlgo = false;
                inFunctionDecl = true;
                continue;
            }
            
            // Parameter in function declaration
            if (inParams && afterType) {
                lex += "p";
                AddIdentifier(ids, word, "param(" + currentType + ")", scope, source, start);
                afterType = false;
                continue;
            }
            
            // Variable declaration after est and type
            if (afterEst && afterType) {
                lex += "i";
                AddIdentifier(ids, word, currentType, scope, source, start);
                afterEst = false;
                afterType = false;
                continue;
            }
            
            // Regular identifier
            lex += "i";
            AddIdentifier(ids, word, "unknown", scope, source, start);
            continue;
        }
        
        // Numbers
        if (isNumber(c)) {
            lex += "l";
            while (i < source.size() && isNumber(source[i])) i++;
            continue;
        }
        
        // String literals
        if (c == '"') {
            lex += "l";
            i++;
            while (i < source.size() && source[i] != '"') i++;
            if (i < source.size()) i++;
            continue;
        }
        
        // Symbols
        switch (c) {
            case '(':
                lex += "(";
                if (inFunctionDecl) {
                    inParams = true;
                }
                break;
                
            case ')':
                lex += ")";
                inParams = false;
                if (inFunctionDecl) {
                    inFunctionDecl = false;
                }
                if (inProclaim) {
                    inProclaim = false;
                }
                break;
                
            case '{':
                lex += "{";
                break;
                
            case '}':
                lex += "}";
                if (insupfunc) {
                    insupfunc = false;
                    scope = "global";
                }
                break;
                
            case ',':
                lex += ",";
                break;
                
            case ';':
                lex += ";";
                afterEst = false;
                afterType = false;
                break;
                
            case '=':
                lex += "=";
                break;
                
            case '+':
                lex += "+";
                break;
                
            default:
                i++;
                continue;
        }
        i++;
    }
    
    log_content.append("Token scanning completed\n");
    
    // Build identifier table
    string idout = "Имя\tТип\tЗначение\tОбласть\tПервоеВхождение\n";
    int id_count = 0;
    for (auto& kv : ids) {
        auto& d = kv.second;
        idout += d.name + "\t" +
                 d.type + "\t" +
                 d.value + "\t" +
                 d.scope + "\t(" +
                 to_string(d.firstRow) + "," +
                 to_string(d.firstCol) + ")\n";
        id_count++;
    }
    
    log_content.append("Identifier table built with " + to_string(id_count) + " entries\n");
    
    // Write output files
    string base_name = output_filename;
    size_t dot_pos = base_name.find_last_of('.');
    if (dot_pos != string::npos) {
        base_name = base_name.substr(0, dot_pos);
    }
    
    log_content.append("Lexical analysis completed successfully\n");
    log_content.append("========================\n");
    FileWork::WriteFile(base_name + ".log", log_content);
    log_content.append("Log file written: " + base_name + ".log\n");
    
    FileWork::WriteFile(base_name + "_lexTable.txt", lex);
    log_content.append("Lexeme table written: " + base_name + "_lexTable.txt\n");
    
    FileWork::WriteFile(base_name + "_idTable.txt", idout);
    log_content.append("Identifier table written: " + base_name + "_idTable.txt\n");
    
    return 0;
}
