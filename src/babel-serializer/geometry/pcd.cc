#include "pcd.hh"

// TODO: replace by cc stuff?
#include <cstdio>

#include <cstdint>

#include <clean-core/function_ptr.hh>
#include <clean-core/function_ref.hh>
#include <clean-core/string_view.hh>

#include <babel-serializer/deserialization_error.hh>

size_t babel::pcd::point_cloud::compute_stride() const
{
    size_t stride = 0;
    for (auto const& f : fields)
        stride += f.total_size();
    return stride;
}

bool babel::pcd::point_cloud::has_field(cc::string_view name) const
{
    for (auto const& f : fields)
        if (f.name == name)
            return true;
    return false;
}

const babel::pcd::field& babel::pcd::point_cloud::get_field(cc::string_view name) const
{
    for (auto const& f : fields)
        if (f.name == name)
            return f;

    CC_UNREACHABLE("field does not exist");
}

size_t babel::pcd::point_cloud::get_field_offset(cc::string_view name) const
{
    size_t offset = 0;
    for (auto const& f : fields)
    {
        if (f.name == name)
            return offset;

        offset += f.total_size();
    }

    CC_UNREACHABLE("field does not exist");
}

void babel::pcd::point_cloud::allocate_data()
{
    CC_ASSERT(!fields.empty() && "fields must be declared first");

    data.resize(points * compute_stride());
}

