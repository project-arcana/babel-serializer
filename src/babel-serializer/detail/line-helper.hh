#pragma once

#include <clean-core/string.hh>

namespace babel::detail
{
// OnLine : (string_view) -> void
// skips empty lines
// merges lines ending with \ (without emitting that itself)
template <class OnLine>
void process_non_empty_lines_ex(cc::string_view lines, OnLine&& on_line)
{
    cc::string line;
    auto ignore_next_new_line = false;
    for (auto const c : lines)
    {
        if (c == '\r')
            continue; // ignore

        if (c == '\n')
        {
            if (!ignore_next_new_line)
            {
                if (!line.empty())
                    on_line(line);
                line.clear();
            }

            ignore_next_new_line = false;
        }
        else if (c == '\\') // \ at line end continues the line
        {
            ignore_next_new_line = true;
        }
        else
        {
            line.push_back(c);
            ignore_next_new_line = false;
        }
    }
}
} // namespace babel::detail
