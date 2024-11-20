#include <cliq/parse.hpp>
#include <array>
#include <print>

namespace {
struct Base {
	int num_0{};
	int num_1{};

	void print_params() const { std::println("params:\n  num_0\t: {}\n  num_1\t: {}\n", num_0, num_1); }

	std::array<cliq::Arg, 2> args{
		cliq::Arg{num_0, cliq::ArgType::Required, "NUM_0"},
		cliq::Arg{num_1, cliq::ArgType::Required, "NUM_1"},
	};
};

struct Add : Base {
	static constexpr std::string_view name_v{"add"};
	static constexpr std::string_view help_v{"add two integers"};

	auto operator()(bool const verbose) const -> int {
		if (verbose) { print_params(); }
		std::println("{} + {} = {}", num_0, num_1, num_0 + num_1);
		return EXIT_SUCCESS;
	}
};

struct Sub : Base {
	static constexpr std::string_view name_v{"sub"};
	static constexpr std::string_view help_v{"subtract two integers"};

	auto operator()(bool const verbose) const -> int {
		if (verbose) { print_params(); }
		std::println("{} - {} = {}", num_0, num_1, num_0 - num_1);
		return EXIT_SUCCESS;
	}
};

struct Mul : Base {
	static constexpr std::string_view name_v{"mul"};
	static constexpr std::string_view help_v{"multiply two integers"};

	auto operator()(bool const verbose) const -> int {
		if (verbose) { print_params(); }
		std::println("{} * {} = {}", num_0, num_1, num_0 * num_1);
		return EXIT_SUCCESS;
	}
};

struct Div : Base {
	static constexpr std::string_view name_v{"div"};
	static constexpr std::string_view help_v{"divide two integers"};

	auto operator()(bool const verbose) const -> int {
		if (verbose) { print_params(); }

		if (num_1 == 0) {
			std::println(stderr, "Division by zero");
			return EXIT_FAILURE;
		}

		std::println("{} / {} = {}", num_0, num_1, num_0 - num_1);
		return EXIT_SUCCESS;
	}
};

auto run(int argc, char const* const* argv) -> int {
	static constexpr auto app_info_v = cliq::AppInfo{
		.help_text = "calculator",
		.version = cliq::version_v,
	};

	auto verbose = false;
	auto add = Add{};
	auto sub = Sub{};
	auto mul = Mul{};
	auto div = Div{};
	auto const args = std::array{
		cliq::Arg{verbose, "v,verbose", "print parameters"}, cliq::Arg{add.args, Add::name_v, Add::help_v}, cliq::Arg{sub.args, Sub::name_v, Sub::help_v},
		cliq::Arg{mul.args, Mul::name_v, Mul::help_v},		 cliq::Arg{div.args, Div::name_v, Div::help_v},
	};

	auto const parse_result = cliq::parse(app_info_v, args, argc, argv);
	if (parse_result.early_return()) { return parse_result.get_return_code(); }

	if (parse_result.get_command_name() == Add::name_v) { return add(verbose); }
	if (parse_result.get_command_name() == Sub::name_v) { return sub(verbose); }
	if (parse_result.get_command_name() == Mul::name_v) { return mul(verbose); }
	if (parse_result.get_command_name() == Div::name_v) { return div(verbose); }

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
