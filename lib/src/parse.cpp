#include <cliq/parse.hpp>
#include <parser.hpp>
#include <algorithm>
#include <utility>

namespace cliq {
namespace {
constexpr auto get_exe_name(std::string_view const arg0) -> std::string_view {
	auto const i = arg0.find_last_of("\\/");
	if (i == std::string_view::npos) { return arg0; }
	return arg0.substr(i);
}
} // namespace

namespace foo {
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
	if (cmd == nullptr) {
		// TODO
		return ParseError::InvalidArgument;
	}

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
		if (option == nullptr) {
			// TODO
			return ParseError::InvalidOption;
		}
		if (!is_last) {
			if (!option->is_flag) {
				// TODO
				return ParseError::MissingArgument;
			}
			[[maybe_unused]] auto const unused = option->assign({});
		} else {
			return parse_last_option(*option);
		}
	}

	return {};
}

auto Parser::parse_word() -> Result {
	auto const* option = find_option(m_scanner.get_key());
	if (option == nullptr) {
		// TODO
		return ParseError::InvalidOption;
	}
	return parse_last_option(*option);
}

auto Parser::parse_last_option(ParamOption const& option) -> Result {
	if (option.is_flag) {
		if (!m_scanner.get_value().empty()) {
			// TODO
			return ParseError::InvalidArgument;
		}
		[[maybe_unused]] auto const unused = option.assign({});
		return {};
	}

	auto value = m_scanner.get_value();
	if (value.empty()) {
		if (m_scanner.peek() != TokenType::Argument) {
			// TODO
			return ParseError::MissingArgument;
		}
		m_scanner.next();
		value = m_scanner.get_value();
	}
	if (!option.assign(value)) {
		// TODO
		return ParseError::InvalidArgument;
	}

	return {};
}

auto Parser::parse_argument() -> Result {
	if (m_has_commands && m_cursor.cmd == nullptr) { return select_command(); }
	return parse_positional();
}

auto Parser::parse_positional() -> Result {
	auto const* pos = next_positional();
	if (pos == nullptr) {
		// TODO
		return ParseError::InvalidArgument;
	}
	if (!pos->assign(m_scanner.get_value())) {
		// TODO
		return ParseError::InvalidArgument;
	}
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
	if (m_has_commands && m_cursor.cmd == nullptr) {
		// TODO
		return ParseError::MissingArgument;
	}

	for (auto const* p = next_positional(); p != nullptr; p = next_positional()) {
		if (p->is_required()) {
			// TODO
			return ParseError::MissingArgument;
		}
	}

	return {};
}
} // namespace foo
} // namespace cliq

[[nodiscard]] auto cliq::parse(AppInfo const& info, std::span<Arg const> args, int argc, char const* const* argv) -> ParseResult {
	auto exe_name = std::string_view{"<app>"};
	auto cli_args = std::span{argv, std::size_t(argc)};
	if (!cli_args.empty()) {
		exe_name = get_exe_name(cli_args.front());
		cli_args = cli_args.subspan(1);
	};
	auto parser = foo::Parser{info, exe_name, cli_args};
	return parser.parse(args);
}
