
#ifndef CPY_OBJECT_HPP
#define CPY_OBJECT_HPP

#include <vector>
#include <string>
#include <Python.h>

namespace cpy {
class Object {
public:
	Object();
	Object(PyObject *obj);
	Object(PyObject *obj, bool assumeOwnership);
	
	~Object();
	Object(const Object &);                 // copy
	Object(Object &&);                      // move
	Object &operator=(const Object &other); // copy assignment

	/* Grab the pointer to the underlying python object, increments its reference count */
	PyObject *getpy() const;

	cpy::Object attr(const std::string &attribute_name) const;
	bool isinstance(cpy::Object cls);
	bool isinstance(PyObject *cls);
	bool isempty() const;
	bool isnone() const;

	bool callable() const;
	cpy::Object call() const;

	cpy::Object call(cpy::Object args) const;
	cpy::Object call(cpy::Object args, cpy::Object kwargs) const;

	cpy::Object callattr(std::string name) const;
	cpy::Object callattr(std::string name, cpy::Object pyargs) const;
	cpy::Object callattr(std::string name, cpy::Object pyargs, cpy::Object pykwargs) const;	

	// Kill these once we've refactored all code directly using CPython functions
	cpy::Object call(PyObject* pyargs) const;
	cpy::Object call(PyObject* pyargs, PyObject* pykwargs) const;
	cpy::Object callattr(std::string name, PyObject* pyargs) const;
	cpy::Object callattr(std::string name, PyObject* pyargs, PyObject* pykwargs) const;

	cpy::Object iter();
	std::string str();
	std::string repr();

	int32_t toInt();

	// Debugging tool to aid in GC related issues.
	bool debug_disable_deallocation;
	std::string debug_tostring();
protected:
	PyObject *obj;
	// std::string name;
	std::string _repr;
};

cpy::Object list(cpy::Object src);
cpy::Object tuple(std::vector<cpy::Object> items);
} // namespace cpy
#endif // CPY_OBJECT_HPP
