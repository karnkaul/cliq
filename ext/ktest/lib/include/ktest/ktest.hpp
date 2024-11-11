#pragma once
#include <string_view>

namespace ktest {
void check_expect(bool pred, std::string_view expr, std::string_view file, int line);
void check_assert(bool pred, std::string_view expr, std::string_view file, int line);

using TestFunc = void (*)();

struct TestCase {
	virtual ~TestCase() = default;

	TestCase(TestCase const&) = delete;
	TestCase(TestCase&&) = delete;
	auto operator=(TestCase const&) = delete;
	auto operator=(TestCase&&) = delete;

	explicit TestCase(std::string_view name);

	virtual void run() const = 0;

	std::string_view name{};
};

auto run_tests() -> int;
} // namespace ktest

#define ASSERT(pred) ktest::check_assert(pred, #pred, __FILE__, __LINE__) // NOLINT(cppcoreguidelines-macro-usage)
#define EXPECT(pred) ktest::check_expect(pred, #pred, __FILE__, __LINE__) // NOLINT(cppcoreguidelines-macro-usage)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define TEST(name)                                                                                                                                             \
	struct TestCase_##name : ktest::TestCase {                                                                                                                 \
		using TestCase::TestCase;                                                                                                                              \
		void run() const final;                                                                                                                                \
	};                                                                                                                                                         \
	TestCase_##name const g_test_case_##name{#name};                                                                                                           \
	void TestCase_##name::run() const
