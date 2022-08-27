#include <cstdint>

#include <nexus/test.hh>

#include <clean-core/any_of.hh>
#include <clean-core/map.hh>
#include <clean-core/optional.hh>
#include <clean-core/string.hh>
#include <clean-core/vector.hh>

#include <babel-serializer/data/json.hh>

namespace
{
struct foo
{
    int x = 2;
    bool b = false;
};
template <class I>
constexpr void introspect(I&& i, foo& v)
{
    i(v.x, "x");
    i(v.b, "b");
}

enum enumA
{
    valA,
    valB
};
enum class enumB
{
    valA,
    valB,
    valC
};
}

TEST("json basics")
{
    // primitives
    for (auto indent : {-1, 0, 2})
    {
        babel::json::write_config cfg;
        cfg.indent = indent;

        CHECK(babel::json::to_string(true, cfg) == "true");
        CHECK(babel::json::to_string(false, cfg) == "false");
        CHECK(babel::json::to_string(cc::nullopt, cfg) == "null");
        CHECK(babel::json::to_string(0, cfg) == "0");
        CHECK(babel::json::to_string(1, cfg) == "1");
        CHECK(babel::json::to_string(-12, cfg) == "-12");
        CHECK(babel::json::to_string(0.5, cfg) == "0.5");
        CHECK(babel::json::to_string(-0.25f, cfg) == "-0.25");
        CHECK(babel::json::to_string(std::byte(100), cfg) == "100");
        CHECK(babel::json::to_string('a', cfg) == "\"a\"");
        CHECK(babel::json::to_string("hello", cfg) == "\"hello\"");
        CHECK(babel::json::to_string(cc::string("hello"), cfg) == "\"hello\"");
        CHECK(babel::json::to_string(valA, cfg) == "0");
        CHECK(babel::json::to_string(valB, cfg) == "1");
        CHECK(babel::json::to_string(enumB::valA, cfg) == "0");
        CHECK(babel::json::to_string(enumB::valB, cfg) == "1");

        // optional
        {
            cc::optional<int> v;
            CHECK(babel::json::to_string(v, cfg) == "null");
            v = 7;
            CHECK(babel::json::to_string(v, cfg) == "7");
        }
    }

    // composites
    {
        int v[] = {1, 2, 3};
        CHECK(babel::json::to_string(v) == "[1,2,3]");
    }
    {
        cc::vector<int> v = {1, 2, 3};
        CHECK(babel::json::to_string(v) == "[1,2,3]");
    }
    {
        cc::vector<cc::vector<int>> v = {{}, {1}, {2, 3}};
        CHECK(babel::json::to_string(v) == "[[],[1],[2,3]]");
    }
    {
        CHECK(babel::json::to_string(foo{}) == "{\"x\":2,\"b\":false}");
    }

    // maps
    {
        cc::map<cc::string, int> m;
        m["abc"] = 17;
        m["de"] = 32;
        auto vA = "{\"abc\":17,\"de\":32}";
        auto vB = "{\"de\":32,\"abc\":17}";
        CHECK(babel::json::to_string(m) == cc::any_of(vA, vB));
    }
    {
        cc::map<int, int> m;
        m[33] = 17;
        m[-24] = 32;
        auto vA = "[[33,17],[-24,32]]";
        auto vB = "[[-24,32],[33,17]]";
        CHECK(babel::json::to_string(m) == cc::any_of(vA, vB));
    }

    // indent
    {
        int v[] = {1, 2, 3};
        CHECK(babel::json::to_string(v, babel::json::write_config{2})
              == "[\n"
                 "  1,\n"
                 "  2,\n"
                 "  3\n"
                 "]");
        CHECK(babel::json::to_string(v, babel::json::write_config{4})
              == "[\n"
                 "    1,\n"
                 "    2,\n"
                 "    3\n"
                 "]");
    }
    {
        cc::vector<int> v;
        CHECK(babel::json::to_string(v, babel::json::write_config{2})
              == "[\n"
                 "]");
    }
    {
        cc::vector<cc::vector<int>> v = {{}, {1}};
        CHECK(babel::json::to_string(v, babel::json::write_config{2})
              == "[\n"
                 "  [\n"
                 "  ],\n"
                 "  [\n"
                 "    1\n"
                 "  ]\n"
                 "]");
    }
    {
        CHECK(babel::json::to_string(foo{}, babel::json::write_config{2})
              == "{\n"
                 "  \"x\": 2,\n"
                 "  \"b\": false\n"
                 "}");
    }
    {
        cc::vector<foo> v = {{2, true}, {3, false}};
        CHECK(babel::json::to_string(v, babel::json::write_config{2})
              == "[\n"
                 "  {\n"
                 "    \"x\": 2,\n"
                 "    \"b\": true\n"
                 "  },\n"
                 "  {\n"
                 "    \"x\": 3,\n"
                 "    \"b\": false\n"
                 "  }\n"
                 "]");
    }
    {
        cc::map<cc::string, int> m;
        m["abc"] = 17;
        m["de"] = 32;
        auto vA = "{\n"
                  "  \"abc\": 17,\n"
                  "  \"de\": 32\n"
                  "}";
        auto vB = "{\n"
                  "  \"de\": 32,\n"
                  "  \"abc\": 17\n"
                  "}";
        CHECK(babel::json::to_string(m, babel::json::write_config{2}) == cc::any_of(vA, vB));
    }
    {
        cc::map<int, int> m;
        m[7] = 17;
        m[9] = 32;
        auto vA = "[\n"
                  "  [7, 17],\n"
                  "  [9, 32]\n"
                  "]";
        auto vB = "[\n"
                  "  [9, 32],\n"
                  "  [7, 17]\n"
                  "]";
        CHECK(babel::json::to_string(m, babel::json::write_config{2}) == cc::any_of(vA, vB));
    }
}

