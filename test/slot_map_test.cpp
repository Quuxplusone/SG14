#include <sg14/slot_map.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <cinttypes>
#include <deque>
#include <forward_list>
#include <iterator>
#include <list>
#include <memory>
#include <random>
#include <type_traits>
#include <utility>

namespace {
namespace TestKey {
struct key_16_8_t {
    uint16_t index;
    uint8_t generation;
};
struct key_11_5_t {  // C++17 only
    uint16_t index : 11;
    uint8_t generation : 5;
};

#if __cplusplus < 201703L
template<int I, class K> auto get(const K& k) { return get(k, std::integral_constant<int, I>{}); }
template<int I, class K> auto& get(K& k) { return get(k, std::integral_constant<int, I>{}); }

const uint16_t& get(const key_16_8_t& k, std::integral_constant<int, 0>) { return k.index; }
const uint8_t& get(const key_16_8_t& k, std::integral_constant<int, 1>) { return k.generation; }
uint16_t& get(key_16_8_t& k, std::integral_constant<int, 0>) { return k.index; }
uint8_t& get(key_16_8_t& k, std::integral_constant<int, 1>) { return k.generation; }
#endif // __cplusplus < 201703L
} // namespace TestKey

namespace TestContainer {

template<class T>
struct Vector {
    using value_type = T;
    using reference = T&;
    using const_reference = const T&;
    using pointer = T*;
    using const_pointer = const T*;
    using size_type = unsigned;
    using iterator = T*;
    using const_iterator = const T*;
    using reverse_iterator = std::reverse_iterator<T*>;
    using const_reverse_iterator = std::reverse_iterator<const T*>;

    Vector() = default;
    template<class T_ = T, class = std::enable_if_t<std::is_copy_constructible<T_>::value>>
    Vector(const Vector& rhs) { *this = rhs; }
    Vector(Vector&& rhs) { *this = std::move(rhs); }
    template<class T_ = T, class = std::enable_if_t<std::is_copy_constructible<T_>::value>>
    void operator=(const Vector& rhs) {
        size_ = rhs.size_;
        data_ = std::make_unique<T[]>(size_);
        std::copy(rhs.begin(), rhs.end(), data_.get());
    }
    void operator=(Vector&& rhs) {
        size_ = rhs.size_;
        data_ = std::move(rhs.data_);
    }
    unsigned size() const { return static_cast<unsigned int>(size_); }
    T *begin() { return data_.get(); }
    T *end() { return data_.get() + size_; }
    const T *begin() const { return data_.get(); }
    const T *end() const { return data_.get() + size_; }
    void pop_back() {
        auto p = std::make_unique<T[]>(size_ - 1);
        std::move(begin(), end() - 1, p.get());
        size_ -= 1;
        data_ = std::move(p);
    }
    template<class U, class = std::enable_if_t<std::is_same<U, T>::value>>
    void emplace_back(U t) {
        auto p = std::make_unique<T[]>(size_ + 1);
        std::move(begin(), end(), p.get());
        size_ += 1;
        data_ = std::move(p);
        *(end() - 1) = std::move(t);
    }
    void clear() {
        size_ = 0;
        data_ = nullptr;
    }
    friend void swap(Vector& a, Vector& b) {
        std::swap(a.size_, b.size_);
        std::swap(a.data_, b.data_);
    }
private:
    size_t size_ = 0;
    std::unique_ptr<T[]> data_ = nullptr;
};

} // namespace TestContainer

template<class T_>
struct Monad {
    using T = T_;
    static T value_of(const T& i) { return i; }
    template<class U> static T from_value(const U& v) { return static_cast<T>(v); }
};
template<class T_>
struct Monad<std::unique_ptr<T_>> {
    using T = std::unique_ptr<T_>;
    static T_ value_of(const T& ptr) { return *ptr; }
    template<class U> static T from_value(const U& v) { return std::make_unique<T_>(v); }
};

} // namespace

