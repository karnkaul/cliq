#include <cliqr/instance.hpp>
#include <exception>
#include <print>

namespace {
struct Foo : cliqr::Command {
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

	[[nodiscard]] auto execute() -> int final {
		std::print("test\t: {}\ncount\t: {}\npath\t: '{}'\npaths\t: ", test, count, path);
		for (auto const& path : paths) { std::print("'{}' ", path); }
		std::println("");
		return EXIT_SUCCESS;
	}
};

struct LongCommand : cliqr::Command {
	[[nodiscard]] auto get_id() const -> std::string_view final { return "command-with-long-ass-id"; }
	[[nodiscard]] auto get_tagline() const -> std::string_view final { return "long command id"; }

	[[nodiscard]] auto execute() -> int final { return EXIT_SUCCESS; }
};

auto run(int const argc, char const* const* argv) -> int {
	auto const create_info = cliqr::CreateInfo{
		.tagline = "cliqr lab (test bench)",
		.version = "cliqr lab\nv0.0",
	};
	auto instance = cliqr::Instance{create_info};
	instance.add_command(std::make_unique<Foo>());
	instance.add_command(std::make_unique<LongCommand>());
	return instance.run(argc, argv).get_return_value();
}
} // namespace

auto main(int argc, char** argv) -> int {
	try {
		return run(argc, argv);
	} catch (std::exception const& e) {
		std::println("PANIC: {}", e.what());
		return EXIT_FAILURE;
	} catch (...) {
		std::println("PANIC!");
		return EXIT_FAILURE;
	}
}
