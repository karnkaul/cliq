#include <cliqr/instance.hpp>
#include <printer.hpp>
#include <scanner.hpp>
#include <algorithm>
#include <array>
#include <cassert>
#include <filesystem>
#include <format>
#include <iostream>
#include <memory>
#include <sstream>
#include <string_view>

namespace cliqr {
namespace fs = std::filesystem;

namespace {
constexpr auto is_space(char const c) { return std::isspace(static_cast<unsigned char>(c)) != 0; }

constexpr void trim(std::string_view& out) {
	while (is_space(out.front())) { out = out.substr(1); }
	while (is_space(out.back())) { out = out.substr(0, out.size() - 1); }
}

constexpr void split_key_single(std::string_view& key, char& single) {
	trim(key);
	if (key.size() == 1) {
		single = key.front();
		key = {};
		return;
	}
	if (key.size() < 3 || key[1] != ',') { return; }
	single = key.front();
	key = key.substr(2);
}

auto missing_arg(std::string_view const option_name) -> CommandResult {
	std::cerr << std::format("Missing argument for '{}'\n", option_name);
	return CommandResult::MissingArgument;
}

auto missing_required(CommandResult const type, std::string_view const name) -> CommandResult {
	std::string_view const type_str = [type] {
		switch (type) {
		default:
		case CommandResult::MissingArgument: return "argument";
		case CommandResult::MissingOption: return "option";
		}
	}();
	std::cerr << std::format("Missing required {}: {}\n", type_str, name);
	return type;
}

auto invalid_arg(std::string_view const name, std::string_view const arg) -> CommandResult {
	std::cerr << std::format("Invalid argument for '{}': '{}'\n", name, arg);
	return CommandResult::InvalidArgument;
}
} // namespace

struct Command::Impl {
	struct Option {
		std::string_view key{};
		char single{};
		std::string_view name{};
		std::string_view description{};
		std::unique_ptr<IBinding> binding{};
		bool required{};
		bool assigned{};
	};

	struct Argument {
		std::string_view name{};
		std::string_view description{};
		std::unique_ptr<IBinding> binding{};
	};

	std::string_view app_name{};
	std::string_view id{};
	std::span<char const* const> args{};
	std::vector<Option> options{};
	std::vector<Argument> arguments{};
	std::optional<Argument> list_argument{};
	std::size_t next_argument{};
	std::string args_text{};

	bool force_args{};
	bool early_return{};

	void print_usage() const {
		auto str = std::format("Usage: {} {} ", app_name, id);
		for (auto const& option : options) {
			str += '[';
			if (option.single != 0) { std::format_to(std::back_inserter(str), "-{}", option.single); }
			if (option.single != 0 && !option.key.empty()) { str += '|'; }
			if (!option.key.empty()) { std::format_to(std::back_inserter(str), "--{}", option.key); }
			if (!option.required) {
				str += "[=";
				option.binding->append_default_value(str);
				str += ']';
			}
			str += "] ";
		}

		for (auto const& argument : arguments) { std::format_to(std::back_inserter(str), "[{}] ", argument.name); }
		if (list_argument) { std::format_to(std::back_inserter(str), "[{}...]", list_argument->name); }
		std::cout << str << "\n";
	}

	void print_help() const {
		auto str = std::ostringstream{};
		str << "Usage: " << app_name << " " << id;
		if (!options.empty()) { str << " [OPTION...]"; }
		for (auto const& arg : arguments) { str << " [" << arg.name << "]"; }
		if (list_argument) { str << " [" << list_argument->name << "...]"; }
		str << "\n\nOPTIONS\n" << std::left;
		auto const width = get_max_width(options, [](auto const& option) { return option.key; }) + 2;
		for (auto const& option : options) {
			str << "  ";
			auto single = std::array<char, 5>{' ', ' ', ' ', ' '};
			if (option.single != 0) {
				single[0] = '-';
				single[1] = option.single;
				if (!option.key.empty()) { single[2] = ','; }
			}
			str << single.data();
			if (!option.key.empty()) {
				str << "--" << std::setw(static_cast<int>(width)) << option.key;
			} else {
				str << std::setw(static_cast<int>(width));
			}
			str << option.description << "\n";
		}
		std::cout << str.str();
	}

	auto parse() -> CommandResult {
		auto scanner = Scanner{args};
		while (scanner.next()) {
			switch (scanner.get_token_type()) {
			case TokenType::OptEnd: force_args = true; break;
			case TokenType::Argument: {
				auto const result = parse_arg(scanner.get_value());
				if (early_return || result != Result::Success) { return result; }
				break;
			}
			case TokenType::Option: {
				auto const result = parse_option(scanner);
				if (early_return || result != Result::Success) { return result; }
				break;
			}
			default: return invalid_arg(id, scanner.get_value());
			}
		}
		for (auto const& option : options) {
			if (option.required && !option.assigned) { return missing_required(Result::MissingOption, option.name); }
		}
		if (next_argument < arguments.size()) { return missing_required(Result::MissingArgument, arguments.at(next_argument).name); }
		return CommandResult::Success;
	}

