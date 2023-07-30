
#include <vector>
#include <duckdb.hpp>
#include <Python.h>

#include <pybind11/pybind11.h>
namespace py = pybind11;

namespace pyudf {
py::object duckdb_to_pyobj(duckdb::Value &value);
py::tuple duckdbs_to_pyobjs(std::vector<duckdb::Value> &values);
py::dict StructToDict(duckdb::Value value);
duckdb::Value ConvertPyObjectToDuckDBValue(PyObject *py_item, duckdb::LogicalType logical_type);
duckdb::Value ConvertPyBindObjectToDuckDBValue(py::object py_item, duckdb::LogicalType logical_type);
void ConvertPyBindObjectsToDuckDBValues(py::iterator pyiter, std::vector<duckdb::LogicalType> logical_types,
                                        std::vector<duckdb::Value> &result);
std::vector<duckdb::LogicalType> PyBindTypesToLogicalTypes(const std::vector<py::object> &pyTypes);
} // namespace pyudf
