#include <cliq/app.hpp>
#include <ktest/ktest.hpp>

namespace {
TEST(basic) {
	EXPECT(1 == 1);
	EXPECT(1 == 2);
	ASSERT(1 == 3);
	EXPECT(1 == 4);
}
} // namespace
