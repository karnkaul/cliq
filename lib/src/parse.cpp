#include <cliq/parse.hpp>
#include <parser.hpp>
#include <algorithm>
#include <print>
#include <utility>

namespace cliq {
namespace {
constexpr auto get_exe_name(std::string_view const arg0) -> std::string_view {
	auto const i = arg0.find_last_of("\\/");
	if (i == std::string_view::npos) { return arg0; }
	return arg0.substr(i + 1);
}

struct ErrorPrinter {
	ErrorPrinter(ErrorPrinter const&) = delete;
	ErrorPrinter(ErrorPrinter&&) = delete;
	auto operator=(ErrorPrinter const&) = delete;
	auto operator=(ErrorPrinter&&) = delete;

	explicit ErrorPrinter(std::string_view exe_name, std::string_view cmd_id = {}) : exe_name(exe_name), cmd_name(cmd_id) {
		str.reserve(400);
		append_error_prefix();
	}

	~ErrorPrinter() {
		if (helpline) { append_helpline(); }
		std::print(stderr, "{}", str);
	}

	[[nodiscard]] auto invalid_value(std::string_view const input, std::string_view const value) -> ParseError {
		helpline = false;
		std::format_to(std::back_inserter(str), "invalid {}: '{}'\n", input, value);
		return ParseError::InvalidArgument;
	}

	[[nodiscard]] auto invalid_option(char const letter) -> ParseError {
		std::format_to(std::back_inserter(str), "invalid option -- '{}'\n", letter);
		return ParseError::InvalidOption;
	}

	[[nodiscard]] auto unrecognized_option(std::string_view const input) -> ParseError {
		std::format_to(std::back_inserter(str), "unrecognized option '--{}'\n", input);
		return ParseError::InvalidOption;
	}

	[[nodiscard]] auto unrecognized_command(std::string_view const input) -> ParseError {
		std::format_to(std::back_inserter(str), "unrecognized command '{}'\n", input);
		return ParseError::InvalidCommand;
	}

	[[nodiscard]] auto extraneous_argument(std::string_view const input) -> ParseError {
		std::format_to(std::back_inserter(str), "extraneous argument '{}'\n", input);
		return ParseError::InvalidArgument;
	}

	[[nodiscard]] auto option_requires_argument(std::string_view const input) -> ParseError {
		if (input.size() == 1) {
			std::format_to(std::back_inserter(str), "option requires an argument -- '{}'\n", input);
		} else {
			std::format_to(std::back_inserter(str), "option '{}' requires an argument\n", input);
		}
		return ParseError::MissingArgument;
	}

	[[nodiscard]] auto option_is_flag(std::string_view const input) -> ParseError {
		if (input.size() == 1) {
			std::format_to(std::back_inserter(str), "option does not take an argument -- '{}'\n", input);
		} else {
			std::format_to(std::back_inserter(str), "option '{}' does not take an argument\n", input);
		}
		return ParseError::InvalidArgument;
	}

	[[nodiscard]] auto missing_argument(std::string_view name) -> ParseError {
		std::format_to(std::back_inserter(str), "missing {}\n", name);
		return ParseError::MissingArgument;
	}

	void append_error_prefix() {
		str += exe_name;
		if (!cmd_name.empty()) { std::format_to(std::back_inserter(str), " {}", cmd_name); }
		str += ": ";
	}

	void append_helpline() {
		std::format_to(std::back_inserter(str), "Try '{}", exe_name);
		if (!cmd_name.empty()) { std::format_to(std::back_inserter(str), " {}", cmd_name); }
		str += " --help' for more information.\n";
	}

