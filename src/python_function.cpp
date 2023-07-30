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
}

PythonFunction::~PythonFunction() {
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
