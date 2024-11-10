#include <cliqr/instance.hpp>
#include <parser.hpp>
#include <scanner.hpp>
#include <storage.hpp>
#include <algorithm>
#include <cassert>
#include <filesystem>

namespace cliqr {
namespace fs = std::filesystem;

struct Command::Impl : Parser {
	Storage storage{};

	bool force_args{};

	void run_builtin(std::string_view const word) const final {
		if (word == "help") {
			print_help(storage);
		} else if (word == "usage") {
			print_usage(storage);
		}
	}

	auto parse(std::span<char const* const> args) -> Result {
		auto scanner = Scanner{args};
		auto result = success_v;
		while (scanner.next()) {
			auto token_type = scanner.get_token_type();
			if (force_args) { token_type = TokenType::Argument; }
			switch (token_type) {
			case TokenType::OptEnd: force_args = true; break;
			case TokenType::Argument: {
				result = parse_argument(storage, scanner);
				if (result != success_v) { return result; }
				break;
			}
			case TokenType::Option: {
				result = parse_option(storage, scanner);
				if (result != success_v) { return result; }
				break;
			}
			default: return ParseError::InvalidArgument;
			}
		}
		if (get_args_parsed() < storage.arguments.size()) { return missing_argument(storage.arguments.at(get_args_parsed()).name); }
		return success_v;
	}
};

void Command::Deleter::operator()(Impl* ptr) const noexcept { std::default_delete<Impl>{}(ptr); }

Command::Command() : m_impl(new Impl) {
	m_impl->storage.builtins = {
		Builtin{"help", "Print this help text"},
		Builtin{"usage", "Print command usage"},
	};
}

void Command::print_usage() const {
	assert(m_impl);
	m_impl->print_usage(m_impl->storage);
}

void Command::print_help() const {
	assert(m_impl);
	m_impl->print_help(m_impl->storage);
}

void Command::bind_option(BindInfo info, std::string_view const key) const {
	if (m_impl == nullptr || key.empty()) { return; }
	m_impl->storage.bind_option(std::move(info), key);
}

void Command::bind_argument(BindInfo info, bool const is_list) const {
	if (m_impl == nullptr) { return; }
	m_impl->storage.bind_argument(std::move(info), is_list);
}

void Command::flag(bool& out, std::string_view const key, std::string_view const name, std::string_view const description) const {
	if (!m_impl || key.empty()) { return; }
	m_impl->storage.bind_option({std::make_unique<Binding<bool>>(out), name, description}, key);
}

struct Instance::Impl : Parser {
	CreateInfo info{};
	Storage storage{};

	std::string exe_name{};

	explicit Impl(CreateInfo info) : info(info) {}

	void run_builtin(std::string_view const input) const final {
		if (input == "help") {
			print_help(storage);
		} else if (input == "version") {
			print_version(info.version);
		}
	}

	auto run(int argc, char const* const* argv) -> Result {
		auto args = std::span{argv, static_cast<std::size_t>(argc)};
		if (set_exe_name(args)) { args = args.subspan(1); }
		initialize(exe_name);

		auto const print_help_and_exit = [this] {
			print_help(storage);
			return success_v;
		};

		auto scanner = Scanner{args};
		if (!scanner.next()) { return print_help_and_exit(); }

		switch (scanner.get_token_type()) {
		case TokenType::Argument: return run_command(scanner.get_value(), scanner);
		case TokenType::Option: {
			auto const result = parse_option(storage, scanner);
			if (should_return(result)) { return result; }
			break;
		}
		default: break;
		}

		return print_help_and_exit();
	}

	auto set_exe_name(std::span<char const* const> args) -> bool {
		if (args.empty()) { return false; }
		exe_name = fs::path{args.front()}.filename().string();
		return true;
	}

	[[nodiscard]] auto run_command(std::string_view const id, Scanner const& scanner) const -> Result {
		auto const it = std::ranges::find_if(storage.commands, [id](auto const& c) { return c->get_id() == id; });
		if (it == storage.commands.end()) { return ErrorPrinter{exe_name}.unrecognized_command(id); }

		auto const& command = *it;
		command->m_impl->initialize(exe_name, command->get_id());
		auto result = command->m_impl->parse(scanner.get_args());
		if (should_return(result)) { return result; }

		result = command->execute();
		if (result != success_v) { command->print_usage(); }
		return result;
	}
};

void Instance::Deleter::operator()(Impl* ptr) const noexcept { std::default_delete<Impl>{}(ptr); }

Instance::Instance(CreateInfo const& info) : m_impl(new Impl(info)) {
	m_impl->storage.builtins = {
		Builtin{"help", "Print this help text"},
		Builtin{"version", "Print the app version"},
	};
}

void Instance::add_command(std::unique_ptr<Command> command) {
	if (!m_impl || !command) { return; }
	m_impl->storage.commands.push_back(std::move(command));
}

auto Instance::run(int argc, char const* const* argv) -> Result {
	if (!m_impl) { return failure_v; }
	return m_impl->run(argc, argv);
}
} // namespace cliqr
