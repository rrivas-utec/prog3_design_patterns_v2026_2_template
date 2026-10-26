#pragma once
#include "catch.hpp"
#include <future>
#include <chrono>
#include "gastos_personales.h"

#include <array>
#include <numeric>
namespace expense_test {
struct Custom { std::string prefix; std::string export_expenses(const ExpenseList& x) const { return prefix+std::to_string(x.size()); } };
struct Broken { std::string export_expenses(const ExpenseList&) const { throw std::runtime_error("export"); } };
}

#if TEST_CASE_INDEX == 1
TEST_CASE("Pregunta 2.1 - basico 1.00x") {

    static_assert(ExpenseExporter<CsvExporter>);
    ExpenseList empty; sort_with(empty,[](const auto& a,const auto& b){return a.amount<b.amount;});
    auto f=make_exporter<CsvExporter>(); REQUIRE(f(empty).find("date,category,detail,amount")!=std::string::npos);
    ExpenseList one{{"2026-06-01","food","lunch",18.5}};
    auto text=f(one); REQUIRE(text.find("food")!=std::string::npos); REQUIRE(text.find("lunch")!=std::string::npos);
    REQUIRE(one.front().amount==18.5);
    auto custom=make_exporter<expense_test::Custom>("rows:"); REQUIRE(custom(one)=="rows:1");

}
#elif TEST_CASE_INDEX == 2
TEST_CASE("Pregunta 2.2 - intermedio 1.25x") {

    ExpenseList x{{"03","food","c",5},{"01","books","a",-3},{"02","food","b",0},{"04","books","d",5}};
    sort_with(x,[](const auto& a,const auto& b){return a.date<b.date;});
    REQUIRE(x.front().date=="01"); REQUIRE(x.back().date=="04");
    sort_with(x,[](const auto& a,const auto& b){return a.amount>b.amount;});
    for(std::size_t i=1;i<x.size();++i) REQUIRE(x[i-1].amount>=x[i].amount);
    sort_with(x,[](const auto& a,const auto& b){return std::pair{a.category,a.detail}<std::pair{b.category,b.detail};});
    REQUIRE(x.front().detail=="a"); REQUIRE(x.back().detail=="c");
    std::array<int,5> a{7,2,2,-1,0}; sort_with(a,std::less<>{}); REQUIRE(a==std::array<int,5>{-1,0,2,2,7});
    std::array<int,6> partition{99,4,1,3,2,88};
    auto middle=std::ranges::subrange(partition.begin()+1,partition.end()-1);
    sort_with(middle,std::less<>{});
    REQUIRE(partition==std::array<int,6>{99,1,2,3,4,88});
    AuditedExporter audited{expense_test::Custom{"payload:"}};
    SummaryExporter summary{expense_test::Custom{"payload:"}};
    REQUIRE(audited.export_expenses(x).find("payload:4")!=std::string::npos);
    REQUIRE(summary.export_expenses(x).find("payload:4")!=std::string::npos);
    REQUIRE(summary.export_expenses(x).find("total")!=std::string::npos);

}
#elif TEST_CASE_INDEX == 3
TEST_CASE("Pregunta 2.3 - avanzado 1.50x") {

    std::vector<int> trace;
    struct Probe { std::vector<int>* trace; std::string export_expenses(const ExpenseList&) const { trace->push_back(1); return "body\n"; } };
    ExpenseList x{{"a","b","c",2},{"d","e","f",3}};
    AuditedExporter a{SummaryExporter{Probe{&trace}}};
    auto text=a.export_expenses(x); REQUIRE(trace==std::vector<int>{1}); REQUIRE(text.find("body")!=std::string::npos); REQUIRE(text.find("total")!=std::string::npos);
    SummaryExporter b{AuditedExporter{Probe{&trace}}};
    auto other=b.export_expenses(x); REQUIRE(trace==std::vector<int>{1,1}); REQUIRE(other.find("body")!=std::string::npos);
    AuditedExporter broken{SummaryExporter{expense_test::Broken{}}}; REQUIRE_THROWS_AS(broken.export_expenses(x),std::runtime_error);
    auto f=make_exporter<expense_test::Broken>(); REQUIRE_THROWS_AS(f(x),std::runtime_error);
    REQUIRE(x.size()==2);

}
#elif TEST_CASE_INDEX == 4
TEST_CASE("Pregunta 2.4 - intensivo 2.00x") {

    for(int rep=0;rep<9;++rep) {
        ExpenseList x; std::vector<double> oracle;
        for(int i=0;i<2003;++i) { double v=((i*97+rep)%701)-350; x.push_back({std::to_string(i),"cat",std::string(i%19,'x'),v}); oracle.push_back(v); }
        std::sort(oracle.begin(),oracle.end());
        sort_with(x,[](const auto& a,const auto& b){return a.amount<b.amount;});
        for(std::size_t i=0;i<x.size();++i) REQUIRE(x[i].amount==oracle[i]);
        auto f=make_exporter<AuditedExporter<SummaryExporter<CsvExporter>>>(SummaryExporter{CsvExporter{}});
        auto text=f(x); REQUIRE(text.find("total")!=std::string::npos);
        for(const auto& e:x) REQUIRE(text.find(e.date)!=std::string::npos);
        REQUIRE(f(x)==text);
    }

    std::vector<std::future<bool>> jobs;
    for(int worker=0;worker<4;++worker) jobs.push_back(std::async(std::launch::async,[worker] {
        ExpenseList x; for(int i=0;i<317;++i)x.push_back({"d","c","x",double((i*71+worker)%401)});
        sort_with(x,[](const auto& a,const auto& b){return a.amount<b.amount;});
        auto f=make_exporter<CsvExporter>();
        return std::is_sorted(x.begin(),x.end(),[](const auto& a,const auto& b){return a.amount<b.amount;}) && f(x)==f(x);
    }));
    for(auto& job:jobs) { REQUIRE(job.wait_for(std::chrono::seconds(10))==std::future_status::ready); REQUIRE(job.get()); }

}
#else
#error "TEST_CASE_INDEX must be 1..4"
#endif
