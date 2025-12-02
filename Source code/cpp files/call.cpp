#include <precomph.h>

//flags and codes:
//-p: preprocessor (code 0)
//-log: log of translator's work (code 1)
//-l: lexical analyser (code 2)
//-s: syntaxis analyser (code 3)
//-o: object code (code 4)
//-r: run project (code 5)

const char* flags[] = {"-p", "-log", "-l", "-s", "-o", "-r"};
const short flagCodes[] = {0, 1, 2, 3, 4, 5};
const unsigned short numFlags = sizeof(flags) / sizeof(flags[0]);

short getFlagCode(const char* arg) {
	for (unsigned short i = 0; i < numFlags; i++) {
		if (strcmp(arg, flags[i]) == 0) {
			return flagCodes[i];
		}
	}
	return -1;
}

short processCall (int argc, char* argv[], string input_files[], string& output_file) {
	if (argc < 3) {	//There is no flag of file
		Error::ThrowConsole(1);
		return -1;
	}
	int i = 1;
	while (i < argc && argv[i][0] != '-') {
		i++;
	}
	if (i == 1) {
		Error::ThrowConsole(0);	//There is no input files
		return -1;
	}
	if (i >= argc) {
		Error::ThrowConsole(1);	//There is no file
		return -1;
	}
	short code = getFlagCode(argv[i]);
	if (code == -1) {
		Error::ThrowConsole(3);	//Incorrect flag
		return -1;
	}
	if (i+1 < argc && strcmp(argv[i+1], "-f") == 0) {
		if (i+2 >= argc) {
			Error::ThrowConsole(2);
			return -1;
		}
	}

	int j = 1;
	while(argv[j][0] != '-') {
		input_files[j - 1] = argv[j];
		j++;
	}
	if (i+1 < argc) {
		output_file = argv[i+1];
	}

	return code;
}
