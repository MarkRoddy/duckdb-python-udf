

#include <cstdarg>
#include <Python.h>
#include <stdexcept>

#include <log.hpp>
#include <cpy/object.hpp>
#include <cpy/exception.hpp>

namespace cpy {
bool _disable_cpy_deallocation = std::getenv("PYTABLES_DISABLE_CPY_AUTO_DEALLOCATION") != nullptr;
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

cpy::Object tuple(std::vector<cpy::Object> items) {
	PyObject *tpl = PyTuple_New(items.size());
	if(!tpl) {
		throw std::runtime_error("Failed to allocate new tuple");
	}
	for (size_t i = 0; i < items.size(); i++) {
		// Reference Counting Note:
		// cpy::Object.getpy() increments the reference so this function owns a ref to
		// the PyObject value. But PyTuple_SetItem() "steals" (their word) a reference,
		// ie - it doesn't increment. So we effectively hand the reference from getpy()
		// to SetItem(), so there's no reference decrementation necessary.		
		PyObject *pyval = items[i].getpy();
		auto result = PyTuple_SetItem(tpl, i, pyval);
		if (PyErr_Occurred()) {
			throw cpy::Exception::gather();
		} else if(0 != result) {
			throw std::runtime_error("Failed to allocate python tuple");
		}
	}
	cpy::Object tobj(tpl, true);
	return tobj;
}

std::string Object::debug_tostring() {
	auto iptr = reinterpret_cast<std::uintptr_t>(obj);
	return "cpy::Object(" + std::to_string(iptr) + ")";
}
	
PyObject *Object::getpy() const {
	Py_INCREF(obj);
	return obj;
}

Object::Object() {
	obj = nullptr;
	debug_disable_deallocation = false;
	_repr = repr();
}

Object::Object(PyObject *obj) : obj(obj) {
	Py_XINCREF(obj);
	debug_disable_deallocation = false;
	_repr = repr();
}

Object::Object(PyObject *obj, bool assumeOwnership) : obj(obj) {
	if (!assumeOwnership) {
		Py_INCREF(obj);
	}
	debug_disable_deallocation = false;
	_repr = repr();
}
	
Object::~Object() {
	if (_disable_cpy_deallocation) {
		pyudf::debug("Skipping Py_XDECREF as it is globally disabled");
	} else if (debug_disable_deallocation) {
		pyudf::debug("Skipping Py_XDECREF as it is disabled for this object");
	} else {
		if(!isempty()) {
			pyudf::debug("Decrementing a Python Object ref count on: '", false);
			pyudf::debug(_repr, false);
			pyudf::debug("'");
		}
		Py_XDECREF(obj);
	}
	obj = nullptr;
}

// copy
Object::Object(const Object &other) : obj(other.getpy()), debug_disable_deallocation(other.debug_disable_deallocation) {
	// Note we call getpy() to ensure that the Py_XINCREF() is called before
	// it is assigned (and theoretically usable). It is unclear if that should
	// be a concern or not.
_repr = repr();	
}

// Move
Object::Object(Object &&other) : obj(other.obj), debug_disable_deallocation(other.debug_disable_deallocation) {
	other.obj = nullptr;
	_repr = repr();
}

Object &Object::operator=(const Object &other) {
	Py_XINCREF(obj);
	obj = other.obj;
	debug_disable_deallocation = other.debug_disable_deallocation;
	_repr = repr();
	return *this;
}

cpy::Object Object::attr(const std::string &attribute_name) const {
	PyObject *attr = PyObject_GetAttrString(obj, attribute_name.c_str());
	if (!attr) {
		// todo: check if no such attribute, or some other error.
		throw std::runtime_error("Failed to access attribute: " + attribute_name);
	} else {
		// std::string child_name;
		// if(name.empty()) {
		// 	child_name = "." + attribute_name;
		// } else {
		// 	child_name = name + "." + attribute_name;
		// }
		return cpy::Object(attr, true);
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

bool Object::isempty() const {
	return (nullptr == obj);
}

bool Object::isnone() const {
	return (Py_None == obj);
}

bool Object::callable() const {
	return PyCallable_Check(obj);
}

cpy::Object Object::call(cpy::Object args) const {
	if(isempty()) {
		throw std::runtime_error("Object::call() on a null pointer");
	}
	if(args.isempty()) {
		throw std::runtime_error("Object::call() with args that is a null pointer");
	}

	PyObject* pyargs = args.getpy();
	cpy::Object result = call(pyargs);
	Py_DECREF(pyargs);
	return result;
}

cpy::Object Object::call(cpy::Object args, cpy::Object kwargs) const {
	auto pyargs = args.getpy();
	auto pykwargs = kwargs.getpy();
	PyObject *result = PyObject_Call(obj, pyargs, pykwargs);
	Py_XDECREF(pyargs);
	Py_XDECREF(pykwargs);
	if (PyErr_Occurred()) {
		Py_XDECREF(result);
		throw cpy::Exception::gather();
	} else if (!result) {
		throw std::runtime_error("Error calling function");
	} else {
		return cpy::Object(result, true);
	}
}

	cpy::Object Object::call() const {
		PyObject *result = PyObject_CallObject(obj, NULL);
		if (PyErr_Occurred()) {
			Py_XDECREF(result);
			throw cpy::Exception::gather();
			// throw std::runtime_error(err.message());
		} else if (!result) {
			throw std::runtime_error("Error calling function");
		} else {
			return cpy::Object(result, true);
		}	
	}

cpy::Object Object::call(PyObject *pyargs) const {
	// cpy::Object args = cpy::Object(pyargs, false, "args for " + name + "()");
	// return call(args);
		// PyObject* pyargs = args.getpy();
	if (!PyTuple_Check(pyargs)) {
		throw std::runtime_error("Arguments passed in as non-Python Tuple");
	}
	// pyargs = PyTuple_New(0);
	PyObject* pykwargs = PyDict_New();
	// PyObject *result = PyObject_CallObject(obj, pyargs);
	// PyObject* result = PyObject_Call(obj, pyargs, pykwargs);
	return call(pyargs, pykwargs);
	// Py_XDECREF(pyargs);

	// if (PyErr_Occurred()) {
	// 	throw cpy::Exception::gather();
	// } else if (!result) {
	// 	throw std::runtime_error("Error calling function");
	// } else {
	// 	return cpy::Object(result, true);
	// }
}

cpy::Object Object::call(PyObject *pyargs, PyObject *pykwargs) const {
	auto args = cpy::Object(pyargs, false);
	auto kwargs = cpy::Object(pykwargs, false);
	return call(args, kwargs);
}
	
	cpy::Object Object::callattr(std::string name) const {
		cpy::Object func = attr(name);
		return func.call();
	}
	cpy::Object Object::callattr(std::string name, cpy::Object pyargs) const {
		cpy::Object func = attr(name);
		return func.call(pyargs);
	}
	cpy::Object Object::callattr(std::string name, cpy::Object pyargs, cpy::Object pykwargs) const {
		cpy::Object func = attr(name);
		return func.call(pyargs, pykwargs);
	}
	
	cpy::Object Object::callattr(std::string name, PyObject* pyargs) const {
		cpy::Object func = attr(name);
		return func.call(pyargs);
	}
	
	cpy::Object Object::callattr(std::string name, PyObject* pyargs, PyObject* pykwargs) const {
		cpy::Object func = attr(name);
		return func.call(pyargs, pykwargs);
	}

cpy::Object Object::iter() {
	PyObject *py_iter = PyObject_GetIter(obj);
	if (!py_iter) {
		// todo: include python error
		throw std::runtime_error("Failed to convert to an iterator");
	} else {
		cpy::Object itrobj(py_iter, true);
		return itrobj;
	}
}

std::string Object::repr() {
	if (isempty()) {
		return "<nullptr>";
	}
	return str();
	PyObject *py_repr = PyObject_Repr(obj);
	if (!py_repr) {
		throw std::runtime_error("Failed to convert to a python string");
	}
	cpy::Object repr(py_repr);
	return repr.str();
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

int32_t Object::toInt() {
	if (!PyLong_Check(obj)) {
		throw std::runtime_error("Can not convert non-PyLong to int");
	} else {
		// todo: error check?
		return (int32_t)PyLong_AsLong(obj);
	}
}
} // namespace cpy
