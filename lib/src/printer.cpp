#include <printer.hpp>
#include <iomanip>
#include <iostream>

void cliqr::print(std::string_view const str) { std::cout << str; }

void cliqr::print_error(std::string_view const str) { std::cerr << str; }

namespace cliqr {
OldPrinter::OldPrinter() { std::cout << "  "; }

auto OldPrinter::single(char const c) -> OldPrinter& {
	if (c != 0) {
		std::cout << '-' << c;
		has_single = true;
	} else {
		std::cout << "  ";
	}
	current_width += 2;
	return *this;
}

auto OldPrinter::key(std::string_view const s) -> OldPrinter& {
	if (!s.empty()) {
		if (has_single) {
			std::cout << ", ";
		} else {
			std::cout << "  ";
		}
		std::cout << "--" << s;
		current_width += (4 + s.size());
	}
	return *this;
}

auto OldPrinter::command(std::string_view const s) -> OldPrinter& {
	std::cout << s;
	current_width += s.size();
	return *this;
}

auto OldPrinter::description(std::string_view const s) -> OldPrinter& {
	if (current_width + 1 > margin) {
		std::cout << "\n  ";
		current_width = 0;
	}
	std::cout << std::setw(static_cast<int>(margin - current_width + s.size())) << s << "\n";
	current_width = 0;
	return *this;
}

TabPrinter::TabPrinter(std::ostream& out, std::size_t const margin) : m_out(out), m_margin(margin) {
	m_out << std::setfill('.') << std::left;
	next_row();
}

auto TabPrinter::next_column(std::string_view const text) -> TabPrinter& {
	if (m_row.column_width > tab_size) {
		next_line();
		start_row();
		m_out << std::setw(static_cast<int>(m_row.margin)) << '_';
	}
	if (!m_row.has_print) { start_row(); }
	m_out << std::setw(static_cast<int>(tab_size + 1)) << text;
	m_row.column_width = text.size();
	m_row.margin += tab_size;

	m_out.flush();

	return *this;
}

auto TabPrinter::next_row() -> TabPrinter& {
	m_out << "\n";
	m_row = {};
	return *this;
}

void TabPrinter::next_line() {
	m_out << "\n";
	m_row.has_print = false;
}

void TabPrinter::start_row() {
	m_out << std::setw(static_cast<int>(m_margin)) << '_';
	m_row.has_print = true;

	m_out.flush();
}
} // namespace cliqr
