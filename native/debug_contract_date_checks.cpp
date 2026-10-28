#include "debug_contract_date.hpp"
#include <chrono>
#include <stdexcept>

int main() {
    using cm::aot::extend_contract_date;
    const auto check = [](cm::calendar::Date date, unsigned year, int day) {
        const auto result = extend_contract_date(date);
        if (!result || result->year != year || result->day != day)
            throw std::runtime_error("Incorrect contract extension date");
    };
    check({0, 2001}, 2002, 0);
    check({58, 2003}, 2004, 58);
    check({59, 2003}, 2004, 60);
    check({58, 2004}, 2005, 58);
    check({59, 2004}, 2005, 58);
    check({60, 2004}, 2005, 59);
    check({365, 2004}, 2005, 364);
    check({59, 1899}, 1900, 59);
    check({59, 1999}, 2000, 60);
    check({59, 2099}, 2100, 59);
    if (extend_contract_date({-1, 2004}) || extend_contract_date({365, 2003}) ||
        extend_contract_date({0, 0}) || extend_contract_date({0, 0xffff}))
        throw std::runtime_error("Invalid date accepted");
    using namespace std::chrono;
    for (int year = 1601; year < 2400; ++year) {
        const sys_days start = sys_days{std::chrono::year{year}/January/1};
        const sys_days next_start = sys_days{std::chrono::year{year + 1}/January/1};
        for (int day = 0; day < (next_start - start).count(); ++day) {
            const year_month_day civil{start + days{day}};
            year_month_day anniversary{std::chrono::year{year + 1}, civil.month(), civil.day()};
            if (!anniversary.ok()) anniversary = std::chrono::year{year + 1}/February/28;
            check({std::int16_t(day), std::uint16_t(year)}, year + 1,
                  int((sys_days{anniversary} - next_start).count()));
        }
    }
}
