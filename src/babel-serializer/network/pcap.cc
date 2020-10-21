#include "pcap.hh"


babel::pcap::data babel::pcap::read(cc::span<const std::byte> data, read_config const& cfg, babel::error_handler on_error)
{
    babel::pcap::data r;

    // TODO: more error handling

    // copy raw data
    if (!cfg.view_only)
        r.bytes = data;

    // create header
    cc::as_byte_span(data, 0, sizeof(r.header)).copy_to(cc::as_byte_span(r.header));

    // create packets

    struct packet_header
    {
        uint32_t ts_sec;   /* timestamp seconds */
        uint32_t ts_usec;  /* timestamp microseconds */
        uint32_t incl_len; /* number of octets of packet saved in file */
        uint32_t orig_len; /* actual length of packet */
    };

    size_t pos = sizeof(r.header);
    while (pos < data.size())
    {
        packet_header header;
        cc::as_byte_span(data, pos, sizeof(header)).copy_to(cc::as_byte_span(header));
        pos += sizeof(header);

        auto& p = r.packets.emplace_back();
        p.timestamp_sec = header.ts_sec;
        p.timestamp_usec = header.ts_usec;
        p.stored_size = header.incl_len;
        p.original_size = header.orig_len;
        p.stored_offset = uint32_t(pos);

        pos += header.incl_len;
    }

    if (pos != data.size())
        on_error(data, {}, "last packet defined more data than actually provided", severity::warning);

    return r;
}
