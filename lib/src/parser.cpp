#include <parser.hpp>
#include <algorithm>
#include <format>
#include <iomanip>
#include <iostream>

namespace cliq {
namespace {
template <typename ContainerT, typename GetStringT>
constexpr auto get_max_width(ContainerT&& container, GetStringT get_string) -> std::size_t {
	auto ret = std::size_t{};
	for (auto const& t : container) { ret = std::max(ret, get_string(t).size()); }
	return ret;
}

[[nodiscard]] auto find_option(Storage const& storage, std::string_view const key) -> Option const* {
	auto const it = std::ranges::find_if(storage.options, [key](Option const& o) { return o.key.word == key; });
	return it == storage.options.end() ? nullptr : &*it;
}

[[nodiscard]] auto find_option(Storage const& storage, char const letter) -> Option const* {
	auto const it = std::ranges::find_if(storage.options, [letter](Option const& o) { return o.key.letter == letter; });
	return it == storage.options.end() ? nullptr : &*it;
}
} // namespace

ErrorPrinter::ErrorPrinter(std::string_view exe_name, std::string_view cmd_id) : exe_name(exe_name), cmd_id(cmd_id) {
	str.reserve(400);
	append_error_prefix();
}

ErrorPrinter::~ErrorPrinter() {
	if (helpline) { append_helpline(); }
	std::cerr << str;
}

auto ErrorPrinter::invalid_value(std::string_view const option, std::string_view value) -> Result {
	helpline = false;
	std::format_to(std::back_inserter(str), "invalid {}: '{}'\n", option, value);
	return ParseError::InvalidArgument;
}

auto ErrorPrinter::invalid_option(char const letter) -> Result {
	std::format_to(std::back_inserter(str), "invalid option -- '{}'\n", letter);
	return ParseError::InvalidOption;
}

auto ErrorPrinter::unrecognized_option(std::string_view const input) -> Result {
	std::format_to(std::back_inserter(str), "unrecognized option '--{}'\n", input);
	return ParseError::InvalidOption;
}

auto ErrorPrinter::unrecognized_command(std::string_view const input) -> Result {
	std::format_to(std::back_inserter(str), "unrecognized command '{}'\n", input);
	return ParseError::InvalidCommand;
}

auto ErrorPrinter::option_requires_argument(std::string_view const input) -> Result {
	if (input.size() == 1) {
		std::format_to(std::back_inserter(str), "option requires an argument -- '{}'\n", input);
	} else {
		std::format_to(std::back_inserter(str), "option '{}' requires an argument\n", input);
	}
	return ParseError::MissingArgument;
}

auto ErrorPrinter::missing_argument(std::string_view const name) -> Result {
	std::format_to(std::back_inserter(str), "missing {}\n", name);
	return ParseError::MissingArgument;
}

void ErrorPrinter::append_error_prefix() {
	str += exe_name;
	if (!cmd_id.empty()) { std::format_to(std::back_inserter(str), " {}", cmd_id); }
	str += ": ";
}

void ErrorPrinter::append_helpline() {
	std::format_to(std::back_inserter(str), "Try '{}", exe_name);
	if (!cmd_id.empty()) { std::format_to(std::back_inserter(str), " {}", cmd_id); }
	str += " --help' for more information.\n";
}

auto HelpText::append_to(std::ostream& out) const -> std::ostream& {
	if (!storage.exec_info.description.empty()) { out << storage.exec_info.description << "\n"; }
	auto const options_width = get_max_width(storage.options, [](auto const& option) -> std::string_view { return option.print_key; });
	auto const builtin_width = get_max_width(storage.builtins, [](auto const& builtin) { return builtin.print_key; });
	auto width = std::max(options_width, builtin_width) + 4;
	out << "Usage: " << exe_name;
	if (!cmd_id.empty()) { out << " " << cmd_id; }
	if (!storage.options.empty()) { out << " [OPTION...]"; }
	for (auto const& arg : storage.arguments) { out << " <" << arg.name << ">"; }
	if (storage.list_argument) { out << " [" << storage.list_argument->name << "...]"; }
	out << "\n" << std::left;
	if (!storage.options.empty() || !storage.builtins.empty()) {
		out << "\nOPTIONS\n";
		auto const print_options = [&](auto const& options) {
			for (auto const& option : options) { out << "  " << std::setw(static_cast<int>(width)) << option.print_key << option.description << "\n"; }
		};
		print_options(storage.options);
		print_options(storage.builtins);
	}
	if (!storage.commands.empty()) {
		width = get_max_width(storage.commands, [](auto const& command) { return command->get_id(); }) + 4;
		out << "\nCOMMANDS\n" << std::left;
		for (auto const& command : storage.commands) {
			out << "  " << std::setw(static_cast<int>(width)) << command->get_id() << command->get_description() << "\n";
		}
	}
	if (!storage.exec_info.epilogue.empty()) { out << "\n" << storage.exec_info.epilogue << "\n"; }
	out << std::right;
	return out;
}

auto UsageText::append_to(std::ostream& out) const -> std::ostream& {
	out << "Usage: " << exe_name << " ";
	if (!cmd_id.empty()) { out << cmd_id << " "; }
	for (auto const& option : storage.options) {
		out << '[';
		if (option.key.letter != 0) {
			out << '-' << option.key.letter;
			if (!option.key.word.empty()) { out << '|'; }
		}
		if (!option.key.word.empty()) { out << "--" << option.key.word; }
		out << "(=" << option.binding->get_default_value() << ")] ";
	}

	for (auto const& argument : storage.arguments) { out << "<" << argument.name << "> "; }
	if (storage.list_argument) { out << "[" << storage.list_argument->name << "...]"; }
	return out;
}

void Parser::initialize(std::string_view const exe_name, std::string_view const cmd_id) {
	m_exe_name = exe_name;
	m_cmd_id = cmd_id;
}

void Parser::print_help(Storage const& storage) const {
	auto const help_text = HelpText{
		.exe_name = m_exe_name,
		.cmd_id = m_cmd_id,
		.storage = storage,
	};
	std::cout << help_text;
}

void Parser::print_usage(Storage const& storage) const {
	auto const usage_text = UsageText{
		.exe_name = m_exe_name,
		.cmd_id = m_cmd_id,
		.storage = storage,
	};
	std::cout << usage_text << "\n";
}

void Parser::print_version(Storage const& storage) { std::cout << storage.exec_info.version << "\n"; }

auto Parser::option_requires_argument(std::string_view const input) const -> Result {
	return ErrorPrinter{m_exe_name, m_cmd_id}.option_requires_argument(input);
}

auto Parser::invalid_option(char const letter) const -> Result { return ErrorPrinter{m_exe_name, m_cmd_id}.invalid_option(letter); }

auto Parser::unrecognized_option(std::string_view const input) const -> Result { return ErrorPrinter{m_exe_name, m_cmd_id}.unrecognized_option(input); }

auto Parser::invalid_value(std::string_view const option, std::string_view const input) const -> Result {
	return ErrorPrinter{m_exe_name, m_cmd_id}.invalid_value(option, input);
}

auto Parser::missing_argument(std::string_view const name) const -> Result { return ErrorPrinter{m_exe_name, m_cmd_id}.missing_argument(name); }

auto Parser::parse_option(Storage const& storage, Scanner& scanner) const -> Result {
	switch (scanner.get_option_type()) {
	case OptionType::Letters: return parse_letters(storage, scanner);
	case OptionType::Word: return parse_word(storage, scanner);
	default: return ParseError::InvalidOption;
	}
}

auto Parser::parse_argument(Storage const& storage, Scanner& scanner) -> Result {
	auto const input = scanner.get_value();
	Argument const* argument = nullptr;
	if (m_next_argument >= storage.arguments.size()) {
		if (storage.list_argument) { argument = &*storage.list_argument; }
	} else {
		argument = &storage.arguments.at(m_next_argument++);
	}

	if (argument != nullptr && !argument->binding->assign_argument(input)) { return invalid_value(argument->name, input); }
	return success_v;
}

auto Parser::parse_letters(Storage const& storage, Scanner& scanner) const -> Result {
	char letter{};
	bool is_last{};
	while (scanner.next_letter(letter, is_last)) {
		auto const input = std::string_view{&letter, 1};
		auto const* option = find_option(storage, letter);
		if (option == nullptr) { return invalid_option(letter); }
		auto const result = [&] {
			if (is_last) { return parse_last_option(input, *option, scanner); }
			return assign(input, *option, {});
		}();
		if (result != success_v) { return result; }
	}
	return success_v;
}

auto Parser::parse_word(Storage const& storage, Scanner& scanner) const -> Result {
	auto const input = scanner.get_key();
	if (try_builtin(storage, input)) { return ExecutedBuiltin{}; }
	auto const* option = find_option(storage, input);
	if (option == nullptr) { return unrecognized_option(input); }
	return parse_last_option(input, *option, scanner);
}

auto Parser::parse_last_option(std::string_view const input, Option const& option, Scanner& scanner) const -> Result {
	auto value = scanner.get_value();
	if (auto const result = get_value_for(input, option, value, scanner); result != success_v) { return result; }
	return assign(input, option, value);
}

auto Parser::try_builtin(Storage const& storage, std::string_view const input) const -> bool {
	auto const it = std::ranges::find_if(storage.builtins, [input](Builtin const& b) { return b.word == input; });
	if (it == storage.builtins.end()) { return false; }
	run_builtin(input);
	return true;
}

auto Parser::get_value_for(std::string_view const input, Option const& option, std::string_view& out_value, Scanner& scanner) const -> Result {
	if (!option.binding->is_flag()) {
		if (out_value.empty()) {
			if (!scanner.next()) { return option_requires_argument(input); }
			out_value = scanner.get_value();
		}
		return success_v;
	}

	if (out_value.empty() && scanner.peek() == TokenType::Argument) {
		scanner.next();
		out_value = scanner.get_value();
	}
	return success_v;
}

[[nodiscard]] auto Parser::assign(std::string_view const input, Option const& option, std::string_view const value) const -> Result {
	if (!option.binding->is_flag() && value.empty()) { return option_requires_argument(input); }
	if (!option.binding->assign_argument(value)) { return invalid_value(option.name, value); }
	return success_v;
}
} // namespace cliq
