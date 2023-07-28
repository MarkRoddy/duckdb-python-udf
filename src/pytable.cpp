
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

#include <pybind11/pybind11.h>
namespace py = pybind11;

using namespace duckdb;
namespace pyudf {

struct PyScanBindData : public TableFunctionData {
	// Function arguments coerced to a tuple used in Python calling semantics,
	py::tuple objArguments;

	// Keyword arguments coerced to a dict to be used in **kwarg calling semantics
	py::dict objKwargs;

	std::vector<LogicalType> return_types;

	// Return value of the function specified
	py::iterator objFunction_result_iterable;

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
	// Py_DECREF(bind_data.function_result_iterable);
	// bind_data.function_result_iterable = nullptr;
	// todo: manually delete//deallocate the py::object values on this struct
}

void PyScan(ClientContext &context, TableFunctionInput &data, DataChunk &output) {
	auto &bind_data = (PyScanBindData &)*data.bind_data;

	auto &local_state = (PyScanLocalState &)*data.local_state;

	if (local_state.done) {
		return;
	}
	py::iterator resultIter = bind_data.objFunction_result_iterable;
	int read_records = 0;
	for (int read_records = 0; read_records < STANDARD_VECTOR_SIZE; ++read_records) {
		if (resultIter == py::iterator::sentinel()) {
			// We've exhausted our iterator
			local_state.done = true;
			FinalizePyTable(bind_data);
			return;
		} else {
			py::handle objRow = *resultIter;
			py::iterator iter_row;
			try {
				iter_row = py::iter(objRow);

			} catch (py::error_already_set &e) {
				// Assuming the error was because the object can't be iterated
				// upon though techincally it could be due to an error in an
				// object's __iter__() function.
				e.restore();
				PyErr_Clear();
				std::string msg = py::cast<std::string>(py::str(e.value()));
				throw std::runtime_error(msg);
				// throw std::runtime_error("Error: function '" + result->pyfunc->function_name() +
				//                  "' did not return an iterator\n");
			}

			std::vector<duckdb::Value> duck_row = {};
			ConvertPyBindObjectsToDuckDBValues(iter_row, bind_data.return_types, duck_row);
			for (long unsigned int i = 0; i < duck_row.size(); i++) {
				auto v = duck_row.at(i);
				// todo: Am I doing this correctly? I have no idea.
				output.SetValue(i, output.size(), v);
			}
			output.SetCardinality(output.size() + 1);
		}

		try {
			// If our Python function returned an iterable object such as a list or a
			// tuple, this operation is safe. But if the function returns an iterator
			// because the function is using the yield keyword, each advancement of the
			// iterator resumes the python function. Because of this, its possible for
			// the function to raise an error when the iterator is advanced here. Due to
			// this we watch out for a Python error, clean up if one happens, and raise
			// a simplified version of the exception.
			++resultIter;
		} catch (py::error_already_set &e) {
			local_state.done = true;
			FinalizePyTable(bind_data);
			std::string msg = py::cast<std::string>(py::str(e.value()));
			throw std::runtime_error(msg);
		}
	}
	// We've fallen out of our for loop which means we've hit STANDARD_VECTOR_SIZE and
	// need to wait for the db engine to initiate our function again.
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
	bind_data->objArguments = duckdbs_to_pyobjs(arguments);

	if (0 < params.count("kwargs")) {
		auto input_kwargs = params["kwargs"];
		auto ik_type = input_kwargs.type().id();
		if (ik_type != LogicalTypeId::STRUCT) {
			throw InvalidInputException("kwargs must be a struct mapping argument names to values");
		}
		py::object kwobj = duckdb_to_pyobj(input_kwargs);
		bind_data->objKwargs = kwobj.cast<py::dict>();
	}
}

void PyBindColumnsAndTypes(ClientContext &context, TableFunctionBindInput &input, unique_ptr<PyScanBindData> &bind_data,
                           std::vector<LogicalType> &return_types, std::vector<std::string> &names) {
	auto names_and_types = input.named_parameters["columns"];
	auto &child_type = names_and_types.type();
	if (names_and_types.IsNull()) {
		// Check if we can grab from the function
		auto types = bind_data->pyfunc->column_types(bind_data->objArguments, bind_data->objKwargs);
		if (types.empty()) {
			// todo: Add a URL to an article on writing Python functions once said article exists
			auto errMsg = "You did not specify a 'columns' argument, and your Python function does not have type "
			              "annotations (or they are incompatible)";
			throw InvalidInputException(errMsg);
		}
		for (auto t : types) {
			return_types.emplace_back(t);
		}
		auto colnames = bind_data->pyfunc->column_names(bind_data->objArguments, bind_data->objKwargs);
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
	py::object maybe_iter;
	PythonException *error;
	std::tie(maybe_iter, error) = result->pyfunc->call(result->objArguments, result->objKwargs);
	if (error) {
		std::string err = error->message;
		error->~PythonException();
		throw std::runtime_error(err);
	}

	py::iterator iter;
	try {
		iter = py::iter(maybe_iter);
	} catch (py::error_already_set &e) {
		// Assuming the error was because the object can't be iterated
		// upon though techincally it could be due to an error in an
		// object's __iter__() function.
		e.restore();
		PyErr_Clear();
		throw std::runtime_error("Error: function '" + result->pyfunc->function_name() +
		                         "' did not return an iterator\n");
	}
	result->objFunction_result_iterable = iter;
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