template<class T, class Key, template<class...> class Container>
static void print_slot_map(const sg14::slot_map<T, Key, Container>& sm)
{
    printf("%d slots:", (int)sm.slots_.size());
    for (auto&& slot : sm.slots_) {
        printf(" %d/%d", (int)slot.first, (int)slot.second);
    }
    printf("\n%d values:", (int)sm.values_.size());
    for (auto&& value : sm) {
        printf(" %d", (int)value);
    }
    EXPECT_EQ(sm.reverse_map_.size(), sm.size());
    printf("\n%d reverse_map:", (int)sm.reverse_map_.size());
    for (auto&& idx : sm.reverse_map_) {
        printf(" %d", (int)idx);
    }
    printf("\nnext_available_slot_index: %d\n", (int)sm.next_available_slot_index_);
}

template<class T, class U = std::conditional_t<std::is_copy_constructible<T>::value, const T&, T&&>>
static U move_if_necessary(T& value) { return static_cast<U>(value); }

template<class SM, class T>
static void BasicTests(T t1, T t2)
{
    SM sm;
    EXPECT_TRUE(sm.empty());
    EXPECT_EQ(sm.size(), 0u);
    SM sm2 = move_if_necessary(sm);
    EXPECT_TRUE(sm2.empty());
    EXPECT_EQ(sm2.size(), 0u);
    auto k1 = sm.insert(std::move(t1));
    auto k2 = sm.insert(move_if_necessary(t2));
    EXPECT_FALSE(sm.empty());
    EXPECT_EQ(sm.size(), 2u);
    EXPECT_EQ(std::next(sm.begin(), 2), sm.end());
    EXPECT_EQ(sm.find(k1), sm.begin());
    EXPECT_EQ(sm.find(k2), std::next(sm.begin()));
    EXPECT_TRUE(sm2.empty());
    EXPECT_EQ(sm2.size(), 0u);
    auto num_removed = sm.erase(k1);
    EXPECT_EQ(num_removed, 1u);
    EXPECT_EQ(sm.size(), 1u);
    EXPECT_EQ(sm.find(k1), sm.end());  // find an expired key
    ASSERT_THROW(sm.at(k1), std::out_of_range);
    EXPECT_EQ(sm.find(k2), sm.begin());  // find a non-expired key
    EXPECT_EQ(sm.at(k2), *sm.begin());
    EXPECT_EQ(sm.find_unchecked(k2), sm.begin());
    EXPECT_EQ(sm[k2], *sm.begin());
    EXPECT_EQ(sm.erase(k1), 0u);  // erase an expired key
    sm.swap(sm2);
    EXPECT_TRUE(sm.empty());
    EXPECT_EQ(sm2.size(), 1u);
    EXPECT_EQ(sm2.find(k1), sm2.end());  // find an expired key
    EXPECT_EQ(sm2.find(k2), sm2.begin());  // find a non-expired key
    EXPECT_EQ(sm2.erase(k1), 0u);  // erase an expired key
}

template<class SM, class TGen>
static void FullContainerStressTest(TGen t)
{
    const int total = 1000;
    SM sm;
    std::vector<typename SM::key_type> keys;
    for (int i=0; i < total; ++i) {
        auto k = sm.insert(t());
        keys.push_back(k);
    }
    EXPECT_TRUE(sm.size() == total);
    std::mt19937 g;
    std::shuffle(keys.begin(), keys.end(), g);
    for (int i = 0; i < total; ++i) {
        EXPECT_TRUE(sm.size() == static_cast<typename SM::size_type>(total - i));
        EXPECT_TRUE(sm.find(keys[i]) != sm.end());
        EXPECT_TRUE(sm.find_unchecked(keys[i]) != sm.end());
        for (int j = std::max(0, i - 10); j < i; ++j) {
            EXPECT_TRUE(sm.find(keys[j]) == sm.end());
        }
        auto erased = sm.erase(keys[i]);
        EXPECT_TRUE(erased == 1);
    }
    EXPECT_TRUE(sm.empty());
}

