#pragma once
#include <cliqr/binding.hpp>
#include <memory>
#include <span>

namespace cliqr {
enum struct CommandResult : int {
	Success = 0,
	Failure = 1,

	InvalidArgument = 100,
	MissingArgument,
	MissingOption,
	MissingCommand,
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

	[[nodiscard]] auto parse_args(std::string_view app_name, std::span<char const* const> args) -> Result;
	[[nodiscard]] auto should_execute() const -> bool;

  protected:
	struct BindInfo {
		std::unique_ptr<IBinding> binding{};
		std::string_view name{};
		std::string_view description{};
	};

	void bind_option(std::string_view key, BindInfo info, bool required) const;
	void bind_argument(BindInfo info, bool is_list) const;

	template <typename Type>
	void required(Type& out, std::string_view const key, std::string_view const name, std::string_view const description) const {
		auto binding = std::make_unique<Binding<Type>>(out);
		bind_option(key, {std::move(binding), name, description}, true);
	}

	template <typename Type>
	void optional(Type& out, std::string_view const key, std::string_view const name, std::string_view const description) const {
		auto binding = std::make_unique<Binding<Type>>(out);
		bind_option(key, {std::move(binding), name, description}, false);
	}

	void flag(bool& out, std::string_view key, std::string_view name, std::string_view description) const;

	template <typename Type>
	void argument(Type& out, std::string_view const name, std::string_view const description) const {
		auto binding = std::make_unique<Binding<Type>>(out);
		bind_argument({std::move(binding), name, description}, false);
	}

	template <typename Type>
		requires(!std::same_as<Type, bool>)
	void arguments(std::vector<Type>& out, std::string_view const name, std::string_view const description) const {
		auto binding = std::make_unique<ListBinding<Type>>(out);
		bind_argument({std::move(binding), name, description}, true);
	}

  private:
	struct Impl;
	struct Deleter {
		void operator()(Impl* ptr) const noexcept;
	};

	std::unique_ptr<Impl, Deleter> m_impl{};
};
} // namespace cliqr
