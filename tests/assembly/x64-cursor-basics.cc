#include <nexus/test.hh>

#include <babel-serializer/assembly/x64.hh>

#include <rich-log/log.hh>

TEST("x64 assembly basic decode")
{
    auto fun = +[](int a, int b) { return a + b; };

    auto p_start = (std::byte const*)fun;
    auto p_end = p_start + 5000; // TODO: how to get a more reliable estimate? get readable pages?

    while (true)
    {
        auto i = babel::x64::decode_one(p_start, p_end);
        if (!i.is_valid())
            break;
        CC_ASSERT(i.size > 0);

        LOG("%s", cc::span(i.data, i.data + i.size));
        LOG("%s", i);
        p_start += i.size;
    }
}
