#include <precomph.h>

//flags and codes:
//-p: preprocessor (code 0)
//-log: log of translator's work (code 1)
//-l: lexical analyser (code 2)
//-s: syntaxis analyser (code 3)
//-o: object code (code 4)
//-r: run project (code 5)

unsigned char flags[5] = {'p', 'o', 'd', 'l', 's'};

short processCall (int argc, char* argv[]) {
	if (argc < 3) {
		Error::ThrowConsole(1);
		return 0;
	}
	return 0;
}
