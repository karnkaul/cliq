#pragma once
#include <cliqr/command.hpp>
#include <cliqr/result.hpp>
#include <memory>

namespace cliqr {
/// \brief Application info.
struct CreateInfo {
	/// \brief One liner app description.
	std::string_view tagline{"[app description]"};
	/// \brief Version text.
	std::string_view version{"unknown"};

	/// \brief Help text epilogue.
	std::string_view epilogue{};
};

/// \brief Entry point and command storage.
class Instance {
  public:
	explicit Instance(CreateInfo const& create_info);

	/// \brief Add a command.
	/// \param command Command to add.
	void add_command(std::unique_ptr<Command> command);

	/// \brief Parse arguments and execute selected command.
	/// Prints help text by default.
	/// \param argc Number of arguments.
	/// \param argv Pointer to arguments.
	auto run(int argc, char const* const* argv) -> Result;

  private:
	struct Impl;
	struct Deleter {
		void operator()(Impl* ptr) const noexcept;
	};

	std::unique_ptr<Impl, Deleter> m_impl{};
};
} // namespace cliqr
