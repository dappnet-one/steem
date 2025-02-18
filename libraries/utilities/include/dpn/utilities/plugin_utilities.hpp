#include <fc/io/json.hpp>

namespace dpn { namespace plugins {

template<typename T>
T dejsonify(const std::string& s) {
   return fc::json::from_string(s).as<T>();
}

// TODO: Move this somewhere else. Also exists in app/plugin.hpp, which will be removed.
#ifndef DPN_LOAD_VALUE_SET
#define DPN_LOAD_VALUE_SET(options, name, container, type) \
if( options.count(name) ) { \
   const std::vector<std::string>& ops = options[name].as<std::vector<std::string>>(); \
   std::transform(ops.begin(), ops.end(), std::inserter(container, container.end()), &dejsonify<type>); \
}
#endif

} } // dpn::plugins
