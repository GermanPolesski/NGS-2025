#include <precomph.h>

static bool allowed[256];

void initWindows1251Table () {
	for (int i = 0; i < 256; i++) {
		allowed[i] = false;
	}

	for (int i = 0x20; i <= 0x7E; i++) {	//ASCII
		allowed[i] = true;
	}
	allowed['\n'] = true;
	allowed['\r'] = true;
	allowed['\t'] = true;

	for (int i = 0x80; i <= 0xFF; i++) {	//Windows-1251 (cyrillic)
		allowed[i] = true;
	}
}

bool isWindows1251(const std::string& text, const std::string& logfile_name) {
    for (size_t i = 0; i < text.size(); i++) {
        unsigned char c = static_cast<unsigned char>(text[i]);

        if ((c >= 0x20 && c <= 0x7E) || c == '\n' || c == '\r' || c == '\t') {
            continue;
        }

        if (c >= 0x80) {
            if (c >= 0xC0 && c <= 0xF7) {
                int row = FileWork::FindRow(text, i);
                int col = FileWork::FindCol(text, i);
                std::string message = "Invalid UTF-8 sequence start (code " +
                                      std::to_string((int)c) + ") at row " +
                                      std::to_string(row) + ", col " +
                                      std::to_string(col) + "\n";
                std::cout << message;
                FileWork::WriteFile(logfile_name, message);
                return false;
            }
            continue;
        }

        int row = FileWork::FindRow(text, i);
        int col = FileWork::FindCol(text, i);
        std::string message = "Invalid symbol (code " + std::to_string((int)c) +
                              ") at row " + std::to_string(row) +
                              ", col " + std::to_string(col) + "\n";
        std::cout << message;
        FileWork::WriteFile(logfile_name, message);
        return false;
    }
    return true;
}
