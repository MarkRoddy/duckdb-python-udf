
#ifndef LOG_HPP
#define LOG_HPP

#include <cstdlib> // for getenv
#include <iostream>

namespace pyudf {
// Initialize at the start of the program
extern bool _debugEnabled;

void debug(const std::string &msg, bool print_endl = true);
} // namespace pyudf
#endif
