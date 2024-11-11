#pragma once
#include <string_view>

namespace cliq {
enum class TokenType {
	None,
	Option,	  // -[-][A-z]+[=[A-z]+]
	Argument, // [A-z]+
	OptEnd,	  // --
};

enum class OptionType {
	None,
	Letters, // -[A-z]+[=[A-z]+]
	Word,	 // --[A-z]+[=[A-z]+]
};

struct Token {
	std::string_view arg{};
	std::string_view value{};
	TokenType token_type{};
	OptionType option_type{};
};

constexpr auto to_token(std::string_view const input) -> Token {
	if (input.empty()) { return {}; }
	auto ret = Token{.arg = input, .value = input};
	if (ret.value == "--") {
		ret.value = {};
		ret.token_type = TokenType::OptEnd;
	} else if (ret.value.starts_with('-')) {
		ret.token_type = TokenType::Option;
		if (ret.value.starts_with("--")) {
			ret.value = ret.value.substr(2);
			ret.option_type = OptionType::Word;
		} else {
			ret.value = ret.value.substr(1);
			ret.option_type = OptionType::Letters;
		}
	} else {
		ret.token_type = TokenType::Argument;
	}
	return ret;
}
} // namespace cliq