template<class SM, class TGen>
static void InsertEraseStressTest(TGen t)
{
    const int total = 1000;
    SM sm;
    std::vector<typename SM::key_type> valid_keys;
    std::vector<typename SM::key_type> expired_keys;
    std::mt19937 g;
    for (int i=0; i < total / 3; ++i) {
        auto k = sm.insert(t());
        valid_keys.push_back(k);
    }
    std::shuffle(valid_keys.begin(), valid_keys.end(), g);
    for (int i = total / 3; i < total; ++i) {
        if (g() % 2 == 0 && !valid_keys.empty()) {
            auto k = valid_keys.back();
            valid_keys.pop_back();
            auto erased = sm.erase(k);
            EXPECT_TRUE(erased == 1);
            expired_keys.push_back(k);
            for (auto&& ek : expired_keys) {
                EXPECT_TRUE(sm.find(ek) == sm.end());
            }
        } else {
            auto k = sm.insert(t());
            size_t random_index = g() % (valid_keys.size() + 1);
            valid_keys.insert(valid_keys.begin() + random_index, k);
        }
    }
}

template<class SM>
static void EraseInLoopTest()
{
    using T = typename SM::mapped_type;
    SM sm;
    for (int i=0; i < 100; ++i) {
        sm.insert(Monad<T>::from_value(i));
    }
    int total = 0;
    for (auto it = sm.begin(); it != sm.end(); /*nothing*/) {
        total += Monad<T>::value_of(*it);
        if (Monad<T>::value_of(*it) > 50) {
            it = sm.erase(it);
        } else {
            ++it;
        }
    }
    EXPECT_TRUE(total == 4950);
    int total2 = 0;
    for (auto&& elt : sm) {
        total2 += Monad<T>::value_of(elt);
    }
    EXPECT_TRUE(total2 == 1275);
}

template<class SM>
static void EraseRangeTest()
{
    using T = typename SM::mapped_type;
    SM sm;
    auto test = [&](int N, int first, int last) {
        sm.erase(sm.begin(), sm.end());
        int expected_total = 0;
        for (int i=0; i < N; ++i) {
            expected_total += i;
            sm.insert(Monad<T>::from_value(i));
        }
        for (auto it = std::next(sm.begin(), first); it != std::next(sm.begin(), last); ++it) {
            expected_total -= Monad<T>::value_of(*it);
        }
        sm.erase(std::next(sm.begin(), first), std::next(sm.begin(), last));
        int actual_total = 0;
        for (auto it = sm.begin(); it != sm.end(); ++it) {
            actual_total += Monad<T>::value_of(*it);
        }
        return (actual_total == expected_total);
    };
    EXPECT_TRUE(test(10, 8, 8));
    EXPECT_TRUE(test(10, 3, 7));
    EXPECT_TRUE(test(10, 0, 10));
    EXPECT_TRUE(test(10, 1, 10));
    EXPECT_TRUE(test(10, 0, 9));
    EXPECT_TRUE(test(10, 1, 9));
    for (int N : { 2, 10, 50 }) {
        for (int i=0; i < N; ++i) {
            for (int j=i; j < N; ++j) {
                EXPECT_TRUE(test(N, i, j));
            }
        }
    }
}

template<class SM>
static void PartitionTest()
{
    using T = typename SM::mapped_type;
    SM sm;
    auto key3 = sm.insert(Monad<T>::from_value(3));
    auto key1 = sm.insert(Monad<T>::from_value(1));
    auto key4 = sm.insert(Monad<T>::from_value(4));
    auto key5 = sm.insert(Monad<T>::from_value(5));
    auto key9 = sm.insert(Monad<T>::from_value(9));
    auto key2 = sm.insert(Monad<T>::from_value(2));
    auto key6 = sm.insert(Monad<T>::from_value(6));

    auto pivot = sm.partition([](const auto& elt) {
        return Monad<T>::value_of(elt) >= 5;
    });

    for (auto it = sm.begin(); it != pivot; ++it) {
        EXPECT_TRUE(Monad<T>::value_of(*it) >= 5);
    }
    for (auto it = pivot; it != sm.end(); ++it) {
        EXPECT_TRUE(Monad<T>::value_of(*it) < 5);
    }

    EXPECT_TRUE(Monad<T>::value_of(*sm.find(key3)) == 3);
    EXPECT_TRUE(Monad<T>::value_of(*sm.find(key1)) == 1);
    EXPECT_TRUE(Monad<T>::value_of(*sm.find(key4)) == 4);
    EXPECT_TRUE(Monad<T>::value_of(*sm.find(key5)) == 5);
    EXPECT_TRUE(Monad<T>::value_of(*sm.find(key9)) == 9);
    EXPECT_TRUE(Monad<T>::value_of(*sm.find(key2)) == 2);
    EXPECT_TRUE(Monad<T>::value_of(*sm.find(key6)) == 6);
}