	auto parse_option(Scanner& scanner) -> Result {
		switch (scanner.get_option_type()) {
		case OptionType::Singles: return parse_singles(scanner);
		case OptionType::Key: return parse_key(scanner);
		default: return invalid_arg(id, scanner.get_value());
		}
	}

	auto parse_singles(Scanner& scanner) -> Result {
		char single{};
		bool is_last{};
		while (scanner.next_single(single, is_last)) {
			auto* option = find_option(single);
			if (option == nullptr) { return invalid_arg(id, {&single, 1}); }
			if (!is_last && !option->binding->is_flag()) { return missing_arg(option->name); }
			if (is_last) {
				if (auto const result = parse_last_option(*option, scanner); result != Result::Success) { return result; }
			} else {
				if (auto const result = assign(*option, {}); result != Result::Success) { return result; }
			}
		}
		return Result::Success;
	}

	auto parse_key(Scanner& scanner) -> Result {
		auto const key = scanner.get_key();
		if (try_builtin(key)) {
			early_return = true;
			return Result::Success;
		}
		auto* option = find_option(key);
		if (option == nullptr) { return invalid_arg(id, key); }
		return parse_last_option(*option, scanner);
	}

	static auto parse_last_option(Option& option, Scanner& scanner) -> Result {
		auto value = scanner.get_value();
		if (auto const result = get_value_for(option, value, scanner); result != Result::Success) { return result; }
		return assign(option, value);
	}

	static auto get_value_for(Option const& option, std::string_view& out_value, Scanner& scanner) -> Result {
		if (!option.binding->is_flag()) {
			if (out_value.empty()) {
				if (!scanner.next()) { return missing_arg(option.name); }
				out_value = scanner.get_value();
			}
			return Result::Success;
		}

		if (scanner.peek() == TokenType::Argument) {
			scanner.next();
			out_value = scanner.get_value();
		}
		return Result::Success;
	}

	static auto assign(Option& optioneter, std::string_view const value) -> Result {
		if (!optioneter.binding->assign_argument(value)) { return invalid_arg(optioneter.name, value); }
		optioneter.assigned = true;
		return Result::Success;
	}

	auto parse_arg(std::string_view const input) -> Result {
		Argument* argument = nullptr;
		if (next_argument >= arguments.size()) {
			if (!list_argument) { return invalid_arg(id, input); }
			argument = &*list_argument;
		} else {
			argument = &arguments.at(next_argument++);
		}

		assert(argument != nullptr);
		if (!argument->binding->assign_argument(input)) { return invalid_arg(argument->name, input); }
		return Result::Success;
	}

	[[nodiscard]] auto find_option(std::string_view const key) -> Option* {
		for (auto& option : options) {
			if (option.key == key) { return &option; }
		}
		return nullptr;
	}

	[[nodiscard]] auto find_option(char const single) -> Option* {
		for (auto& option : options) {
			if (option.single != 0 && option.single == single) { return &option; }
		}
		return nullptr;
	}

