#pragma once
#include <cliqr/command.hpp>

namespace cliqr {
struct CreateInfo {
	std::string_view tagline{"[app description]"};
	std::string_view version{"unknown"};

	std::string_view epilogue{};
};

class Instance {
  public:
	using Result = CommandResult;

	explicit Instance(CreateInfo const& create_info) : m_info(create_info) {}

	void add_command(std::unique_ptr<Command> command);

	auto run(int argc, char const* const* argv) -> Result;

  private:
	struct Impl;

	CreateInfo m_info{};
	std::vector<std::unique_ptr<Command>> m_commands{};
};
} // namespace cliqr
