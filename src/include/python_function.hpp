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

	~PythonFunction();

	std::pair<PyObject *, PythonException *> call(PyObject *args) const;
	std::pair<PyObject *, PythonException *> call(PyObject *args, PyObject *kwargs) const;
	std::pair<py::object, PythonException *> call(py::tuple args) const;
	std::pair<py::object, PythonException *> call(py::tuple args, py::dict kwargs) const;

	std::string function_name() {
		return function_name_;
	}
	std::string module_name() {
		return module_name_;
	}

protected:
	void init(const std::string &module_name, const std::string &function_name);
	PyObject *function;

private:
	std::string module_name_;
	std::string function_name_;
	PyObject *module;
};

} // namespace pyudf
#endif // PYTHONFUNCTION_H
