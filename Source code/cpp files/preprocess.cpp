#include <precomph.h>

short Preprocess (string input_files[], string& output_file) {
    string source_code = FileWork::ReadFile(input_files[0]);
    string preprocessed_code = source_code;
    string log_content;
    string additional_code;

    log_content.append("=====Preprocessor log=====\n");

    // проверка начала секции
    if (source_code.substr(0, 28) != "[preprocessor section begin]") {
        size_t err_pos = 0;
        int row = FileWork::FindRow(source_code, err_pos);
        int col = FileWork::FindCol(source_code, err_pos);
        log_content.append("Error 76 at row " + to_string(row) + ", col " + to_string(col) + '\n');

        string log_file = input_files[0] + ".log";
        FileWork::WriteFile(log_file, log_content);

        Error::ThrowConsole(76);
        return -1;
    }

    // ##program
    size_t pos = source_code.find("##program");
    if (pos != string::npos) {
        size_t start = source_code.find('"', pos);
        size_t end   = source_code.find('"', start + 1);
        if (start == string::npos || end == string::npos) {
            int row = FileWork::FindRow(source_code, pos);
            int col = FileWork::FindCol(source_code, pos);
            log_content.append("Error: malformed ##program at row " + to_string(row) + ", col " + to_string(col) + '\n');

            string log_file = input_files[0] + ".log";
            FileWork::WriteFile(log_file, log_content);

            Error::ThrowConsole(77);
            return -1;
        }
        output_file = source_code.substr(start + 1, end - start - 1) + ".txt";
        log_content.append("New filename: " + output_file + '\n');
    }

    // ##inaddition
    pos = source_code.find("##inaddition");
    unsigned short inp = 1;
    while (pos != string::npos) {
        size_t start = source_code.find('"', pos);
        size_t end   = source_code.find('"', start + 1);
        if (start == string::npos || end == string::npos) {
            int row = FileWork::FindRow(source_code, pos);
            int col = FileWork::FindCol(source_code, pos);
            log_content.append("Error: malformed ##inaddition at row " + to_string(row) + ", col " + to_string(col) + '\n');

            string log_file = input_files[0] + ".log";
            FileWork::WriteFile(log_file, log_content);

            Error::ThrowConsole(78);
            return -1;
        }

        additional_code = FileWork::ReadFile(input_files[inp]);
        size_t func_sec_before = additional_code.find("[function section begin]");
        size_t func_sec_after  = additional_code.find("[function section end]");
        if (func_sec_before != string::npos && func_sec_after != string::npos) {
            string functions = additional_code.substr(func_sec_before + 23,
                                    func_sec_after - (func_sec_before + 23));
            size_t insert_pos = preprocessed_code.find("[function section begin]");
            if (insert_pos != string::npos) {
                preprocessed_code.insert(insert_pos + 23, functions);
                int row = FileWork::FindRow(preprocessed_code, insert_pos);
                int col = FileWork::FindCol(preprocessed_code, insert_pos);
                log_content.append("Inserted functions from " + input_files[inp] +
                                   " at row " + to_string(row) + ", col " + to_string(col) + '\n');
            }
        }

        size_t pos_perceive = 0;
        while ((pos_perceive = additional_code.find("##perceive", pos_perceive)) != string::npos) {
            size_t line_end = additional_code.find('\n', pos_perceive);
            if (line_end == string::npos) line_end = additional_code.size();
            string macro = additional_code.substr(pos_perceive, line_end - pos_perceive);
            size_t end_of_macro_section = preprocessed_code.find("[preprocessor section end]");
            if (end_of_macro_section != string::npos) {
                preprocessed_code.insert(end_of_macro_section, macro + "\n");
                int row = FileWork::FindRow(preprocessed_code, end_of_macro_section);
                int col = FileWork::FindCol(preprocessed_code, end_of_macro_section);
                log_content.append("Inserted macro {" + macro + "} from " + input_files[inp] +
                                   " at row " + to_string(row) + ", col " + to_string(col) + '\n');
            }
            pos_perceive = line_end + 1;
        }

        inp++;
        pos = source_code.find("##inaddition", pos + 12);
    }

    // ##perceive
    pos = preprocessed_code.find("##perceive");
    while (pos != string::npos) {
        size_t line_end = preprocessed_code.find('\n', pos);
        if (line_end == string::npos) line_end = preprocessed_code.size();

        size_t macro_name_start = pos + 11;
        size_t macro_name_end   = preprocessed_code.find(' ', macro_name_start);
        if (macro_name_end == string::npos || macro_name_end > line_end) {
            int row = FileWork::FindRow(preprocessed_code, pos);
            int col = FileWork::FindCol(preprocessed_code, pos);
            log_content.append("Error: malformed ##perceive at row " + to_string(row) + ", col " + to_string(col) + '\n');

            string log_file = input_files[0] + ".log";
            FileWork::WriteFile(log_file, log_content);

            Error::ThrowConsole(79);
            return -1;
        }

        string macro_name  = preprocessed_code.substr(macro_name_start, macro_name_end - macro_name_start);
        size_t macro_value_start = macro_name_end + 1;
        if (macro_value_start > line_end) {
            int row = FileWork::FindRow(preprocessed_code, pos);
            int col = FileWork::FindCol(preprocessed_code, pos);
            log_content.append("Error: missing macro value at row " + to_string(row) + ", col " + to_string(col) + '\n');

            string log_file = input_files[0] + ".log";
            FileWork::WriteFile(log_file, log_content);

            Error::ThrowConsole(80);
            return -1;
        }
        string macro_value = preprocessed_code.substr(macro_value_start, line_end - macro_value_start);

        size_t pos_to_replace = 0;
        while ((pos_to_replace = preprocessed_code.find(macro_name, pos_to_replace)) != string::npos) {
            if (!(pos_to_replace >= pos && pos_to_replace < line_end)) {
                preprocessed_code.replace(pos_to_replace, macro_name.size(), macro_value);
                int row = FileWork::FindRow(preprocessed_code, pos_to_replace);
                int col = FileWork::FindCol(preprocessed_code, pos_to_replace);
                log_content.append("Inserted " + macro_value + " instead of " + macro_name +
                                   " at row " + to_string(row) + ", col " + to_string(col) + '\n');
                pos_to_replace += macro_value.size();
            } else {
                pos_to_replace = line_end;
            }
        }

        preprocessed_code.erase(pos, line_end - pos + 1);
        pos = preprocessed_code.find("##perceive", pos);
    }

    log_content.append("==========================\n\n");

    string log_file = output_file;
    size_t dot_pos = log_file.rfind(".txt");
    if (dot_pos != string::npos) {
        log_file.replace(dot_pos, 4, ".log");
    } else {
        log_file += ".log";
    }
    FileWork::WriteFile(log_file, log_content);

    string prep_file = output_file;
    dot_pos = prep_file.rfind(".txt");
    if (dot_pos != string::npos) {
        prep_file.replace(dot_pos, 4, "_prep.txt");
    } else {
        prep_file += "_prep.txt";
    }
    FileWork::WriteFile(prep_file, preprocessed_code);
    cout << "Preprocessed successfully!\n";
    cout << prep_file << " was created\n";
    cout << log_file << " log wrote here\n";

    return 0;
}
