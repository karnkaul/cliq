#include <ktest/ktest.hpp>
#include <parser_old.hpp>

namespace {
using namespace cliq;
using namespace old;

struct Fixture : old::Parser {
	std::vector<char const*> args{};
	Storage storage{};

	Scanner scanner{args};

	mutable std::string_view ran_builtin{};

	Fixture(std::initializer_list<char const*> args) { set_args(args); }

	void set_args(std::initializer_list<char const*> args) {
		this->args = args;
		scanner = Scanner{this->args};
		scanner.next();
	}

	void run_builtin(std::string_view input) const final { ran_builtin = input; }
};

TEST(builtin) {
	auto fixture = Fixture{"--help"};
	fixture.storage.builtins = {Builtin::help()};
	auto const result = fixture.parse_option(fixture.storage, fixture.scanner);
	EXPECT(result.executed_builtin());
	EXPECT(fixture.ran_builtin == "help");
}

TEST(flags) {
	bool f{};
	bool t{};
	auto fixture = Fixture{"-ft"};
	fixture.storage.options.push_back(Option{.key = {'f', "flag"}, .binding = std::make_unique<Binding<bool>>(f)});
	fixture.storage.options.push_back(Option{.key = {'t', "test"}, .binding = std::make_unique<Binding<bool>>(t)});
	auto result = fixture.parse_option(fixture.storage, fixture.scanner);
	EXPECT(result == EXIT_SUCCESS);
	EXPECT(f && t);

	f = t = false;
	fixture.set_args({"-tf"});
	result = fixture.parse_option(fixture.storage, fixture.scanner);
	EXPECT(result == EXIT_SUCCESS);
	EXPECT(f && t);

	f = t = false;
	fixture.set_args({"--test=true", "--flag=true"});
	result = fixture.parse_option(fixture.storage, fixture.scanner);
	EXPECT(result == EXIT_SUCCESS);
	EXPECT(fixture.scanner.next());
	result = fixture.parse_option(fixture.storage, fixture.scanner);
	EXPECT(result == EXIT_SUCCESS);
	EXPECT(f && t);
}

TEST(letter) {
	int x{};
	auto fixture = Fixture{"-x", "42"};
	fixture.storage.options.push_back(Option{.key = {'x', {}}, .binding = std::make_unique<Binding<int>>(x)});
	auto const result = fixture.parse_option(fixture.storage, fixture.scanner);
	EXPECT(result == EXIT_SUCCESS);
	EXPECT(x == 42);
}

TEST(word) {
	float y{};
	auto fixture = Fixture{"--yaw", "3.14"};
	fixture.storage.options.push_back(Option{.key = {'y', "yaw"}, .binding = std::make_unique<Binding<float>>(y)});
	auto const result = fixture.parse_option(fixture.storage, fixture.scanner);
	EXPECT(result == EXIT_SUCCESS);
	EXPECT(std::abs(y - 3.14f) < 0.001f);
}

TEST(invalid_option) {
	auto fixture = Fixture{"-x"};
	auto const result = fixture.parse_option(fixture.storage, fixture.scanner);
	ASSERT(result.get_parse_error().has_value());
	EXPECT(result.get_parse_error().value() == ParseError::InvalidOption);
}

TEST(missing_argument) {
	int x{};
	auto fixture = Fixture{"-x"};
	fixture.storage.options.push_back(Option{.key = {'x', {}}, .binding = std::make_unique<Binding<int>>(x)});
	auto const result = fixture.parse_option(fixture.storage, fixture.scanner);
	ASSERT(result.get_parse_error().has_value());
	EXPECT(result.get_parse_error().value() == ParseError::MissingArgument);
	EXPECT(result.get_return_code() == EXIT_FAILURE);
}

TEST(invalid_argument) {
	int x{};
	auto fixture = Fixture{"-x=abc"};
	fixture.storage.options.push_back(Option{.key = {'x', {}}, .binding = std::make_unique<Binding<int>>(x)});
	auto result = fixture.parse_option(fixture.storage, fixture.scanner);
	ASSERT(result.get_parse_error().has_value());
	EXPECT(result.get_parse_error().value() == ParseError::InvalidArgument);
	EXPECT(result.get_return_code() == EXIT_FAILURE);

	fixture.set_args({"abc"});
	fixture.storage.arguments.push_back(Argument{.binding = std::move(fixture.storage.options.front().binding)});
	fixture.storage.options.clear();
	result = fixture.parse_argument(fixture.storage, fixture.scanner);
	ASSERT(result.get_parse_error().has_value());
	EXPECT(result.get_parse_error().value() == ParseError::InvalidArgument);
	EXPECT(result.get_return_code() == EXIT_FAILURE);
}
} // namespace
