#pragma once
#include <cstdlib>
#include <optional>

namespace cliqr {
/// \brief Error parsing passed arguments.
enum class ParseError : int {
	InvalidCommand,
	InvalidOption,
	InvalidValue,
	InvalidArgument,
	MissingArgument,
};

struct ExecutedBuiltin {};

/// \brief Result of parsing args / running selected command.
class Result {
  public:
	constexpr Result(int command_result) : m_command_result(command_result) {}
	constexpr Result(ParseError parse_error) : m_parse_error(parse_error) {}
	constexpr Result(ExecutedBuiltin const& /*executed_builtin*/) : m_executed_builtin(true) {}

	/// \brief Check if cliqr executed a builtin option (like "--help")
	/// \returns true if cliqr executed a builtin option.
	[[nodiscard]] constexpr auto executed_builtin() const -> bool { return m_executed_builtin; }

	/// \brief Get the command result.
	/// \returns Return code of the command if it was run, otherwise EXIT_SUCCESS.
	[[nodiscard]] constexpr auto get_command_result() const -> int { return m_command_result; }

	/// \brief Get the error that occurred during argument parsing.
	/// \returns Argument parsing error, if any.
	[[nodiscard]] constexpr auto get_parse_error() const -> std::optional<ParseError> { return m_parse_error; }

	/// \brief Get the return value for main.
	/// \returns EXIT_FAILURE on parse error, otherwise the command result.
	[[nodiscard]] constexpr auto get_return_value() const -> int { return m_parse_error ? EXIT_FAILURE : get_command_result(); }

	/// \brief Compare equality with another Result.
	constexpr auto operator==(Result const& rhs) const {
		if (m_executed_builtin) { return rhs.m_executed_builtin; }
		if (m_parse_error) { return rhs.m_parse_error && *rhs.m_parse_error == *m_parse_error; }
		return m_command_result == rhs.m_command_result;
	}

  private:
	int m_command_result{EXIT_SUCCESS};
	std::optional<ParseError> m_parse_error{};
	bool m_executed_builtin{};
};

/// \brief Result representing success.
inline constexpr auto success_v = Result{EXIT_SUCCESS};
/// \brief Result representing generic failure.
inline constexpr auto failure_v = Result{EXIT_FAILURE};
} // namespace cliqr
