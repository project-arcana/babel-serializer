#include <nexus/test.hh>

#include <babel-serializer/data/csv.hh>

TEST("babel csv header-only")
{
    auto data = "foo, bla, \"ah ha\"";
    auto config = babel::csv::read_config();
    config.has_header = true;

    auto csv = babel::csv::read(data, config);
    CHECK(csv.header.size() == 3);
    CHECK(csv.header[0] == "foo");
    CHECK(csv.header[1] == "bla");
    CHECK(csv.header[2] == "ah ha");
    CHECK(csv.column_count == 3);
    CHECK(csv.row_count() == 0);
}

TEST("babel csv header-with-data")
{
    auto data = "foo, bla, \"ah ha\"\n1,2,foobar";
    auto config = babel::csv::read_config();
    config.has_header = true;

    auto csv = babel::csv::read(data, config);
    CHECK(csv.header.size() == 3);
    CHECK(csv.header[0] == "foo");
    CHECK(csv.header[1] == "bla");
    CHECK(csv.header[2] == "ah ha");
    CHECK(csv.column_count == 3);
    CHECK(csv.row_count() == 1);
    CHECK(csv.column("foo")[0].get_int() == 1);
    CHECK(csv.column("bla")[0].get_int() == 2);
    CHECK(csv.column("ah ha")[0].get_string() == "foobar");
}

TEST("babel csv trailing empty")
{
    auto data = "1,2,3,4\n5,6";
    auto config = babel::csv::read_config();
    config.has_header = false;

    auto csv = babel::csv::read(data, config);
    CHECK(csv.header.size() == 0);
    CHECK(csv.column_count == 4);
    CHECK(csv.row_count() == 2);
    CHECK(csv[0][0].get_int() == 1);
    CHECK(csv[0][1].get_int() == 2);
    CHECK(csv[0][2].get_int() == 3);
    CHECK(csv[0][3].get_int() == 4);
    CHECK(csv[1][0].get_int() == 5);
    CHECK(csv[1][1].get_int() == 6);
    CHECK(csv[1][2].raw_token.empty());
    CHECK(csv[1][3].raw_token.empty());
}

TEST("babel csv trailing empty start")
{
    auto data = "1,2\n3,4,5,6";
    auto config = babel::csv::read_config();
    config.has_header = false;

    auto csv = babel::csv::read(data, config);
    CHECK(csv.header.size() == 0);
    CHECK(csv.column_count == 4);
    CHECK(csv.row_count() == 2);
    CHECK(csv[0][0].get_int() == 1);
    CHECK(csv[0][1].get_int() == 2);
    CHECK(csv[0][2].raw_token.empty());
    CHECK(csv[0][3].raw_token.empty());
    CHECK(csv[1][0].get_int() == 3);
    CHECK(csv[1][1].get_int() == 4);
    CHECK(csv[1][2].get_int() == 5);
    CHECK(csv[1][3].get_int() == 6);
}
