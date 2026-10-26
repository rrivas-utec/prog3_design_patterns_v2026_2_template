#pragma once
#include "catch.hpp"
#include <future>
#include <chrono>
#include "cafeteria.h"

#include <sstream>
namespace cafe_test {
inline DrinkOrder order(Size s = Size::Small) {
    return Builder<DrinkOrder>{}.set(&DrinkOrder::customer_name,"Ana").set(&DrinkOrder::drink,"Latte").set(&DrinkOrder::size,s).build();
}
struct Legacy { std::vector<std::string> texts; void plain_old_format(std::string_view s) { texts.emplace_back(s); } };
}

#if TEST_CASE_INDEX == 1
TEST_CASE("Pregunta 1.1 - basico 1.00x") {

    REQUIRE_THROWS(Builder<DrinkOrder>{}.build());
    REQUIRE_THROWS(Builder<DrinkOrder>{}.set(&DrinkOrder::customer_name,"Ana").set(&DrinkOrder::drink,"Latte").build());
    REQUIRE_THROWS(Builder<DrinkOrder>{}.set(&DrinkOrder::size,Size::Small).set(&DrinkOrder::drink,"Latte").build());
    REQUIRE_THROWS(Builder<DrinkOrder>{}.set(&DrinkOrder::size,Size::Small).set(&DrinkOrder::customer_name,"Ana").build());
    auto b = Builder<DrinkOrder>{};
    b.set(&DrinkOrder::customer_name,"Ana").set(&DrinkOrder::drink,"Tea").set(&DrinkOrder::size,Size::Small);
    auto o = std::move(b).build();
    REQUIRE(o.customer_name == "Ana"); REQUIRE(o.drink == "Tea");
    REQUIRE(o.size == Size::Small); REQUIRE(o.extras.empty()); REQUIRE_FALSE(o.oat_milk);
    EventBus<OrderCreatedEvent>{}.publish({o});

}
#elif TEST_CASE_INDEX == 2
TEST_CASE("Pregunta 1.2 - intermedio 1.25x") {

    cafe_test::Legacy legacy; LegacyTicketPrinterAdapter printer{legacy};
    static_assert(TicketPrinter<decltype(printer)>);
    const std::vector<Size> sizes{Size::Small,Size::Medium,Size::Large};
    const std::vector<std::string> names{"small","medium","large"};
    for(std::size_t i=0;i<sizes.size();++i) {
        auto o = Builder<DrinkOrder>{}.set(&DrinkOrder::customer_name,"A B").set(&DrinkOrder::drink,"Tea").set(&DrinkOrder::size,sizes[i]).set(&DrinkOrder::oat_milk,true).set(&DrinkOrder::extras,std::vector<std::string>{"x","y"}).build();
        printer.print(o);
        REQUIRE(legacy.texts.back().find("Cliente: A B") != std::string::npos);
        REQUIRE(legacy.texts.back().find("Bebida: Tea") != std::string::npos);
        REQUIRE(legacy.texts.back().find(names[i]) != std::string::npos);
        REQUIRE(o.oat_milk); REQUIRE(o.extras == std::vector<std::string>{"x","y"});
    }
    EventBus<OrderCreatedEvent> stateful; std::vector<int> counts;
    stateful.subscribe([n=0,&counts](const auto&) mutable { counts.push_back(++n); });
    stateful.publish({cafe_test::order()}); stateful.publish({cafe_test::order()});
    REQUIRE(counts==std::vector<int>{1,2});
    std::ostringstream output; LegacyTicketPrinter real{output}; real.plain_old_format("hello");
    REQUIRE(output.str() == "[LEGACY-TICKET]\nhello\n");

}
#elif TEST_CASE_INDEX == 3
TEST_CASE("Pregunta 1.3 - avanzado 1.50x") {

    std::vector<std::string> trace;
    struct Printer { std::vector<std::string>& trace; bool fail=false; void print(const DrinkOrder& o) { trace.push_back(o.drink); if(fail) throw std::runtime_error("printer"); } };
    Printer p{trace}; EventBus<OrderCreatedEvent> bus;
    bus.subscribe([&](const auto& e){ trace.push_back(e.order.customer_name); });
    bus.subscribe([&](const auto& e){ trace.push_back(e.order.drink+" seen"); });
    OrderService service{p,bus}; auto o=cafe_test::order(); service.confirm(o);
    REQUIRE(trace == std::vector<std::string>{"Latte","Ana","Latte seen"});
    trace.clear(); p.fail=true; REQUIRE_THROWS_AS(service.confirm(o),std::runtime_error);
    REQUIRE(trace == std::vector<std::string>{"Latte"});
    p.fail=false; trace.clear(); service.confirm(o);
    REQUIRE(trace.size()==3);

}
#elif TEST_CASE_INDEX == 4
TEST_CASE("Pregunta 1.4 - intensivo 2.00x") {

    for(int repetition=0;repetition<7;++repetition) {
        cafe_test::Legacy legacy; LegacyTicketPrinterAdapter p{legacy}; EventBus<OrderCreatedEvent> bus;
        std::vector<std::string> actual, expected;
        for(int j=0;j<13;++j) bus.subscribe([&,j](const auto& e){ actual.push_back(std::to_string(j)+":"+e.order.customer_name); });
        OrderService service{p,bus};
        for(int i=0;i<137;++i) {
            auto o=cafe_test::order(); o.customer_name=std::to_string(i); o.extras.resize(i%11,"extra");
            for(int j=0;j<13;++j) expected.push_back(std::to_string(j)+":"+o.customer_name);
            service.confirm(o);
        }
        REQUIRE(actual==expected); REQUIRE(legacy.texts.size()==137);
    }

    std::vector<std::future<bool>> jobs;
    for(int worker=0;worker<4;++worker) jobs.push_back(std::async(std::launch::async,[] {
        cafe_test::Legacy legacy; LegacyTicketPrinterAdapter printer{legacy};
        EventBus<OrderCreatedEvent> bus; int seen=0;
        bus.subscribe([&](const auto&){++seen;}); OrderService service{printer,bus};
        for(int i=0;i<211;++i)service.confirm(cafe_test::order());
        return seen==211 && legacy.texts.size()==211;
    }));
    for(auto& job:jobs) { REQUIRE(job.wait_for(std::chrono::seconds(10))==std::future_status::ready); REQUIRE(job.get()); }

}
#else
#error "TEST_CASE_INDEX must be 1..4"
#endif
