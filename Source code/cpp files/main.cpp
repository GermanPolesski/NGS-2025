#include <precomph.h>

int main(int argc, char* argv[]) {
	try {
		if (!strcmp(argv[1], "test")) {
			cout << "Caused";
			Error::ThrowConsole(0);
		}	
	}
	catch (const char* e) {
		cout << e << '\n';
	}

	return 0;
}
