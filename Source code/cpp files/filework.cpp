#include <precomph.h>

string ReadFile (const string& filename) {
	fstream file(filename, ios::in);
	if (!file.is_open()) {
		Error::ThrowConsole(5);
		return "";
	}

	string content;
	string line;
	while (getline(file, line)) {
		content += line + '\n';
	}

	file.close();
	return content;
}