TEST("json escaping")
{
    CHECK(babel::json::to_string("h\"el\tl\\o") == "\"h\\\"el\\tl\\\\o\"");
    cc::map<cc::string, cc::string> m;
    m["a\"b"] = "c\nd";
    CHECK(babel::json::to_string(m) == "{\"a\\\"b\":\"c\\nd\"}");
}

TEST("json parsing raw ref")
{
    auto single_node = [](cc::string_view json) -> babel::json::json_ref::node {
        auto jref = babel::json::read_ref(json);
        CHECK(jref.nodes.size() == 1);
        CHECK(jref.nodes.front().is_leaf());
        return jref.nodes.front();
    };

    CHECK(single_node("123").token == "123");
    CHECK(single_node("true").token == "true");
    CHECK(single_node("false").token == "false");
    CHECK(single_node("null").token == "null");
    CHECK(single_node("-123e8").token == "-123e8");
    CHECK(single_node("\"abc\"").token == "\"abc\"");

    CHECK(single_node("123").get_int() == 123);
    CHECK(single_node("true").get_boolean() == true);
    CHECK(single_node("false").get_boolean() == false);
    CHECK(single_node("null").is_null());
    CHECK(single_node("-123e8").get_double() == -123e8);
    CHECK(single_node("\"abc\"").get_string() == "abc");

    CHECK(single_node("\"a\\n\\\\bc\"").get_string() == "a\n\\bc");

    // array
    for (auto json : {
             "[1,2,3]",        //
             "[  1, 2 ,3]",    //
             "[1, 2  ,3 ]",    //
             " [1, 2  ,3 ]  ", //
         })
    {
        auto jref = babel::json::read_ref(json);
        CHECK(jref.nodes.size() == 4);
        CHECK(jref.nodes[0].is_array());
        CHECK(jref.nodes[0].first_child == 1);
        CHECK(jref.nodes[1].is_number());
        CHECK(jref.nodes[1].next_sibling == 2);
        CHECK(jref.nodes[2].next_sibling == 3);
        CHECK(jref.nodes[1].get_int() == 1);
        CHECK(jref.nodes[2].get_int() == 2);
        CHECK(jref.nodes[3].get_int() == 3);
        CHECK(jref.nodes[0].token == cc::string_view(json).trim());
    }

    // map
    for (auto json : {
             "{\"a\":1,\"b\":true}",         //
             "{  \"a\":1 ,  \"b\" :true}",   //
             "{\"a\"  : 1,  \"b\":  true }", //
         })
    {
        auto jref = babel::json::read_ref(json);
        CHECK(jref.nodes.size() == 5);
        CHECK(jref.nodes[0].is_object());
        CHECK(jref.nodes[0].first_child == 1);
        CHECK(jref.nodes[1].is_string());
        CHECK(jref.nodes[2].is_number());
        CHECK(jref.nodes[3].is_string());
        CHECK(jref.nodes[4].is_boolean());
        CHECK(jref.nodes[1].next_sibling == 2);
        CHECK(jref.nodes[2].next_sibling == 3);
        CHECK(jref.nodes[3].next_sibling == 4);
        CHECK(jref.nodes[1].get_string() == "a");
        CHECK(jref.nodes[2].get_int() == 1);
        CHECK(jref.nodes[3].get_string() == "b");
        CHECK(jref.nodes[4].get_boolean() == true);
    }

    // nested array
    {
        auto json = "[[]]";
        auto jref = babel::json::read_ref(json);
        CHECK(jref.nodes.size() == 2);
        CHECK(jref.nodes[0].is_array());
        CHECK(jref.nodes[1].is_array());
        CHECK(jref.nodes[0].first_child == 1);
        CHECK(jref.nodes[0].token == "[[]]");
        CHECK(jref.nodes[1].token == "[]");
    }
    {
        auto json = "[[[]]]";
        auto jref = babel::json::read_ref(json);
        CHECK(jref.nodes.size() == 3);
        CHECK(jref.nodes[0].is_array());
        CHECK(jref.nodes[1].is_array());
        CHECK(jref.nodes[2].is_array());
        CHECK(jref.nodes[0].first_child == 1);
        CHECK(jref.nodes[1].first_child == 2);
        CHECK(jref.nodes[0].token == "[[[]]]");
        CHECK(jref.nodes[1].token == "[[]]");
        CHECK(jref.nodes[2].token == "[]");
    }
}