	[[nodiscard]] auto try_builtin(std::string_view const key) const -> bool {
		if (key == "help") {
			print_help();
			return true;
		}
		if (key == "usage") {
			print_usage();
			return true;
		}
		return false;
	}
};

void Command::Deleter::operator()(Impl* ptr) const noexcept { std::default_delete<Impl>{}(ptr); }

Command::Command() : m_impl(new Impl) {}

void Command::print_usage() const {
	if (!m_impl) { return; }
	m_impl->print_usage();
}

void Command::print_help() const {
	if (!m_impl) { return; }
	m_impl->print_help();
}

auto Command::parse_args(std::string_view app_name, std::span<char const* const> args) -> Result {
	if (!m_impl) { return Result::Failure; }
	m_impl->app_name = app_name;
	m_impl->args = args;
	m_impl->id = get_id();
	return m_impl->parse();
}

auto Command::should_execute() const -> bool {
	if (!m_impl) { return false; }
	return !m_impl->early_return;
}

void Command::bind_option(std::string_view const key, BindInfo info, bool const required) const {
	if (m_impl == nullptr || key.empty()) { return; }
	auto option = Impl::Option{
		.key = key,
		.name = info.name,
		.description = info.description,
		.binding = std::move(info.binding),
		.required = required,
	};
	split_key_single(option.key, option.single);
	m_impl->options.push_back(std::move(option));
}

void Command::bind_argument(BindInfo info, bool const is_list) const {
	if (m_impl == nullptr) { return; }
	auto argument = Impl::Argument{
		.name = info.name,
		.description = info.description,
		.binding = std::move(info.binding),
	};
	if (is_list) {
		m_impl->list_argument.emplace(std::move(argument));
	} else if (m_impl->list_argument) {
		// cannot bind individual args after list arg
		return;
	} else {
		m_impl->arguments.push_back(std::move(argument));
	}

	std::string_view const suffix = is_list ? "..." : "";
	std::format_to(std::back_inserter(m_impl->args_text), " {}{}", info.name, suffix);
}

void Command::flag(bool& out, std::string_view const key, std::string_view const name, std::string_view const description) const {
	auto binding = std::make_unique<Binding<bool>>(out);
	bind_option(key, {std::move(binding), name, description}, false);
}

void Instance::add_command(std::unique_ptr<Command> command) {
	if (!command) { return; }
	m_commands.push_back(std::move(command));
}

auto Instance::run(int argc, char const* const* argv) -> Result {
	auto args = std::span{argv, static_cast<std::size_t>(argc)};
	auto arg0 = std::string_view{};
	if (!args.empty()) {
		arg0 = args.front();
		args = args.subspan(1);
	}

	m_app_name = "<app_name>";
	if (!arg0.empty()) { m_app_name = fs::path{arg0}.filename().string(); }

	auto const missing_command = [this] {
		missing_required(Result::MissingArgument, "command");
		print_usage();
		return Result::MissingCommand;
	};

	auto scanner = Scanner{args};
	if (!scanner.next()) { return missing_command(); }

	switch (scanner.get_token_type()) {
	case TokenType::Argument: break;
	case TokenType::Option: {
		if (try_builtin(scanner.get_key())) { return Result::Success; }
		return invalid_arg(m_app_name, scanner.get_key());
	}
	default: return missing_command();
	}

	std::string_view const id = scanner.get_value();
	auto const find_command = [id](auto const& command) { return command->get_id() == id; };
	auto const it = std::ranges::find_if(m_commands, find_command);
	if (it == m_commands.end()) {
		invalid_arg("command", id);
		print_usage();
		return Result::InvalidArgument;
	}

	auto const& command = *it;
	if (auto const result = command->parse_args(m_app_name, scanner.get_args()); result != Result::Success) { return result; }
	if (!command->should_execute()) { return Result::Success; }

	auto const result = command->execute();
	if (result != Result::Success) { command->print_usage(); }
	return result;
}

auto Instance::try_builtin(std::string_view const input) const -> bool {
	if (input == "help") {
		print_help();
		return true;
	}
	if (input == "usage") {
		print_usage();
		return true;
	}
	if (input == "version") {
		std::cout << m_version << "\n";
		return true;
	}
	return false;
}

void Instance::print_help() const {
	std::cout << m_tagline << "\n\n";
	static constexpr std::string_view options_v[] = {"--help", "--usage", "--version"};
	std::cout << std::format("  {} [OPTION]\n  {} [COMMAND] [...]\n\nOPTIONS\n{:>10}\n\nCOMMANDS\n", m_app_name, m_app_name, "--help");
	print_command_list();
}

void Instance::print_usage() const { std::cout << std::format("Usage: {} [--help|--usage|--version|COMMAND]\n", m_app_name); }

void Instance::print_command_list() const {
	// auto printer = TabPrinter{std::cout};
	// printer.tab_size = 4;
	// for (auto const& cmd : m_commands) { printer.next_column(cmd->get_id()).next_column(cmd->get_tagline()).next_row(); }

	auto const width = get_max_width(m_commands, [](auto const& command) { return command->get_id(); }) + 2;
	auto str = std::stringstream{};
	str << std::left;
	for (auto const& cmd : m_commands) { str << "  " << std::setw(static_cast<int>(width)) << cmd->get_id() << cmd->get_tagline() << "\n"; }
	std::cout << str.str() << "\n";

	// auto str = std::string{};
	// for (auto const& cmd : m_commands) { std::format_to(std::back_inserter(str), "\t{}\t{}\n", cmd->get_id(), cmd->get_tagline()); }
	// std::cout << str;
}
} // namespace cliqr

namespace cliqr {
namespace {
struct Foo : Command {
	bool test{};
	int count{5};
	std::string_view path{};
	std::vector<std::string_view> paths{};

	explicit Foo() {
		flag(test, "t", "test", "test flag");
		optional(count, "c,count", "count", "test int");
		argument(path, "path", "test path");
		arguments(paths, "paths", "test paths");
	}

	[[nodiscard]] auto get_id() const -> std::string_view final { return "foo"; }
	[[nodiscard]] auto get_tagline() const -> std::string_view final { return "test command"; }

	[[nodiscard]] auto execute() -> Result final {
		std::cout << std::format("test\t: {}\ncount\t: {}\npath\t: '{}'\npaths\t: ", test, count, path);
		for (auto const& path : paths) { std::cout << std::format("'{}' ", path); }
		std::cout << "\n";
		return Result::Success;
	}
};

struct LongCommand : Command {
	[[nodiscard]] auto get_id() const -> std::string_view final { return "command-with-long-ass-id"; }
	[[nodiscard]] auto get_tagline() const -> std::string_view final { return "long command id"; }

	[[nodiscard]] auto execute() -> Result final { return Result::Success; }
};
} // namespace

auto lab(int const argc, char const* const argv[]) -> int {
	auto context = cliqr::Instance{"cliqr lab (test bench)", "cliqr lab\nv0.0"};
	context.add_command(std::make_unique<Foo>());
	context.add_command(std::make_unique<LongCommand>());
	return static_cast<int>(context.run(argc, argv));
}
} // namespace cliqr
