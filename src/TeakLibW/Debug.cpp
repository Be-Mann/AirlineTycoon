#include "StdAfx.h"

#include <cstdio>
#include <stdexcept>

const char *ExcAssert = "Assert (called from %s:%li) failed!";
const char *ExcGuardian = "Con/Des guardian %lx failed!";
const char *ExcStrangeMem = "Strange new: %li bytes!";
const char *ExcOutOfMem = "Couldn't allocate %li bytes!";
const char *ExcNotImplemented = "Function not implemented!";
const char *ExcImpossible = "Impossible Event %s occured";

HDU Hdu;

HDU::HDU() : Log(nullptr) {
    char *base = SDL_GetBasePath();
    const char *file = "debug.log";
    BUFFER_V<char> path(strlen(base) + strlen(file) + 1);
    strcpy(path.data(), base);
    strcat(path.data(), file);
    // Log = fopen(path.data(), "w");
    Log = stdout;
    SDL_free(base);
}

HDU::~HDU() { Close(); }

void HDU::Close() {
    if (Log != nullptr) {
        fclose(Log);
    }
    Log = nullptr;
}

void HDU::HercPrintf(SLONG /*unused*/, const char *format, ...) {
    if (Log == nullptr) {
        return;
    }
    va_list args;
    va_start(args, format);
    vfprintf(Log, format, args);
    va_end(args);
    fprintf(Log, "\n");
    fflush(Log);
}

void HDU::HercPrintf(const char *format, ...) {
    if (Log == nullptr) {
        return;
    }
    va_list args;
    va_start(args, format);
    vfprintf(Log, format, args);
    va_end(args);
    fprintf(Log, "\n");
    fflush(Log);
}

void HDU::HercPrintfRed(const char *format, ...) {
    if (Log == nullptr) {
        return;
    }
    static SLONG numErrors = 0;
    fprintf(Log, "\e[1;31mERR %d: ", numErrors++);
    va_list args;
    va_start(args, format);
    vfprintf(Log, format, args);
    va_end(args);
    fprintf(Log, "\e[m\n");
    fflush(Log);
}

void HDU::HercPrintfOrange(const char *format, ...) {
    if (Log == nullptr) {
        return;
    }
    static SLONG numWarnings = 0;
    fprintf(Log, "\e[1;33mWARN %d: ", numWarnings++);
    va_list args;
    va_start(args, format);
    vfprintf(Log, format, args);
    va_end(args);
    fprintf(Log, "\e[m\n");
    fflush(Log);
}

void HDU::HercPrintfGreen(const char *format, ...) {
    if (Log == nullptr) {
        return;
    }
    fprintf(Log, "\e[1;32m");
    va_list args;
    va_start(args, format);
    vfprintf(Log, format, args);
    va_end(args);
    fprintf(Log, "\e[m\n");
    fflush(Log);
}

void here(char *file, SLONG line) { Hdu.HercPrintf(0, "Here in %s, line %li", file, line); }

SLONG TeakLibW_Exception(char *file, SLONG line, const char *format, ...) {
    char buffer[128];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    // MessageBeep(0);
    // MessageBeep(0x30u);
    Hdu.HercPrintf(1, "====================================================================");
    Hdu.HercPrintf(0, "Exception in File %s, Line %li:", file, line);
    Hdu.HercPrintf(0, buffer);
    Hdu.HercPrintf(1, "====================================================================");
    Hdu.HercPrintf(0, "C++ Exception thrown. Programm will probably be terminated.");
    // Hdu.Close();
    // DebugBreak();
    throw std::runtime_error(buffer);
}
