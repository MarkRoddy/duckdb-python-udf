#ifndef PYTHONFUNCTION_H
#define PYTHONFUNCTION_H

#include <Python.h>
#include <string>
#include <utility>
#include <python_exception.hpp>
#include <pybind11/pybind11.h>
namespace py = pybind11;

namespace pyudf {

std::pair<std::string, std::string> parse_func_specifier(std::string function_specifier);

class PythonFunction {
public:
	PythonFunction(const std::string &module_name, const std::string &function_name);
	PythonFunction(const std::string &module_and_function);
	PythonFunction(py::object function);

	~PythonFunction();

	py::object call(py::tuple args) const;
	py::object call(py::tuple args, py::dict kwargs) const;


	std::string function_name() {
		return function_name_;
	}
	std::string module_name() {
		return module_name_;
	}

protected:
	void init(const std::string &module_name, const std::string &function_name);
	void init_old(const std::string &module_name, const std::string &function_name);
	PyObject* function;
	py::function functionObj;

private:
	std::string module_name_;
	std::string function_name_;

	PyObject *module;
};

} // namespace pyudf
#endif // PYTHONFUNCTION_H
