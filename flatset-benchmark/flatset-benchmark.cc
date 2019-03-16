
#define stdext stdext1
 #include "flatset-sorted.h"
#undef stdext
#undef STDEXT_HAS_SORTED_UNIQUE
#define stdext stdext2
 #include "flatset-levelorder-faro.h"
#undef stdext
#undef STDEXT_HAS_SORTED_UNIQUE
#define stdext stdext3
 #include "flatset-levelorder-permute.h"
#undef stdext
#undef STDEXT_HAS_SORTED_UNIQUE
#define stdext stdext4
 #include "flatset-unsorted.h"
#undef stdext
#undef STDEXT_HAS_SORTED_UNIQUE

#include <benchmark/benchmark.h>
#include <boost/container/flat_set.hpp>
#include <random>
#include <vector>

template<class FS>
static void single_insertion(benchmark::State& state)
{
    const int N = state.range(0);
    std::mt19937 g;
    FS fs;
    std::vector<int> data(N);
    for (int& elt : data) elt = g();

    for (auto _ : state) {
        state.PauseTiming();
        fs.clear();
        state.ResumeTiming();
        for (const int& v : data) {
            fs.insert(v);
        }
        benchmark::DoNotOptimize(fs);
    }
}

template<class FS>
static void range_insertion(benchmark::State& state)
{
    const int N = state.range(0);
    std::mt19937 g;
    FS fs;
    std::vector<int> data(N);
    for (int& elt : data) elt = g();

    for (auto _ : state) {
        state.PauseTiming();
        fs.clear();
        state.ResumeTiming();
        fs.insert(data.begin(), data.end());
    }
}

template<class FS>
static void single_deletion(benchmark::State& state)
{
    const int N = state.range(0);
    std::mt19937 g;
    FS fs;
    std::vector<int> data(N);
    for (int& elt : data) elt = g();

    for (auto _ : state) {
        state.PauseTiming();
        fs.insert(data.begin(), data.end());
        state.ResumeTiming();
        for (const int& v : data) {
            auto count = fs.erase(v);
            benchmark::DoNotOptimize(count);
        }
    }
}

template<class FS>
static void searching(benchmark::State& state)
{
    const int N = state.range(0);
    std::mt19937 g;
    std::vector<int> data(N);
    for (int& elt : data) elt = g();
    FS fs(data.begin(), data.end());

    for (auto _ : state) {
        for (const int& v : data) {
            auto it = fs.find(v);
            benchmark::DoNotOptimize(it);
        }
    }
}

template<class FS>
static void iteration(benchmark::State& state)
{
    const int N = state.range(0);
    std::mt19937 g;
    std::vector<int> data(N);
    for (int& elt : data) elt = g();
    FS fs(data.begin(), data.end());

    for (auto _ : state) {
        unsigned result = 0;
        benchmark::DoNotOptimize(result);
        for (const int& v : fs) {
            result += v;
        }
        benchmark::DoNotOptimize(result);
    }
}

using Set = std::set<int>;
using FSSorted = stdext1::flat_set<int>;
using FSFaro = stdext2::flat_set<int>;
using FSPermute = stdext3::flat_set<int>;
using FSUnsorted = stdext4::flat_set<int>;
using FSBoost = boost::container::flat_set<int>;

#if 1
BENCHMARK_TEMPLATE(single_insertion, Set)->Arg(10'000);
BENCHMARK_TEMPLATE(range_insertion, Set)->Arg(10'000);
BENCHMARK_TEMPLATE(single_deletion, Set)->Arg(10'000);
#endif
BENCHMARK_TEMPLATE(searching, Set)->Arg(100'000);
BENCHMARK_TEMPLATE(iteration, Set)->Arg(100'000);
#if 1
BENCHMARK_TEMPLATE(single_insertion, FSUnsorted)->Arg(10'000);
BENCHMARK_TEMPLATE(range_insertion, FSUnsorted)->Arg(10'000);
BENCHMARK_TEMPLATE(single_deletion, FSUnsorted)->Arg(10'000);
#endif
BENCHMARK_TEMPLATE(searching, FSUnsorted)->Arg(100'000);
BENCHMARK_TEMPLATE(iteration, FSUnsorted)->Arg(100'000);
#if 1
BENCHMARK_TEMPLATE(single_insertion, FSSorted)->Arg(10'000);
BENCHMARK_TEMPLATE(range_insertion, FSSorted)->Arg(10'000);
BENCHMARK_TEMPLATE(single_deletion, FSSorted)->Arg(10'000);
#endif
BENCHMARK_TEMPLATE(searching, FSSorted)->Arg(100'000);
BENCHMARK_TEMPLATE(iteration, FSSorted)->Arg(100'000);
#if 1
BENCHMARK_TEMPLATE(single_insertion, FSBoost)->Arg(10'000);
BENCHMARK_TEMPLATE(range_insertion, FSBoost)->Arg(10'000);
BENCHMARK_TEMPLATE(single_deletion, FSBoost)->Arg(10'000);
#endif
BENCHMARK_TEMPLATE(searching, FSBoost)->Arg(100'000);
BENCHMARK_TEMPLATE(iteration, FSBoost)->Arg(100'000);
#if 1
BENCHMARK_TEMPLATE(single_insertion, FSFaro)->Arg(10'000);
BENCHMARK_TEMPLATE(range_insertion, FSFaro)->Arg(10'000);
BENCHMARK_TEMPLATE(single_deletion, FSFaro)->Arg(10'000);
#endif
BENCHMARK_TEMPLATE(searching, FSFaro)->Arg(100'000);
BENCHMARK_TEMPLATE(iteration, FSFaro)->Arg(100'000);
#if 1
BENCHMARK_TEMPLATE(single_insertion, FSPermute)->Arg(10'000);
BENCHMARK_TEMPLATE(range_insertion, FSPermute)->Arg(10'000);
BENCHMARK_TEMPLATE(single_deletion, FSPermute)->Arg(10'000);
#endif
BENCHMARK_TEMPLATE(searching, FSPermute)->Arg(100'000);
BENCHMARK_TEMPLATE(iteration, FSPermute)->Arg(100'000);

BENCHMARK_MAIN();
