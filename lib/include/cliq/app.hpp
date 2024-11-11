#pragma once
#include <cliq/app_info.hpp>
#include <cliq/command.hpp>
#include <cliq/result.hpp>

namespace cliq {
/// \brief Interface for the main application.
class IApp : public Polymorphic {
  public:
	/// \brief Set application info.
	/// \param info AppInfo to set.
	virtual void set_info(AppInfo const& info) = 0;

	/// \brief Parse arguments and execute selected command.
	/// Prints help text by default.
	/// \param argc Number of arguments.
	/// \param argv Pointer to arguments.
	virtual auto run(int argc, char const* const* argv) -> Result = 0;
};

/// \brief Base type for an app that is also a Command.
/// For multiple commands, use CommandListApp.
class CommandApp : public IApp, public Command {
  public:
	CommandApp();

	/// \brief Set application info.
	/// \param info AppInfo to set.
	void set_info(AppInfo const& app_info) final { m_app_info = app_info; }

	/// \brief Parse arguments and execute selected command.
	/// Prints help text by default.
	/// \param argc Number of arguments.
	/// \param argv Pointer to arguments.
	auto run(int argc, char const* const* argv) -> Result override;

  protected:
	/// \brief Default execution is a noop.
	auto execute() -> int override { return EXIT_SUCCESS; }

  private:
	[[nodiscard]] auto get_id() const -> std::string_view final { return "[n/a]"; }
	[[nodiscard]] auto get_description() const -> std::string_view final { return m_app_info.description; }

	AppInfo m_app_info{};
};

/// \brief Base type for an App that stores a list of Commands.
/// For an app that functions as its own Command, use CommandApp.
class CommandListApp : public IApp {
  public:
	CommandListApp();

	/// \brief Set application info.
	/// \param info AppInfo to set.
	void set_info(AppInfo const& app_info) final;

	/// \brief Parse arguments and execute selected command.
	/// Prints help text by default.
	/// \param argc Number of arguments.
	/// \param argv Pointer to arguments.
	auto run(int argc, char const* const* argv) -> Result override;

	/// \brief Add a command.
	/// \param command Command to add.
	void add_command(std::unique_ptr<Command> command);

	/// \brief Add a command.
	/// Must be default constructible.
	template <std::derived_from<Command>... Ts>
	void add_commands() {
		(add_command(std::make_unique<Ts>()), ...);
	}

  private:
	struct Impl;
	struct Deleter {
		void operator()(Impl* ptr) const noexcept;
	};

	std::unique_ptr<Impl, Deleter> m_impl{};
};
} // namespace cliq
