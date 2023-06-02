#include <nexus/test.hh>

#include <clean-core/experimental/ringbuffer.hh>
#include <clean-core/set.hh>
#include <clean-core/vector.hh>

#include <babel-serializer/assembly/x64.hh>

#include <rich-log/log.hh>

static int test(int a, int b) { return a + b; }

TEST("x64 assembly basic decode")
{
    auto fun = +[](int a, int b) { return a + b; };

    cc::set<std::byte const*> seen_starts;
    cc::ringbuffer<std::byte const*> start_queue;
    start_queue.push_back((std::byte const*)test);
    start_queue.push_back((std::byte const*)fun);

    auto decode_block = [&](std::byte const* p_start)
    {
        if (!seen_starts.add(p_start))
            return; // already decoded

        LOG("");
        LOG("block: %s ...", cc::span(p_start, p_start + 30));
        auto p_end = p_start + 5000; // TODO: how to get a more reliable estimate? get readable pages?

        while (true)
        {
            auto i = babel::x64::decode_one(p_start, p_end);
            if (!i.is_valid())
                break;

            LOG("%<22s %s", i.as_span(), i);
            p_start += i.size;

            if (i.mnemonic == babel::x64::mnemonic::ret)
                break;

            if (is_relative_call(i))
                start_queue.push_back(relative_call_target(i));
        }
    };

    while (!start_queue.empty())
    {
        auto p = start_queue.pop_front();
        decode_block(p);
    }
}
