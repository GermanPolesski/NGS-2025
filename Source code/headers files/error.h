#include <cstring>

namespace Error {
	struct error {
		unsigned short id;
		unsigned short line;
		unsigned short col;
		char message[100];
	
		error() : id(0), line(0), col(0) {
			message[0] = '\0';
		}
		error(unsigned short i, const char* msg) : id(i), line(0), col() {
			strncpy(message, msg, sizeof(message) - 1);
			message[sizeof(message) - 1] = '\0';
		}

		 error(unsigned short i, unsigned short l, unsigned short c, const char* msg) : id(i), line(l), col(c)
		{
			strncpy(message, msg, sizeof(message) - 1);
			message[sizeof(message) - 1] = '\0';
		}
	};
	error getError (unsigned short id);
	void ThrowConsole (unsigned short id);
}

