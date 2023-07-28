
#include <string>
#include <Python.h>
#include <stdexcept>
#include <cpy/object.hpp>
#include <cpy/exception.hpp>

namespace cpy {
std::string Exception::message() const {
	return _message;
}

std::string Exception::traceback() const {
	return _traceback;
}

// copy
Exception::Exception(const Exception &other) : Exception(other._type, other._message, other._traceback) {
}

// move
Exception::Exception(Exception &&other) : Exception(other._type, other._message, other._traceback) {
}

// copy assignment
Exception &Exception::operator=(const Exception &other) {
	_type = other._type;
	_message = other._message;
	_traceback = other._traceback;
	return *this;
}

Exception::Exception(std::string _type, std::string _message, std::string _traceback)
    // : std::runtime_error(_message), _type(_type), _message(_message), _traceback(_traceback) {
	: std::runtime_error(_message), _type(_type), _message(_message), _traceback(_traceback) {
}

	
Exception Exception::gather() {
	if (!PyErr_Occurred()) {
		throw std::runtime_error("Attempt create exception object w/o underlying Python exception");
	}

	PyObject *po_type, *po_value, *po_traceback;
	PyErr_Fetch(&po_type, &po_value, &po_traceback);	

	cpy::Object pytype;
	if (po_type) {
		pytype = cpy::Object(po_type, true);
	}
	
	cpy::Object pymessage;
	if (po_value) {
		pymessage = cpy::Object(po_value, true);
	}

	cpy::Object pytraceback;
	if (po_traceback) {
		// pytraceback = cpy::Object(po_traceback, true);
	}

	std::string type;
	std::string message;
	std::string traceback;
	if (!pytype.isempty()) {
		type = pytype.str();
	}
	if (!pymessage.isempty()) {
		message = pymessage.str();
	}
	if (!pytraceback.isempty()) {
		traceback = pytraceback.str();
	}
	PyErr_Clear();
	return Exception(type, message, traceback);
}
} // namespace cpy
