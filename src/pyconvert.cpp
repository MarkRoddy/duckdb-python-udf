
#include <pyconvert.hpp>
#include <duckdb.hpp>
#include <Python.h>
#include <iostream>
#include <unordered_map>
#include <log.hpp>
#include <stdexcept>

namespace pyudf {

py::dict StructToDict(duckdb::Value value) {
	py::dict py_value;
	auto &child_type = value.type();
	auto &struct_children = duckdb::StructValue::GetChildren(value);
	D_ASSERT(duckdb::StructType::GetChildCount(child_type) == struct_children.size());
	for (idx_t i = 0; i < struct_children.size(); i++) {
		duckdb::Value name = duckdb::StructType::GetChildName(child_type, i);
		duckdb::Value val = struct_children[i];

		auto pyName = duckdb_to_pyobj(name);
		auto pyValue = duckdb_to_pyobj(val);
		py_value[pyName] = pyValue;
	}
	return py_value;
}

py::object duckdb_to_pyobj(duckdb::Value &value) {
	py::object py_value;
	switch (value.type().id()) {
	case duckdb::LogicalTypeId::BOOLEAN:
		py_value = py::cast(value.GetValue<bool>());
		break;
	case duckdb::LogicalTypeId::TINYINT:
		py_value = py::cast(value.GetValue<int8_t>());
		break;
	case duckdb::LogicalTypeId::SMALLINT:
		py_value = py::cast(value.GetValue<int16_t>());
		break;
	case duckdb::LogicalTypeId::INTEGER:
		py_value = py::cast(value.GetValue<int32_t>());
		break;
	case duckdb::LogicalTypeId::BIGINT:
		py_value = py::cast(value.GetValue<int64_t>());
		break;
	case duckdb::LogicalTypeId::FLOAT:
		py_value = py::cast(value.GetValue<float>());
		break;
	case duckdb::LogicalTypeId::DOUBLE:
		py_value = py::cast(value.GetValue<double>());
		break;
	case duckdb::LogicalTypeId::VARCHAR:
		py_value = py::cast(value.GetValue<std::string>().c_str());
		break;
	case duckdb::LogicalTypeId::STRUCT:
		py_value = StructToDict(value);
		break;
	default:
		debug("Unhandled Logical Type: " + value.type().ToString());
		py_value = py::none();
	}
	return py_value;
}

py::tuple duckdbs_to_pyobjs(std::vector<duckdb::Value> &values) {
	py::tuple py_tuple(values.size());
	for (size_t i = 0; i < values.size(); i++) {
		py::object pyvalue = duckdb_to_pyobj(values[i]);
		py_tuple[i] = pyvalue;
	}
	return py_tuple;
}

duckdb::Value ConvertPyBindObjectToDuckDBValue(py::handle py_item, duckdb::LogicalType logical_type) {
	duckdb::Value value;
	bool conversion_failed = false;

	switch (logical_type.id()) {
	case duckdb::LogicalTypeId::BOOLEAN:
		if (py::isinstance<py::bool_>(py_item)) {
			// value = duckdb::Value(Py_True == py_item);
			value = duckdb::Value(py_item.cast<bool>());
		} else {
			conversion_failed = true;
		}
		break;
	case duckdb::LogicalTypeId::TINYINT:
	case duckdb::LogicalTypeId::SMALLINT:
	case duckdb::LogicalTypeId::INTEGER:
		if(py::isinstance<py::int_>(py_item)) {
			value = duckdb::Value(py_item.cast<int32_t>());
		} else {
			conversion_failed = true;
		}
		break;
	case duckdb::LogicalTypeId::FLOAT:
	case duckdb::LogicalTypeId::DOUBLE:
		if(py::isinstance<py::float_>(py_item)) {
			value = duckdb::Value(py_item.cast<float>());
		} else {
			conversion_failed = true;
		}
		break;
	case duckdb::LogicalTypeId::VARCHAR:
		if(py::isinstance<py::str>(py_item)) {
			value = duckdb::Value(py_item.cast<std::string>());
		} else {
			conversion_failed = true;
		}
		break;
	default:
		conversion_failed = true;
	}

	if (conversion_failed) {
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
		duckdb::Value value = ConvertPyBindObjectToDuckDBValue(obj, logical_type);
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
	if (!py::isinstance<py::type>(pyType)) {
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
