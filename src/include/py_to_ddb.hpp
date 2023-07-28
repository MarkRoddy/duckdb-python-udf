#ifndef PYTODDB_H
#define PYTODDB_H

#include <Python.h>
#include <duckdb.hpp>
#include <cpy/object.hpp>
#include <cpy/module.hpp>

namespace pyudf {

// std::pair<std::string, std::string> parse_func_specifier(std::string function_specifier);

class PyToDDB {
public:
	PyToDDB();

	duckdb::Value convert(PyObject* src, duckdb::LogicalType dest_type);
	duckdb::Value convert(cpy::Object src, duckdb::LogicalType dest_type);
	duckdb::Value toTime(cpy::Object src);
	duckdb::Value toDate(cpy::Object src);
	duckdb::Value toTimeStampTZ(cpy::Object src);

protected:
	duckdb::dtime_t toDtime(cpy::Object src);
	duckdb::date_t toDate_t(cpy::Object src);
	cpy::Object dtToUtc(cpy::Object src);

private:
	// Cache import of the datetime module
	cpy::Module modDatetime;

	// Cache attributes of the datetime module
	cpy::Object clsTime;
	cpy::Object clsDate;
	cpy::Object clsDatetime;
	cpy::Object tzUtc;
};

} // namespace pyudf
#endif // PYTODDB_H