babel::pcd::point_cloud babel::pcd::read(cc::span<const std::byte> data)
{
    point_cloud pts;

    // TODO: error positions are wrong

    size_t prev_pos = 0;
    size_t pos = 0;
    auto get_line = [&]() -> cc::string_view {
        prev_pos = pos;
        if (pos >= data.size())
            throw deserialization_error(pos, "excepted line");

        // skip comment lines
        while (char(data[pos]) == '#')
        {
            while (pos < data.size() && char(data[pos]) != '\n')
                ++pos;
            if (pos < data.size() && char(data[pos]) == '\n')
                ++pos;
        }

        auto p_start = reinterpret_cast<char const*>(&data[pos]);
        while (pos < data.size())
        {
            auto c = char(data[pos]);
            if (c == '\n' || c == '#')
                break;

            ++pos;
        }
        auto p_end = reinterpret_cast<char const*>(data.data() + pos);

        // ignore rest of comment
        while (pos < data.size())
        {
            auto c = char(data[pos]);
            if (c == '\n')
                break;

            ++pos;
        }

        // point behind newline
        if (pos < data.size() && char(data[pos]) == '\n')
            ++pos;

        return cc::string_view(p_start, p_end);
    };

    auto parse_int = [&](cc::string_view s) -> int {
        cc::string ss = s; // null-terminated
        int i;
        auto r = std::sscanf(ss.c_str(), "%d", &i);
        if (r == 0)
            throw deserialization_error(pos, "unable to parse int");
        return i;
    };
    auto parse_int64 = [&](cc::string_view s) -> int64_t {
        cc::string ss = s; // null-terminated
        int64_t i;
        auto r = std::sscanf(ss.c_str(), "%ld", &i);
        if (r == 0)
            throw deserialization_error(pos, "unable to parse int");
        return i;
    };
    auto parse_uint64 = [&](cc::string_view s) -> uint64_t {
        cc::string ss = s; // null-terminated
        uint64_t i;
        auto r = std::sscanf(ss.c_str(), "%lu", &i);
        if (r == 0)
            throw deserialization_error(pos, "unable to parse int");
        return i;
    };
    auto parse_float = [&](cc::string_view s) -> float {
        cc::string ss = s; // null-terminated
        float f;
        auto r = std::sscanf(ss.c_str(), "%f", &f);
        if (r == 0)
            throw deserialization_error(pos, "unable to parse int");
        return f;
    };
    auto parse_double = [&](cc::string_view s) -> double {
        cc::string ss = s; // null-terminated
        double f;
        auto r = std::sscanf(ss.c_str(), "%lf", &f);
        if (r == 0)
            throw deserialization_error(pos, "unable to parse int");
        return f;
    };

    // version
    {
        auto s = get_line();
        if (!s.starts_with("VERSION "))
            throw deserialization_error(prev_pos, "expected VERSION line");

        pts.version = s.subview(8).trim();
        // TODO: make me into some warning feature
        if (pts.version != "0.7" && pts.version != ".7" && pts.version != "7")
            throw deserialization_error(prev_pos, "expected VERSION 0.7");
    }

    // fields
    {
        auto s = get_line();
        if (!s.starts_with("FIELDS "))
            throw deserialization_error(prev_pos, "expected FIELDS line");
        s = s.subview(7).trim();

        for (auto name : s.split())
        {
            auto& f = pts.fields.emplace_back();
            f.name = name;
        }
    }

    // size
    {
        auto s = get_line();
        if (!s.starts_with("SIZE "))
            throw deserialization_error(prev_pos, "expected SIZE line");
        s = s.subview(5).trim();

        size_t i = 0;
        for (auto v : s.split())
        {
            if (i >= pts.fields.size())
                throw deserialization_error(prev_pos, "too many entries");

            pts.fields[i].size = parse_int(v);

            ++i;
        }
        if (i != pts.fields.size())
            throw deserialization_error(prev_pos, "too few entries");
    }

    // type
    {
        auto s = get_line();
        if (!s.starts_with("TYPE "))
            throw deserialization_error(prev_pos, "expected TYPE line");
        s = s.subview(5).trim();

        size_t i = 0;
        for (auto v : s.split())
        {
            if (i >= pts.fields.size())
                throw deserialization_error(prev_pos, "too many entries");

            if (v != "I" && v != "U" && v != "F")
                throw deserialization_error(prev_pos, "unknown type");

            pts.fields[i].type = v[0];

            ++i;
        }
        if (i != pts.fields.size())
            throw deserialization_error(prev_pos, "too few entries");
    }

    // count
    {
        auto s = get_line();
        if (!s.starts_with("COUNT "))
            throw deserialization_error(prev_pos, "expected COUNT line");
        s = s.subview(5).trim();

        size_t i = 0;
        for (auto v : s.split())
        {
            if (i >= pts.fields.size())
                throw deserialization_error(prev_pos, "too many entries");

            pts.fields[i].count = parse_int(v);

            ++i;
        }
        if (i != pts.fields.size())
            throw deserialization_error(prev_pos, "too few entries");
    }

    // width
    {
        auto s = get_line();
        if (!s.starts_with("WIDTH "))
            throw deserialization_error(prev_pos, "expected WIDTH line");
        pts.width = parse_int(s.subview(6).trim());
    }
    // height
    {
        auto s = get_line();
        if (!s.starts_with("HEIGHT "))
            throw deserialization_error(prev_pos, "expected HEIGHT line");
        pts.height = parse_int(s.subview(7).trim());
    }

    // viewpoint
    {
        auto s = get_line();
        if (!s.starts_with("VIEWPOINT "))
            throw deserialization_error(prev_pos, "expected VIEWPOINT line");
        s = s.subview(10).trim();

        cc::vector<float> vp;
        for (auto v : s.split())
            vp.push_back(parse_float(v));
        if (vp.size() != 3 + 4)
            throw deserialization_error(prev_pos, "expected 7 floats");

        pts.viewpoint.position = {vp[0], vp[1], vp[2]};
        pts.viewpoint.rotation = {vp[3], vp[4], vp[5], vp[6]};
    }

    // points
    {
        auto s = get_line();
        if (!s.starts_with("POINTS "))
            throw deserialization_error(prev_pos, "expected POINTS line");
        pts.points = parse_int(s.subview(7).trim());
    }

    // data
    {
        auto s = get_line();
        if (!s.starts_with("DATA "))
            throw deserialization_error(prev_pos, "expected DATA line");
        s = s.subview(5).trim();

        pts.allocate_data();

        if (s == "binary")
        {
            if (data.size() - pos != pts.data.size())
                throw deserialization_error(prev_pos, "DATA size mismatch");

            // just a raw copy
            std::memcpy(pts.data.data(), data.data() + pos, pts.data.size());
        }
        else if (s == "ascii")
        {
            // build deserializers
            cc::vector<size_t> offsets;
            cc::vector<cc::function_ref<void(std::byte*, cc::string_view)>> parsers;
            {
                size_t offset = 0;
                for (auto const& f : pts.fields)
                {
                    for (auto i = 0; i < f.count; ++i)
                    {
                        offsets.push_back(offset);
                        switch (f.type)
                        {
                        case 'I':
                            switch (f.size)
                            {
                            case 1:
                                parsers.push_back([&](std::byte* d, cc::string_view s) { //
                                    *reinterpret_cast<int8_t*>(d) = parse_int(s);
                                });
                                break;
                            case 2:
                                parsers.push_back([&](std::byte* d, cc::string_view s) { //
                                    *reinterpret_cast<int16_t*>(d) = parse_int(s);
                                });
                                break;
                            case 4:
                                parsers.push_back([&](std::byte* d, cc::string_view s) { //
                                    *reinterpret_cast<int32_t*>(d) = parse_int(s);
                                });
                                break;
                            case 8:
                                parsers.push_back([&](std::byte* d, cc::string_view s) { //
                                    *reinterpret_cast<int64_t*>(d) = parse_int64(s);
                                });
                                break;
                            default:
                                throw deserialization_error(prev_pos, "invalid field size");
                            }
                            break;
                        case 'U':
                            switch (f.size)
                            {
                            case 1:
                                parsers.push_back([&](std::byte* d, cc::string_view s) { //
                                    *reinterpret_cast<uint8_t*>(d) = parse_uint64(s);
                                });
                                break;
                            case 2:
                                parsers.push_back([&](std::byte* d, cc::string_view s) { //
                                    *reinterpret_cast<uint16_t*>(d) = parse_uint64(s);
                                });
                                break;
                            case 4:
                                parsers.push_back([&](std::byte* d, cc::string_view s) { //
                                    *reinterpret_cast<uint32_t*>(d) = parse_uint64(s);
                                });
                                break;
                            case 8:
                                parsers.push_back([&](std::byte* d, cc::string_view s) { //
                                    *reinterpret_cast<uint64_t*>(d) = parse_uint64(s);
                                });
                                break;
                            default:
                                throw deserialization_error(prev_pos, "invalid field size");
                            }
                            break;
                        case 'F':
                            switch (f.size)
                            {
                            case 4:
                                parsers.push_back([&](std::byte* d, cc::string_view s) { //
                                    *reinterpret_cast<float*>(d) = parse_float(s);
                                });
                                break;
                            case 8:
                                parsers.push_back([&](std::byte* d, cc::string_view s) { //
                                    *reinterpret_cast<double*>(d) = parse_double(s);
                                });
                                break;
                            default:
                                throw deserialization_error(prev_pos, "invalid field size");
                            }
                            break;
                        default:
                            CC_UNREACHABLE("unknown type");
                        }

                        offset += f.size;
                    }
                }
                CC_ASSERT(offsets.size() == parsers.size() && "invalid type somewhere");
            }

            // each point line by line
            for (size_t p = 0; p < pts.points; ++p)
            {
                s = get_line();
                auto stride = pts.compute_stride();

                size_t i = 0;
                for (auto v : s.split())
                {
                    if (i >= parsers.size())
                        throw deserialization_error(prev_pos, "too many entries");

                    auto d = pts.data.data() + stride * p + offsets[i];
                    parsers[i](d, v);

                    ++i;
                }
                if (i != parsers.size())
                    throw deserialization_error(prev_pos, "too few entries");
            }
        }
        else
            throw deserialization_error(prev_pos, "unexpected DATA format");
    }

    return pts;
}
