#pragma once

#include <cstdint>

#include <clean-core/array.hh>
#include <clean-core/function_ref.hh>
#include <clean-core/sentinel.hh>
#include <clean-core/span.hh>
#include <clean-core/vector.hh>

#include <babel-serializer/errors.hh>
#include <babel-serializer/file.hh>

namespace babel::pcap
{
/// pcap reader class
/// Usage:
/// auto reader = reader("my/file/path.pcap");
/// auto header = reader.header();
/// for(auto p : reader.packets())
///     my_process_package_data(p.data());
/// NOTE: You can provide your own raw data by providing a cc::span<std::byte const> instead of a filepath
/// NOTE: Currently only supports little endian and microsecond timestamps
class reader
{
public:
    /// header of the entire pcap file
    struct header
    {
        uint32_t magic_number;  ///< magic number
        uint16_t version_major; ///< major version number
        uint16_t version_minor; ///< minor version number
        int32_t thiszone;       ///< GMT to local correction
        uint32_t sigfigs;       ///< accuracy of timestamps
        uint32_t snap_length;   ///< max length of captured packets, in octets
        uint32_t network;       ///< data link type
    };
    /// single packet data
    struct packet
    {
        uint32_t timestamp_sec = 0;     ///< timestamp seconds
        uint32_t timestamp_usec = 0;    ///< timestamp microseconds
        uint32_t original_size = 0;     ///< original length of packet (NOT the stored one)
        cc::span<std::byte const> data; ///< raw data of the packet
    };

public: // ctor
    /// allocating constructor that will read the whole file into memory
    reader(cc::string_view filepath, error_handler on_error = default_error_handler)
    {
        _on_error = on_error;
        _file_data = file::read_all_bytes(filepath, on_error); // should this be lazy instead?
        _data = _file_data.data();
        _size = _file_data.size();
    }
    /// non-allocating constructor that references into some other data
    reader(cc::span<std::byte const> data, error_handler on_error = default_error_handler)
    {
        _on_error = on_error;
        _data = data.data();
        _size = data.size();
    }

private:
    struct packet_iterator
    {
        struct packet_header
        {
            uint32_t ts_sec;   /* timestamp seconds */
            uint32_t ts_usec;  /* timestamp microseconds */
            uint32_t incl_len; /* number of octets of packet saved in file */
            uint32_t orig_len; /* actual length of packet */
        };

        packet_iterator(cc::span<std::byte const> data, error_handler on_error) : _data{data}, _on_error{on_error}
        {
            if (!_data.empty())
                _current_packet = read_packet();
        }

        packet operator*() { return _current_packet; }

        packet_iterator& operator++()
        {
            _pos += sizeof(packet_header) + _current_packet.data.size();
            if (_pos < _data.size())
                _current_packet = read_packet();
            return *this;
        }

        packet_iterator operator++(int)
        {
            auto i = *this;
            operator++();
            return i;
        }

        bool operator!=(cc::sentinel) const { return _pos < _data.size(); }

    private:
        packet read_packet();
        cc::span<std::byte const> _data;
        packet _current_packet;
        size_t _pos = 0;
        error_handler _on_error;
    };

    struct packet_range
    {
        packet_range(cc::span<std::byte const> data, error_handler on_error) : data{data}, on_error{on_error} {}

        cc::span<std::byte const> data; ///< data without the header
        error_handler on_error;
        packet_iterator begin() const { return {data, on_error}; }
        cc::sentinel end() const { return {}; }
    };

public: // data
    header header()
    {
        struct header h;
        if (_size < sizeof(h))
        {
            _on_error(cc::span(_data, _size), cc::span(_data, _size).last(0), "Unexpected end of file", severity::error);
            return {};
        }
        cc::as_byte_span(h).copy_from(cc::as_byte_span(_data, 0, sizeof(h)));
        return h;
    }

    packet_range packets() { return {cc::span(_data, _size).subspan(sizeof(struct header)), _on_error}; }

    std::byte const* data() const { return _data; }
    size_t size() const { return _size; }

private:
    std::byte const* _data = nullptr;
    size_t _size = 0;
    cc::array<std::byte> _file_data; // only used when using the allocating constructor
    error_handler _on_error = default_error_handler;
};

struct read_config
{
    /// if true, does not copy data into bytes
    /// packet::stored_offset and packet::stored_size can then be used into the data passed into read
    bool view_only = false;
};

// see https://wiki.wireshark.org/Development/LibpcapFileFormat
struct data
{
    // NOTE: this is the global header of a pcap file
    struct header
    {
        uint32_t magic_number;  ///< magic number
        uint16_t version_major; ///< major version number
        uint16_t version_minor; ///< minor version number
        int32_t thiszone;       ///< GMT to local correction
        uint32_t sigfigs;       ///< accuracy of timestamps
        uint32_t snap_length;   ///< max length of captured packets, in octets
        uint32_t network;       ///< data link type
    } header;

    struct packet
    {
        uint32_t timestamp_sec;  ///< timestamp seconds
        uint32_t timestamp_usec; ///< timestamp microseconds
        uint32_t original_size;  ///< original length of packet (NOT the stored one)

        uint32_t stored_size; ///< actual stored length of the packet (might have been truncated)
        size_t stored_offset; ///< offset into data::bytes
    };

    /// vector of all packets
    cc::vector<packet> packets;

    /// returns a view on the packet data
    /// NOTE: does not work if cfg.view_only was true
    cc::span<std::byte const> packet_data(packet const& p) { return cc::as_byte_span(bytes).subspan(p.stored_offset, p.stored_size); }

    /// raw bytes of the pcap file (indexed by packet data)
    cc::array<std::byte> bytes;
};


/// reads a pcap dump from memory
data read(cc::span<std::byte const> data, read_config const& cfg = {}, error_handler on_error = default_error_handler);

/// reads a pcap dump from memory calling on_header once for the header and then on_packet for each contained packet
void read(cc::span<std::byte const> data,
          cc::function_ref<void(data::packet const&, cc::span<std::byte const>)> on_packet,
          cc::function_ref<void(struct data::header const&)> on_header = [](auto) {},
          read_config const& cfg = {},
          error_handler on_error = default_error_handler);
}
