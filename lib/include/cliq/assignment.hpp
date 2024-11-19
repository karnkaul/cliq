#pragma once
#include <cliq/concepts.hpp>
#include <charconv>
#include <vector>

namespace cliq {
using Assignment = bool (*)(void* binding, std::string_view value);

inline auto assign_to(bool& out, std::string_view /*value*/) -> bool {
	out = true;
	return true;
}

template <NumberT Type>
auto assign_to(Type& out, std::string_view const value) {
	auto const* last = value.begin() + value.size();
	auto const [ptr, ec] = std::from_chars(value.begin(), last, out);
	return ptr == last && ec == std::errc{};
}

template <StringyT Type>
auto assign_to(Type& out, std::string_view const value) -> bool {
	out = value;
	return true;
}

template <typename Type>
auto assign_to(std::vector<Type>& out, std::string_view const value) -> bool {
	auto t = Type{};
	if (!assign_to(t, value)) { return false; }
	out.push_back(std::move(t));
	return true;
}

template <typename Type>
[[nodiscard]] auto assignment() -> Assignment {
	return [](void* binding, std::string_view const value) { return assign_to(*static_cast<Type*>(binding), value); };
}
} // namespace cliq
