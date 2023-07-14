
#ifndef CPY_EXCEPTION_HPP
#define CPY_EXCEPTION_HPP

#include <string>
#include <Python.h>
#include <cpy/object.hpp>

namespace cpy {
class Exception {

public:
	Exception();
	Exception(cpy::Object pytype, cpy::Object pymessage, cpy::Object pytraceback, std::string _type,
	          std::string _message, std::string _traceback);

	Exception(const Exception &);                 // copy
	Exception(Exception &&);                      // move
	Exception &operator=(const Exception &other); // copy assignment

	std::string message() const;
	std::string traceback() const;

private:
	cpy::Object pytype;
	cpy::Object pymessage;
	cpy::Object pytraceback;

	std::string _type;
	std::string _message;
	std::string _traceback;
};

} // namespace cpy
#endif // CPY_MODULE_HPP
