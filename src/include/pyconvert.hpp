
#include <vector>
#include <duckdb.hpp>
#include <Python.h>

#include <pybind11/pybind11.h>
namespace py = pybind11;


namespace pyudf {
PyObject *duckdb_to_py(duckdb::Value &value);
PyObject *duckdbs_to_pys(std::vector<duckdb::Value> &values);
py::object duckdb_to_pyobj(duckdb::Value &value);
py::tuple duckdbs_to_pyobjs(std::vector<duckdb::Value> &values);
PyObject *StructToDict(duckdb::Value value);
duckdb::Value ConvertPyObjectToDuckDBValue(PyObject *py_item, duckdb::LogicalType logical_type);
duckdb::Value ConvertPyBindObjectToDuckDBValue(py::object py_item, duckdb::LogicalType logical_type);
void ConvertPyObjectsToDuckDBValues(PyObject *py_iterator, std::vector<duckdb::LogicalType> logical_types,
                                    std::vector<duckdb::Value> &result);
void ConvertPyBindObjectsToDuckDBValues(py::iterator pyiter, std::vector<duckdb::LogicalType> logical_types,
                                    std::vector<duckdb::Value> &result);
PyObject *pyObjectToIterable(PyObject *py_object);
std::vector<duckdb::LogicalType> PyTypesToLogicalTypes(const std::vector<PyObject *> &pyTypes);

// Duplicates functionality of PyUnicode_AsUTF8() which is not part of the limited ABI
char *Unicode_AsUTF8(PyObject *unicodeObject);

// Wrapper around isinstance(), similar to the PyObject_IsInstance() function that isn't
// part of the limited ABI.
bool PyIsInstance(PyObject *instance, PyObject *classObj);
} // namespace pyudf
