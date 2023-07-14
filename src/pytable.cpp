
#include <Python.h>
#include <iostream>
#include <stdexcept>
#include <duckdb.hpp>
#include <duckdb/parser/expression/constant_expression.hpp>
#include <duckdb/parser/expression/function_expression.hpp>
#include <pytable.hpp>
#include "python_function.hpp"
#include "python_table_function.hpp"
#include <pyconvert.hpp>
#include <log.hpp>

#include <typeinfo>

using namespace duckdb;
namespace pyudf {

struct PyScanBindData : public TableFunctionData {
	// Function arguments coerced to a tuple used in Python calling semantics,
	PyObject *arguments;

	// Keyword arguments coerced to a dict to be used in **kwarg calling semantics
	PyObject *kwargs;

	std::vector<LogicalType> return_types;

	// Return value of the function specified
	PyObject *function_result_iterable;

	pyudf::PythonTableFunction *pyfunc;
};

struct PyScanLocalState : public LocalTableFunctionState {
	bool done = false;
};

struct PyScanGlobalState : public GlobalTableFunctionState {
	PyScanGlobalState() : GlobalTableFunctionState() {
	}
};

void FinalizePyTable(PyScanBindData &bind_data) {
	// Free the iterable returned by our python function call
	Py_DECREF(bind_data.function_result_iterable);
	bind_data.function_result_iterable = nullptr;

	// Free each entry of the arguments tuple and the tuple itself
	Py_ssize_t size = PyTuple_Size(bind_data.arguments);
	for (Py_ssize_t i = 0; i < size; i++) {
		PyObject *arg = PyTuple_GetItem(bind_data.arguments, i);
		// Traced a SegFault that is prevented by not issueing this decref. There
		// is probably some side effect here that I'm not accounting for, and not
		// an issue with this decref per say.
		// Py_DECREF(arg);
	}
}

void PyScan(ClientContext &context, TableFunctionInput &data, DataChunk &output) {
	auto &bind_data = (PyScanBindData &)*data.bind_data;

	auto &local_state = (PyScanLocalState &)*data.local_state;

	if (local_state.done) {
		return;
	}

	PyObject *result = bind_data.function_result_iterable;
	if (nullptr == result) {
		throw std::runtime_error("Where did our iterator go?");
	}

	PyObject *row;
	int read_records = 0;
	while ((read_records < STANDARD_VECTOR_SIZE) && (row = PyIter_Next(result))) {
		auto iter_row = pyObjectToIterable(row);
		// todo: acquire the GIL
		if (PyErr_Occurred()) {
			PythonException err;
			// todo: release the GIL
			throw std::runtime_error(err.message);
		}
		// todo: release the GIL
		else if (!iter_row) {
			// todo: cleanup?
			throw std::runtime_error("Error: Row record not iterable as expected");
		} else {
			std::vector<duckdb::Value> duck_row = {};
			ConvertPyObjectsToDuckDBValues(iter_row, bind_data.return_types, duck_row);
			for (long unsigned int i = 0; i < duck_row.size(); i++) {
				auto v = duck_row.at(i);
				// todo: Am I doing this correctly? I have no idea.
				output.SetValue(i, output.size(), v);
			}
			Py_DECREF(row);
			output.SetCardinality(output.size() + 1);
			read_records++;
		}
	}

	// PyIter_Next will return null if the iterator is exhausted or if an
	// exception has occurred during resumption of the underlying function,
	// so at this point we need to check which of these is the case.
	if (PyErr_Occurred()) {
		PythonException error = PythonException();

		// Shouldn't be necessary, but mark our scan as complete for good measure.
		local_state.done = true;

		// Clean everything up
		FinalizePyTable(bind_data);
		throw std::runtime_error(error.message);
	}
	if (!row) {
		// We've exhausted our iterator
		local_state.done = true;
		FinalizePyTable(bind_data);
		return;
	}
}

void PyBindFunctionAndArgs(ClientContext &context, TableFunctionBindInput &input,
                           unique_ptr<PyScanBindData> &bind_data) {
	auto params = input.named_parameters;
	std::string module_name;
	std::string function_name;
	std::vector<Value> arguments;
	if ((0 < params.count("module")) && (0 < params.count("func"))) {
		module_name = input.named_parameters["module"].GetValue<std::string>();
		function_name = input.named_parameters["func"].GetValue<std::string>();
		arguments = input.inputs;
	} else if ((0 == params.count("module")) && (0 == params.count("func"))) {
		// Neither module nor func specified, infer this from arg list.
		auto func_spec_value = input.inputs[0];
		if (func_spec_value.type().id() != LogicalTypeId::VARCHAR) {
			throw InvalidInputException(
			    "First argument must be string specifying 'module:func' if name parameters not supplied");
		}
		auto func_specifier = func_spec_value.GetValue<std::string>();
		std::tie(module_name, function_name) = parse_func_specifier(func_specifier);

		/* Since we had to infer our functions specifier from the argument list, we treat
		   everything after the first argument as input to the python function. */
		arguments = std::vector<Value>(input.inputs.begin() + 1, input.inputs.end());
	} else if (0 < params.count("module")) {
		throw InvalidInputException("Module specified w/o a corresponding function");
	} else if (0 < params.count("func")) {
		throw InvalidInputException("Function specified w/o a corresponding module");
	} else {
		throw InvalidInputException("I don't know how logic works");
	}

	bind_data->pyfunc = new pyudf::PythonTableFunction(module_name, function_name);
	bind_data->arguments = duckdbs_to_pys(arguments);
	if (NULL == bind_data->arguments) {
		throw IOException("Failed coerce function arguments");
	}

	if (0 < params.count("kwargs")) {
		auto input_kwargs = params["kwargs"];
		auto ik_type = input_kwargs.type().id();
		if (ik_type != LogicalTypeId::STRUCT) {
			throw InvalidInputException("kwargs must be a struct mapping argument names to values");
		}
		bind_data->kwargs = duckdb_to_py(input_kwargs);
	}
}

void PyBindColumnsAndTypes(ClientContext &context, TableFunctionBindInput &input, unique_ptr<PyScanBindData> &bind_data,
                           std::vector<LogicalType> &return_types, std::vector<std::string> &names) {
	auto names_and_types = input.named_parameters["columns"];
	auto &child_type = names_and_types.type();
	if (names_and_types.IsNull()) {
		// Check if we can grab from the function
		auto types = bind_data->pyfunc->column_types(bind_data->arguments, bind_data->kwargs);
		if (types.empty()) {
			// todo: Add a URL to an article on writing Python functions once said article exists
			auto errMsg = "You did not specify a 'columns' argument, and your Python function does not have type "
			              "annotations (or they are incompatible)";
			throw InvalidInputException(errMsg);
		}
		for (auto t : types) {
			return_types.emplace_back(t);
		}
		auto colnames = bind_data->pyfunc->column_names(bind_data->arguments, bind_data->kwargs);
		for (auto n : colnames) {
			names.push_back(n);
		}
		if (return_types.size() != names.size()) {
			// Show the names:
			debug("Column Names:");
			for (auto n : names) {
				debug(n);
			}
			// Show the types:
			debug("Column Types:");
			for (auto t : types) {
				debug(t.ToString());
			}
			throw InvalidInputException("Python function reported a mismatched number of column names and types");
		} else if (0 == names.size()) {
			throw InvalidInputException("Python function reported it contains zero columns");
		}
		bind_data->return_types = types;
		return;
	}
	if (child_type.id() != LogicalTypeId::STRUCT) {
		throw InvalidInputException("columns requires a struct mapping column names to data types");
	}
	auto &struct_children = StructValue::GetChildren(names_and_types);
	D_ASSERT(StructType::GetChildCount(child_type) == struct_children.size());
	for (idx_t i = 0; i < struct_children.size(); i++) {
		auto &name = StructType::GetChildName(child_type, i);
		auto &val = struct_children[i];
		names.push_back(name);
		if (val.type().id() != LogicalTypeId::VARCHAR) {
			throw BinderException("we require a type specification as string");
		}
		return_types.emplace_back(TransformStringToLogicalType(StringValue::Get(val), context));
	}
	if (names.empty()) {
		throw BinderException("require at least a single column as input!");
	}
	bind_data->return_types = std::vector<LogicalType>(return_types);
}

unique_ptr<FunctionData> PyBind(ClientContext &context, TableFunctionBindInput &input,
                                std::vector<LogicalType> &return_types, std::vector<std::string> &names) {
	auto result = make_uniq<PyScanBindData>();
	PyBindFunctionAndArgs(context, input, result);
	PyBindColumnsAndTypes(context, input, result, return_types, names);
	debug("PyBindColumnsAndTypes: Num Column Names:" + to_string(names.size()));
	debug("PyBindColumnsAndTypes: Num Column types:" + to_string(return_types.size()));

	// Invoke the function and grab a copy of the iterable it returns.
	PyObject *iter;
	PythonException *error;
	std::tie(iter, error) = result->pyfunc->call(result->arguments, result->kwargs);
	if (!iter) {
		std::string err = error->message;
		error->~PythonException();
		throw std::runtime_error(err);
	} else if (!PyIter_Check(iter)) {
		Py_DECREF(iter);
		throw std::runtime_error("Error: function '" + result->pyfunc->function_name() +
		                         "' did not return an iterator\n");
	}
	result->function_result_iterable = iter;
	return std::move(result);
}

unique_ptr<GlobalTableFunctionState> PyInitGlobalState(ClientContext &context, TableFunctionInitInput &input) {
	auto result = make_uniq<PyScanGlobalState>();
	// result.function = get_python_function()
	return std::move(result);
}

unique_ptr<LocalTableFunctionState> PyInitLocalState(ExecutionContext &context, TableFunctionInitInput &input,
                                                     GlobalTableFunctionState *global_state) {
	// Leaving tehese commented out in case we need them in the future.
	// auto bind_data = (const PyScanBindData *)input.bind_data;
	// auto &gstate = (PyScanGlobalState &)*global_state;
	auto local_state = make_uniq<PyScanLocalState>();

	return std::move(local_state);
}

unique_ptr<CreateTableFunctionInfo> GetPythonTableFunction() {
	auto py_table_function = duckdb::TableFunction("pytable", {}, PyScan, (table_function_bind_t)PyBind,
	                                               PyInitGlobalState, PyInitLocalState);

	// todo: don't configure this for older versions of duckdb
	py_table_function.varargs = LogicalType::ANY;

	py_table_function.named_parameters["module"] = LogicalType::VARCHAR;
	py_table_function.named_parameters["func"] = LogicalType::VARCHAR;
	py_table_function.named_parameters["columns"] = LogicalType::ANY;
	py_table_function.named_parameters["kwargs"] = LogicalType::ANY;

	CreateTableFunctionInfo py_table_function_info(py_table_function);
	return make_uniq<CreateTableFunctionInfo>(py_table_function_info);
}

} // namespace pyudf
