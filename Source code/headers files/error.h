#ifndef ERROR_H
#define ERROR_H

#include <cstring>
#include <iostream>

namespace Error {
    struct error_info {
        unsigned short id;
        unsigned short line;
        unsigned short col;
        char message[100];
    
        error_info() : id(0), line(0), col(0) {
            message[0] = '\0';
        }
        error_info(unsigned short i, const char* msg) : id(i), line(0), col(0) {
            strncpy(message, msg, sizeof(message) - 1);
            message[sizeof(message) - 1] = '\0';
        }

        error_info(unsigned short i, unsigned short l, unsigned short c, const char* msg) 
            : id(i), line(l), col(c) {
            strncpy(message, msg, sizeof(message) - 1);
            message[sizeof(message) - 1] = '\0';
        }
    };
    
    error_info getErrorID(unsigned short id);
    void ThrowConsole(unsigned short id, bool critical = false);
}

#endif // ERROR_H