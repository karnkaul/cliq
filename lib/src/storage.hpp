#pragma once
#include <cliq/app_info.hpp>
#include <cliq/command.hpp>
#include <optional>

namespace cliq::old {
struct OptionKey {
	char letter{};
	std::string_view word{};

	static constexpr auto is_space(char const c) { return c == ' ' || c == '\t'; }

	static constexpr void trim(std::string_view& out) {
		while (is_space(out.front())) { out = out.substr(1); }
		while (is_space(out.back())) { out = out.substr(0, out.size() - 1); }
	}

	static constexpr auto make(std::string_view const input) -> OptionKey {
		auto ret = OptionKey{.word = input};
		trim(ret.word);
		if (ret.word.size() == 1) {
			ret.letter = ret.word.front();
			ret.word = {};
			return ret;
		}
		if (ret.word.size() < 3 || ret.word[1] != ',') { return ret; }
		ret.letter = ret.word.front();
		ret.word = ret.word.substr(2);
		return ret;
	}
};

struct Option {
	OptionKey key{};
	std::string_view name{};
	std::string_view description{};
	std::string print_key{};
	std::unique_ptr<IBinding> binding{};
};

struct Argument {
	std::string_view name{};
	std::string_view description{};
	std::unique_ptr<IBinding> binding{};
	bool assigned{};
};

struct Builtin {
	std::string_view word{};
	std::string_view description{};
	std::string print_key{};

	explicit Builtin(std::string_view word, std::string_view description);

	[[nodiscard]] static auto usage() -> Builtin;
	[[nodiscard]] static auto help() -> Builtin;
	[[nodiscard]] static auto version() -> Builtin;
};

using ExecInfo = AppInfo;

struct Storage {
	enum class ArgType : int { Required, List, Implicit };

	std::vector<Option> options{};

	std::vector<Argument> arguments{};
	std::optional<Argument> list_argument{};
	std::optional<Argument> implicit_argument{};
	std::string args_text{};

	std::vector<std::unique_ptr<Command>> commands{};

	std::vector<Builtin> builtins{};

	ExecInfo exec_info{};

	static auto get_print_key(char letter, std::string_view word) -> std::string;

	void bind_option(BindInfo info, std::string_view key);
	void bind_argument(BindInfo info, ArgType type);
};
} // namespace cliq::old