template<class SM>
static void ReserveTest()
{
    using T = typename SM::mapped_type;
    SM sm;
    auto k = sm.emplace(Monad<T>::from_value(1));
    (void)k;
    EXPECT_TRUE(sm.size() == 1);

    auto original_cap = sm.slot_count();
    static_assert(std::is_same<decltype(original_cap), typename SM::size_type>::value, "");
    EXPECT_TRUE(original_cap >= 1);

    sm.reserve_slots(original_cap + 3);
    EXPECT_TRUE(sm.slot_count() >= original_cap + 3);
    EXPECT_TRUE(sm.size() == 1);

    sm.emplace(Monad<T>::from_value(2));
    sm.emplace(Monad<T>::from_value(3));
    sm.emplace(Monad<T>::from_value(4));
    EXPECT_TRUE(sm.size() == 4);
}

template<class SM, class = decltype(std::declval<const SM&>().capacity())>
static void VerifyCapacityExists(bool expected)
{
    EXPECT_TRUE(expected);
    SM sm;
    auto n = sm.capacity();
    static_assert(std::is_same<decltype(n), typename SM::size_type>::value, "");
    EXPECT_TRUE(n == 0);
    sm.reserve(100);
    EXPECT_TRUE(sm.capacity() >= 100);
    EXPECT_TRUE(sm.slot_count() >= 100);
}

template<class SM, class Bool>
void VerifyCapacityExists(Bool expected)
{
    EXPECT_TRUE(!expected);
    SM sm;
    sm.reserve(100);
    EXPECT_TRUE(sm.slot_count() >= 100);
}

