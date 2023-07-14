

#ifndef CPY_FUNCTION_HPP
#define CPY_FUNCTION_HPP

#include <Python.h>
#include <cpy/object.hpp>

namespace cpy {
class Function : public cpy::Object {
public:
	using cpy::Object::Object;
	Function(cpy::Object src);
};
cpy::Function import(const std::string &module_name, const std::string &function_name);
} // namespace cpy
#endif // CPY_FUNCTION_HPP
