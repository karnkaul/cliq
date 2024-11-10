#pragma once
#include <cliqr/polymorphic.hpp>
#include <charconv>
#include <concepts>
#include <memory>
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

	/// \brief Get the default value of the bound parameter.
	/// \returns Default value of bound parameter.
	[[nodiscard]] virtual auto get_default_value() const -> std::string = 0;
};

/// \brief Used for custom bindings.
struct BindInfo {
	std::unique_ptr<IBinding> binding{};
	std::string_view name{};
	std::string_view description{};
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

	[[nodiscard]] auto get_default_value() const -> std::string final { return ref ? "true" : "false"; }
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

	[[nodiscard]] auto get_default_value() const -> std::string final { return std::string{ref}; }
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

	[[nodiscard]] auto get_default_value() const -> std::string final { return std::to_string(ref); }
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

	[[nodiscard]] auto get_default_value() const -> std::string final { return "..."; }
};
} // namespace cliqr
