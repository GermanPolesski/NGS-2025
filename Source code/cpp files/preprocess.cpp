#include <precomph.h>

short Preprocess (string input_files[], string& output_file) {
	string source_code = ReadFile(input_files[0]);
	string preprocessed_code = source_code;
	string additional_code;

	cout << "\n===Source code===\n";
	cout << source_code << endl;
	cout << "==================\n\n";

	cout << "=====Preprocessor log=====\n";

	if (source_code.substr(0, 28) != "[preprocessor section begin]") {
		Error::ThrowConsole(76);
	}
	
	size_t pos = source_code.find("##program");	//##program
	if (pos != string::npos) {
		size_t start = source_code.find('"', pos);
		size_t end = source_code.find('"', start + 1);
		if (start != string::npos && end != string::npos) {
			output_file = source_code.substr(start + 1, end - start - 1);
			output_file.append(".txt");
			cout << "New output file name: " << output_file << endl;
		}
	}
	
	pos = source_code.find("##inaddition");	//##inaddition
	unsigned short inp = 1;
	while (pos != string::npos) {
		size_t start = source_code.find('"', pos);
		size_t end = source_code.find('"', start + 1);
		if (start != string::npos && end != string::npos) {
			additional_code = ReadFile(input_files[inp]);
			size_t func_sec_after = additional_code.find("[function section end]");
			size_t func_sec_before = additional_code.find("[function section begin]");
			if (func_sec_before != string::npos && func_sec_after != string::npos) {
				string functions = additional_code.substr(func_sec_before + 23, func_sec_after - (func_sec_before + 23));
				preprocessed_code.insert(source_code.find("[function section begin]") + 23, functions);
				cout << "Succesfully inserted functions from " << input_files[inp] << endl;
			}

			size_t pos_perceive = 0;
			while ((pos_perceive = additional_code.find("##perceive", pos_perceive)) != string::npos) {
				size_t line_end = additional_code.find('\n', pos_perceive);
				if (line_end == string::npos) {
					line_end = additional_code.size();
				}
				string macro = additional_code.substr(pos_perceive, line_end - pos_perceive);
				size_t end_of_macro_section = preprocessed_code.find("[preprocessor section end]");
				if (end_of_macro_section != string::npos) {
					preprocessed_code.insert(end_of_macro_section, macro + "\n");
					cout << "Succesfully inserted macro {" << macro << "} from " << input_files[inp] << endl;
				}
				pos_perceive = line_end;
			}
		}
		inp++;
		pos = source_code.find("##inaddition", pos + 12);
	}
	
	pos = preprocessed_code.find("##perceive"); // ##perceive
	while (pos != string::npos) {
		size_t line_end = preprocessed_code.find('\n', pos);
		if (line_end == string::npos) {
			line_end = preprocessed_code.size();
		}

		size_t macro_name_start = pos + 11;
		size_t macro_name_end = preprocessed_code.find(' ', macro_name_start);
		if (macro_name_end == string::npos || macro_name_end > line_end) {
			break;
		}

 		string macro_name  = preprocessed_code.substr(macro_name_start,
		macro_name_end - macro_name_start);
		size_t macro_value_start = macro_name_end + 1;
		if (macro_value_start > line_end) {
			break;
		}
		string macro_value = preprocessed_code.substr(macro_value_start, line_end - macro_value_start);

		size_t pos_to_replace = 0;
		while ((pos_to_replace = preprocessed_code.find(macro_name, pos_to_replace)) != string::npos) {
			if (!(pos_to_replace >= pos && pos_to_replace < line_end)) {
				preprocessed_code.replace(pos_to_replace, macro_name.size(), macro_value);
				pos_to_replace += macro_value.size();
			} else {
				pos_to_replace = line_end;
			}
	}

	cout << "Inserted " << macro_value << " instead of " << macro_name << endl;

	pos = preprocessed_code.find("##perceive", line_end);
	}


	cout << "=========================\n\n";

	cout << "\n===Preprocessed code===\n";
	cout << preprocessed_code << endl;
	cout << "=========================\n";
	return 0;
}
