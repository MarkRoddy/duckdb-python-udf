
#include <Python.h>
#include <stdexcept>
#include <cpy/module.hpp>

namespace cpy {
Module::Module(const std::string &module_name) : cpy::Object(doImport(module_name), true) {
	module_name_ = module_name;
}

PyObject *Module::doImport(const std::string &module_name) {
	auto obj = PyImport_ImportModule(module_name.c_str());
	if (!obj) {
		// PyErr_Print();
		// todo: Throw an exception class that gives access to the Python error
		throw std::runtime_error("Failed to import module: " + module_name);
	} else {
		return obj;
	}
}

} // namespace cpy