TEST(slot_map, MemberTypedefs)
{
    if (true) {
        using SM = sg14::slot_map<int>;
        static_assert(std::is_same<typename SM::key_type, std::pair<unsigned, unsigned>>::value, "");
        static_assert(std::is_same<typename SM::mapped_type, int>::value, "");
        static_assert(std::is_same<typename SM::key_index_type, unsigned>::value, "");
        static_assert(std::is_same<typename SM::key_generation_type, unsigned>::value, "");
        static_assert(std::is_same<typename SM::container_type, std::vector<int>>::value, "");
        static_assert(std::is_same<typename SM::reference, int&>::value, "");
        static_assert(std::is_same<typename SM::const_reference, const int&>::value, "");
        static_assert(std::is_same<typename SM::pointer, int*>::value, "");
        static_assert(std::is_same<typename SM::const_pointer, const int*>::value, "");
        static_assert(std::is_same<typename SM::iterator, std::vector<int>::iterator>::value, "");
        static_assert(std::is_same<typename SM::const_iterator, std::vector<int>::const_iterator>::value, "");
        static_assert(std::is_same<typename SM::reverse_iterator, std::vector<int>::reverse_iterator>::value, "");
        static_assert(std::is_same<typename SM::const_reverse_iterator, std::vector<int>::const_reverse_iterator>::value, "");
        static_assert(std::is_same<typename SM::size_type, std::vector<int>::size_type>::value, "");
        static_assert(std::is_same<typename SM::value_type, int>::value, "");
    }
    if (true) {
        using SM = sg14::slot_map<bool>;
        static_assert(std::is_same<typename SM::key_type, std::pair<unsigned, unsigned>>::value, "");
        static_assert(std::is_same<typename SM::mapped_type, bool>::value, "");
        static_assert(std::is_same<typename SM::key_index_type, unsigned>::value, "");
        static_assert(std::is_same<typename SM::key_generation_type, unsigned>::value, "");
        static_assert(std::is_same<typename SM::container_type, std::vector<bool>>::value, "");
        static_assert(std::is_same<typename SM::reference, std::vector<bool>::reference>::value, "");
        static_assert(std::is_same<typename SM::const_reference, std::vector<bool>::const_reference>::value, "");
        static_assert(std::is_same<typename SM::pointer, std::vector<bool>::pointer>::value, "");
        static_assert(std::is_same<typename SM::const_pointer, std::vector<bool>::const_pointer>::value, "");
        static_assert(std::is_same<typename SM::iterator, std::vector<bool>::iterator>::value, "");
        static_assert(std::is_same<typename SM::const_iterator, std::vector<bool>::const_iterator>::value, "");
        static_assert(std::is_same<typename SM::reverse_iterator, std::vector<bool>::reverse_iterator>::value, "");
        static_assert(std::is_same<typename SM::const_reverse_iterator, std::vector<bool>::const_reverse_iterator>::value, "");
        static_assert(std::is_same<typename SM::size_type, std::vector<bool>::size_type>::value, "");
        static_assert(std::is_same<typename SM::value_type, bool>::value, "");
    }
    if (true) {
        using SM = sg14::slot_map<double, TestKey::key_16_8_t>;
        static_assert(std::is_same<typename SM::key_type, TestKey::key_16_8_t>::value, "");
        static_assert(std::is_same<typename SM::mapped_type, double>::value, "");
        static_assert(std::is_same<typename SM::key_index_type, uint16_t>::value, "");
        static_assert(std::is_same<typename SM::key_generation_type, uint8_t>::value, "");
        static_assert(std::is_same<typename SM::container_type, std::vector<double>>::value, "");
        static_assert(std::is_same<typename SM::reference, double&>::value, "");
        static_assert(std::is_same<typename SM::const_reference, const double&>::value, "");
        static_assert(std::is_same<typename SM::pointer, double*>::value, "");
        static_assert(std::is_same<typename SM::const_pointer, const double*>::value, "");
        static_assert(std::is_same<typename SM::iterator, std::vector<double>::iterator>::value, "");
        static_assert(std::is_same<typename SM::const_iterator, std::vector<double>::const_iterator>::value, "");
        static_assert(std::is_same<typename SM::reverse_iterator, std::vector<double>::reverse_iterator>::value, "");
        static_assert(std::is_same<typename SM::const_reverse_iterator, std::vector<double>::const_reverse_iterator>::value, "");
        static_assert(std::is_same<typename SM::size_type, std::vector<double>::size_type>::value, "");
        static_assert(std::is_same<typename SM::value_type, double>::value, "");
    }
    if (true) {
        using SM = sg14::slot_map<int, std::pair<char, int>, TestContainer::Vector>;
        static_assert(std::is_same<typename SM::key_type, std::pair<char, int>>::value, "");
        static_assert(std::is_same<typename SM::mapped_type, int>::value, "");
        static_assert(std::is_same<typename SM::key_index_type, char>::value, "");
        static_assert(std::is_same<typename SM::key_generation_type, int>::value, "");
        static_assert(std::is_same<typename SM::container_type, TestContainer::Vector<int>>::value, "");
        static_assert(std::is_same<typename SM::reference, int&>::value, "");
        static_assert(std::is_same<typename SM::const_reference, const int&>::value, "");
        static_assert(std::is_same<typename SM::pointer, int*>::value, "");
        static_assert(std::is_same<typename SM::const_pointer, const int*>::value, "");
        static_assert(std::is_same<typename SM::iterator, TestContainer::Vector<int>::iterator>::value, "");
        static_assert(std::is_same<typename SM::const_iterator, TestContainer::Vector<int>::const_iterator>::value, "");
        static_assert(std::is_same<typename SM::reverse_iterator, TestContainer::Vector<int>::reverse_iterator>::value, "");
        static_assert(std::is_same<typename SM::const_reverse_iterator, TestContainer::Vector<int>::const_reverse_iterator>::value, "");
        static_assert(std::is_same<typename SM::size_type, unsigned>::value, "");
        static_assert(std::is_same<typename SM::value_type, int>::value, "");
    }
#if __cplusplus >= 201703L
    if (true) {
        using SM = sg14::slot_map<double, TestKey::key_11_5_t>;
        static_assert(std::is_same<typename SM::key_type, TestKey::key_11_5_t>::value, "");
        static_assert(std::is_same<typename SM::mapped_type, double>::value, "");
        static_assert(std::is_same<typename SM::key_index_type, uint16_t>::value, "");
        static_assert(std::is_same<typename SM::key_generation_type, uint8_t>::value, "");
        static_assert(std::is_same<typename SM::container_type, std::vector<double>>::value, "");
        static_assert(std::is_same<typename SM::reference, double&>::value, "");
        static_assert(std::is_same<typename SM::const_reference, const double&>::value, "");
        static_assert(std::is_same<typename SM::pointer, double*>::value, "");
        static_assert(std::is_same<typename SM::const_pointer, const double*>::value, "");
        static_assert(std::is_same<typename SM::iterator, std::vector<double>::iterator>::value, "");
        static_assert(std::is_same<typename SM::const_iterator, std::vector<double>::const_iterator>::value, "");
        static_assert(std::is_same<typename SM::reverse_iterator, std::vector<double>::reverse_iterator>::value, "");
        static_assert(std::is_same<typename SM::const_reverse_iterator, std::vector<double>::const_reverse_iterator>::value, "");
        static_assert(std::is_same<typename SM::size_type, std::vector<double>::size_type>::value, "");
        static_assert(std::is_same<typename SM::value_type, double>::value, "");
    }
#endif // __cplusplus >= 201703L
}

