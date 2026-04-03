#include "hexes/fs/hot_reloader.hpp"
#include <hexes/lua/behavior.hpp>
#include <sol/state.hpp>

namespace hexes::lua {

template <typename T>
Behavior<T>::Behavior(std::string script_path) : reloader_(script_path, [this](fs::HotReloader hr) { _reload(hr); }) {
  // module_ = std::make_unique<sol::table>(reloader_.get_module());
}

template<typename T>
T Behavior<T>::update() {
  return T();
}

template<typename T>
void Behavior<T>::_reload() {
  // module_ = std::make_unique<sol::table>(reloader_.get_module());
  // using c++26's reflection api get the abstract methods from T and check if they are implemented in the module_ table, if not throw an error
  
  

}
}
