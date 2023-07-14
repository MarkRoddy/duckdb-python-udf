

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
Exception::Exception(const Exception &other)
    : Exception(other.pytype, other.pymessage, other.pytraceback, other._type, other._message, other._traceback) {
}

// move
Exception::Exception(Exception &&other)
    : Exception(other.pytype, other.pymessage, other.pytraceback, other._type, other._message, other._traceback) {
}

// copy assignment
Exception &Exception::operator=(const Exception &other) {
	pytype = other.pytype;
	pymessage = other.pymessage;
	pytraceback = other.pytraceback;
	_type = other._type;
	_message = other._message;
	_traceback = other._traceback;
	return *this;
}

Exception::Exception(cpy::Object pytype, cpy::Object pymessage, cpy::Object pytraceback, std::string _type,
                     std::string _message, std::string _traceback)
    : pytype(pytype), pymessage(pymessage), pytraceback(pytraceback), _type(_type), _message(_message),
      _traceback(_traceback) {
}

Exception::Exception() {
	if (!PyErr_Occurred()) {
		throw std::runtime_error("Attempt create exception object w/o underlying Python exception");
	}

	PyObject *po_type, *po_value, *po_traceback;
	PyErr_Fetch(&po_type, &po_value, &po_traceback);

	if (po_type) {
		pytype = cpy::Object(po_type, true);
	}
	if (po_value) {
		pymessage = cpy::Object(po_value, true);
	}
	if (po_traceback) {
		// pytraceback = cpy::Object(po_traceback, true);
	}

	if (!pytype.isempty()) {
		_type = pytype.str();
	}
	if (!pymessage.isempty()) {
		_message = pymessage.str();
	}
	if (!pytraceback.isempty()) {
		_traceback = pytraceback.str();
	}
	PyErr_Clear();
}
} // namespace cpy
