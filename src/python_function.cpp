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

void PythonFunction::init(const std::string &module_name, const std::string &function_name) {
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

std::pair<py::object, PythonException *> PythonFunction::call(py::tuple args, py::dict kwargs) const {
	PyObject *ptr;
	PythonException *error;
	std::tie(ptr, error) = call(args.ptr(), kwargs.ptr());
	py::object obj;
	if (ptr) {
		obj = py::reinterpret_steal<py::object>(ptr);
	}
	return std::make_pair(obj, error);
}
	
std::pair<PyObject *, PythonException *> PythonFunction::call(PyObject *args, PyObject *kwargs) const {
	PyObject *result = PyObject_Call(function, args, kwargs);

	if (result == nullptr) {
		PythonException *error = new PythonException();
		return std::make_pair(nullptr, error);
	} else {
		return std::make_pair(result, nullptr);
	}
}

std::pair<PyObject *, PythonException *> PythonFunction::call(PyObject *args) const {
	PyObject *result = PyObject_CallObject(function, args);

	if (result == nullptr) {
		PythonException *error = new PythonException();
		return std::make_pair(nullptr, error);
	} else {
		return std::make_pair(result, nullptr);
	}
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
