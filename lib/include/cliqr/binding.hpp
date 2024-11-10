#pragma once
#include <cliqr/polymorphic.hpp>
#include <array>
#include <charconv>
#include <concepts>
#include <string>
#include <vector>

namespace cliqr {
template <typename Type>
concept StringyT = std::same_as<Type, std::string> || std::same_as<Type, std::string_view>;

template <typename Type>
concept NumberT = std::integral<Type> || std::floating_point<Type>;

template <typename Type>
concept NotBoolT = !std::same_as<Type, bool>;

/// \brief Interface for binding options and arguments to variables.
class IBinding : Polymorphic {
  public:
	/// \brief Check if the binding a flag.
	/// Flags can be entered as concatenated letters, thus such bindings must not require a value for assignment.
	/// \returns true if flag.
	[[nodiscard]] virtual auto is_flag() const -> bool = 0;

	/// \brief Assign an argument to the bound parameter.
	/// \param value Argument to bind.
	/// \returns true if successful.
	[[nodiscard]] virtual auto assign_argument(std::string_view value) const -> bool = 0;

	/// \brief Append the default value of the bound parameter.
	/// \param out String to append to.
	virtual void append_default_value(std::string& out) const = 0;
};

/// \brief Customization point.
template <typename Type>
class Binding;

template <>
class Binding<bool> : public IBinding {
  public:
	bool& ref; // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)

	explicit Binding(bool& out) : ref(out) {}

	[[nodiscard]] auto is_flag() const -> bool final { return true; }

	[[nodiscard]] auto assign_argument(std::string_view value) const -> bool final {
		ref = !(value == "false" || value == "0");
		return true;
	}

	void append_default_value(std::string& out) const final { out += (ref ? "true" : "false"); }
};

template <StringyT Type>
class Binding<Type> : public IBinding {
  public:
	Type& ref; // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)

	explicit Binding(Type& out) : ref(out) {}

	[[nodiscard]] auto is_flag() const -> bool final { return false; }

	[[nodiscard]] auto assign_argument(std::string_view value) const -> bool final {
		ref = Type{value};
		return true;
	}

	void append_default_value(std::string& out) const final { out += ref; }
};

template <NumberT Type>
class Binding<Type> : public IBinding {
  public:
	Type& ref; // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)

	explicit Binding(Type& out) : ref(out) {}

	[[nodiscard]] auto is_flag() const -> bool final { return false; }

	[[nodiscard]] auto assign_argument(std::string_view value) const -> bool final {
		auto const* end = value.data() + value.size();
		auto const [ptr, ec] = std::from_chars(value.data(), end, ref);
		return ec == std::errc{} && ptr == end;
	}

	void append_default_value(std::string& out) const final {
		auto buf = std::array<char, 16>{};
		std::to_chars(buf.data(), buf.data() + buf.size(), ref);
		out += buf.data();
	}
};

/// \brief Binding for an arbitary number of arguments.
template <typename Type>
class ListBinding : public IBinding {
  public:
	std::vector<Type>& ref; // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)

	explicit ListBinding(std::vector<Type>& out) : ref(out) {}

	[[nodiscard]] auto is_flag() const -> bool final { return false; }

	[[nodiscard]] auto assign_argument(std::string_view value) const -> bool final {
		auto t = Type{};
		if (!Binding<Type>{t}.assign_argument(value)) { return false; }
		ref.push_back(std::move(t));
		return true;
	}

	void append_default_value(std::string& out) const final { out += "..."; }
};
} // namespace cliqr