	std::string_view exe_name{};
	std::string_view cmd_name{};
	bool helpline{true};
	std::string str;
};
} // namespace

auto Parser::parse(std::span<Arg const> args) -> Result {
	m_args = args;
	m_cursor = {};
	m_has_commands = std::ranges::any_of(m_args, [](Arg const& a) { return std::holds_alternative<ParamCommand>(a.get_param()); });

	auto result = Result{};

	while (m_scanner.next()) {
		result = parse_next();
		if (result.early_return()) { return result; }
	}

	result = check_required();
	if (result.early_return()) { return result; }

	if (m_cursor.cmd != nullptr) { return m_cursor.cmd->name; }

	return result;
}

auto Parser::select_command() -> Result {
	auto const name = m_scanner.get_value();
	auto const* cmd = find_command(name);
	if (cmd == nullptr) { return ErrorPrinter{m_exe_name}.unrecognized_command(name); }

	m_args = cmd->args;
	m_cursor = Cursor{.cmd = cmd};
	return {};
}

auto Parser::parse_next() -> Result {
	switch (m_scanner.get_token_type()) {
	case TokenType::Argument: return parse_argument();
	case TokenType::Option: return parse_option();
	case TokenType::ForceArgs: return {};
	default:
	case TokenType::None: std::unreachable(); return {};
	}

	return {};
}

auto Parser::parse_option() -> Result {
	switch (m_scanner.get_option_type()) {
	case OptionType::Letters: return parse_letters();
	case OptionType::Word: return parse_word();
	default:
	case OptionType::None: break;
	}

	std::unreachable();
	return {};
}

auto Parser::parse_letters() -> Result {
	auto letter = char{};
	auto is_last = false;
	while (m_scanner.next_letter(letter, is_last)) {
		auto const* option = find_option(letter);
		if (option == nullptr) { return ErrorPrinter{m_exe_name, get_cmd_name()}.invalid_option(letter); }
		if (!is_last) {
			if (!option->is_flag) { return ErrorPrinter{m_exe_name, get_cmd_name()}.option_requires_argument({&letter, 1}); }
			[[maybe_unused]] auto const unused = option->assign({});
		} else {
			return parse_last_option(*option, {&letter, 1});
		}
	}

	return {};
}

auto Parser::parse_word() -> Result {
	auto const word = m_scanner.get_key();
	auto const* option = find_option(word);
	if (option == nullptr) { return ErrorPrinter{m_exe_name, get_cmd_name()}.unrecognized_option(word); }
	return parse_last_option(*option, word);
}

auto Parser::parse_last_option(ParamOption const& option, std::string_view input) -> Result {
	if (option.is_flag) {
		if (!m_scanner.get_value().empty()) { return ErrorPrinter{m_exe_name, get_cmd_name()}.option_is_flag(input); }
		[[maybe_unused]] auto const unused = option.assign({});
		return {};
	}

	auto value = m_scanner.get_value();
	if (value.empty()) {
		if (m_scanner.peek() != TokenType::Argument) { return ErrorPrinter{m_exe_name, get_cmd_name()}.option_requires_argument(input); }
		m_scanner.next();
		value = m_scanner.get_value();
	}
	if (!option.assign(value)) { return ErrorPrinter{m_exe_name, get_cmd_name()}.invalid_value(input, value); }

	return {};
}

auto Parser::parse_argument() -> Result {
	if (m_has_commands && m_cursor.cmd == nullptr) { return select_command(); }
	return parse_positional();
}

auto Parser::parse_positional() -> Result {
	auto const* pos = next_positional();
	if (pos == nullptr) { return ErrorPrinter{m_exe_name, get_cmd_name()}.extraneous_argument(m_scanner.get_value()); }
	if (!pos->assign(m_scanner.get_value())) { return ErrorPrinter{m_exe_name, get_cmd_name()}.invalid_value(pos->name, m_scanner.get_value()); }
	return {};
}

auto Parser::find_option(char const letter) const -> ParamOption const* {
	for (auto const& arg : m_args) {
		auto const* ret = std::get_if<ParamOption>(&arg.get_param());
		if (ret != nullptr && ret->letter == letter) { return ret; }
	}
	return nullptr;
}

auto Parser::find_option(std::string_view const word) const -> ParamOption const* {
	for (auto const& arg : m_args) {
		auto const* ret = std::get_if<ParamOption>(&arg.get_param());
		if (ret != nullptr && ret->word == word) { return ret; }
	}
	return nullptr;
}

auto Parser::find_command(std::string_view const name) const -> ParamCommand const* {
	for (auto const& arg : m_args) {
		auto const* ret = std::get_if<ParamCommand>(&arg.get_param());
		if (ret != nullptr && ret->name == name) { return ret; }
	}
	return nullptr;
}

auto Parser::next_positional() -> ParamPositional const* {
	auto& index = m_cursor.next_pos;
	for (; index < m_args.size(); ++index) {
		auto const& arg = m_args[index];
		auto const* ret = std::get_if<ParamPositional>(&arg.get_param());
		if (ret != nullptr) {
			++index;
			return ret;
		}
	}
	return nullptr;
}

auto Parser::check_required() -> Result {
	if (m_has_commands && m_cursor.cmd == nullptr) { return ErrorPrinter{m_exe_name}.missing_argument("command"); }

	for (auto const* p = next_positional(); p != nullptr; p = next_positional()) {
		if (p->is_required()) { return ErrorPrinter{m_exe_name, get_cmd_name()}.missing_argument(p->name); }
	}

	return {};
}
} // namespace cliq

[[nodiscard]] auto cliq::parse(AppInfo const& info, std::span<Arg const> args, int argc, char const* const* argv) -> Result {
	auto exe_name = std::string_view{"<app>"};
	auto cli_args = std::span{argv, std::size_t(argc)};
	if (!cli_args.empty()) {
		exe_name = get_exe_name(cli_args.front());
		cli_args = cli_args.subspan(1);
	};
	auto parser = Parser{info, exe_name, cli_args};
	return parser.parse(args);
}
