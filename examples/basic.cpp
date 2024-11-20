#include <cliq/parse.hpp>
#include <array>
#include <print>

namespace {
auto run(int argc, char const* const* argv) -> int {
	static constexpr auto app_info_v = cliq::AppInfo{
		.description = "multiply two numbers",
		.version = cliq::version_v,
	};
	auto symbol = std::string_view{"x"};
	auto debug = bool{};
	auto num_0 = int{};
	auto num_1 = int{};

	auto const args = std::array{
		cliq::Arg{num_0, cliq::ArgType::Required, "NUM_0", "integer 0"},
		cliq::Arg{num_1, cliq::ArgType::Required, "NUM_1", "integer 1"},
		cliq::Arg{debug, "d,debug", "print all parameters"},
		cliq::Arg{symbol, "s,symbol", "multiplication symbol"},
	};
	auto const parse_result = cliq::parse(app_info_v, args, argc, argv);
	if (parse_result.early_return()) { return parse_result.get_return_code(); }

	if (debug) { std::println("params:\n  symbol\t: {}\n  verbose\t: {}\n  num_0\t\t: {}\n  num_1\t\t: {}\n", symbol, debug, num_0, num_1); }
	std::println("{} {} {} = {}", num_0, symbol, num_1, num_0 * num_1);

	return EXIT_SUCCESS;
}
} // namespace

auto main(int argc, char** argv) -> int {
	try {
		return run(argc, argv);
	} catch (std::exception const& e) {
		std::println(stderr, "PANIC: {}", e.what());
		return EXIT_FAILURE;
	} catch (...) {
		std::println(stderr, "FATAL ERROR");
		return EXIT_FAILURE;
	}
}
