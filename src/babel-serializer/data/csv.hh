#pragma once

#include <clean-core/optional.hh>
#include <clean-core/strided_span.hh>
#include <clean-core/string.hh>
#include <clean-core/string_view.hh>
#include <clean-core/vector.hh>

#include <babel-serializer/errors.hh>

namespace babel::csv
{
struct read_config
{
    /// the separator used to separate values of a single column
    char separator = ',';

    /// if true, the first line is parsed as a header and its entries can be used to access the columns of the csv
    bool has_header = true;
};

/// a non-owning read-only view on a csv string
struct csv_ref
{
    struct entry
    {
        cc::string_view raw_token;

        bool empty() const { return raw_token.empty(); }
        cc::string get_string() const;
        int32_t get_int() const;
        uint32_t get_uint() const;
        float get_float() const;
        double get_double() const;
        int64_t get_int64() const;
        uint64_t get_uint64() const;
    };

    cc::vector<entry> entries;
    cc::vector<cc::string> header; // is empty if no header present
    size_t column_count = 0;

    size_t row_count() const { return entries.size() / column_count; }
    size_t col_count() const { return column_count; }

    cc::strided_span<entry const> column(size_t index) const
    {
        return cc::strided_span<entry const>(entries.data() + index, entries.size() / column_count, column_count * sizeof(entry));
    }

    cc::strided_span<entry const> column(cc::string_view name) const
    {
        CC_ASSERT(!header.empty() && "a header must be present to access columns this way");
        for (size_t i = 0; i < header.size(); ++i)
        {
            if (header[i] == name)
                return column(i);
        }
        CC_UNREACHABLE("column name does not exist");
    }

    cc::span<entry const> row(size_t index) const
    {
        CC_ASSERT(index < row_count());
        return cc::span(entries).subspan(index * column_count, column_count);
    }

    cc::span<entry const> operator[](size_t row_index) const { return row(row_index); }

    entry const& operator()(size_t row, size_t col) const { return entries[row + col * column_count]; }
};

csv_ref read(cc::string_view csv_string, read_config const& config = {}, error_handler on_error = default_error_handler);
} // namespace babel::csv
