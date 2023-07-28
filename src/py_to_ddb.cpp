
#include <duckdb/common/types/time.hpp>
#include <py_to_ddb.hpp>
#include <cpy/module.hpp>
#include <cpy/exception.hpp>
#include <log.hpp>

namespace pyudf {
	PyToDDB::PyToDDB() {
		modDatetime = cpy::Module("datetime");
			
		clsTime = modDatetime.attr("time");
		clsDate = modDatetime.attr("date");
		clsDatetime = modDatetime.attr("datetime");

		cpy::Object tzcls = modDatetime.attr("timezone");
		tzUtc = tzcls.attr("utc");
	}

	duckdb::Value PyToDDB::convert(PyObject* src, duckdb::LogicalType dest_type) {
		duckdb::Value value;
		cpy::Object cpy_src(src, "val_to_convert");
		
		PyObject *py_value;
		bool conversion_failed = false;

		switch (dest_type.id()) {
		case duckdb::LogicalTypeId::BOOLEAN:
			if (!PyBool_Check(src)) {
				conversion_failed = true;
			} else {
				value = duckdb::Value(Py_True == src);
			}
			break;
		case duckdb::LogicalTypeId::TINYINT:
		case duckdb::LogicalTypeId::SMALLINT:
		case duckdb::LogicalTypeId::INTEGER:
			if (!PyLong_Check(src)) {
				conversion_failed = true;
			} else {
				value = duckdb::Value((int32_t)PyLong_AsLong(src));
			}
			break;
		case duckdb::LogicalTypeId::FLOAT:
		case duckdb::LogicalTypeId::DOUBLE:
			if (!PyFloat_Check(src)) {
				conversion_failed = true;
			} else {
				value = duckdb::Value(PyFloat_AsDouble(src));
			}
			break;
		case duckdb::LogicalTypeId::VARCHAR:
			if (!PyUnicode_Check(src)) {
				conversion_failed = true;
			} else {
				py_value = PyUnicode_AsUTF8String(src);
				value = duckdb::Value(PyBytes_AsString(py_value));
				Py_DECREF(py_value);
			}
			break;
		case duckdb::LogicalTypeId::TIME:
			if(!cpy_src.isinstance(clsTime)) {
				conversion_failed = true;
			} else {
				value = toTime(cpy_src);
			}
			break;
		case duckdb::LogicalTypeId::DATE:
			if(!cpy_src.isinstance(clsDate)) {
				conversion_failed = true;
			} else {
				value = toDate(cpy_src);
			}
			break;
		case duckdb::LogicalTypeId::TIMESTAMP_TZ:
			if(!cpy_src.isinstance(clsDatetime)) {
				conversion_failed = true;
			} else {
				value = toTimeStampTZ(cpy_src);
			}
			break;

		default:
			conversion_failed = true;
		}

		if (conversion_failed) {
			// DUCKDB_API Value(std::nullptr_t val); // NOLINT: Allow implicit conversion from `nullptr_t`
			value = duckdb::Value((std::nullptr_t)NULL);
		}
		return value;

	// 			return Value::TIME(dtime_t(0));
	// case LogicalTypeId::TIMESTAMP:
	// 	return Value::TIMESTAMP(Date::FromDate(Timestamp::MIN_YEAR, Timestamp::MIN_MONTH, Timestamp::MIN_DAY),
	// 	                        dtime_t(0));
	// case LogicalTypeId::TIMESTAMP_SEC:
	// 	return MinimumValue(LogicalType::TIMESTAMP).DefaultCastAs(LogicalType::TIMESTAMP_S);
	// case LogicalTypeId::TIMESTAMP_MS:
	// 	return MinimumValue(LogicalType::TIMESTAMP).DefaultCastAs(LogicalType::TIMESTAMP_MS);
	// case LogicalTypeId::TIMESTAMP_NS:
	// 	return Value::TIMESTAMPNS(timestamp_t(NumericLimits<int64_t>::Minimum()));
	// case LogicalTypeId::TIME_TZ:
	// 	return Value::TIMETZ(dtime_t(0));
	// case LogicalTypeId::TIMESTAMP_TZ:
	// 	return Value::TIMESTAMPTZ(Timestamp::FromDatetime(
	// 	    Date::FromDate(Timestamp::MIN_YEAR, Timestamp::MIN_MONTH, Timestamp::MIN_DAY), dtime_t(0)));

	}
	
	duckdb::Value PyToDDB::convert(cpy::Object src, duckdb::LogicalType dest_type) {
		PyObject* pysrc = src.getpy();
		duckdb::Value val = convert(pysrc, dest_type);
		Py_DECREF(pysrc);
		return val;
	}

	duckdb::dtime_t PyToDDB::toDtime(cpy::Object src) {
		int32_t hour = src.attr("hour").toInt();
		int32_t minute = src.attr("minute").toInt();
		int32_t second = src.attr("second").toInt();
		int32_t microsec = src.attr("microsecond").toInt();
		duckdb::dtime_t time = duckdb::Time::FromTime(hour, minute, second, microsec);
		return time;
	}

	duckdb::date_t PyToDDB::toDate_t(cpy::Object src) {
		int32_t year = src.attr("year").toInt();
		int32_t month = src.attr("month").toInt();
		int32_t day = src.attr("day").toInt();
		// DUCKDB_API static date_t FromDate(int32_t year, int32_t month, int32_t day);
		duckdb::date_t date = duckdb::Date::FromDate(year, month, day);
		return date;
	}

	duckdb::Value PyToDDB::toTime(cpy::Object src) {
		duckdb::dtime_t time = toDtime(src);
		return duckdb::Value::TIME(time);
	}

	duckdb::Value PyToDDB::toDate(cpy::Object src) {
		duckdb::date_t date = toDate_t(src);
		return duckdb::Value::DATE(date);
	}

	cpy::Object PyToDDB::dtToUtc(cpy::Object src) {
		cpy::Object src_tz = src.attr("tzinfo");
		if (src_tz.isnone()) {
			// No timezone specified so we don't do anything.
			return src;
		} else {
			cpy::Object args = cpy::tuple({});
			cpy::Object func = src.attr("astimezone");
			cpy::Object result = func.call(args);
			return result;
		}
	}

	duckdb::Value PyToDDB::toTimeStampTZ(cpy::Object src) {
		try {
			// DUCKDB_API static Value TIMESTAMP(date_t date, dtime_t time);
			// DUCKDB_API static Value TIMESTAMP(timestamp_t timestamp);
			// DUCKDB_APIn static Value TIMESTAMPTZ(timestamp_t timestamp);
			// DUCKDB_API static Value TIMESTAMP(int32_t year, int32_t month, int32_t day, int32_t hour, int32_t min, int32_t sec,
			//                               int32_t micros);
			// cpy::Object tz = src.callattr("tzinfo");
			
			// if (tz.isnone()) {
			// no timezone, do nothing.
			// }
			src = dtToUtc(src);
			duckdb::dtime_t time = toDtime(src);
			duckdb::date_t date = toDate_t(src);
			duckdb::timestamp_t ts = duckdb::Timestamp::FromDatetime(date, time);
			return duckdb::Value::TIMESTAMPTZ(ts);
		} catch (cpy::Exception &ex) {
			debug("Python Error converting datetime.datetime: " + ex.message());
			return duckdb::Value((std::nullptr_t)NULL);
		}
	}
		
	
} // namespace pyudf
