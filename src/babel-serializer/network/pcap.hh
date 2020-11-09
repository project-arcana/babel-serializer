#pragma once

#include <cstdint>

#include <clean-core/array.hh>
#include <clean-core/function_ref.hh>
#include <clean-core/span.hh>
#include <clean-core/vector.hh>

#include <babel-serializer/errors.hh>

namespace babel::pcap
{
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
