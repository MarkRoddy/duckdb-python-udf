
#include <pyconvert.hpp>
#include <duckdb.hpp>
#include <Python.h>
#include <iostream>
#include <unordered_map>
#include <log.hpp>
#include <stdexcept>

namespace pyudf {

// Duplicates functionality of PyUnicode_AsUTF8() which is not part of the limited ABI
char *Unicode_AsUTF8(PyObject *unicodeObject) {
	PyObject *utf8 = PyUnicode_AsUTF8String(unicodeObject);
	if (utf8 == nullptr) {
		PyErr_Print();
		return nullptr;
	}

	char *bytes = PyBytes_AsString(utf8);
	if (bytes == nullptr) {
		Py_DECREF(utf8);
		PyErr_Print();
		return nullptr;
	}

	char *result = strdup(bytes);
	Py_DECREF(utf8);

	if (result == nullptr) {
		PyErr_SetString(PyExc_MemoryError, "Out of memory");
		PyErr_Print();
		return nullptr;
	}

	return result;
}

PyObject *duckdb_to_py(duckdb::Value &value) {
	PyObject *py_value = nullptr;

	switch (value.type().id()) {
	case duckdb::LogicalTypeId::BOOLEAN:
		py_value = PyBool_FromLong(value.GetValue<bool>());
		break;
	case duckdb::LogicalTypeId::TINYINT:
		py_value = PyLong_FromLong(value.GetValue<int8_t>());
		break;
	case duckdb::LogicalTypeId::SMALLINT:
		py_value = PyLong_FromLong(value.GetValue<int16_t>());
		break;
	case duckdb::LogicalTypeId::INTEGER:
		py_value = PyLong_FromLong(value.GetValue<int32_t>());
		break;
	case duckdb::LogicalTypeId::BIGINT:
		py_value = PyLong_FromLongLong(value.GetValue<int64_t>());
		break;
	case duckdb::LogicalTypeId::FLOAT:
		py_value = PyFloat_FromDouble(value.GetValue<float>());
		break;
	case duckdb::LogicalTypeId::DOUBLE:
		py_value = PyFloat_FromDouble(value.GetValue<double>());
		break;
	case duckdb::LogicalTypeId::VARCHAR:
		py_value = PyUnicode_FromString(value.GetValue<std::string>().c_str());
		break;
	case duckdb::LogicalTypeId::STRUCT:
		py_value = StructToDict(value).ptr();
		Py_INCREF(py_value);
		break;
	default:
		debug("Unhandled Logical Type: " + value.type().ToString());
		Py_INCREF(Py_None);
		py_value = Py_None;
	}
	return py_value;
}

py::dict StructToDict(duckdb::Value value) {
	py::dict py_value;
	auto &child_type = value.type();
	auto &struct_children = duckdb::StructValue::GetChildren(value);
	D_ASSERT(duckdb::StructType::GetChildCount(child_type) == struct_children.size());
	for (idx_t i = 0; i < struct_children.size(); i++) {
		duckdb::Value name = duckdb::StructType::GetChildName(child_type, i);
		duckdb::Value val = struct_children[i];

		auto pyName = duckdb_to_py(name);
		auto pyValue = duckdb_to_py(val);
		py_value[pyName] = pyValue;
	}
	return py_value;
}

py::object duckdb_to_pyobj(duckdb::Value &value) {
	PyObject *ptr = duckdb_to_py(value);
	return py::reinterpret_steal<py::object>(ptr);
}

PyObject *duckdbs_to_pys(std::vector<duckdb::Value> &values) {
	PyObject *py_tuple = PyTuple_New(values.size());

	for (size_t i = 0; i < values.size(); i++) {
		PyObject *py_value = nullptr;
		py_value = duckdb_to_py(values[i]);
		PyTuple_SetItem(py_tuple, i, py_value);
	}

	return py_tuple;
}

py::tuple duckdbs_to_pyobjs(std::vector<duckdb::Value> &values) {
	PyObject *ptr = duckdbs_to_pys(values);
	if (nullptr == ptr) {
		throw duckdb::IOException("Failed coerce duckdb values to python values");
	} else {
		return py::reinterpret_steal<py::tuple>(ptr);
	}
}

duckdb::Value ConvertPyBindObjectToDuckDBValue(py::object py_item, duckdb::LogicalType logical_type) {
	return ConvertPyObjectToDuckDBValue(py_item.ptr(), logical_type);
}

duckdb::Value ConvertPyObjectToDuckDBValue(PyObject *py_item, duckdb::LogicalType logical_type) {
	duckdb::Value value;
	PyObject *py_value;
	bool conversion_failed = false;

	switch (logical_type.id()) {
	case duckdb::LogicalTypeId::BOOLEAN:
		if (!PyBool_Check(py_item)) {
			conversion_failed = true;
		} else {
			value = duckdb::Value(Py_True == py_item);
		}
		break;
	case duckdb::LogicalTypeId::TINYINT:
	case duckdb::LogicalTypeId::SMALLINT:
	case duckdb::LogicalTypeId::INTEGER:
		if (!PyLong_Check(py_item)) {
			conversion_failed = true;
		} else {
			value = duckdb::Value((int32_t)PyLong_AsLong(py_item));
		}
		break;
	// case duckdb::LogicalTypeId::BIGINT:
	//   if (!PyLong_Check(py_item)) {
	//     conversion_failed = true;
	//   } else {
	//     value = duckdb::Value(PyLong_AsLongLong(py_item));
	//   }
	//   break;
	case duckdb::LogicalTypeId::FLOAT:
	case duckdb::LogicalTypeId::DOUBLE:
		if (!PyFloat_Check(py_item)) {
			conversion_failed = true;
		} else {
			value = duckdb::Value(PyFloat_AsDouble(py_item));
		}
		break;
	case duckdb::LogicalTypeId::VARCHAR:
		if (!PyUnicode_Check(py_item)) {
			conversion_failed = true;
		} else {
			py_value = PyUnicode_AsUTF8String(py_item);
			value = duckdb::Value(PyBytes_AsString(py_value));
			Py_DECREF(py_value);
		}
		break;
		// Add more cases for other LogicalTypes here
	case duckdb::LogicalTypeId::STRUCT:
		py_value = StructToDict(value).ptr();
		Py_INCREF(py_value);
		break;
	default:
		conversion_failed = true;
	}

	if (conversion_failed) {
		// DUCKDB_API Value(std::nullptr_t val); // NOLINT: Allow implicit conversion from `nullptr_t`
		value = duckdb::Value((std::nullptr_t)NULL);
	}
	return value;
}

void ConvertPyBindObjectsToDuckDBValues(py::iterator it, std::vector<duckdb::LogicalType> logical_types,
                                        std::vector<duckdb::Value> &result) {
	size_t index = 0;
	while (it != py::iterator::sentinel()) {
		py::handle obj = *it;

		if (index >= logical_types.size()) {
			std::string error_message = "A row with " + std::to_string(index + 1) + " values was detected though " +
			                            std::to_string(logical_types.size()) + " columns were expected";
			throw duckdb::InvalidInputException(error_message);
		}
		duckdb::LogicalType logical_type = logical_types[index];
		duckdb::Value value = ConvertPyObjectToDuckDBValue(obj.ptr(), logical_type);
		result.push_back(value);
		++it;
		index++;
	}
	if (index != logical_types.size()) {
		std::string error_message = "A row with " + std::to_string(index) + " values was detected though " +
		                            std::to_string(logical_types.size()) + " columns were expected";
		throw duckdb::InvalidInputException(error_message);
	}
}

duckdb::LogicalType PyTypeToDuckDBLogicalType(py::object pyType) {
	if(!py::isinstance<py::type>(pyType)) {
		debug("Trying to convert a python object that is not a Type into a duckdb type, returning invalid");
		return duckdb::LogicalType::INVALID;
	} else {
		if (pyType.is(py::type::of(py::str()))) {
			return duckdb::LogicalType::VARCHAR;
		} else if (pyType.is(py::type::of(py::int_()))) {
			return duckdb::LogicalType::INTEGER;
		} else if (pyType.is(py::type::of(py::float_()))) {
			return duckdb::LogicalType::DOUBLE;
		} else {
			debug("Trying to convert an unknonwn type, returning invalid");
			return duckdb::LogicalType::INVALID;
		}
	}
}

std::vector<duckdb::LogicalType> PyBindTypesToLogicalTypes(const std::vector<py::object> &pyTypes) {
	std::vector<duckdb::LogicalType> result;
	for (auto pyobj : pyTypes) {
		duckdb::LogicalType converted = PyTypeToDuckDBLogicalType(pyobj);
		result.emplace_back(converted);
	}
	return result;
}

} // namespace pyudf
