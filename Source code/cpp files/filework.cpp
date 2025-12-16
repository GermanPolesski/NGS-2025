#include <precomph.h>

namespace FileWork {
    string ReadFile(const string& filename) {
        ifstream file(filename);
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

    short WriteFile(const string output_file, string data) {
        string filename = output_file.empty() ? "default.log" : output_file;

        ofstream logfile(filename, ios::out);
        if (!logfile.is_open()) {
            Error::ThrowConsole(5);
            return -1;
        }

        logfile << data << endl;
        logfile.close();
        return 0;
    }

        int FindRow(string text, size_t pos) {
        int row = 1;
        for (size_t i = 0; i < pos && i < text.size(); i++) {
            if (text[i] == '\n') row++;
        }
        return row;
        }

        int FindCol(string text, size_t pos) {
        int col = 1;
        for (size_t i = pos; i > 0; i--) {
            if (text[i - 1] == '\n') break;
            col++;
        }
        return col;
        }
        bool fileExists(const std::string& filename) {
        return fs::exists(filename);
        }

        std::string getCurrentDateTime() {
            std::time_t now = std::time(nullptr);
            std::tm* tm_now = std::localtime(&now);
            char buffer[80];
            std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm_now);
            return std::string(buffer);
        }
}