template<class SM>
void BoundsCheckingTest()
{
    using T = typename SM::mapped_type;
    SM sm;
    const auto& csm = sm;

    sm.emplace(Monad<T>::from_value(1));
    typename SM::key_type k = sm.emplace(Monad<T>::from_value(2));
    sm.clear();

    typename SM::iterator it = sm.find(k);
    EXPECT_TRUE(it == sm.end());

    typename SM::const_iterator cit = csm.find(k);
    EXPECT_TRUE(cit == sm.end());
}

template<class SM>
static void GenerationsDontSkipTest()
{
    using T = typename SM::mapped_type;
    SM sm;
    auto k1 = sm.emplace(Monad<T>::from_value(1));
    int original_cap = static_cast<int>(sm.slot_count());
    for (int i=1; i < original_cap; ++i) {
        sm.emplace(Monad<T>::from_value(i));
    }
    EXPECT_TRUE(sm.size() == sm.slot_count());

    sm.erase(k1);
    auto k2 = sm.emplace(Monad<T>::from_value(2));

#if __cplusplus < 201703L
    using std::get;
    EXPECT_TRUE(get<0>(k2) == get<0>(k1));
    EXPECT_TRUE(get<1>(k2) == get<1>(k1) + 1);
#else
    auto [idx1, gen1] = k1;
    auto [idx2, gen2] = k2;
    EXPECT_TRUE(idx2 == idx1);
    EXPECT_TRUE(gen2 == gen1 + 1);
#endif
}

template<class SM>
static void IndexesAreUsedEvenlyTest()
{
    using T = typename SM::mapped_type;
    SM sm;
    auto k1 = sm.emplace(Monad<T>::from_value(1));
    auto k2 = sm.emplace(Monad<T>::from_value(2));
    int original_cap = static_cast<int>(sm.slot_count());
    for (int i=2; i < original_cap; ++i) {
        sm.emplace(Monad<T>::from_value(i));
    }
    EXPECT_TRUE(sm.size() == sm.slot_count());

    sm.erase(k1);
    sm.erase(k2);
    EXPECT_TRUE(sm.size() == sm.slot_count() - 2);

    // There are now two slots available.
    // So consecutive insertions should prefer to
    // use different slots, rather than sticking to
    // just one slot and bumping its generation count.
    k1 = sm.emplace(Monad<T>::from_value(1));
    sm.erase(k1);
    k2 = sm.emplace(Monad<T>::from_value(2));
    sm.erase(k2);

#if __cplusplus < 201703L
    using std::get;
    EXPECT_TRUE(get<0>(k2) != get<0>(k1));
#else
    auto [idx1, gen1] = k1;
    auto [idx2, gen2] = k2;
    EXPECT_TRUE(idx2 != idx1);
#endif
}

TEST(slot_map, Basic)
{
    // Test the most basic slot_map.
    using slot_map_1 = sg14::slot_map<int>;
    static_assert(std::is_nothrow_move_constructible<slot_map_1>::value, "preserve nothrow-movability of vector");
    BasicTests<slot_map_1>(42, 37);
    BoundsCheckingTest<slot_map_1>();
    FullContainerStressTest<slot_map_1>([]() { return 1; });
    InsertEraseStressTest<slot_map_1>([i=3]() mutable { return ++i; });
    EraseInLoopTest<slot_map_1>();
    EraseRangeTest<slot_map_1>();
    PartitionTest<slot_map_1>();
    ReserveTest<slot_map_1>();
    VerifyCapacityExists<slot_map_1>(true);
    GenerationsDontSkipTest<slot_map_1>();
    IndexesAreUsedEvenlyTest<slot_map_1>();
}

