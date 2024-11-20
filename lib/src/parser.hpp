#pragma once
#include <cliq/result.hpp>
#include <scanner.hpp>
#include <storage.hpp>

namespace cliq {
struct ErrorPrinter {
	ErrorPrinter(ErrorPrinter const&) = delete;
	ErrorPrinter(ErrorPrinter&&) = delete;
	auto operator=(ErrorPrinter const&) = delete;
	auto operator=(ErrorPrinter&&) = delete;

	explicit ErrorPrinter(std::string_view exe_name, std::string_view cmd_id = {});
	~ErrorPrinter();

	[[nodiscard]] auto invalid_value(std::string_view option, std::string_view value) -> Result;
	[[nodiscard]] auto invalid_option(char letter) -> Result;
	[[nodiscard]] auto unrecognized_option(std::string_view input) -> Result;
	[[nodiscard]] auto unrecognized_command(std::string_view input) -> Result;
	[[nodiscard]] auto option_requires_argument(std::string_view input) -> Result;
	[[nodiscard]] auto missing_argument(std::string_view name) -> Result;

	void append_error_prefix();
	void append_helpline();

	std::string_view exe_name{};
	std::string_view cmd_id{};
	bool helpline{true};
	std::string str;
};

struct HelpText {
	std::string_view exe_name{};
	std::string_view cmd_id{};
	Storage const& storage;

	[[nodiscard]] auto append_to(std::ostream& out) const -> std::ostream&;

	friend auto operator<<(std::ostream& out, HelpText const& help_text) -> std::ostream& { return help_text.append_to(out); }
};

struct UsageText {
	std::string_view exe_name{};
	std::string_view cmd_id{};
	Storage const& storage;

	[[nodiscard]] auto append_to(std::ostream& out) const -> std::ostream&;

	friend auto operator<<(std::ostream& out, UsageText const& usage_text) -> std::ostream& { return usage_text.append_to(out); }
};

class Parser : public Polymorphic {
  public:
	virtual void run_builtin(std::string_view input) const = 0;

	void initialize(std::string_view exe_name, std::string_view cmd_id = {});

	void print_help(Storage const& storage) const;
	void print_usage(Storage const& storage) const;
	static void print_version(Storage const& storage);

	[[nodiscard]] auto option_requires_argument(std::string_view input) const -> Result;
	[[nodiscard]] auto invalid_option(char letter) const -> Result;
	[[nodiscard]] auto unrecognized_option(std::string_view input) const -> Result;
	[[nodiscard]] auto invalid_value(std::string_view option, std::string_view input) const -> Result;
	[[nodiscard]] auto missing_argument(std::string_view name) const -> Result;

	[[nodiscard]] auto get_args_parsed() const -> std::size_t { return m_next_argument; }

	auto parse_option(Storage const& storage, Scanner& scanner) const -> Result;
	auto parse_argument(Storage& storage, Scanner& scanner) -> Result;

  private:
	auto parse_letters(Storage const& storage, Scanner& scanner) const -> Result;
	auto parse_word(Storage const& storage, Scanner& scanner) const -> Result;
	auto parse_last_option(std::string_view input, Option const& option, Scanner& scanner) const -> Result;

	[[nodiscard]] auto try_builtin(Storage const& storage, std::string_view input) const -> bool;

	auto get_value_for(std::string_view input, Option const& option, std::string_view& out_value, Scanner& scanner) const -> Result;
	[[nodiscard]] auto assign(std::string_view input, Option const& option, std::string_view value) const -> Result;

	std::string_view m_exe_name{};
	std::string_view m_cmd_id{};

	std::size_t m_next_argument{};
};
} // namespace cliq

#include <cliq/app_info.hpp>
#include <cliq/parse_result.hpp>
#include <scanner.hpp>

namespace cliq::foo {
class Parser {
  public:
	using Result = ParseResult;

	explicit Parser(AppInfo const& info, std::string_view const exe_name, std::span<char const* const> cli_args)
		: m_info(info), m_exe_name(exe_name), m_scanner(cli_args) {}

	[[nodiscard]] auto parse(std::span<Arg const> args) -> Result;

  private:
	struct Cursor {
		ParamCommand const* cmd{};
		std::size_t next_pos{};
	};

	auto select_command() -> Result;
	auto parse_next() -> Result;
	auto parse_option() -> Result;
	auto parse_letters() -> Result;
	auto parse_word() -> Result;
	auto parse_last_option(ParamOption const& option) -> Result;
	auto parse_argument() -> Result;
	auto parse_positional() -> Result;

	[[nodiscard]] auto find_option(char letter) const -> ParamOption const*;
	[[nodiscard]] auto find_option(std::string_view word) const -> ParamOption const*;
	[[nodiscard]] auto find_command(std::string_view name) const -> ParamCommand const*;

	[[nodiscard]] auto next_positional() -> ParamPositional const*;

	[[nodiscard]] auto check_required() -> Result;

	AppInfo const& m_info;
	std::string_view m_exe_name;

	Scanner m_scanner;
	std::span<Arg const> m_args{};
	Cursor m_cursor{};
	bool m_has_commands{};
};
} // namespace cliq::foo