TEST("json parsing")
{
    CHECK(babel::json::read<int32_t>("123") == 123);
    CHECK(babel::json::read<int32_t>("-123") == -123);
    CHECK(babel::json::read<bool>("false") == false);
    CHECK(babel::json::read<float>("1.25") == 1.25f);
    CHECK(babel::json::read<double>("-1.25e1") == -12.5);
    CHECK(babel::json::read<char>("\"a\"") == 'a');
    CHECK(babel::json::read<char>("\"\\n\"") == '\n');
    CHECK(babel::json::read<cc::string>("\"ab\\ncd\"") == "ab\ncd");
    CHECK(babel::json::read<foo>("{\"x\": -12,\"b\":true}").x == -12);
    CHECK(babel::json::read<foo>("{\"x\": -12,\"b\":true}").b == true);
    CHECK(babel::json::read<cc::map<cc::string, int>>("{\"a\": 3,\"b\":7}") == cc::map<cc::string, int>{{"a", 3}, {"b", 7}});
    CHECK(babel::json::read<cc::map<int, int>>("[[1,3],[2,8]]") == cc::map<int, int>{{1, 3}, {2, 8}});
    CHECK(babel::json::read<cc::optional<int>>("null").has_value() == false);
    CHECK(babel::json::read<cc::optional<int>>("17").has_value());
    CHECK(babel::json::read<cc::optional<int>>("17").value() == 17);
    CHECK(babel::json::read<cc::vector<int>>("[1,2,3]") == cc::vector<int>{1, 2, 3});
    CHECK(babel::json::read<cc::array<bool, 3>>("[true,false,true]") == cc::array<bool, 3>{{true, false, true}});
    CHECK(babel::json::read<enumA>("0") == valA);
    CHECK(babel::json::read<enumA>("1") == valB);
    CHECK(babel::json::read<enumB>("0") == enumB::valA);
    CHECK(babel::json::read<enumB>("1") == enumB::valB);
}
