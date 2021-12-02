#include "errors.hh"

#include <clean-core/assert.hh>
#include <clean-core/utility.hh>

#include <rich-log/log.hh>

#include <babel-serializer/source_map.hh>

void babel::default_error_handler(cc::span<const std::byte> data, cc::span<const std::byte> pos, cc::string_view message, babel::severity s)
{
    auto map = source_map(data);

    auto ls = pos.empty() ? 0 : map.line_of(reinterpret_cast<char const*>(&pos.front()));
    auto le = pos.empty() ? 0 : map.line_of(reinterpret_cast<char const*>(&pos.back()));

    auto padding = 2;
    auto lls = cc::max(0, ls - padding);
    auto lle = cc::min(int(map.lines().size()) - 1, le + padding);

    // TODO: somehow underline / mark / colorize offending pos?
    // TODO: what about binary files?

    switch (s)
    {
    case severity::warning:
        RICH_LOG_WARN("deserialization error: {}", message);
        for (auto l = lls; l <= lle; ++l)
            RICH_LOG_WARN("  {} {} {}", l, ls <= l && l <= le ? '>' : '|', map.lines()[l]);
        break;
    case severity::error:
        RICH_LOG_ERROR("deserialization error: {}", message);
        for (auto l = lls; l <= lle; ++l)
            RICH_LOG_ERROR("  {} {} {}", l, ls <= l && l <= le ? '>' : '|', map.lines()[l]);
        CC_UNREACHABLE("deserialization error");
        break;
    }
}
