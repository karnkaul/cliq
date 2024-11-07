#include <cliqr/cliqr.hpp>
#include <exception>
#include <print>

namespace cliqr {
extern auto lab(int, char const* const*) -> int;
}

auto main(int argc, char** argv) -> int {
	try {
		return cliqr::lab(argc, argv);
	} catch (std::exception const& e) {
		std::println("PANIC: {}", e.what());
		return EXIT_FAILURE;
	} catch (...) {
		std::println("PANIC!");
		return EXIT_FAILURE;
	}
}
