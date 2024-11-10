#include <cliqr/instance.hpp>
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

[[nodiscard]] auto make_string(std::size_t const reserved) -> std::string {
	auto ret = std::string{};
	ret.reserve(reserved);
	return ret;
}

struct OptionKey {
	char letter{};
	std::string_view word{};

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

template <typename ContainerT, typename GetStringT>
constexpr auto get_max_width(ContainerT&& container, GetStringT get_string) -> std::size_t {
	auto ret = std::size_t{};
	for (auto const& t : container) { ret = std::max(ret, get_string(t).size()); }
	return ret;
}

auto get_print_key(char const letter, std::string_view const word) -> std::string {
	auto ret = std::string(4, ' ');
	if (letter != 0) {
		ret[0] = '-';
		ret[1] = letter;
		if (!word.empty()) { ret[2] = ','; }
	}
	if (!word.empty()) { std::format_to(std::back_inserter(ret), "--{}", word); }
	return ret;
}
} // namespace

struct Command::Impl {
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
	};

	std::string_view exe_name{};
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
		auto str = std::format("Usage: {} {} ", exe_name, id);
		for (auto const& option : options) {
			str += '[';
			if (option.key.letter != 0) { std::format_to(std::back_inserter(str), "-{}", option.key.letter); }
			if (option.key.letter != 0 && !option.key.word.empty()) { str += '|'; }
			if (!option.key.word.empty()) { std::format_to(std::back_inserter(str), "--{}", option.key.word); }
			str += "(=";
			option.binding->append_default_value(str);
			str += ")] ";
		}

		for (auto const& argument : arguments) { std::format_to(std::back_inserter(str), "<{}> ", argument.name); }
		if (list_argument) { std::format_to(std::back_inserter(str), "[{}...]", list_argument->name); }
		std::cout << str << "\n";
	}

	void print_help() const {
		auto const width = get_max_width(options, [](auto const& option) -> std::string_view { return option.print_key; }) + 4;
		auto str = std::ostringstream{};
		str << "Usage: " << exe_name << " " << id;
		if (!options.empty()) { str << " [OPTION...]"; }
		for (auto const& arg : arguments) { str << " <" << arg.name << ">"; }
		if (list_argument) { str << " [" << list_argument->name << "...]"; }
		str << "\n";
		if (!options.empty()) {
			str << "\nOPTIONS\n" << std::left;
			for (auto const& option : options) { str << "  " << std::setw(static_cast<int>(width)) << option.print_key << option.description << "\n"; }
		}
		std::cout << str.str();
	}

	auto parse() -> Result {
		auto scanner = Scanner{args};
		while (scanner.next()) {
			auto token_type = scanner.get_token_type();
			if (force_args) { token_type = TokenType::Argument; }
			switch (token_type) {
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
			default: return Result::InvalidArgument;
			}
		}
		if (next_argument < arguments.size()) { return missing_argument(arguments.at(next_argument).name); }
		return Result::Success;
	}

	auto parse_option(Scanner& scanner) -> Result {
		switch (scanner.get_option_type()) {
		case OptionType::Letters: return parse_letters(scanner);
		case OptionType::Word: return parse_word(scanner);
		default: return Result::InvalidOption;
		}
	}

	auto parse_letters(Scanner& scanner) -> Result {
		char letter{};
		bool is_last{};
		while (scanner.next_letter(letter, is_last)) {
			auto const input = std::string_view{&letter, 1};
			auto* option = find_option(letter);
			if (option == nullptr) { return invalid_option(letter); }
			auto const result = [&] {
				if (is_last) { return parse_last_option(input, *option, scanner); }
				return assign(input, *option, {});
			}();
			if (result != Result::Success) { return result; }
		}
		return Result::Success;
	}

	auto parse_word(Scanner& scanner) -> Result {
		auto const input = scanner.get_key();
		if (try_builtin(input)) {
			early_return = true;
			return Result::Success;
		}
		auto* option = find_option(input);
		if (option == nullptr) { return unrecognized_option(input); }
		return parse_last_option(input, *option, scanner);
	}

	auto parse_last_option(std::string_view const input, Option& option, Scanner& scanner) const -> Result {
		auto value = scanner.get_value();
		if (auto const result = get_value_for(input, option, value, scanner); result != Result::Success) { return result; }
		return assign(input, option, value);
	}

	auto get_value_for(std::string_view const input, Option const& option, std::string_view& out_value, Scanner& scanner) const -> Result {
		if (!option.binding->is_flag()) {
			if (out_value.empty()) {
				if (!scanner.next()) { return option_requires_argument(input); }
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

	auto assign(std::string_view const input, Option& option, std::string_view const value) const -> Result {
		if (!option.binding->is_flag() && value.empty()) { return option_requires_argument(input); }
		if (!option.binding->assign_argument(value)) { return invalid_value(option.name, value); }
		return Result::Success;
	}

	auto parse_arg(std::string_view const input) -> Result {
		Argument* argument = nullptr;
		if (next_argument >= arguments.size()) {
			if (list_argument) { argument = &*list_argument; }
		} else {
			argument = &arguments.at(next_argument++);
		}

		if (argument != nullptr && !argument->binding->assign_argument(input)) { return invalid_value(argument->name, input); }
		return Result::Success;
	}

	[[nodiscard]] auto find_option(std::string_view const key) -> Option* {
		for (auto& option : options) {
			if (option.key.word == key) { return &option; }
		}
		return nullptr;
	}

	[[nodiscard]] auto find_option(char const letter) -> Option* {
		for (auto& option : options) {
			if (option.key.letter == letter) { return &option; }
		}
		return nullptr;
	}

	[[nodiscard]] auto try_builtin(std::string_view const word) const -> bool {
		if (word == "help") {
			print_help();
			return true;
		}
		if (word == "usage") {
			print_usage();
			return true;
		}
		return false;
	}

	[[nodiscard]] auto invalid_value(std::string_view const name, std::string_view value) const -> Result {
		auto printer = ErrorPrinter{*this};
		printer.append_helpline = false;
		std::format_to(std::back_inserter(printer.str), "invalid {}: '{}'\n", name, value);
		return Result::InvalidValue;
	}

	[[nodiscard]] auto invalid_option(char const letter) const -> Result {
		auto printer = ErrorPrinter{*this};
		std::format_to(std::back_inserter(printer.str), "invalid option -- '{}'\n", letter);
		return Result::InvalidOption;
	}

	[[nodiscard]] auto unrecognized_option(std::string_view const input) const -> Result {
		auto printer = ErrorPrinter{*this};
		std::format_to(std::back_inserter(printer.str), "unrecognized option '--{}'\n", input);
		return Result::InvalidOption;
	}

	[[nodiscard]] auto option_requires_argument(std::string_view const input) const -> Result {
		auto printer = ErrorPrinter{*this};
		if (input.size() == 1) {
			std::format_to(std::back_inserter(printer.str), "option requires an argument -- '{}'\n", input);
		} else {
			std::format_to(std::back_inserter(printer.str), "option '{}' requires an argument\n", input);
		}
		return Result::MissingArgument;
	}

	[[nodiscard]] auto missing_argument(std::string_view const name) const -> Result {
		auto printer = ErrorPrinter{*this};
		std::format_to(std::back_inserter(printer.str), "missing {}\n", name);
		return Result::MissingArgument;
	}

	void append_error_prefix(std::string& out) const { std::format_to(std::back_inserter(out), "{} {}: ", exe_name, id); }
	void append_helpline(std::string& out) const { std::format_to(std::back_inserter(out), "Try '{} {} --help' for more information.", exe_name, id); }

	struct ErrorPrinter {
		ErrorPrinter(ErrorPrinter const&) = delete;
		ErrorPrinter(ErrorPrinter&&) = delete;
		auto operator=(ErrorPrinter const&) = delete;
		auto operator=(ErrorPrinter&&) = delete;

		explicit ErrorPrinter(Impl const& impl, std::size_t const reserved = 200) : impl(impl), str(make_string(reserved)) { impl.append_error_prefix(str); }

		~ErrorPrinter() {
			if (!append_helpline) { return; }
			impl.append_helpline(str);
			std::cerr << str << "\n";
		}

		bool append_helpline{true};

		Impl const& impl; // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)
		std::string str;
	};
};

void Command::Deleter::operator()(Impl* ptr) const noexcept { std::default_delete<Impl>{}(ptr); }

Command::Command() : m_impl(new Impl) {}

void Command::print_usage() const {
	assert(m_impl);
	m_impl->print_usage();
}

void Command::print_help() const {
	assert(m_impl);
	m_impl->print_help();
}

void Command::bind_option(std::string_view const key, BindInfo info) const {
	if (m_impl == nullptr || key.empty()) { return; }
	auto option = Impl::Option{
		.key = OptionKey::make(key),
		.name = info.name,
		.description = info.description,
		.binding = std::move(info.binding),
	};
	option.print_key = get_print_key(option.key.letter, option.key.word);
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
	bind_option(key, {std::move(binding), name, description});
}

struct Instance::Impl {
	CreateInfo const& info; // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)
	std::span<std::unique_ptr<Command> const> commands{};

	std::string exe_name{};

	auto run(int argc, char const* const* argv) -> Result {
		auto args = std::span{argv, static_cast<std::size_t>(argc)};
		if (set_exe_name(args)) { args = args.subspan(1); }

		auto const print_help_and_exit = [this] {
			print_help();
			return Result::Success;
		};

		auto scanner = Scanner{args};
		if (!scanner.next()) { return print_help_and_exit(); }

		switch (scanner.get_token_type()) {
		case TokenType::Argument: return run_command(scanner.get_value(), scanner);
		case TokenType::Option: return parse_option(scanner);
		default: return print_help_and_exit();
		}
	}

	auto set_exe_name(std::span<char const* const> args) -> bool {
		if (args.empty()) { return false; }
		exe_name = fs::path{args.front()}.filename().string();
		return true;
	}

	[[nodiscard]] auto parse_option(Scanner& scanner) const -> Result {
		switch (scanner.get_option_type()) {
		case OptionType::Letters: {
			auto input = char{};
			scanner.next_letter(input);
			return invalid_option(input);
		}
		case OptionType::Word: {
			auto const input = scanner.get_key();
			if (!try_builtin(input)) { return unrecognized_option(input); }
			return Result::Success;
		}
		default: return Result::InvalidOption;
		}
	}

	[[nodiscard]] auto run_command(std::string_view const id, Scanner const& scanner) const -> Result {
		auto const find_command = [id](auto const& command) { return command->get_id() == id; };
		auto const it = std::ranges::find_if(commands, find_command);
		if (it == commands.end()) { return unrecognized_command(id); }

		auto const& command = *it;
		auto result = parse_command_args(*command, scanner.get_args());
		if (result != Result::Success || command->m_impl->early_return) { return result; }

		result = command->execute();
		if (result != Result::Success) { command->print_usage(); }
		return result;
	}

	[[nodiscard]] auto parse_command_args(Command& command, std::span<char const* const> args) const -> Result {
		command.m_impl->id = command.get_id();
		command.m_impl->exe_name = exe_name;
		command.m_impl->args = args;
		return command.m_impl->parse();
	}

	void print_help() const {
		struct Option {
			std::string_view word{};
			std::string_view description{};
		};
		static constexpr auto options = std::array{
			Option{"--help", "Print this help text"},
			Option{"--version", "Print the app version"},
		};
		static constexpr auto width = get_max_width(options, [](auto const& option) { return option.word; }) + 4;

		auto str = std::ostringstream{};
		str << std::left;
		str << info.tagline << "\n\n";
		str << "  " << exe_name << " [OPTION]\n  " << exe_name << " [COMMAND] [...]\n\nOPTIONS\n";
		for (auto const option : options) { str << "  " << std::setw(static_cast<int>(width)) << option.word << option.description << "\n"; }
		std::cout << str.str() << "\nCOMMANDS\n";
		print_command_list();
		if (!info.epilogue.empty()) { std::cout << info.epilogue << "\n"; }
	}

	void print_command_list() const {
		auto const width = get_max_width(commands, [](auto const& command) { return command->get_id(); }) + 4;
		auto str = std::ostringstream{};
		str << std::left;
		for (auto const& cmd : commands) { str << "  " << std::setw(static_cast<int>(width)) << cmd->get_id() << cmd->get_tagline() << "\n"; }
		std::cout << str.str();
	}

	[[nodiscard]] auto try_builtin(std::string_view const input) const -> bool {
		if (input == "help") {
			print_help();
			return true;
		}
		if (input == "version") {
			std::cout << info.version << "\n";
			return true;
		}
		return false;
	}

	[[nodiscard]] auto invalid_option(char const input) const -> Result {
		auto printer = ErrorPrinter{*this};
		std::format_to(std::back_inserter(printer.str), "invalid option -- '{}'\n", input);
		return Result::InvalidOption;
	}

	[[nodiscard]] auto unrecognized_option(std::string_view const input) const -> Result {
		auto printer = ErrorPrinter{*this};
		std::format_to(std::back_inserter(printer.str), "unrecognized option '--{}'\n", input);
		return Result::InvalidOption;
	}

	[[nodiscard]] auto unrecognized_command(std::string_view const input) const -> Result {
		auto printer = ErrorPrinter{*this};
		std::format_to(std::back_inserter(printer.str), "unrecognized command '{}'\n", input);
		return Result::InvalidCommand;
	}

	struct ErrorPrinter {
		ErrorPrinter(ErrorPrinter const&) = delete;
		ErrorPrinter(ErrorPrinter&&) = delete;
		auto operator=(ErrorPrinter const&) = delete;
		auto operator=(ErrorPrinter&&) = delete;

		explicit ErrorPrinter(Impl const& impl, std::size_t const reserved = 200) : impl(impl), str(make_string(reserved)) { append_error_prefix(); }

		~ErrorPrinter() {
			if (!helpline) { return; }
			append_helpline();
			std::cerr << str << "\n";
		}

		void append_error_prefix() { std::format_to(std::back_inserter(str), "{}: ", impl.exe_name); }
		void append_helpline() { std::format_to(std::back_inserter(str), "Try '{} --help' for more information.", impl.exe_name); }

		bool helpline{true};

		Impl const& impl; // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)
		std::string str;
	};
};

void Instance::add_command(std::unique_ptr<Command> command) {
	if (!command) { return; }
	m_commands.push_back(std::move(command));
}

auto Instance::run(int argc, char const* const* argv) -> Result { return Impl{.info = m_info, .commands = m_commands}.run(argc, argv); }
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
		required(path, "path", "test path");
		list(paths, "paths", "test paths");
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

auto lab(int const argc, char const* const* argv) -> int {
	auto const create_info = cliqr::CreateInfo{
		.tagline = "cliqr lab (test bench)",
		.version = "cliqr lab\nv0.0",
	};
	auto context = cliqr::Instance{create_info};
	context.add_command(std::make_unique<Foo>());
	context.add_command(std::make_unique<LongCommand>());
	return static_cast<int>(context.run(argc, argv));
}
} // namespace cliqr
