
#include <Python.h>
#include <stdexcept>
#include <cpy/module.hpp>
#include <cpy/function.hpp>

namespace cpy {
cpy::Function import(const std::string &module_name, const std::string &function_name) {
	auto module = cpy::Module(module_name);
	cpy::Object func = module.attr(function_name);
	if (!func.callable()) {
		throw std::runtime_error("Attribute found in module, but it is not callable");
	} else {
		cpy::Function funcObj(func.getpy(), true);
		return funcObj;
	}
}
Function::Function(cpy::Object src) : cpy::Object(src.getpy(), true) {
}
} // namespace cpy
