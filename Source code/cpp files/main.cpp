#include <precomph.h>

int main(int argc, char* argv[]) {
	try {
		string input_files[10];
		string output_filename;

		short call = processCall(argc, argv, input_files, output_filename);
		if (call == -1) {
			return -1;
		}
		switch(call) {
			case 0:
				cout << "Preprocessing\n";
				Preprocess(input_files, output_filename);
				break;
			case 1:
				cout << "Writing log file\n";
				break;
			case 2:
				cout << "Lexical analysis\n";
				break;
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
