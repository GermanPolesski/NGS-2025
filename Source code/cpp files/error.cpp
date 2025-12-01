#include <precomph.h>

namespace Error {
	//Errors 0-30: incorrect call to compiler
	//Errors 31-75: problems with output file
	//Errors 76-100: incorrect using of preprocessor
	//Errors 101-200: lexical analysis errors
	//Errors 201-300: syntaxis analysis errors
	//Errors 301-400: Semantical analysis problems
	//Errors 401-500: Object code generation problems
	//Errors 501-1000: Other errors
	error errors[1000] {
		error(0, "No input files in call"),
		error(1, "No flag in call"),
		error(2, "No files found"),
		error(3, "Incorrect flag"),
		error(4, "Unknown error with call to compiler"),
		error(999, "Error caused by an error"),
		error(1000, "Unknown error")
	};
	error getError (unsigned short id) {
		if (id > 1000) {
			return errors[999];
		}
		return errors[id];
	}
	void ThrowConsole (unsigned short id) {
		cout << '\n' << errors[id].id << ": " << errors[id].message;
	}
}
