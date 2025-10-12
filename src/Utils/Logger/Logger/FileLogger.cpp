#include "src/Utils/Logger/Logger.hpp"

void Logger::Log(const char* Channel, const char* Format, ...) {

	char filename[128];
	sprintf(filename, "C:\\%s.txt", Channel);

    FILE* fp = fopen(filename, "a");
    if (fp == NULL) {
        return; // optionally log or handle error
    }

    va_list args;
    va_start(args, Format);
    
    vfprintf(fp, Format, args);      // Write formatted content
    
    va_end(args);
    fclose(fp);
}

