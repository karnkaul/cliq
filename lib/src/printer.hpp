#pragma once
#include <algorithm>
#include <string_view>

namespace cliqr {
void print(std::string_view str);
void print_error(std::string_view str);

struct OldPrinter {
	std::size_t margin{20};

	bool has_single{};
	std::size_t current_width{};

	OldPrinter();

	auto single(char c) -> OldPrinter&;
	auto key(std::string_view s) -> OldPrinter&;
	auto command(std::string_view s) -> OldPrinter&;
	auto description(std::string_view s) -> OldPrinter&;
};

template <typename ContainerT, typename GetStringT>
constexpr auto get_max_width(ContainerT&& container, GetStringT get_string) -> std::size_t {
	auto ret = std::size_t{};
	for (auto const& t : container) { ret = std::max(ret, get_string(t).size()); }
	return ret;
}

class TabPrinter {
  public:
	explicit TabPrinter(std::ostream& out, std::size_t margin = 2);

	auto next_column(std::string_view text) -> TabPrinter&;
	auto next_row() -> TabPrinter&;

	std::size_t margin{2};
	std::size_t tab_size{20};

  private:
	struct Row {
		std::size_t margin{};
		std::size_t column_width{};
		bool has_print{};
	};

	void next_line();
	void start_row();

	std::ostream& m_out;

	Row m_row{};
	std::size_t m_margin{};
};
} // namespace cliqr
