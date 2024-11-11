#include <cliq/app.hpp>
#include <print>

namespace {
struct Multiplier : cliq::CommandApp {
	static constexpr auto info_v = cliq::AppInfo{
		.description = "multiply two numbers",
		.version = cliq::version_v,
	};

	Multiplier() {
		set_info(info_v);
		flag(debug, "debug", "debug mode", "print all parameters");
		optional(symbol, "s,symbol", "symbol", "multiplication symbol");
		required(num_0, "NUM0", "first integer");
		required(num_1, "NUM1", "second integer");
	}

	auto execute() -> int final {
		if (debug) { std::println("params:\n  symbol\t: {}\n  verbose\t: {}\n  num_0\t\t: {}\n  num_1\t\t: {}\n", symbol, debug, num_0, num_1); }
		std::println("{} {} {} = {}", num_0, symbol, num_1, num_0 * num_1);
		return EXIT_SUCCESS;
	}

	std::string_view symbol{"x"};
	bool debug{};
	int num_0{};
	int num_1{};
};
} // namespace

auto main(int argc, char** argv) -> int {
	try {
		return Multiplier{}.run(argc, argv).get_return_code();
	} catch (std::exception const& e) {
		std::println(stderr, "PANIC: {}", e.what());
		return EXIT_FAILURE;
	} catch (...) {
		std::println(stderr, "FATAL ERROR");
		return EXIT_FAILURE;
	}
}
