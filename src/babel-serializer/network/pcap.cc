#include "pcap.hh"

#include <rich-log/log.hh>
#include <clean-core/format.hh>


babel::pcap::data babel::pcap::read(cc::span<const std::byte> data, read_config const& cfg, babel::error_handler on_error)
{
    babel::pcap::data r;

    read(data, [&](auto const& packet, auto) { r.packets.push_back(packet); }, [&](auto const& header) { r.header = header; }, cfg, on_error);

    // copy raw data
    if (!cfg.view_only)
        r.bytes = data;

    return r;
}

void babel::pcap::read(cc::span<std::byte const> data, cc::function_ref<void (data::packet const&, cc::span<std::byte const>)> on_packet,cc::function_ref<void (struct data::header const&)> on_header, babel::pcap::read_config const& cfg, babel::error_handler on_error)
{
    // TODO: more error handling
    // TODO: Endianness, nanosecond timestamps (using magic number)

    struct data::header h;

    if(data.size() < sizeof(h))
        on_error(data, data, "not enough data to read header", severity::error);

    cc::as_byte_span(data, 0, sizeof(h)).copy_to(cc::as_byte_span(h));

    CC_ASSERT(h.magic_number == 0xa1b2c3d4 && "currently only supports little endian and microsecond timestamps");

    on_header(h);

    // create packets

    struct packet_header
    {
        uint32_t ts_sec;   /* timestamp seconds */
        uint32_t ts_usec;  /* timestamp microseconds */
        uint32_t incl_len; /* number of octets of packet saved in file */
        uint32_t orig_len; /* actual length of packet */
    };

    size_t pos = sizeof(h);
    while (pos < data.size())
    {
        packet_header header;
        cc::as_byte_span(data, pos, sizeof(header)).copy_to(cc::as_byte_span(header));
        pos += sizeof(header);

        data::packet p;
        p.timestamp_sec = header.ts_sec;
        p.timestamp_usec = header.ts_usec;
        p.stored_size = header.incl_len;
        p.original_size = header.orig_len;
        p.stored_offset = uint32_t(pos);

        on_packet(p, data.subspan(p.stored_offset, p.stored_size));

        pos += header.incl_len;
    }

    if (pos != data.size())
        on_error(data, {}, "last packet defined more data than actually provided", severity::warning);

}
