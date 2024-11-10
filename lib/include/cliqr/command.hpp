#pragma once
#include <cliqr/binding.hpp>
#include <cliqr/result.hpp>

namespace cliqr {
/// \brief Abstract base for customizable commands.
class Command : public Polymorphic {
  public:
	using Result = cliqr::Result;

	explicit Command();

	/// \brief Get the string ID of this command.
	/// \returns ID of this command.
	[[nodiscard]] virtual auto get_id() const -> std::string_view = 0;

	/// \brief Get the help tagline for this command.
	/// \returns Tagline for this command.
	[[nodiscard]] virtual auto get_tagline() const -> std::string_view = 0;

	/// \brief Execute this command.
	/// \returns Result of execution.
	[[nodiscard]] virtual auto execute() -> int = 0;

	void print_usage() const;
	void print_help() const;

  protected:
	/// \brief Bind an option.
	/// \param info Binding info.
	/// \param key Key for the option.
	void bind_option(BindInfo info, std::string_view key) const;
	/// \brief Bind an argument.
	/// \param info Binding info.
	/// \param is_list Whether the binding is a list.
	void bind_argument(BindInfo info, bool is_list) const;

	/// \brief Bind a required argument. Uses Binding<Type>.
	/// \param out Parameter to bind to.
	/// \param name Name of argument. Displayed in help / usage.
	/// \param description Description of argument. Displayed in help / usage.
	template <typename Type>
	void required(Type& out, std::string_view const name, std::string_view const description) const {
		bind_argument({std::make_unique<Binding<Type>>(out), name, description}, false);
	}

	/// \brief Bind an optional argument. Uses Binding<Type>.
	/// \param out Parameter to bind to.
	/// \param key Key of option in the form of 'f,foo'. Use 'f' for letter-only, and 'foo' for word-only.
	/// \param name Name of option. Displayed in help / usage.
	/// \param description Description of option. Displayed in help / usage.
	template <typename Type>
	void optional(Type& out, std::string_view const key, std::string_view const name, std::string_view const description) const {
		bind_option({std::make_unique<Binding<Type>>(out), name, description}, key);
	}

	/// \brief Bind an optional flag (boolean option). Uses Binding<bool>.
	/// \param out Parameter to bind to.
	/// \param key Key of option in the form of 'f,foo'. Use 'f' for letter-only, and 'foo' for word-only.
	/// \param name Name of option. Displayed in help / usage.
	/// \param description Description of option. Displayed in help / usage.
	void flag(bool& out, std::string_view key, std::string_view name, std::string_view description) const;

	/// \brief Bind a list argument. Uses ListBinding<Type>.
	/// \param out Parameter to bind to.
	/// \param name Name of argument. Displayed in help / usage.
	/// \param description Description of argument. Displayed in help / usage.
	template <NotBoolT Type>
	void list(std::vector<Type>& out, std::string_view const name, std::string_view const description) const {
		bind_argument({std::make_unique<ListBinding<Type>>(out), name, description}, true);
	}

  private:
	struct Impl;
	struct Deleter {
		void operator()(Impl* ptr) const noexcept;
	};

	std::unique_ptr<Impl, Deleter> m_impl{};

	friend class Instance;
};
} // namespace cliqr
