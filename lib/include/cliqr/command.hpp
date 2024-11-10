#pragma once
#include <cliqr/binding.hpp>
#include <memory>

namespace cliqr {
enum struct CommandResult : int {
	Success = 0,
	Failure = 1,

	ParseErrorFirst = 1000,
	InvalidCommand = ParseErrorFirst,
	InvalidOption,
	InvalidValue,
	InvalidArgument,
	MissingArgument,
	ParseErrorLast = MissingArgument,
};

class Command : public Polymorphic {
  public:
	using Result = CommandResult;

	explicit Command();

	[[nodiscard]] virtual auto get_id() const -> std::string_view = 0;
	[[nodiscard]] virtual auto get_tagline() const -> std::string_view = 0;

	[[nodiscard]] virtual auto execute() -> Result = 0;

	void print_usage() const;
	void print_help() const;

  protected:
	struct BindInfo {
		std::unique_ptr<IBinding> binding{};
		std::string_view name{};
		std::string_view description{};
	};

	void bind_option(std::string_view key, BindInfo info) const;
	void bind_argument(BindInfo info, bool is_list) const;

	template <typename Type>
	void required(Type& out, std::string_view const name, std::string_view const description) const {
		auto binding = std::make_unique<Binding<Type>>(out);
		bind_argument({std::move(binding), name, description}, false);
	}

	template <typename Type>
	void optional(Type& out, std::string_view const key, std::string_view const name, std::string_view const description) const {
		auto binding = std::make_unique<Binding<Type>>(out);
		bind_option(key, {std::move(binding), name, description});
	}

	void flag(bool& out, std::string_view key, std::string_view name, std::string_view description) const;

	template <NotBoolT Type>
	void list(std::vector<Type>& out, std::string_view const name, std::string_view const description) const {
		auto binding = std::make_unique<ListBinding<Type>>(out);
		bind_argument({std::move(binding), name, description}, true);
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
