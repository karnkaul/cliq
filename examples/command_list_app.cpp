#include <cliq/app.hpp>
#include <print>

namespace {
struct Base : cliq::Command {
	Base() {
		required(num_0, "NUM0", "first integer");
		required(num_1, "NUM1", "second integer");
	}

	int num_0{};
	int num_1{};
};

struct Add : Base {
	[[nodiscard]] auto get_id() const -> std::string_view final { return "add"; }
	[[nodiscard]] auto get_description() const -> std::string_view final { return "add two numbers"; }

	[[nodiscard]] auto execute() -> int final {
		std::println("{} + {} = {}", num_0, num_1, num_0 + num_1);
		return EXIT_SUCCESS;
	}
};

struct Sub : Base {
	[[nodiscard]] auto get_id() const -> std::string_view final { return "sub"; }
	[[nodiscard]] auto get_description() const -> std::string_view final { return "subtract two numbers"; }

	[[nodiscard]] auto execute() -> int final {
		std::println("{} - {} = {}", num_0, num_1, num_0 - num_1);
		return EXIT_SUCCESS;
	}
};

struct Mul : Base {
	[[nodiscard]] auto get_id() const -> std::string_view final { return "mul"; }
	[[nodiscard]] auto get_description() const -> std::string_view final { return "multiply two numbers"; }

	[[nodiscard]] auto execute() -> int final {
		std::println("{} x {} = {}", num_0, num_1, num_0 * num_1);
		return EXIT_SUCCESS;
	}
};

struct Div : Base {
	[[nodiscard]] auto get_id() const -> std::string_view final { return "div"; }
	[[nodiscard]] auto get_description() const -> std::string_view final { return "divide two numbers"; }

	[[nodiscard]] auto execute() -> int final {
		if (num_1 == 0) {
			std::println(stderr, "Division by zero");
			return EXIT_FAILURE;
		}
		std::println("{} / {} = {}", num_0, num_1, num_0 / num_1);
		return EXIT_SUCCESS;
	}
};

struct App : cliq::CommandListApp {
	static constexpr auto info_v = cliq::AppInfo{
		.description = "calculator",
		.version = cliq::version_v,
	};

	App() {
		set_info(info_v);
		add_commands<Add, Sub, Mul, Div>();
	}
};
} // namespace

auto main(int argc, char** argv) -> int {
	try {
		return App{}.run(argc, argv).get_return_code();
	} catch (std::exception const& e) {
		std::println(stderr, "PANIC: {}", e.what());
		return EXIT_FAILURE;
	} catch (...) {
		std::println(stderr, "FATAL ERROR");
		return EXIT_FAILURE;
	}
}
