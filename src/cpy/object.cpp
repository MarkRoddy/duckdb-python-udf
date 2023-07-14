
#include <Python.h>
#include <stdexcept>
#include <cpy/object.hpp>
#include <cpy/exception.hpp>

namespace cpy {
cpy::Object list(cpy::Object src) {
	auto pysrc = src.getpy();
	PyObject *pylist = PySequence_List(pysrc);
	Py_DECREF(pysrc);
	if (!pylist) {
		throw std::runtime_error("Failed to convert to list");
	} else {
		cpy::Object list(pylist, true);
		return list;
	}
}

PyObject *Object::getpy() {
	Py_XINCREF(obj);
	return obj;
}

Object::Object() {
	obj = nullptr;
}

Object::Object(PyObject *obj) : obj(obj) {
	Py_XINCREF(obj);
}

Object::Object(PyObject *obj, bool assumeOwnership) : obj(obj) {
	if (!assumeOwnership) {
		Py_INCREF(obj);
	}
}

Object::~Object() {
	Py_XDECREF(obj);
	obj = nullptr;
}

// copy
Object::Object(const Object &other) : obj(other.obj) {
	Py_XINCREF(obj);
}

// Move
Object::Object(Object &&other) : obj(other.obj) {
	other.obj = nullptr;
}

Object &Object::operator=(const Object &other) {
	Py_XINCREF(obj);
	obj = other.obj;
	return *this;
}

cpy::Object Object::attr(const std::string &attribute_name) {
	PyObject *attr = PyObject_GetAttrString(obj, attribute_name.c_str());
	if (!attr) {
		// todo: check if no such attribute, or some other error.
		throw std::runtime_error("Failed to access attribute: " + attribute_name);
	} else {
		return cpy::Object(attr);
	}
}

bool Object::isinstance(cpy::Object cls) {
	PyObject *clsptr = cls.getpy();
	bool is_cls = isinstance(clsptr);
	Py_XDECREF(clsptr);
	return is_cls;
}

bool Object::isinstance(PyObject *cls) {
	int is_cls = PyObject_IsInstance(obj, cls);
	return is_cls;
}

bool Object::isempty() {
	return (nullptr == obj);
}

bool Object::isnone() {
	return (Py_None == obj);
}

bool Object::callable() {
	return PyCallable_Check(obj);
}

cpy::Object Object::call(cpy::Object args) const {
	auto pyargs = args.getpy();
	PyObject *result = PyObject_CallObject(obj, pyargs);
	Py_XDECREF(pyargs);

	if (PyErr_Occurred()) {
		throw cpy::Exception();
	} else if (!result) {
		throw std::runtime_error("Error calling function");
	} else {
		return cpy::Object(result, true);
	}
}

cpy::Object Object::call(cpy::Object args, cpy::Object kwargs) const {
	auto pyargs = args.getpy();
	auto pykwargs = kwargs.getpy();
	PyObject *result = PyObject_Call(obj, pyargs, pykwargs);
	Py_XDECREF(pyargs);
	Py_XDECREF(pykwargs);
	if (PyErr_Occurred()) {
		throw cpy::Exception();
	} else if (!result) {
		throw std::runtime_error("Error calling function");
	} else {
		return cpy::Object(result, true);
	}
}

cpy::Object Object::call(PyObject *pyargs) const {
	auto args = cpy::Object(pyargs);
	return call(args);
}

cpy::Object Object::call(PyObject *pyargs, PyObject *pykwargs) const {
	auto args = cpy::Object(pyargs);
	auto kwargs = cpy::Object(pykwargs);
	return call(args, kwargs);
}

cpy::Object Object::iter() {
	PyObject *py_iter = PyObject_GetIter(obj);
	if (!py_iter) {
		// todo: include python error
		throw std::runtime_error("Failed to convert to an iterator");
	} else {
		cpy::Object itrobj(py_iter);
		Py_DECREF(py_iter);
		return itrobj;
	}
}

std::string Object::str() {
	if (isempty()) {
		throw std::runtime_error("Attempt to convert a nullptr to a string");
	}
	PyObject *py_value_str = PyObject_Str(obj);
	if (!py_value_str) {
		throw std::runtime_error("Failed to convert to a python string");
	}
	PyObject *py_value_unicode = PyUnicode_AsUTF8String(py_value_str);
	if (!py_value_unicode) {
		Py_DECREF(py_value_str);
		throw std::runtime_error("Failed to convert python string to unicode");
	}
	char *str = PyBytes_AsString(py_value_unicode);
	if (!str) {
		Py_DECREF(py_value_unicode);
		Py_DECREF(py_value_str);
		throw std::runtime_error("Failed to convert python unicode to c++ string");
	}
	Py_DECREF(py_value_unicode);
	Py_DECREF(py_value_str);
	return str;
}

} // namespace cpy
