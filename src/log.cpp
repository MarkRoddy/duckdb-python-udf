
#include <cstdlib> // for getenv
#include <iostream>
#include <log.hpp>

namespace pyudf {
bool _debugEnabled = std::getenv("PYTABLES_DEBUG") != nullptr;
void debug(const std::string &msg, bool print_endl) {
	if (_debugEnabled) {
		if (print_endl) {
			std::cerr << msg << std::endl;
		} else {
			std::cerr << msg;
		}
	}
}
} // namespace pyudf
