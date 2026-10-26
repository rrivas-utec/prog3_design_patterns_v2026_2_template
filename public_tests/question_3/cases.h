#pragma once
#include "catch.hpp"
#include <future>
#include <chrono>
#include "sensores_inteligentes.h"

#include <limits>
namespace sensor_test {
struct Sequence { int n=0; Reading read() { return {SensorType::Noise,double(n++),"R"}; } };
inline SensorConfig config() { return Builder<SensorConfig>{}.set(&SensorConfig::type,SensorType::Co2).set(&SensorConfig::room,"L-301").set(&SensorConfig::provider,"vendor-x").set(&SensorConfig::warning_threshold,900).set(&SensorConfig::critical_threshold,1200).set(&SensorConfig::sample_rate_seconds,5).build(); }
inline AlertLevel classify(const Reading& r,const SensorConfig& c) { if(r.value>=c.critical_threshold) return AlertLevel::Critical; if(r.value>=c.warning_threshold) return AlertLevel::Warning; return AlertLevel::Normal; }
}

#if TEST_CASE_INDEX == 1
TEST_CASE("Pregunta 3.1 - basico 1.00x") {

    auto c=sensor_test::config(); REQUIRE(c.type==SensorType::Co2); REQUIRE(c.room=="L-301"); REQUIRE(c.provider=="vendor-x"); REQUIRE(c.warning_threshold==900); REQUIRE(c.critical_threshold==1200); REQUIRE(c.sample_rate_seconds==5);
    AlertStrategy strategy=sensor_test::classify;
    for(double value: {0.,899.,900.,1199.,1200.,1300.}) {
        Reading r{c.type,value,c.room};
        auto expected=value>=1200?AlertLevel::Critical:value>=900?AlertLevel::Warning:AlertLevel::Normal;
        REQUIRE(strategy(r,c)==expected);
    }
    EventBus<AlertRaised>{}.publish({{SensorType::Noise,0,"R"},AlertLevel::Normal});

}
#elif TEST_CASE_INDEX == 2
TEST_CASE("Pregunta 3.2 - intermedio 1.25x") {

    VendorProbe vendor{1300}; VendorProbeAdapter adapted{vendor,SensorType::Co2,"L-301"};
    static_assert(Sensor<decltype(adapted)>);
    LoggingSensor decorated{LoggingSensor{adapted}};
    auto r=decorated.read(); REQUIRE(r.value==1300); REQUIRE(r.room=="L-301"); REQUIRE(r.type==SensorType::Co2);
    auto f=make_sensor<sensor_test::Sequence>(); auto g=make_sensor<sensor_test::Sequence>();
    REQUIRE(f().value==0); REQUIRE(f().value==1); REQUIRE(g().value==0); REQUIRE(f().value==2);
    auto factory=make_sensor<decltype(decorated)>(decorated); REQUIRE(factory().value==1300);

}
#elif TEST_CASE_INDEX == 3
TEST_CASE("Pregunta 3.3 - avanzado 1.50x") {

    std::vector<std::string> trace;
    struct Probe { std::vector<std::string>* trace; bool fail=false; Reading read() { trace->push_back("read"); if(fail) throw std::runtime_error("probe"); return {SensorType::Co2,1300,"L-301"}; } };
    LoggingSensor sensor{Probe{&trace}}; EventBus<AlertRaised> bus;
    bus.subscribe([&](const auto& e){ trace.push_back(e.reading.room); });
    bus.subscribe([&](const auto& e){ trace.push_back(e.level==AlertLevel::Critical?"critical":"other"); });
    AlertStrategy strategy=[&](const Reading& r,const SensorConfig& c){trace.push_back("classify");return sensor_test::classify(r,c);};
    auto monitor=[&] {auto r=sensor.read();auto level=strategy(r,sensor_test::config());if(level!=AlertLevel::Normal)bus.publish({r,level});};
    monitor(); REQUIRE(trace==std::vector<std::string>{"read","classify","L-301","critical"});
    sensor.inner.fail=true; trace.clear(); REQUIRE_THROWS_AS(monitor(),std::runtime_error); REQUIRE(trace==std::vector<std::string>{"read"});
    sensor.inner.fail=false; trace.clear(); strategy=[](const auto&,const auto&){return AlertLevel::Normal;}; monitor(); REQUIRE(trace==std::vector<std::string>{"read"});

}
#elif TEST_CASE_INDEX == 4
TEST_CASE("Pregunta 3.4 - intensivo 2.00x") {

    for(int rep=0;rep<7;++rep) {
        auto f=make_sensor<LoggingSensor<sensor_test::Sequence>>(sensor_test::Sequence{});
        EventBus<AlertRaised> bus; std::vector<double> actual,expected; int count=0;
        bus.subscribe([&](const auto& e){actual.push_back(e.reading.value);});
        bus.subscribe([&](const auto&){++count;});
        auto c=sensor_test::config(); c.warning_threshold=73+rep; c.critical_threshold=311+rep;
        for(int i=0;i<1531;++i) {
            auto r=f(); REQUIRE(r.value==i); REQUIRE(r.room=="R"); REQUIRE(r.type==SensorType::Noise);
            auto level=sensor_test::classify(r,c);
            if(i>=73+rep)expected.push_back(i);
            if(level!=AlertLevel::Normal)bus.publish({r,level});
        }
        REQUIRE(actual==expected); REQUIRE(count==static_cast<int>(expected.size()));
    }

    std::vector<std::future<bool>> jobs;
    for(int worker=0;worker<4;++worker) jobs.push_back(std::async(std::launch::async,[] {
        auto f=make_sensor<sensor_test::Sequence>(); EventBus<AlertRaised> bus;
        double sum=0; bus.subscribe([&](const auto& e){sum+=e.reading.value;});
        for(int i=0;i<613;++i)bus.publish({f(),AlertLevel::Warning});
        return sum==612.*613./2.;
    }));
    for(auto& job:jobs) { REQUIRE(job.wait_for(std::chrono::seconds(10))==std::future_status::ready); REQUIRE(job.get()); }

}
#else
#error "TEST_CASE_INDEX must be 1..4"
#endif
