#include <duckdb.hpp>
#include <python_function.hpp>
#include <python_exception.hpp>
#include <stdexcept>
#include <typeinfo>

namespace pyudf {

PythonFunction::PythonFunction(const std::string &function_specifier) {
	std::string module_name;
	std::string function_name;
	std::tie(module_name, function_name) = parse_func_specifier(function_specifier);
	init(module_name, function_name);
}

PythonFunction::PythonFunction(const std::string &module_name, const std::string &function_name) {
	init(module_name, function_name);
}

PythonFunction::PythonFunction(py::object function) : functionObj(function) {
}

void PythonFunction::init(const std::string &module_name, const std::string &function_name) {
	module_name_ = module_name;
	function_name_ = function_name;
	py::module_ module;
	try {
		module = py::module_::import(module_name.c_str());
	} catch (py::error_already_set &e) {
		e.restore();
		PyErr_Clear();
		throw std::runtime_error("Failed to import module: " + module_name);
	}

	if (!py::hasattr(module, function_name.c_str())) {
		throw std::runtime_error("Failed to find function: " + function_name);
	}

	py::object maybe_function = module.attr(function_name.c_str());
	try {
		functionObj = maybe_function.cast<py::function>();
	} catch (py::error_already_set &e) {
		e.restore();
		PyErr_Clear();
		throw std::runtime_error("Function is not callable: " + function_name);
	}
	init_old(module_name, function_name);
}

void PythonFunction::init_old(const std::string &module_name, const std::string &function_name) {
	module_name_ = module_name;
	function_name_ = function_name;
	module = nullptr;
	function = nullptr;
	PyObject *module_obj = PyImport_ImportModule(module_name.c_str());
	if (!module_obj) {
		PyErr_Print();
		throw std::runtime_error("Failed to import module: " + module_name);
	}

	module = module_obj;

	PyObject *function_obj = PyObject_GetAttrString(module, function_name.c_str());
	if (!function_obj) {
		PyErr_Print();
		throw std::runtime_error("Failed to find function: " + function_name);
	}

	if (!PyCallable_Check(function_obj)) {
		Py_DECREF(function_obj);
		throw std::runtime_error("Function is not callable: " + function_name);
	}
	function = function_obj;
}

PythonFunction::~PythonFunction() {
	Py_DECREF(function);
	Py_DECREF(module);
}

py::object PythonFunction::call(py::tuple args) const {
	return functionObj(*args);
}

py::object PythonFunction::call(py::tuple args, py::dict kwargs) const {
	return functionObj(*args, **kwargs);
}

std::pair<std::string, std::string> parse_func_specifier(std::string specifier) {
	auto delim_location = specifier.find(":");
	if (delim_location == std::string::npos) {
		throw duckdb::InvalidInputException("Function specifier lacks a ':' to delineate module and function");
	} else {
		auto module = specifier.substr(0, delim_location);
		auto function = specifier.substr(delim_location + 1, (specifier.length() - delim_location));
		return {module, function};
	}
}

} // namespace pyudf
