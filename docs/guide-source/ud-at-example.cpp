#include <iostream>
#include <string>
#include <chrono>
#include <random>
#include <limits>
#include <exception>

#include "spdn.h"

class my_function_package : public pdn::default_function_package<char8_t>
{
public:
    using base_type = pdn::default_function_package<char8_t>;
    struct date_time
    {
        int year;
        int month;
        int day;
        int hour;
        int minute;
        int second;
        static auto current() -> date_time
        {
            using namespace std::chrono;
            auto now = system_clock::now();
            auto zt = zoned_time{ locate_zone("UTC"), now };
            auto local_time = zt.get_local_time();

            auto day = floor<days>(local_time);
            auto ymd = year_month_day{ day };
            auto hms = hh_mm_ss{ local_time - day };

            return {
                .year   = static_cast<int>(ymd.year()),
                .month  = static_cast<int>(static_cast<unsigned>(ymd.month())),
                .day    = static_cast<int>(static_cast<unsigned>(ymd.day())),
                .hour   = static_cast<int>(hms.hours().count()),
                .minute = static_cast<int>(hms.minutes().count()),
                .second = static_cast<int>(hms.seconds().count())
            };
        }
    };
    struct random_int
    {
        std::default_random_engine eng;
        std::uniform_int_distribution<int> dist;
        static constexpr auto max = std::numeric_limits<int>::max();
        static constexpr auto min = std::numeric_limits<int>::min();
        auto next()
        {
            return dist(eng);
        }
        random_int() : eng{ std::random_device{}() }, dist{ min, max } {}
    };
    auto generate_constant(const pdn::unicode::u8string& iden) -> ::std::optional<pdn::u8entity>
    {
        using namespace std::string_view_literals;
        if (iden == u8"parse_date_utc_year"sv)
        {
            return std::make_optional<pdn::u8entity>(date_time::current().year);
        }
        if (iden == u8"parse_date_utc_month"sv)
        {
            return std::make_optional<pdn::u8entity>(date_time::current().month);
        }
        if (iden == u8"parse_date_utc_day"sv)
        {
            return std::make_optional<pdn::u8entity>(date_time::current().day);
        }
        if (iden == u8"parse_time_utc_hour"sv)
        {
            return std::make_optional<pdn::u8entity>(date_time::current().hour);
        }
        if (iden == u8"parse_time_utc_minute"sv)
        {
            return std::make_optional<pdn::u8entity>(date_time::current().minute);
        }
        if (iden == u8"parse_time_utc_second"sv)
        {
            return std::make_optional<pdn::u8entity>(date_time::current().second);
        }
        if (iden == u8"random"sv)
        {
            return std::make_optional<pdn::u8entity>(rd.next());
        }
        return base_type::generate_constant(iden);
    }
private:
    inline static random_int rd{};
};

int main() try
{
    using namespace std::string_view_literals;
    auto spdn_content = u8R"(
parse_date_time_utc {
    year:   @parse_date_utc_year
    month:  @parse_date_utc_month
    day:    @parse_date_utc_day
    hour:   @parse_time_utc_hour
    minute: @parse_time_utc_minute
    second: @parse_time_utc_second
}
random_list [
    @random,
    @random,
    @random,
    @random,
    @random,
]
)"sv;
    auto mfp = my_function_package{};
    auto e = pdn::parse(spdn_content, mfp, mfp, mfp, pdn::utf8_tag);
    const auto& dt = e[u8"parse_date_time_utc"sv];
    std::cout
        << "parse_date_time_utc:\n"
        << "    year   : " << dt[u8"year"sv  ].as_int() << "\n"
        << "    month  : " << dt[u8"month"sv ].as_int() << "\n"
        << "    day    : " << dt[u8"day"sv   ].as_int() << "\n"
        << "    hour   : " << dt[u8"hour"sv  ].as_int() << "\n"
        << "    minute : " << dt[u8"minute"sv].as_int() << "\n"
        << "    second : " << dt[u8"second"sv].as_int() << "\n";
    std::cout << "random_list:\n";
    for (const auto& rd : e[u8"random_list"sv].as_list())
    {
        std::cout << "    " << rd.as_int() << "\n";
    }
}
catch (std::exception& e)
{
    std::cerr << e.what() << "\n";
}
catch (...)
{
    std::cerr << "unknown exception\n";
}
