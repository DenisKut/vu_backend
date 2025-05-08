#include "pch.h"
#include <cxxabi.h>

QString Utils::demangleClassName(const char* name) {
    int status = 0;
    char* demangled = abi::__cxa_demangle(name, nullptr, nullptr, &status);
    QString full = (status == 0 && demangled) ? QString(demangled) : QString(name);
    free(demangled);
    int pos = full.lastIndexOf("::");
    return (pos != -1) ? full.mid(pos + 2) : full;
}
