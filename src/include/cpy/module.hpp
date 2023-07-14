
#ifndef CPY_MODULE_HPP
#define CPY_MODULE_HPP

#include <string>
#include <Python.h>
#include <cpy/object.hpp>

namespace cpy {
class Module : public cpy::Object {

public:
	Module(const std::string &module_name);

private:
	std::string module_name_;
	static PyObject *doImport(const std::string &module_name);
};

} // namespace cpy
#endif // CPY_MODULE_HPP
