

#include <sol/sol.hpp>
#include <hexes/lua/globals.hpp>

int test_call() {
  return 42;
}

namespace hexes::lua {

Globals::Globals() {

  lua_.open_libraries(sol::lib::base, sol::lib::package, sol::lib::string);
  // Expose a global `config` table to Lua, which is read-only and reflects
  // the current AppConfig values. This is just an example; in a real app you
  // might want to support writing to the config from Lua, or have multiple
  // tables for different subsystems.
  lua_["config"] = lua_.create_table_with(
    "title",  "hexes",
    "width",  80,
    "height", 24
  );

  // expose a test function to Lua to demonstrate that this class can be used for arbitrary globals, not just
  lua_["test_call"] = &test_call;
}

// inject globals into a module
void Globals::configure_lua_module(sol::object& module) {
  if (!module.is<sol::table>()) std::invalid_argument("module must be a table");

  sol::table t = module;
  t["test_call"] = &test_call;
}


Globals& Globals::instance() {
  static Globals instance;
  return instance;
}

}
