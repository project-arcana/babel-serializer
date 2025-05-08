#include "errors.hh"

#include <cstdio>

#include <clean-core/assert.hh>
#include <clean-core/bits.hh>
#include <clean-core/utility.hh>

#include <rich-log/log.hh>

#include <babel-serializer/detail/log.hh>
#include <babel-serializer/source_map.hh>

void babel::default_error_handler(cc::span<const std::byte> data, cc::span<const std::byte> pos, cc::string_view message, babel::severity s)
{
    auto map = source_map(data);

    // binary mode
    if (map.is_binary())
    {
        auto os = pos.empty() ? 0 : int64_t(pos.begin() - data.begin());
        auto oe = pos.empty() ? 0 : int64_t(pos.end() - 1 - data.begin());

        auto padding = 2 * 16;

        auto oos = cc::max(int64_t(0), os - padding);
        auto ooe = cc::min(int64_t(data.size()) - 1, oe + padding);

        oos = cc::align_down(oos, 16);
        ooe = cc::align_up(ooe + 1, 16);

        auto skip_o = oos + 16 * 10;
        auto skip_cnt = cc::align_up(oe - os - 16 * 16, 16);

        auto addr_size_chars = cc::count_leading_zeros(data.size()) / 4;

        cc::string line;

        auto make_line_for = [&](int64_t start_i)
        {
            line.clear();

            // "  >" or "  |"
            line += "  ";
            if (os <= start_i + 15 && start_i <= oe)
                line += '>';
            else
                line += '|';

            // address
            line += ' ';
            char buffer[16 + 1];
            auto res = std::snprintf(buffer, sizeof(buffer), "%.16zx", size_t(start_i));
            line += cc::string_view(buffer, res).subview(res - addr_size_chars - 1);
            line += ' ';

            enum
            {
                none,
                red,
                gray
            } curr_color
                = none;

            // hex part
            for (auto i = 0; i < 16; ++i)
            {
                auto is_red = os <= start_i + i && start_i + i <= oe;
                if (is_red && curr_color != red)
                {
                    line += "\u001b[38;5;196m"; // red
                    curr_color = red;
                }
                else if (!is_red && curr_color != gray)
                {
                    line += "\u001b[38;5;244m"; // gray
                    curr_color = gray;
                }

                line += ' ';
                if (start_i + i < int64_t(data.size()))
                    line += cc::to_string(data[start_i + i]);
                else
                    line += "  ";
            }
            line += "\u001b[0m"; // color reset

            // string part
            line += "  ";
            curr_color = none;
            for (auto i = 0; i < 16; ++i)
            {
                auto is_red = os <= start_i + i && start_i + i <= oe;
                if (is_red && curr_color != red)
                {
                    line += "\u001b[38;5;196m"; // red
                    curr_color = red;
                }
                else if (!is_red && curr_color != gray)
                {
                    line += "\u001b[38;5;244m"; // gray
                    curr_color = gray;
                }

                if (start_i + i < int64_t(data.size()))
                {
                    auto c = char(data[start_i + i]);
                    if (cc::is_printable(c))
                        line += c;
                    else
                        line += '.';
                }
                else
                    line += ' ';
            }
            line += "\u001b[0m"; // color reset
        };

        cc::string log_message;
        {
            cc::format_to(log_message, "deserialization error: {}\n", message);

            auto const border_lines = 3;
            auto const last_line = int64_t((data.size() + 15) / 16);

            // start of file
            for (auto ipre = 0; ipre < border_lines * 16; ipre += 16)
                if (oos > ipre)
                {
                    make_line_for(ipre);
                    cc::format_to(log_message, "{}\n", line);
                }
            if (oos > border_lines * 16)
                log_message += "  ...\n";

            // actual stuff
            auto start_i = oos;
            while (start_i < ooe)
            {
                if (start_i == skip_o)
                {
                    log_message += "  ...\n";
                    cc::format_to(log_message, "  ... skipping {} bytes\n", skip_cnt);
                    log_message += "  ...\n";
                    start_i += skip_cnt;
                }

                make_line_for(start_i);
                cc::format_to(log_message, "{}\n", line);

                start_i += 16;
            }

            // end of file
            if (ooe < (last_line - border_lines) * 16)
                log_message += "  ...\n";
            for (auto ipre = (last_line - border_lines) * 16; ipre < int64_t(data.size()); ipre += 16)
                if (ipre > ooe)
                {
                    make_line_for(ipre);
                    cc::format_to(log_message, "{}\n", line);
                }

            if (!log_message.empty())
                log_message.pop_back();
        }

        switch (s)
        {
        case severity::warning:
            RICH_LOG_WARN("%s", log_message);
            break;
        case severity::error:
            RICH_LOG_ERROR("%s", log_message);
            break;
        }
    }
    // text mode
    else
    {
        // ls is the line where the highlighted position starts
        // le is the line where the highlighted position ends
        auto const ls = pos.empty() ? 0 : map.line_of(reinterpret_cast<char const*>(&pos.front()));
        auto const le = pos.empty() ? 0 : map.line_of(reinterpret_cast<char const*>(&pos.back()));

        // we want to show "padding" many lines before and after the end
        auto const padding = 2;
        // the "real" start/end is thus lls/lle
        // which are clamped to the available number of lines
        // they are guaranteed <= and >= ls/le, but not necessarily by +- padding
        auto const lls = cc::max(0, ls - padding);
        auto const lle = cc::min(int(map.lines().size()) - 1, le + padding);

        // we don't want to show too many lines at once (imagine showing 1000 lines)
        // thus, if we show more than "max_real_lines" of the highlighted content
        // we want to skip ahead to the last "tail_real_lines" of the highlighted content
        auto const max_real_lines = 4;
        auto const tail_real_lines = 4; // symmetrix looks a bit better
        // skip_l is the last real line of the head that we want to show
        auto const skip_l = ls + max_real_lines - 1;
        // skip_target_l is the line we want to jump to
        auto const skip_target_l = le - tail_real_lines + 1;
        // skip_cnt is the number of lines that are not shown
        auto const skip_cnt = skip_target_l - skip_l;

        auto max_line_s = cc::to_string(map.lines().size());
        auto line_ellipsis = cc::string("...");
        while (line_ellipsis.size() < max_line_s.size())
            line_ellipsis.insert(0, " ");

        cc::string log_message;
        {
            cc::format_to(log_message, "deserialization error: {}\n", message);

            cc::string line;
            auto print_line = [&](int l)
            {
                line.clear();
                line += "  ";

                auto is_focus_line = ls <= l && l <= le;

                auto lnr = cc::to_string(l);
                for (auto i = 0; i < int(max_line_s.size()) - int(lnr.size()); ++i)
                    line += ' ';
                line += lnr;
                line += ' ';
                line += is_focus_line ? '>' : '|';
                line += ' ';
                if (is_focus_line)
                {
                    enum
                    {
                        none,
                        red,
                        gray
                    } curr_color
                        = none;

                    auto src_line = map.lines()[l];
                    for (auto p = src_line.begin(); p < src_line.end(); ++p)
                    {
                        auto is_red = pos.begin() <= (void*)p && (void*)p < pos.end();
                        if (is_red && curr_color != red)
                        {
                            line += "\u001b[38;5;196m"; // red
                            curr_color = red;
                        }
                        else if (!is_red && curr_color != gray)
                        {
                            line += "\u001b[38;5;244m"; // gray
                            curr_color = gray;
                        }
                        line += *p;
                    }
                }
                else
                {
                    line += "\u001b[38;5;244m"; // gray
                    line += map.lines()[l];
                }
                line += "\u001b[0m"; // color reset

                cc::format_to(log_message, "{}\n", line);
            };

            auto const border_lines = 3;
            for (auto l = 0; l < border_lines; ++l)
                if (l < lls)
                    print_line(l);
            if (lls > border_lines)
                cc::format_to(log_message, "  {}\n", line_ellipsis);

            for (auto l = lls; l <= lle; ++l)
            {
                print_line(l);
                if (skip_cnt > 3 && l == skip_l) // skip if we reach skip_l (and have a real skip)
                {
                    cc::format_to(log_message, "  {}\n", line_ellipsis);
                    cc::format_to(log_message, "  {} skipping {} lines\n", line_ellipsis, skip_cnt);
                    cc::format_to(log_message, "  {}\n", line_ellipsis);
                    l += skip_cnt - 1;
                }
            }

            if (lle < int(map.lines().size()) - border_lines - 1)
                cc::format_to(log_message, "  {}\n", line_ellipsis);
            for (auto l = int(map.lines().size()) - border_lines; l < int(map.lines().size()); ++l)
                if (l > lle)
                    print_line(l);
        };

        switch (s)
        {
        case severity::warning:
            RICH_LOG_WARN("%s", log_message);
            break;
        case severity::error:
            RICH_LOG_ERROR("%s", log_message);
            break;
        }
    }
}
