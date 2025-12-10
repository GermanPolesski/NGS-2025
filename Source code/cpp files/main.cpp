#include <precomph.h>

int main(int argc, char* argv[]) {
	try {
		string input_files[10];
		string output_filename;

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
				cout << "Compilation Terminated\n";
				return -1;
			}
		}

		switch(call) {
			case 0: 
				{
				cout << "Preprocessing...\n";
				if (!Preprocess(input_files, output_filename) == 0) {
					cout << "Unknown error occured. Preproccesing process was terminated.\n";
					return 1;
				}
				break; 
				}
			case 1:
				cout << "Writing log file\n";
				break;
			case 2: {
                                cout << "Lexical analysis...\n";
				
				//if there is no preprocessed file
				if (input_files[1].find("_prep") == string::npos) {
					cout << "Preprocessing...\n";
					if (Preprocess(input_files, output_filename) != 0) {
						Error::ThrowConsole(78);
						return 1;
					}
				}

				string prep_file = output_filename;
				size_t pos = prep_file.rfind(".txt");
				if (pos != string::npos) {
					prep_file.replace(pos, 4, "_prep.txt"); } else {
						prep_file += "_prep.txt";
					}

				if (LexAnalysis(prep_file, output_filename) == 0) {
					cout << "Lexical analysis completed successfully!\n"; } else {
						Error::ThrowConsole(101);
						return 1;
					}

				break;
				}
			case 3:
				cout << "Syntaxis analysis\n";
				break;
			case 4:
				cout << "Wirting obj file\n";
				break;
			case 5:
				cout << "Running\n";
				break;
		}
	}
	catch (const char* e) {
		cout << e << '\n';
	}

	return 0;
}