TEST(slot_map, CustomKeyType)
{
    // Test slot_map with a custom key type (C++14 destructuring).
    using slot_map_2 = sg14::slot_map<unsigned long, TestKey::key_16_8_t>;
    BasicTests<slot_map_2>(425, 375);
    BoundsCheckingTest<slot_map_2>();
    FullContainerStressTest<slot_map_2>([]() { return 42; });
    InsertEraseStressTest<slot_map_2>([i=5]() mutable { return ++i; });
    EraseInLoopTest<slot_map_2>();
    EraseRangeTest<slot_map_2>();
    PartitionTest<slot_map_2>();
    ReserveTest<slot_map_2>();
    VerifyCapacityExists<slot_map_2>(true);
    GenerationsDontSkipTest<slot_map_2>();
    IndexesAreUsedEvenlyTest<slot_map_2>();

#if __cplusplus >= 201703L
    // Test slot_map with a custom key type (C++17 destructuring).
    using slot_map_3 = sg14::slot_map<int, TestKey::key_11_5_t>;
    BasicTests<slot_map_3>(42, 37);
    BoundsCheckingTest<slot_map_3>();
    FullContainerStressTest<slot_map_3>([]() { return 42; });
    InsertEraseStressTest<slot_map_3>([i=3]() mutable { return ++i; });
    EraseInLoopTest<slot_map_3>();
    EraseRangeTest<slot_map_3>();
    PartitionTest<slot_map_3>();
    ReserveTest<slot_map_3>();
    VerifyCapacityExists<slot_map_3>(true);
    GenerationsDontSkipTest<slot_map_3>();
    IndexesAreUsedEvenlyTest<slot_map_3>();
#endif // __cplusplus >= 201703L
}

TEST(slot_map, DequeContainer)
{
    // Test slot_map with a custom (but standard and random-access) container type.
    using slot_map_4 = sg14::slot_map<int, std::pair<unsigned, unsigned>, std::deque>;
    BasicTests<slot_map_4>(415, 315);
    BoundsCheckingTest<slot_map_4>();
    FullContainerStressTest<slot_map_4>([]() { return 37; });
    InsertEraseStressTest<slot_map_4>([i=7]() mutable { return ++i; });
    EraseInLoopTest<slot_map_4>();
    EraseRangeTest<slot_map_4>();
    PartitionTest<slot_map_4>();
    ReserveTest<slot_map_4>();
    VerifyCapacityExists<slot_map_4>(false);
    GenerationsDontSkipTest<slot_map_4>();
    IndexesAreUsedEvenlyTest<slot_map_4>();
}

TEST(slot_map, CustomRAContainer)
{
    // Test slot_map with a custom (non-standard, random-access) container type.
    using slot_map_5 = sg14::slot_map<int, std::pair<unsigned, unsigned>, TestContainer::Vector>;
    static_assert(!std::is_nothrow_move_constructible<slot_map_5>::value, "preserve non-nothrow-movability of Vector");
    BasicTests<slot_map_5>(415, 315);
    BoundsCheckingTest<slot_map_5>();
    FullContainerStressTest<slot_map_5>([]() { return 37; });
    InsertEraseStressTest<slot_map_5>([i=7]() mutable { return ++i; });
    EraseInLoopTest<slot_map_5>();
    EraseRangeTest<slot_map_5>();
    PartitionTest<slot_map_5>();
    ReserveTest<slot_map_5>();
    VerifyCapacityExists<slot_map_5>(false);
    GenerationsDontSkipTest<slot_map_5>();
    IndexesAreUsedEvenlyTest<slot_map_5>();
}

