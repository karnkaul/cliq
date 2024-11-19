#include <cliq/arg.hpp>

namespace cliq {
Arg::Arg(bool& out, std::string_view const key, std::string_view const help_text)
	: m_param(ParamOption{assignment<bool>(), &out, true, to_letter(key), to_word(key), help_text}) {}

Arg::Arg(std::span<Arg const> args, std::string_view const name, std::string_view const help_text) : m_param(ParamCommand{args, name, help_text}) {}
} // namespace cliq
