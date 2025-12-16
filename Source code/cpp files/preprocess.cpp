#include <precomph.h>
#include <sstream>
#include <string>
#include <vector>
#include <cstring>

using namespace std;

static inline bool extract_quoted(const string& s, size_t start_pos, string& out) {
    size_t q1 = s.find('"', start_pos);
    if (q1 == string::npos) return false;
    size_t q2 = s.find('"', q1 + 1);
    if (q2 == string::npos) return false;
    out = s.substr(q1 + 1, q2 - q1 - 1);
    return true;
}

short Preprocess(string input_files[], string& output_file) {
    string log_content;
    log_content.append("=====Preprocessor log=====\n");

    // 1) Read main source
    const string main_file = input_files[0];
    string preprocessed_code = FileWork::ReadFile(main_file);
    if (preprocessed_code.empty()) {
        log_content.append("Error: main file is empty or unreadable: " + main_file + "\n");
        FileWork::WriteFile(main_file + ".log", log_content);
        Error::ThrowConsole(5);
        return -1;
    }
    log_content.append("Main file read: " + main_file + "\n");

    // 2) Validate preprocessor section header
    if (preprocessed_code.find("[preprocessor section begin]") == string::npos) {
        size_t err_pos = 0;
        int row = FileWork::FindRow(preprocessed_code, err_pos);
        int col = FileWork::FindCol(preprocessed_code, err_pos);
        log_content.append("Error 76 at row " + to_string(row) + ", col " + to_string(col) + "\n");
        FileWork::WriteFile(main_file + ".log", log_content);
        Error::ThrowConsole(76);
        return -1;
    }

    // 3) Handle ##program "name" → output_file = name.txt
    {
        size_t pos = preprocessed_code.find("##program");
        if (pos != string::npos) {
            string out_name;
            if (!extract_quoted(preprocessed_code, pos, out_name)) {
                int row = FileWork::FindRow(preprocessed_code, pos);
                int col = FileWork::FindCol(preprocessed_code, pos);
                log_content.append("Error: malformed ##program at row " + to_string(row) + ", col " + to_string(col) + "\n");
                FileWork::WriteFile(main_file + ".log", log_content);
                Error::ThrowConsole(77);
                return -1;
            }
            output_file = out_name + ".txt";
            log_content.append("Output filename set to: " + output_file + "\n");
            
            // Remove ##program directive
            size_t line_end = preprocessed_code.find('\n', pos);
            if (line_end == string::npos) line_end = preprocessed_code.size();
            preprocessed_code.erase(pos, line_end - pos + 1);
        } else {
            // default: derive from main_file
            size_t dot = main_file.find_last_of('.');
            output_file = (dot == string::npos) ? (main_file + ".txt")
                                               : (main_file.substr(0, dot) + ".txt");
            log_content.append("Output filename derived: " + output_file + "\n");
        }
    }

    // 4) Handle ##inaddition "file" - insert functions and macros
    {
        size_t pos = 0;
        int added_count = 0;
        
        while ((pos = preprocessed_code.find("##inaddition", pos)) != string::npos) {
            // Extract filename from quotes
            size_t quote_start = preprocessed_code.find('"', pos);
            if (quote_start == string::npos) {
                int row = FileWork::FindRow(preprocessed_code, pos);
                int col = FileWork::FindCol(preprocessed_code, pos);
                log_content.append("Error: malformed ##inaddition at row " + to_string(row) +
                                   ", col " + to_string(col) + "\n");
                FileWork::WriteFile(main_file + ".log", log_content);
                Error::ThrowConsole(78);
                return -1;
            }
            
            size_t quote_end = preprocessed_code.find('"', quote_start + 1);
            if (quote_end == string::npos) {
                int row = FileWork::FindRow(preprocessed_code, pos);
                int col = FileWork::FindCol(preprocessed_code, pos);
                log_content.append("Error: malformed ##inaddition at row " + to_string(row) +
                                   ", col " + to_string(col) + "\n");
                FileWork::WriteFile(main_file + ".log", log_content);
                Error::ThrowConsole(78);
                return -1;
            }
            
            string add_filename = preprocessed_code.substr(quote_start + 1, 
                                                         quote_end - quote_start - 1);
            
            // Read additional file
            string additional_code = FileWork::ReadFile(add_filename);
            if (additional_code.empty()) {
                log_content.append("Warning: additional file empty or not found: " + 
                                   add_filename + "\n");
                
                // Remove directive anyway
                size_t line_end = preprocessed_code.find('\n', pos);
                if (line_end == string::npos) line_end = preprocessed_code.size();
                preprocessed_code.erase(pos, line_end - pos + 1);
                continue;
            }
            
            log_content.append("Processing additional file: " + add_filename + "\n");
            
            // Try both singular and plural markers for function section
            string func_begin_marker = "[function section begin]";
            string func_end_marker = "[function section end]";
            
            size_t func_begin = additional_code.find(func_begin_marker);
            size_t func_end = additional_code.find(func_end_marker);
            
            // If not found with singular, try plural
            if (func_begin == string::npos || func_end == string::npos) {
                func_begin_marker = "[functions section begin]";
                func_end_marker = "[functions section end]";
                func_begin = additional_code.find(func_begin_marker);
                func_end = additional_code.find(func_end_marker);
            }
            
            if (func_begin != string::npos && func_end != string::npos && func_end > func_begin) {
                // Extract functions (excluding markers)
                string functions = additional_code.substr(
                    func_begin + func_begin_marker.length(), 
                    func_end - func_begin - func_begin_marker.length()
                );
                
                // Find where to insert in main file (try both singular and plural)
                string target_marker = "[functions section begin]";
                size_t insert_pos = preprocessed_code.find(target_marker);
                
                if (insert_pos == string::npos) {
                    // Try singular
                    target_marker = "[function section begin]";
                    insert_pos = preprocessed_code.find(target_marker);
                }
                
                if (insert_pos != string::npos) {
                    // Insert after the marker
                    size_t insert_after = insert_pos + target_marker.length();
                    
                    // Trim leading/trailing whitespace from functions
                    size_t func_start = functions.find_first_not_of(" \t\n\r");
                    if (func_start != string::npos) {
                        size_t func_end_pos = functions.find_last_not_of(" \t\n\r");
                        functions = functions.substr(func_start, func_end_pos - func_start + 1);
                    }
                    
                    preprocessed_code.insert(insert_after, "\n" + functions + "\n");
                    
                    log_content.append("Successfully inserted functions from " + 
                                       add_filename + " after " + target_marker + "\n");
                    added_count++;
                } else {
                    log_content.append("Warning: function section marker not found in main file\n");
                }
            } else {
                log_content.append("Warning: function section not found in " + 
                                   add_filename + " (tried both singular and plural markers)\n");
            }
            
            // Extract and add ##perceive macros from additional file
            size_t macro_pos = 0;
            while ((macro_pos = additional_code.find("##perceive", macro_pos)) != string::npos) {
                size_t line_end = additional_code.find('\n', macro_pos);
                if (line_end == string::npos) line_end = additional_code.size();
                
                string macro_line = additional_code.substr(macro_pos, line_end - macro_pos);
                
                // Find preprocessor section end in main file to insert before it
                size_t pp_end = preprocessed_code.find("[preprocessor section end]");
                if (pp_end != string::npos) {
                    preprocessed_code.insert(pp_end, macro_line + "\n");
                    log_content.append("Added macro from " + add_filename + ": " + macro_line + "\n");
                }
                
                macro_pos = (line_end == additional_code.size()) ? line_end : (line_end + 1);
            }

            // Remove the ##inaddition directive
            size_t line_end = preprocessed_code.find('\n', pos);
            if (line_end == string::npos) line_end = preprocessed_code.size();
            preprocessed_code.erase(pos, line_end - pos + 1);
            
            // Continue searching from the beginning (since we modified the string)
            pos = 0;
        }
        
        log_content.append("Processed ##inaddition directives: " + 
                          to_string(added_count) + "\n");
    }

    // 5) Process all ##perceive macros
    {
        vector<pair<string, string>> macros;
        
        // First pass: collect all macros
        size_t pos = 0;
        while ((pos = preprocessed_code.find("##perceive", pos)) != string::npos) {
            size_t line_end = preprocessed_code.find('\n', pos);
            if (line_end == string::npos) line_end = preprocessed_code.size();
            
            string macro_line = preprocessed_code.substr(pos, line_end - pos);
            
            // Parse macro: ##perceive NAME VALUE
            size_t name_start = pos + 10; // Skip "##perceive"
            while (name_start < line_end && (preprocessed_code[name_start] == ' ' || preprocessed_code[name_start] == '\t')) {
                name_start++;
            }
            
            size_t name_end = name_start;
            while (name_end < line_end && preprocessed_code[name_end] != ' ' && preprocessed_code[name_end] != '\t') {
                name_end++;
            }
            
            if (name_end >= line_end) {
                log_content.append("Error: invalid ##perceive format\n");
                break;
            }
            
            string macro_name = preprocessed_code.substr(name_start, name_end - name_start);
            
            size_t value_start = name_end;
            while (value_start < line_end && (preprocessed_code[value_start] == ' ' || preprocessed_code[value_start] == '\t')) {
                value_start++;
            }
            
            if (value_start >= line_end) {
                log_content.append("Error: missing value in ##perceive\n");
                break;
            }
            
            string macro_value = preprocessed_code.substr(value_start, line_end - value_start);
            
            // Trim trailing whitespace from value
            size_t value_end = macro_value.find_last_not_of(" \t\r");
            if (value_end != string::npos) {
                macro_value = macro_value.substr(0, value_end + 1);
            }
            
            macros.push_back({macro_name, macro_value});
            log_content.append("Macro defined: " + macro_name + " = " + macro_value + "\n");
            
            // Remove macro line
            preprocessed_code.erase(pos, line_end - pos + 1);
        }
        
        // Second pass: replace all occurrences of macros
        for (const auto& macro : macros) {
            size_t replace_pos = 0;
            while ((replace_pos = preprocessed_code.find(macro.first, replace_pos)) != string::npos) {
                // Check if it's a whole word (not part of another identifier)
                bool is_whole_word = true;
                if (replace_pos > 0) {
                    char prev = preprocessed_code[replace_pos - 1];
                    if (isalnum(prev) || prev == '_') {
                        is_whole_word = false;
                    }
                }
                
                if (replace_pos + macro.first.length() < preprocessed_code.length()) {
                    char next = preprocessed_code[replace_pos + macro.first.length()];
                    if (isalnum(next) || next == '_') {
                        is_whole_word = false;
                    }
                }
                
                if (is_whole_word) {
                    preprocessed_code.replace(replace_pos, macro.first.length(), macro.second);
                    replace_pos += macro.second.length();
                    log_content.append("  Replaced " + macro.first + " with " + macro.second + "\n");
                } else {
                    replace_pos += macro.first.length();
                }
            }
        }
    }

    // 6) Remove ALL section markers including both singular and plural versions
    {
        // Remove all possible section markers
        vector<pair<string, string>> markers_to_remove = {
            {"[preprocessor section begin]", "Preprocessor section begin"},
            {"[preprocessor section end]", "Preprocessor section end"},
            {"[functions section begin]", "Functions section begin (plural)"},
            {"[functions section end]", "Functions section end (plural)"},
            {"[function section begin]", "Function section begin (singular)"},
            {"[function section end]", "Function section end (singular)"},
            {"[superior function begin]", "Superior function begin"},
            {"[superior function end]", "Superior function end"}
        };
        
        for (const auto& marker : markers_to_remove) {
            const string& marker_text = marker.first;
            const string& marker_name = marker.second;
            
            size_t pos = 0;
            while ((pos = preprocessed_code.find(marker_text, pos)) != string::npos) {
                preprocessed_code.erase(pos, marker_text.length());
                log_content.append("Removed: " + marker_name + "\n");
                // Don't advance pos since we just removed the marker
            }
        }
        
        // Remove any remaining individual brackets just in case
        size_t pos = 0;
        while ((pos = preprocessed_code.find('[')) != string::npos) {
            preprocessed_code.erase(pos, 1);
            log_content.append("Removed stray '['\n");
        }
        
        pos = 0;
        while ((pos = preprocessed_code.find(']')) != string::npos) {
            preprocessed_code.erase(pos, 1);
            log_content.append("Removed stray ']'\n");
        }
    }

    // 7) Remove @...@ comments
    {
        size_t at_pos = 0;
        while ((at_pos = preprocessed_code.find('@', at_pos)) != string::npos) {
            size_t next_at = preprocessed_code.find('@', at_pos + 1);
            if (next_at != string::npos) {
                // Check for tab before comment
                size_t remove_start = at_pos;
                if (remove_start > 0 && preprocessed_code[remove_start - 1] == '\t') {
                    remove_start--;
                }
                
                preprocessed_code.erase(remove_start, next_at - remove_start + 1);
                at_pos = remove_start;
            } else {
                preprocessed_code.erase(at_pos, 1);
            }
        }
    }

    // 8) Clean up empty lines and whitespace
    {
        istringstream iss(preprocessed_code);
        ostringstream oss;
        string line;
        int removed_count = 0;
        
        while (getline(iss, line)) {
            // Trim leading and trailing whitespace
            size_t start = line.find_first_not_of(" \t\r");
            if (start == string::npos) {
                removed_count++;
                continue;
            }
            
            size_t end = line.find_last_not_of(" \t\r");
            string trimmed = line.substr(start, end - start + 1);
            
            if (!trimmed.empty()) {
                oss << trimmed << "\n";
            }
        }
        
        preprocessed_code = oss.str();
        
        // Remove trailing newline if present
        if (!preprocessed_code.empty() && preprocessed_code.back() == '\n') {
            preprocessed_code.pop_back();
        }
        
        if (removed_count > 0) {
            log_content.append("Removed " + to_string(removed_count) + " empty lines\n");
        }
    }

    // 9) Final cleanup - remove any leftover markers that might have been missed
    {
        // Remove any markers that might have been left
        vector<string> leftover_markers = {
            "section begin]", "section end]", "function begin]", "function end]",
            "[preprocessor", "[functions", "[function", "[superior"
        };
        
        for (const auto& marker : leftover_markers) {
            size_t pos = 0;
            while ((pos = preprocessed_code.find(marker, pos)) != string::npos) {
                // Remove from the previous '[' if exists, or just the marker
                size_t start = preprocessed_code.rfind('[', pos);
                if (start != string::npos && start < pos) {
                    preprocessed_code.erase(start, pos + marker.length() - start);
                    pos = start;
                } else {
                    preprocessed_code.erase(pos, marker.length());
                }
            }
        }
    }

    log_content.append("==========================\n");

    // 10) Write output files
    string base_name = output_file;
    size_t dot_pos = base_name.find_last_of('.');
    if (dot_pos != string::npos) {
        base_name = base_name.substr(0, dot_pos);
    }
    
    string log_file = base_name + ".log";
    string prep_file = base_name + "_prep.txt";
    
    // Write preprocessed code
    if (!FileWork::WriteFile(prep_file, preprocessed_code)) {
        log_content.append("Error: could not write preprocessed file\n");
        FileWork::WriteFile(log_file, log_content);
    }
    
    // Write log
    FileWork::WriteFile(log_file, log_content);
    
    // Set output file for next stages
    output_file = prep_file;
    
    cout << "Preprocessing completed successfully!\n";
    cout << "  Output file: " << prep_file << "\n";
    cout << "  Log file:    " << log_file << "\n";
    
    return 0;
}