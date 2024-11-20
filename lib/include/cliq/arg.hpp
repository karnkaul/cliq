#pragma once
#include <cliq/assignment.hpp>
#include <cstdint>
#include <span>
#include <string_view>
#include <variant>

namespace cliq {
class Arg;

enum class ArgType : std::int8_t { Optional, Required };

struct ParamOption {
	Assignment assignment;
	void* binding;
	bool is_flag;
	char letter;
	std::string_view word;
	std::string_view help_text;

	[[nodiscard]] auto assign(std::string_view const value) const -> bool { return assignment(binding, value); }
};

struct ParamPositional {
	ArgType arg_type;
	Assignment assignment;
	void* binding;
	std::string_view name;
	std::string_view help_text;

	[[nodiscard]] constexpr auto is_required() const -> bool { return arg_type == ArgType::Required; }
	[[nodiscard]] auto assign(std::string_view const value) const -> bool { return assignment(binding, value); }
};

struct ParamCommand {
	std::span<Arg const> args;
	std::string_view name;
	std::string_view help_text;
};

using Param = std::variant<ParamOption, ParamPositional, ParamCommand>;

class Arg {
  public:
	// Named options
	Arg(bool& out, std::string_view key, std::string_view help_text = {});

	template <NumberT Type>
	Arg(Type& out, std::string_view const key, std::string_view const help_text = {})
		: m_param(ParamOption{assignment<Type>(), &out, false, to_letter(key), to_word(key), help_text}) {}

	template <StringyT Type>
	Arg(Type& out, std::string_view const key, std::string_view const help_text = {})
		: m_param(ParamOption{assignment<Type>(), &out, false, to_letter(key), to_word(key), help_text}) {}

	// Positional arguments
	template <NumberT Type>
	Arg(Type& out, ArgType const type, std::string_view const name, std::string_view const help_text = {})
		: m_param(ParamPositional{type, assignment<Type>(), &out, name, help_text}) {}

	template <StringyT Type>
	Arg(Type& out, ArgType const type, std::string_view const name, std::string_view const help_text = {})
		: m_param(ParamPositional{type, assignment<Type>(), &out, name, help_text}) {}

	// Commands
	Arg(std::span<Arg const> args, std::string_view name, std::string_view help_text = {});

	[[nodiscard]] auto get_param() const -> Param const& { return m_param; }

	static constexpr auto to_letter(std::string_view const key) -> char {
		if (key.size() == 1 || (key.size() > 2 && key[1] == ',')) { return key.front(); }
		return '\0';
	}

	static constexpr auto to_word(std::string_view const key) -> std::string_view {
		if (key.size() > 1) {
			if (key[1] == ',') { return key.substr(2); }
			return key;
		}
		return {};
	}

  private:
	Param m_param;
};
} // namespace cliq
