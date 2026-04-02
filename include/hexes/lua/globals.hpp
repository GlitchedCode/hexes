

#include <sol/forward.hpp>
#include <sol/state.hpp>
namespace hexes::lua {

  class Globals {
    public:
      explicit Globals(sol::state& lua);
  
      [[nodiscard]] sol::table config() const noexcept;

      void configure_lua_module(sol::object& module);
  
    private:
      sol::state& lua_;
  };

}
