
#include <python_function.hpp>
#include <python_table_function.hpp>
#include <config.h>
#include <pyconvert.hpp>
#include <string>
#include <log.hpp>

namespace pyudf {

PythonTableFunction::PythonTableFunction(const std::string &function_specifier) : PythonFunction(function_specifier) {
	init();
}

PythonTableFunction::PythonTableFunction(const std::string &module_name, const std::string &function_name)
    : PythonFunction(module_name, function_name) {
	init();
}

void PythonTableFunction::init() {
	py::module_ mod;
	try {
		mod = py::module_::import("ducktables");
	} catch (py::error_already_set &e) {
		if (e.matches(PyExc_ImportError)) {
			debug("Error importing ducktables, assuming it isn't installed");
			e.restore();
			PyErr_Clear();
			return;
		} else {
			throw;
		}
	}

	// Wrap the function in our Python decorator
	py::object decorator = mod.attr("DuckTableSchemaWrapper");
	if (py::isinstance((py::object)functionObj, decorator)) {
		debug("Function already wrapped in the decorator");
		return;
	} else {
		debug("Function si not wrapped. Wrapping in hopes it has type annotations");
		py::tuple args = py::make_tuple(functionObj);
		py::function wrapped = decorator(*args);
		debug("Seems like I wrapped the new object?");
		functionObj = wrapped;
	}
}

std::vector<std::string> PythonTableFunction::column_names(py::tuple args, py::dict kwargs) {
	std::vector<std::string> result;
	if (!py::hasattr(functionObj, "column_names")) {
		return result;
	} else {
		py::object method = functionObj.attr("column_names");
		py::list pyColNames = method(*args, **kwargs);
		for (auto item : pyColNames) {
			result.push_back(item.cast<std::string>());
		}
		return result;
	}
}

std::vector<duckdb::LogicalType> PythonTableFunction::column_types(py::tuple args, py::dict kwargs) {
	std::vector<duckdb::LogicalType> result;
	std::vector<py::object> pytypes;
	if (!py::hasattr(functionObj, "column_types")) {
		debug("No 'column_types' attribute on the python function. Is ducktables installed?");
		return result;
	} else {
		py::object method = functionObj.attr("column_types");
		py::object retVal = method(*args, **kwargs);
		if (retVal.is_none()) {
			debug("The 'column_types' function returned a None value. Is everything ok?");
			return result;
		}
		py::list pyColTypes = retVal.cast<py::list>();
		for (py::handle item : pyColTypes) {
			// The handle to object dance is admittedly kind of silly
			py::object obj = py::reinterpret_borrow<py::object>(item);
			pytypes.push_back(obj);
		}
		auto ddb_types = PyBindTypesToLogicalTypes(pytypes);
		return ddb_types;
	}
}

} // namespace pyudf
