

#include "hexes/fs/hot_reloader.hpp"
#include <memory>
#include <sol/forward.hpp>
#include <string>

namespace hexes::lua {
  template<typename T>
  class Behavior {
    public:
      Behavior<T>(std::string script_path);
      Behavior<T>(const Behavior<T>&) = delete;
      virtual ~Behavior() = default;

      [[nodiscard]] virtual T update() = 0;

      virtual void _reload() = 0;

    private:
      fs::HotReloader reloader_;
      std::unique_ptr<sol::table> module_;
  };
}
