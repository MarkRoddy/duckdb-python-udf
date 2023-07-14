
#ifndef CPY_OBJECT_HPP
#define CPY_OBJECT_HPP

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
	PyObject *getpy();

	cpy::Object attr(const std::string &attribute_name);
	bool isinstance(cpy::Object cls);
	bool isinstance(PyObject *cls);
	bool isempty();
	bool isnone();

	bool callable();
	cpy::Object call(cpy::Object args) const;
	cpy::Object call(cpy::Object args, cpy::Object kwargs) const;
	cpy::Object call(PyObject *pyargs) const;
	cpy::Object call(PyObject *pyargs, PyObject *pykwargs) const;

	cpy::Object iter();

	std::string str();

protected:
	PyObject *obj;
};

cpy::Object list(cpy::Object src);
} // namespace cpy
#endif // CPY_OBJECT_HPP
