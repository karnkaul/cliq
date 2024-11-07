#pragma once
#include <cliqr/command.hpp>

namespace cliqr {
class Instance {
  public:
	using Result = CommandResult;

	explicit Instance(std::string_view tagline, std::string_view version) : m_tagline(tagline), m_version(version) {}

	void add_command(std::unique_ptr<Command> command);

	auto run(int argc, char const* const* argv) -> Result;

  private:
	[[nodiscard]] auto try_builtin(std::string_view input) const -> bool;

	void print_usage() const;
	void print_help() const;
	void print_command_list() const;

	std::string_view m_tagline{};
	std::string_view m_version{};
	std::string m_app_name{};
	std::vector<std::unique_ptr<Command>> m_commands{};
};
} // namespace cliqr
