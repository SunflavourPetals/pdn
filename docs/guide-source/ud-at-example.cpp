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
        using namespace pdn::unicode_literals;
        if (iden == u8"parsing_time"sv)
        {
            using object = pdn::u8entity::object;
            auto current_date_time = date_time::current();
            auto obj = object{};
            obj[u8"year"_s]   = current_date_time.year;
            obj[u8"month"_s]  = current_date_time.month;
            obj[u8"day"_s]    = current_date_time.day;
            obj[u8"hour"_s]   = current_date_time.hour;
            obj[u8"minute"_s] = current_date_time.minute;
            obj[u8"second"_s] = current_date_time.second;
            return pdn::make_proxy<object>(::std::move(obj));
        }
        if (iden == u8"random"sv)
        {
            return rd.next();
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
parsing_time: @parsing_time
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
    const auto& dt = e[u8"parsing_time"sv];
    std::cout
        << "parsing_time:\n"
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
