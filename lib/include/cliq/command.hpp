#pragma once
#include <cliq/binding_old.hpp>
#include <cliq/build_version.hpp>
#include <cliq/result_old.hpp>

namespace cliq::old {
/// \brief Abstract base for executable commands with bound options and arguments.
class Command : public Polymorphic {
  public:
	using Result = cliq::old::Result;

	explicit Command();

	/// \brief Get the string ID of this command.
	/// \returns ID of this command.
	[[nodiscard]] virtual auto get_id() const -> std::string_view = 0;

	/// \brief Get the help description for this command.
	/// \returns Help description for this command.
	[[nodiscard]] virtual auto get_description() const -> std::string_view { return get_id(); }

	/// \brief Get the help epilogue for this command.
	/// \returns Help epilogue for this command.
	[[nodiscard]] virtual auto get_epilogue() const -> std::string_view { return {}; }

	/// \brief Execute this command.
	/// \returns Result of execution.
	[[nodiscard]] virtual auto execute() -> int = 0;

	void print_usage() const;
	void print_help() const;

  protected:
	enum class ArgType : int {
		Required, // General positional argument
		List,	  // Last positional argument: variadic
		Implicit  // Last positional argument: optional
	};

	/// \brief Bind an option.
	/// \param info Binding info.
	/// \param key Key for the option.
	void bind_option(BindInfo info, std::string_view key) const;
	/// \brief Bind an argument.
	/// \param info Binding info.
	/// \param type Argument type.
	void bind_argument(BindInfo info, ArgType type) const;

	/// \brief Bind a required positional argument. Uses Binding<Type>.
	/// \param out Parameter to bind to.
	/// \param name Name of argument. Displayed in help / usage.
	/// \param description Description of argument. Displayed in help / usage.
	template <typename Type>
	void required(Type& out, std::string_view const name, std::string_view const description) const {
		bind_argument({std::make_unique<Binding<Type>>(out), name, description}, ArgType::Required);
	}

	/// \brief Bind an optional named argument. Uses Binding<Type>.
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

	/// \brief Bind the last positional arguments as a list. Uses ListBinding<Type>.
	/// \param out Parameter to bind to.
	/// \param name Name of argument. Displayed in help / usage.
	/// \param description Description of argument. Displayed in help / usage.
	/// Cannot bind any more positional arguments after this.
	template <NotBoolT Type>
	void list(std::vector<Type>& out, std::string_view const name, std::string_view const description) const {
		bind_argument({std::make_unique<ListBinding<Type>>(out), name, description}, ArgType::List);
	}

	/// \brief Bind the last positional argument as optional/implicit.
	/// \param out Parameter to bind to.
	/// \param name Name of argument. Displayed in help / usage.
	/// \param description Description of argument. Displayed in help / usage.
	/// Cannot bind any more positional arguments after this.
	template <NotBoolT Type>
	void implicit(Type& out, std::string_view const name, std::string_view const description) const {
		bind_argument({std::make_unique<Binding<Type>>(out), name, description}, ArgType::Implicit);
	}

  private:
	struct Impl;
	struct Deleter {
		void operator()(Impl* ptr) const noexcept;
	};

	std::unique_ptr<Impl, Deleter> m_impl{};

	friend class CommandApp;
	friend class CommandListApp;
};
} // namespace cliq::old