TEST(slot_map, CustomBidiContainer)
{
    // Test slot_map with a custom (standard, bidirectional-access) container type.
    using slot_map_6 = sg14::slot_map<int, std::pair<unsigned, unsigned>, std::list>;
    static_assert(std::is_nothrow_move_constructible<slot_map_6>::value == std::is_nothrow_move_constructible<std::list<int>>::value,
                  "preserve implementation-defined nothrow-movability of std::list");
    BasicTests<slot_map_6>(415, 315);
    BoundsCheckingTest<slot_map_6>();
    FullContainerStressTest<slot_map_6>([]() { return 37; });
    InsertEraseStressTest<slot_map_6>([i=7]() mutable { return ++i; });
    EraseInLoopTest<slot_map_6>();
    EraseRangeTest<slot_map_6>();
    PartitionTest<slot_map_6>();
    ReserveTest<slot_map_6>();
    VerifyCapacityExists<slot_map_6>(false);
    GenerationsDontSkipTest<slot_map_6>();
    IndexesAreUsedEvenlyTest<slot_map_6>();
}

TEST(slot_map, MoveOnlyValueType)
{
    // Test slot_map with a move-only value_type.
    // Sadly, standard containers do not propagate move-only-ness, so we must use our custom Vector instead.
    using slot_map_7 = sg14::slot_map<std::unique_ptr<int>, std::pair<unsigned, int>, TestContainer::Vector>;
    static_assert(std::is_move_constructible<slot_map_7>::value, "");
    static_assert(std::is_move_assignable<slot_map_7>::value, "");
    static_assert(! std::is_copy_constructible<slot_map_7>::value, "");
    static_assert(! std::is_copy_assignable<slot_map_7>::value, "");
    BasicTests<slot_map_7>(std::make_unique<int>(1), std::make_unique<int>(2));
    BoundsCheckingTest<slot_map_7>();
    FullContainerStressTest<slot_map_7>([]() { return std::make_unique<int>(1); });
    InsertEraseStressTest<slot_map_7>([i=7]() mutable { return std::make_unique<int>(++i); });
    EraseInLoopTest<slot_map_7>();
    EraseRangeTest<slot_map_7>();
    PartitionTest<slot_map_7>();
    ReserveTest<slot_map_7>();
    VerifyCapacityExists<slot_map_7>(false);
    GenerationsDontSkipTest<slot_map_7>();
    IndexesAreUsedEvenlyTest<slot_map_7>();
}

#if __cpp_concepts >= 202002
template<template<class...> class Ctr, class T = int>
concept SlotMapContainer =
    requires(Ctr<T> c, const Ctr<T> cc, T t) {
        { Ctr<T>{} };  // default constructible, destructible
        { Ctr<T>(cc) };  // copy constructible
        { Ctr<T>(static_cast<Ctr<T>&&>(c)) };  // move constructible
        { c = cc };  // copy assignable
        { c = static_cast<Ctr<T>&&>(c) };  // move assignable
        typename Ctr<T>::value_type;
        typename Ctr<T>::size_type;
        typename Ctr<T>::reference;
        typename Ctr<T>::const_reference;
        typename Ctr<T>::pointer;
        typename Ctr<T>::const_pointer;
        typename Ctr<T>::iterator;
        typename Ctr<T>::const_iterator;
        typename Ctr<T>::reverse_iterator;
        typename Ctr<T>::const_reverse_iterator;
        { c.emplace_back(t) };
        { c.pop_back() };
        { c.begin() } -> std::same_as<typename Ctr<T>::iterator>;
        { c.end() } -> std::same_as<typename Ctr<T>::iterator>;
        { cc.size() } -> std::same_as<typename Ctr<T>::size_type>;
        { cc.begin() } -> std::same_as<typename Ctr<T>::const_iterator>;
        { cc.end() } -> std::same_as<typename Ctr<T>::const_iterator>;
        { std::next(c.begin()) } -> std::same_as<typename Ctr<T>::iterator>;
        { std::next(cc.begin()) } -> std::same_as<typename Ctr<T>::const_iterator>;
    };
static_assert(SlotMapContainer<std::vector>);
static_assert(SlotMapContainer<std::deque>);
static_assert(SlotMapContainer<std::list>);
static_assert(!SlotMapContainer<std::forward_list>);
static_assert(!SlotMapContainer<std::pair>);
#endif // __cpp_concepts >= 202002
