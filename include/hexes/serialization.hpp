#pragma once

#include <glaze/beve/read.hpp>
#include <glaze/beve/write.hpp>
#include <glaze/glaze.hpp>
#include <yaml-cpp/yaml.h>

#include <optional>
#include <string>
#include <string_view>
#include <vector>

// Callers must provide a glz::meta<T> specialization for their types, e.g.:
//
//   template<> struct glz::meta<MyStruct> {
//       using T = MyStruct;
//       static constexpr auto value = glz::object("x", &T::x, "y", &T::y);
//   };
//
// The same specialization is used for JSON, YAML, and binary — no extra work
// needed per format.

namespace hexes {

// ── JSON ──────────────────────────────────────────────────────────────────────

template<class T>
[[nodiscard]] std::optional<std::string> to_json(const T& value) {
    std::string out;
    if (auto ec = glz::write_json(value, out); ec) return std::nullopt;
    return out;
}

template<class T>
[[nodiscard]] bool from_json(T& value, std::string_view input) {
    return !glz::read_json(value, input);
}

// ── Binary (BEVE) ─────────────────────────────────────────────────────────────

template<class T>
[[nodiscard]] std::optional<std::vector<std::byte>> to_binary(const T& value) {
    std::vector<std::byte> out;
    if (auto ec = glz::write_beve(value, out); ec) return std::nullopt;
    return out;
}

template<class T>
[[nodiscard]] bool from_binary(T& value, const std::vector<std::byte>& input) {
    return !glz::read_beve(value, input);
}

// ── YAML ──────────────────────────────────────────────────────────────────────
// Strategy: glaze owns the schema (via glz::meta<T>).
//   to_yaml  — serialize T to JSON via glaze, parse that JSON as a YAML node
//              (JSON is valid YAML), then re-emit in YAML syntax.
//   from_yaml — parse YAML into a node tree, convert that tree to a JSON string,
//               then parse with glaze. One glz::meta<T> covers all three formats.

namespace detail {

// Recursively convert a YAML::Node to a JSON string.
inline std::string yaml_to_json(const YAML::Node& node) {
    switch (node.Type()) {
        case YAML::NodeType::Null:
            return "null";

        case YAML::NodeType::Scalar: {
            auto s = node.as<std::string>();
            // Pass booleans and numbers through unquoted.
            if (s == "true" || s == "false" || s == "null") return s;
            // Check for a number: integer or float.
            try {
                std::size_t pos{};
                std::stoll(s, &pos);
                if (pos == s.size()) return s;
            } catch (...) {}
            try {
                std::size_t pos{};
                std::stod(s, &pos);
                if (pos == s.size()) return s;
            } catch (...) {}
            // Escape as a JSON string.
            std::string out;
            out.reserve(s.size() + 2);
            out += '"';
            for (unsigned char c : s) {
                if      (c == '"')  out += "\\\"";
                else if (c == '\\') out += "\\\\";
                else if (c == '\n') out += "\\n";
                else if (c == '\r') out += "\\r";
                else if (c == '\t') out += "\\t";
                else if (c < 0x20) { // other control chars
                    char buf[7];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", c);
                    out += buf;
                }
                else out += static_cast<char>(c);
            }
            out += '"';
            return out;
        }

        case YAML::NodeType::Sequence: {
            std::string out = "[";
            bool first = true;
            for (const auto& item : node) {
                if (!first) out += ',';
                out += yaml_to_json(item);
                first = false;
            }
            out += ']';
            return out;
        }

        case YAML::NodeType::Map: {
            std::string out = "{";
            bool first = true;
            for (const auto& kv : node) {
                if (!first) out += ',';
                out += '"';
                out += kv.first.as<std::string>();
                out += "\":";
                out += yaml_to_json(kv.second);
                first = false;
            }
            out += '}';
            return out;
        }

        default:
            return "null";
    }
}

} // namespace detail

template<class T>
[[nodiscard]] std::optional<std::string> to_yaml(const T& value) {
    auto json = to_json(value);
    if (!json) return std::nullopt;
    try {
        YAML::Node node = YAML::Load(*json);
        YAML::Emitter emitter;
        emitter << node;
        return std::string{emitter.c_str()};
    } catch (const YAML::Exception&) {
        return std::nullopt;
    }
}

template<class T>
[[nodiscard]] bool from_yaml(T& value, std::string_view input) {
    try {
        YAML::Node node = YAML::Load(std::string{input});
        auto json = detail::yaml_to_json(node);
        return from_json(value, json);
    } catch (const YAML::Exception&) {
        return false;
    }
}

} // namespace hexes
