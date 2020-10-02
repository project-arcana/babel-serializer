#pragma once

#include <cstddef> // std::byte

#include <clean-core/span.hh>
#include <clean-core/strided_span.hh>
#include <clean-core/string.hh>
#include <clean-core/vector.hh>

#include <babel-serializer/errors.hh>

namespace babel::ply
{
struct read_config
{
};

/// See http://paulbourke.net/dataformats/ply/
struct geometry
{
    struct property
    {
        enum class type
        {
            invalid,
            char_t,
            uchar_t,
            short_t,
            ushort_t,
            int_t,
            uint_t,
            float_t,
            double_t
        };

        cc::string name;
        type type = type::invalid;
        bool is_list = false;
        int data_start = -1;
        int data_stride = -1;
    };

    struct element
    {
        cc::string name;
        int count = -1;
        int properties_start = -1;
        int properties_count = -1;
    };

    cc::vector<property> properties;
    cc::vector<element> elements;
    cc::vector<std::byte> data;

public: // api
    bool has_element(cc::string_view name) const;

    element& get_element(cc::string_view name);
    element const& get_element(cc::string_view name) const;

    bool has_property(element const& element, cc::string_view name) const;

    cc::span<property> get_properties(cc::string_view element_name);
    cc::span<property const> get_properties(cc::string_view element_name) const;

    property& get_property(element const& element, cc::string_view name);
    property const& get_property(element const& element, cc::string_view name) const;

    template <class T>
    cc::strided_span<T> get_data(property const& property)
    {
    }

    template <class T>
    cc::strided_span<T const> get_data(property const& property) const;
};

geometry read(cc::span<const std::byte> data, babel::ply::read_config const& cfg = {}, error_handler on_error = default_error_handler);

}
