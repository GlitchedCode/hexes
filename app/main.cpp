// app/main.cpp — interactive FTXUI application with Lua hot-reload
//
// Controls:
//   q / Ctrl-C  exit

#include <hexes/fs/hot_reloader.hpp>
#include <hexes/serialization.hpp>

#include <sol/forward.hpp>
#include <sol/sol.hpp>

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include <atomic>
#include <chrono>
#include <filesystem>
#include <string>
#include <thread>

// ── Application config (serialized to JSON on startup) ───────────────────────

struct AppConfig {
    std::string title  = "hexes";
    int         width  = 80;
    int         height = 24;
};

template<> struct glz::meta<AppConfig> {
    using T = AppConfig;
    static constexpr auto value = glz::object(
        "title",  &T::title,
        "width",  &T::width,
        "height", &T::height
    );
};

// ── Lua module helper ─────────────────────────────────────────────────────────

// Clears the module cache, re-requires the file, and calls greet("world").
// Returns the greeting string, or an error description.
static std::string load_example(sol::state& lua, const std::filesystem::path& path) {
    sol::table loaded = lua["package"]["loaded"];
    loaded["example"] = sol::lua_nil;

    sol::object mod = lua.require_file("example", path.string());
    if (!mod.valid()) return "(load error)";

    sol::table t = mod;
    sol::protected_function fn = t["greet"];
    if (!fn.valid()) return "(greet() not found)";

    auto result = fn("world");
    if (!result.valid()) {
        sol::error err = result;
        return std::string("(call error: ") + err.what() + ")";
    }
    return result.get<std::string>();
}

// ── Main ──────────────────────────────────────────────────────────────────────

int main() {
    // ── Config JSON round-trip ────────────────────────────────────────────────
    AppConfig cfg;
    auto json = hexes::to_json(cfg).value_or("{}");

    // ── Lua state ─────────────────────────────────────────────────────────────
    sol::state lua;
    lua.open_libraries(sol::lib::base, sol::lib::package, sol::lib::string);

    const std::filesystem::path script = "scripts/example.lua";
    std::string lua_output = std::filesystem::exists(script)
        ? load_example(lua, script)
        : "(script not found)";

    // ── Hot-reload watcher ────────────────────────────────────────────────────
    hexes::fs::HotReloader reloader{script};

    // ── FTXUI interactive screen ──────────────────────────────────────────────
    using namespace ftxui;

    auto screen = ScreenInteractive::Fullscreen();
    std::atomic<bool> app_running{true};

    // Refresh thread — drives the event loop at ~10 Hz so hot-reload is snappy.
    std::thread refresh([&] {
        while (app_running.load(std::memory_order_relaxed)) {
            std::this_thread::sleep_for(std::chrono::milliseconds{100});
            screen.PostEvent(Event::Custom);
        }
    });

    auto renderer = Renderer([&]() -> Element {
        // Service hot-reload on the FTXUI (main) thread — sol::state is not thread-safe.
        if (reloader.poll_and_reset()) {
            lua_output = load_example(lua, script);
        }

        return vbox({
            text(" " + cfg.title + " ") | bold | color(Color::Cyan) | hcenter,
            separator(),
            hbox({text("config JSON : "), text(json)       | color(Color::Green)}),
            hbox({text("lua output  : "), text(lua_output) | color(Color::Yellow)}),
            hbox({text("hot-reload  : "), text("watching " + reloader.path().string()) | color(Color::Blue)}),
            separator(),
            text("press q to quit") | color(Color::GrayDark) | hcenter,
        });
    });

    auto component = CatchEvent(renderer, [&](Event event) {
        if (event == Event::Character('q') || event == Event::Character('Q')) {
            screen.ExitLoopClosure()();
            return true;
        }
        return false;
    });

    screen.Loop(component);

    app_running.store(false, std::memory_order_relaxed);
    refresh.join();

    return 0;
}
