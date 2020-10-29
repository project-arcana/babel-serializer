#pragma once

#include <cstddef>     // std::byte
#include <type_traits> // std::is_same_v

#include <clean-core/assert.hh>
#include <clean-core/span.hh>
#include <clean-core/strided_span.hh>
#include <clean-core/string.hh>
#include <clean-core/vector.hh>

#include <babel-serializer/errors.hh>

/// TODO: Iterators

/// See http://paulbourke.net/dataformats/ply/
namespace babel::ply
{
struct read_config
{
};
/// valid types that can appear in a ply file
/// signed integers are two's complement
/// float and double are IEEE 754
enum class type
{
    invalid,
    char_t,   // 1 byte
    uchar_t,  // 1 byte
    short_t,  // 2 byte
    ushort_t, // 2 byte
    int_t,    // 4 byte
    uint_t,   // 4 byte
    float_t,  // 4 byte
    double_t  // 8 byte
};

struct geometry
{
    /// A ply element definition with a 'name' and the number of entries ('count') of this type of element.
    /// Each element consists of a set of properties which are consecutively stored in 'properties'.
    /// An element's properties range from the indices 'properties_start' to 'properties_start + properties_count'
    struct element
    {
        cc::string name;
        int count = -1;
        int properties_start = -1; // index of the first property
        int properties_count = -1; // number of properties
    };

    /// A ply property definition with a 'name' and a 'type'.
    /// A property is a list if 'list_size_type' is NOT invalid
    /// A properties data is stored in 'data'
    /// If a property is a list data only stores a list_property_entry which
    /// contains indices pointing into 'list_data'
    struct property
    {
        cc::string name;
        type type = type::invalid;
        enum type list_size_type = type::invalid;
        int data_start = -1;  // start index pointing into data
        int data_stride = -1; // stride in bytes

        constexpr bool is_list() const { return list_size_type != type::invalid; }
    };

    /// To allow the use of cc::strided_span to access a propertys data, lists are stored separately in 'list_data'.
    /// This means for lists an additional layer of indirection is necessary.
    /// Therefore the content of 'data' for a list property is this list struct.
    struct list_property_entry
    {
        int start_idx = -1; // index into list_data
        int size = -1;      // the number of elements, not bytes
    };

public: // member
    cc::vector<property> properties;
    cc::vector<element> elements;
    cc::vector<std::byte> data;
    cc::vector<std::byte> list_data;

public: // api
    bool has_element(cc::string_view name) const;

    element& get_element(cc::string_view name);
    element const& get_element(cc::string_view name) const;

    bool has_property(element const& element, cc::string_view name) const;

    property& get_property(element const& element, cc::string_view name);
    property const& get_property(element const& element, cc::string_view name) const;

    cc::span<property> get_properties(cc::string_view element_name);
    cc::span<property const> get_properties(cc::string_view element_name) const;

    template <class T>
    cc::strided_span<T> get_data(element const& element, property const& property)
    {
        if constexpr (std::is_same_v<T, list_property_entry>)
            CC_ASSERT(property.is_list());
        return {&data[property.data_start], element.count, property.data_stride};
    }

    template <class T>
    cc::strided_span<T const> get_data(element const& element, property const& property) const
    {
        if constexpr (std::is_same_v<T, list_property_entry>)
            CC_ASSERT(property.is_list());
        return {&data[property.data_start], element.count, property.data_stride};
    }

    template <class T>
    cc::span<T> get_data(list_property_entry const& list)
    {
        return {&list_data[list.start_idx], list.size};
    }

    template <class T>
    cc::span<T const> get_data(list_property_entry const& list) const
    {
        return {&list_data[list.start_idx], list.size};
    }
};

geometry read(cc::span<const std::byte> data, babel::ply::read_config const& cfg = {}, error_handler on_error = default_error_handler);

}
