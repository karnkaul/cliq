#pragma once
#include <cliq/app_info.hpp>
#include <cliq/arg.hpp>
#include <cliq/build_version.hpp>
#include <cliq/parse_result.hpp>

namespace cliq {
[[nodiscard]] auto parse(AppInfo const& info, std::span<Arg const> args, int argc, char const* const* argv) -> ParseResult;
}
