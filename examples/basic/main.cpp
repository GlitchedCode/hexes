// examples/basic/main.cpp
//
// Demonstrates the hexes library API without FTXUI:
//   1. JSON / YAML / binary round-trip via hexes::serialization
//   2. Lua scripting via sol2
//   3. Live hot-reload detection via hexes::LuaHotReloader
//
// Build and run from the repo root:
//   cmake -B build && cmake --build build -j
//   ./build/example_basic
//
// While the example is running, edit scripts/example.lua and save —
// you'll see the new greet() output printed within ~250 ms.

#include <hexes/fs/hot_reloader.hpp>
#include <hexes/serialization.hpp>
#include <hexes/transform2d.hpp>

#include <sol/sol.hpp>

#include <chrono>
#include <cstdio>
#include <filesystem>
#include <thread>

// ── 1. A simple struct to serialize ──────────────────────────────────────────

struct Config {
  std::string name = "hexes-example";
  int version = 1;
  bool debug = true;
};

template <> struct glz::meta<Config> {
  using T = Config;
  static constexpr auto value =
      glz::object("name", &T::name, "version", &T::version, "debug", &T::debug);
};

// ── Helper: run greet() from the loaded Lua module ───────────────────────────

static void call_greet(sol::state &lua) {
  sol::table mod = lua["package"]["loaded"]["example"];
  if (!mod.valid()) {
    std::printf("  (example module not loaded)\n");
    return;
  }
  sol::protected_function fn = mod["greet"];
  if (!fn.valid()) {
    std::printf("  (greet() not found)\n");
    return;
  }
  auto result = fn("world");
  if (result.valid()) {
    std::printf("  lua: %s\n", result.get<std::string>().c_str());
  }
}

// ── Main
// ──────────────────────────────────────────────────────────────────────

int main() {
  // ── Serialization demo ────────────────────────────────────────────────────
  std::puts("=== hexes serialization ===");

  Config cfg;

  // JSON
  auto json = hexes::to_json(cfg);
  std::printf("JSON  : %s\n", json.value_or("(error)").c_str());

  Config cfg2;
  hexes::from_json(cfg2, json.value_or("{}"));
  std::printf("  round-trip name: %s\n", cfg2.name.c_str());

  // YAML
  auto yaml = hexes::to_yaml(cfg);
  std::printf("YAML  :\n%s\n", yaml.value_or("(error)").c_str());

  // Binary
  auto bin = hexes::to_binary(cfg);
  std::printf("binary: %zu bytes\n\n", bin ? bin->size() : 0u);

  // ── Transform2D demo ──────────────────────────────────────────────────────
  std::puts("=== hexes Transform2D ===");

  hexes::Transform2D t;
  t.position = {3.f, 4.f};
  t.rotation = 3.14159f / 4.f; // 45°
  t.scale    = {2.f, 2.f};

  auto t_json = hexes::to_json(t);
  std::printf("JSON  : %s\n", t_json.value_or("(error)").c_str());

  // Round-trip through YAML
  auto t_yaml = hexes::to_yaml(t);
  std::printf("YAML  :\n%s\n", t_yaml.value_or("(error)").c_str());

  hexes::Transform2D t2;
  if (t_yaml) { [[maybe_unused]] bool ok = hexes::from_yaml(t2, *t_yaml); }
  std::printf("  round-trip position: (%.2f, %.2f)\n\n", t2.position.x, t2.position.y);

  // Apply and compose
  hexes::Vec2 p = t.apply({1.f, 0.f});
  std::printf("  apply({1,0}): (%.4f, %.4f)\n", p.x, p.y);

  hexes::Transform2D child;
  child.position = {1.f, 0.f};
  child.rotation = 3.14159f / 4.f;
  hexes::Transform2D world = t.compose(child);
  std::printf("  compose position: (%.4f, %.4f)\n\n", world.position.x, world.position.y);

  // ── Lua scripting demo ────────────────────────────────────────────────────
  std::puts("=== hexes lua scripting ===");

  sol::state lua;
  lua.open_libraries(sol::lib::base, sol::lib::package, sol::lib::string);

  const std::filesystem::path script = "scripts/example.lua";

  if (!std::filesystem::exists(script)) {
    std::printf("Script not found: %s\n", script.c_str());
    std::printf("Run from the directory containing the 'scripts/' folder.\n");
    return 1;
  }

  // Initial load
  lua.require_file("example", script.string());
  call_greet(lua);

  // ── Hot-reload demo ───────────────────────────────────────────────────────
  std::puts("\n=== hot-reload (edit scripts/example.lua to trigger) ===");
  std::printf("Watching for changes for 10 seconds...\n\n");

  hexes::fs::FileWatcher::instance().initialize_watchers();
  auto reloader =
      hexes::fs::FileWatcher::instance().watch_file(script);

  auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{10};

  while (std::chrono::steady_clock::now() < deadline) {
    std::this_thread::sleep_for(std::chrono::milliseconds{100});

    if (reloader->poll_and_reset()) {
      std::puts("[hot-reload] change detected — reloading script...");

      // Clear the cached module so require re-executes the file.
      sol::table loaded = lua["package"]["loaded"];
      loaded["example"] = sol::lua_nil;

      lua.require_file("example", script.string());
      call_greet(lua);
    }
  }

  std::puts("\ndone.");
  return 0;
}
