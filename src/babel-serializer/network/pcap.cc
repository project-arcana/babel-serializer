#include "pcap.hh"

#include <clean-core/utility.hh>

namespace
{
template <class T>
T byteswap(T v)
{
    static_assert(std::is_pod_v<T>);
    std::byte d[sizeof(v)];
    cc::as_byte_span(v).copy_to(cc::as_byte_span(d));
    for (size_t i = 0; i < (sizeof(v) / 2); ++i)
        cc::swap(d[i], d[sizeof(v) - 1 - i]);
    cc::as_byte_span(v).copy_from(cc::as_byte_span(d));
    return v;
}
}

babel::pcap::detail::packet_iterator::packet_iterator(cc::span<const std::byte> data, const babel::pcap::read_config& cfg, babel::error_handler on_error, bool swap_endianness)
  : _data{data}, _cfg{cfg}, _on_error{on_error}, _swap_endianness{swap_endianness}
{
    if (!_data.empty())
        _current_packet = read_packet();
}

babel::pcap::detail::packet_iterator& babel::pcap::detail::packet_iterator::operator++()
{
    _pos += sizeof(packet_header) + _current_packet.data.size();
    if (_pos < _data.size())
        _current_packet = read_packet();
    return *this;
}

babel::pcap::packet babel::pcap::detail::packet_iterator::read_packet()
{
    packet_header ph;
    if (_pos + sizeof(ph) > _data.size())
    {
        _on_error(_data, _data.last(0), "Unexpected end of file", severity::warning);
        return {};
    }
    _data.subspan(_pos, sizeof(ph)).copy_to(cc::as_byte_span(ph));
    if (_pos + sizeof(ph) + ph.incl_len > _data.size())
    {
        _on_error(_data, _data.last(0), "Unexpected end of file", severity::warning);
        return {};
    }
    if (_swap_endianness)
    {
        ph.incl_len = byteswap(ph.incl_len);
        ph.orig_len = byteswap(ph.orig_len);
        ph.ts_sec = byteswap(ph.ts_sec);
        ph.ts_usec = byteswap(ph.ts_usec);
    }
    packet p;
    p.original_size = ph.orig_len;
    p.timestamp_usec = ph.ts_usec;
    p.timestamp_sec = ph.ts_sec;
    p.data = _data.subspan(_pos + sizeof(ph), ph.incl_len);
    return p;
}

babel::pcap::header babel::pcap::header_of(cc::span<const std::byte> data, const babel::pcap::read_config& cfg, babel::error_handler on_error)
{
    (void)cfg; // for now unused

    header h;
    if (data.size() < sizeof(h))
    {
        on_error(data, data.last(0), "Unexpected end of file", severity::error);
        return {};
    }
    data.subspan(0, sizeof(h)).copy_to(cc::as_byte_span(h));
    if (h.swapped_endianness())
    {
        h.version_major = byteswap(h.version_major);
        h.thiszone = byteswap(h.thiszone);
        h.sigfigs = byteswap(h.sigfigs);
        h.snap_length = byteswap(h.snap_length);
        h.network = byteswap(h.network);
    }
    return h;
}

babel::pcap::detail::packet_range babel::pcap::packets_of(cc::span<const std::byte> data, const babel::pcap::read_config& cfg, babel::error_handler on_error)
{
    auto const h = header_of(data);
    return detail::packet_range(data.subspan(sizeof(header)), cfg, on_error, h.swapped_endianness());
}
