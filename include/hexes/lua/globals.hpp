

#include <sol/forward.hpp>
#include <sol/state.hpp>
namespace hexes::lua {

  class Globals {
    public:
  
      [[nodiscard]] sol::table config() const noexcept;

      void configure_lua_module(sol::object& module);
      
      static sol::state& lua() noexcept { return Globals::instance().lua_; }
      static Globals& instance();
  
    private:
      sol::state lua_;
      Globals();
      ~Globals() = default;
  };

}
