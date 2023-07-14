
#include "python_function.hpp"
#include <Python.h>
#include <string>
#include <vector>
#include <utility>
#include <duckdb.hpp>

namespace pyudf {
class PythonTableFunction : public PythonFunction {
public:
	PythonTableFunction(const std::string &function_specifier);
	PythonTableFunction(const std::string &module_name, const std::string &function_name);
	std::vector<std::string> column_names(PyObject *args, PyObject *kwargs);
	std::vector<duckdb::LogicalType> column_types(PyObject *args, PyObject *kwargs);

private:
	std::vector<PyObject *> pycolumn_types(PyObject *args, PyObject *kwargs);
	std::vector<PyObject *> call_to_list(std::string attr_name, PyObject *args, PyObject *kwargs);
	PyObject *wrap_function(PyObject *function);
	PyObject *import_decorator();
	PyObject *import_decorator_class();
	PyObject *import_from_ducktables(std::string attr_name);
};

} // namespace pyudf
