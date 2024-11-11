#include <storage.hpp>
#include <format>

namespace cliq {
Builtin::Builtin(std::string_view const word, std::string_view const description)
	: word(word), description(description), print_key(Storage::get_print_key({}, word)) {}

auto Builtin::usage() -> Builtin { return Builtin{"usage", "display usage"}; }
auto Builtin::help() -> Builtin { return Builtin{"help", "display this help and exit"}; }
auto Builtin::version() -> Builtin { return Builtin{"version", "output version information and exit"}; }

auto Storage::get_print_key(char const letter, std::string_view const word) -> std::string {
	auto ret = std::string(4, ' ');
	if (letter != 0) {
		ret[0] = '-';
		ret[1] = letter;
		if (!word.empty()) { ret[2] = ','; }
	}
	if (!word.empty()) { std::format_to(std::back_inserter(ret), "--{}", word); }
	return ret;
}

void Storage::bind_option(BindInfo info, std::string_view const key) {
	if (key.empty()) { return; }
	auto option = Option{
		.key = OptionKey::make(key),
		.name = info.name,
		.description = info.description,
		.binding = std::move(info.binding),
	};
	option.print_key = get_print_key(option.key.letter, option.key.word);
	options.push_back(std::move(option));
}

void Storage::bind_argument(BindInfo info, bool const is_list) {
	auto argument = Argument{
		.name = info.name,
		.description = info.description,
		.binding = std::move(info.binding),
	};
	if (is_list) {
		list_argument.emplace(std::move(argument));
	} else if (list_argument) {
		// cannot bind individual args after list arg
		return;
	} else {
		arguments.push_back(std::move(argument));
	}

	std::string_view const suffix = is_list ? "..." : "";
	std::format_to(std::back_inserter(args_text), " {}{}", info.name, suffix);
}
} // namespace cliq
