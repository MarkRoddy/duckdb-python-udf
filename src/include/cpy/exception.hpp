
#ifndef CPY_EXCEPTION_HPP
#define CPY_EXCEPTION_HPP

#include <string>
#include <stdexcept>
#include <Python.h>
#include <cpy/object.hpp>

namespace cpy {
class Exception : std::runtime_error {

public:
	static Exception gather();
	// Exception();
	Exception(std::string _type, std::string _message, std::string _traceback);

	Exception(const Exception &);                 // copy
	Exception(Exception &&);                      // move
	Exception &operator=(const Exception &other); // copy assignment

	std::string message() const;
	std::string traceback() const;

private:
	std::string _type;
	std::string _message;
	std::string _traceback;
	
};

} // namespace cpy
#endif // CPY_MODULE_HPP
