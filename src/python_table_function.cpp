
#include <python_function.hpp>
#include <python_table_function.hpp>
#include <config.h>
#include <pyconvert.hpp>
#include <string>
#include <log.hpp>

namespace pyudf {

PythonTableFunction::PythonTableFunction(const std::string &function_specifier) : PythonFunction(function_specifier) {
	auto wrapped = wrap_function(function);
	if (wrapped) {
		debug("Successfully wrapped the function");
		function = wrapped;
	} else {
		debug("Failed to find function wrapper, this may be ok?");
	}
}

PythonTableFunction::PythonTableFunction(const std::string &module_name, const std::string &function_name)
    : PythonFunction(module_name, function_name) {
	auto wrapped = wrap_function(function);
	if (wrapped) {
		debug("Successfully wrapped the function");
		function = wrapped;
	} else {
		debug("Failed to find function wrapper, this may be ok?");
	}
}

PyObject *PythonTableFunction::wrap_function(PyObject *function) {
	debug("About to import the decorator");
	PyObject *decorator = import_decorator();
	if (!decorator) {
		// todo: raise error?
		debug("Not able to find importable verison either, exitting");
		return nullptr;
	}
	if (!decorator) {
		debug("Our decorator is null?!?!");
		return nullptr;
	}
	if (!PyCallable_Check(decorator)) {
		debug("Our decorator isn't callable?!?!");
		return nullptr;
	}

	auto decorator_cls = import_decorator_class();
	if (!decorator_cls) {
		debug("We didn't get a decorator class even though we have a decoartor? Weird. Skipping check");
	} else {
		auto is_decorator = PyIsInstance(function, decorator_cls);
		if (is_decorator) {
			debug("Our function is already wrapped. Nothing to do here.");
			Py_DECREF(decorator);
			Py_DECREF(decorator_cls);
			return function;
		} else {
			debug("our function is not wrapped, applying the decorator");
		}
	}

	// Otherwise, go ahead and wrappe it
	debug("Creating a tuple for arguments");
	PyObject *args = PyTuple_New(1);
	debug("Setting our function as hte only argument");
	PyTuple_SetItem(args, 0, function);

	if (!function) {
		debug("function is nullptr");
		return nullptr;
	} else if (function == Py_None) {
		debug("function is None");
		return nullptr;
	} else if (!PyCallable_Check(function)) {
		debug("function is not callable");
		return nullptr;
	}

	if (!args) {
		debug("args is nullptr\n");
		return nullptr;
	} else if (args == Py_None) {
		debug("args is None\n");
		return nullptr;
	} else if (!PyTuple_Check(args)) {
		debug("args is not a tuple\n");
		return nullptr;
	}

	if (PyErr_Occurred()) {
		PyErr_Print();
		return nullptr;
	}

	debug("Calling the function to wrap our function");
	PyObject *wrapped_function = PyObject_CallObject(decorator, args);
	debug("Completed calling the function to wrapp");
	if (!wrapped_function) {
		debug("Error wrapping the function");
		PyErr_Print();
		return nullptr;
	}
	debug("Succeeded in wrapping the function?");
	// todo: handle exception from wrapping?
	return wrapped_function;
}

PyObject *PythonTableFunction::import_from_ducktables(std::string attr_name) {
	debug("Calling import module");
	PyObject *module_obj = PyImport_ImportModule("ducktables");
	debug("Completed calling import module");
	if (!module_obj) {
		debug("Module not found returning null");
		// todo: Maybe we want to report this error if it isn't just import error?
		PyErr_Print();
		return nullptr;
	}
	debug("Getting the attribute...");
	PyObject *attr = PyObject_GetAttrString(module_obj, attr_name.c_str());
	if (!attr) {
		// todo: Maybe we want to report this error if it isn't just import error?
		debug("Attribute not found returning null");
		PyErr_Print();
	}
	Py_DECREF(module_obj);
	return attr;
}
PyObject *PythonTableFunction::import_decorator() {
	auto function_obj = import_from_ducktables("ducktable");
	return function_obj;
}

PyObject *PythonTableFunction::import_decorator_class() {
	auto cls_obj = import_from_ducktables("DuckTableSchemaWrapper");
	return cls_obj;
}

// Given an attribute on our Python Function object, call it with the supplied arguments and coerce an expected
// list-like object
std::vector<PyObject *> PythonTableFunction::call_to_list(std::string attr_name, PyObject *args, PyObject *kwargs) {
	std::vector<PyObject *> items;

	// Get the 'column_types' method
	PyObject *method = PyObject_GetAttrString(function, attr_name.c_str());

	if (!method) {
		debug("No '" + attr_name + "' attribute on our method object");
		return items;
	}

	if (!PyCallable_Check(method)) {
		debug("Our method has a '" + attr_name + "' attribute, but it is not callabe");
		Py_DECREF(method);
		return items;
	}

	// Invoke the method with args and kwargs
	PyObject *result = PyObject_Call(method, args, kwargs);
	if (!result) {
		debug("The function's " + attr_name + " method returned null. Did an error occur?");
		Py_DECREF(method);
		return items;
	}

	// Try to convert our result into a list. This way users can return a tuple or list or iterable or whatever.
	PyObject *listResult = PySequence_List(result);
	if (!listResult) {
		debug("Unable to convert " + attr_name + "() return to a list. Maybe an error?");
		Py_DECREF(result);
		Py_DECREF(method);
		return items;
	}

	// Copy the objects to our return vector
	Py_ssize_t listSize = PyList_Size(listResult);

	// Iterate over the list elements
	for (Py_ssize_t i = 0; i < listSize; ++i) {
		PyObject *listItem = PyList_GetItem(listResult, i);

		// Add the list item to the vector
		Py_INCREF(listItem);
		items.push_back(listItem);
	}

	Py_DECREF(result);
	Py_DECREF(listResult);
	Py_DECREF(method);
	return items;
}

std::vector<PyObject *> PythonTableFunction::pycolumn_types(PyObject *args, PyObject *kwargs) {
	return call_to_list("column_types", args, kwargs);
}

std::vector<duckdb::LogicalType> PythonTableFunction::column_types(PyObject *args, PyObject *kwargs) {
	auto python_types = pycolumn_types(args, kwargs);
	// todo: check for a Python error?
	// todo: decrement references to types
	auto ddb_types = PyTypesToLogicalTypes(python_types);
	return ddb_types;
}

std::vector<std::string> PythonTableFunction::column_names(PyObject *args, PyObject *kwargs) {
	std::vector<PyObject *> pyColumnNames = call_to_list("column_names", args, kwargs);
	std::vector<std::string> columnNames;

	for (auto listItem : pyColumnNames) {
		if (PyUnicode_Check(listItem)) {
			const char *columnName = Unicode_AsUTF8(listItem);
			columnNames.emplace_back(columnName);
			// todo: free columnName?????
			// todo: free listItem;
		} else {
			debug("One of the column names isn't a unicode value? What do we do here?");
			// todo: this will break something by leaving out a column name
		}
	}
	return columnNames;
}

} // namespace pyudf
