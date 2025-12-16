#include <precomph.h>

static bool allowed[256];

void initWindows1251Table() {
    for (int i = 0; i < 256; i++) {
        allowed[i] = false;
    }

    //ASCII printable characters
    for (int i = 0x20; i <= 0x7E; i++) {
        allowed[i] = true;
    }
    
    //special characters
    allowed['\n'] = true;  // 0x0A
    allowed['\r'] = true;  // 0x0D
    allowed['\t'] = true;  // 0x09

    //Windows-1251 Cyrillic range
    for (int i = 0x80; i <= 0xFF; i++) {
        allowed[i] = true;
    }
}

bool isWindows1251(const std::string& text, const std::string& logfile_name) {
    static bool initialized = false;
    if (!initialized) {
        initWindows1251Table();
        initialized = true;
    }

    std::string log_content = "Encoding check:\n";
    
    for (size_t i = 0; i < text.size(); i++) {
        unsigned char c = static_cast<unsigned char>(text[i]);
        
        if (allowed[c]) {
            continue;
        }
        
        int row = FileWork::FindRow(text, i);
        int col = FileWork::FindCol(text, i);
        std::string message = "Invalid character (code " + std::to_string((int)c) +
                              ") at row " + std::to_string(row) +
                              ", col " + std::to_string(col) + "\n";
        
        log_content.append(message);
        std::cout << message;
        FileWork::WriteFile(logfile_name, log_content);
        return false;
    }
    
    log_content.append("All characters are valid Windows-1251.\n");
    FileWork::WriteFile(logfile_name, log_content);
    return true;
}
