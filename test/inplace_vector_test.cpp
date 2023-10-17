#if __cplusplus >= 201703

#include <sg14/inplace_vector.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <functional>
#include <list>
#include <memory>
#include <random>
#if defined(__cpp_lib_ranges_to_container)
#include <ranges>
#endif
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>

struct Seq {
    std::vector<const char*> v_;

    template<class... Chars>
    explicit Seq(const Chars*... ts) : v_{ts...} {}

    template<class V>
    friend bool operator==(const V& v, const Seq& seq) {
        return std::equal(v.begin(), v.end(), seq.v_.begin(), seq.v_.end());
    }
};

struct MoveOnly {
    MoveOnly(const char *s) : s_(std::make_unique<std::string>(s)) {}
    friend bool operator==(const MoveOnly& a, const MoveOnly& b) { return *a.s_ == *b.s_; }
    friend bool operator!=(const MoveOnly& a, const MoveOnly& b) { return *a.s_ != *b.s_; }
    std::unique_ptr<std::string> s_;
};

struct MoveOnlyNT {
    MoveOnlyNT(const char *s) : s_(s) {}
    MoveOnlyNT(MoveOnlyNT&&) = default;
    MoveOnlyNT(const MoveOnlyNT&) = delete;
    MoveOnlyNT& operator=(MoveOnlyNT&&) = default;
    MoveOnlyNT& operator=(const MoveOnlyNT&) = delete;
    ~MoveOnlyNT() {} // deliberately non-trivial
    friend bool operator==(const MoveOnlyNT& a, const MoveOnlyNT& b) { return a.s_ == b.s_; }
    friend bool operator!=(const MoveOnlyNT& a, const MoveOnlyNT& b) { return a.s_ != b.s_; }

    std::string s_;
};

TEST(inplace_vector, TrivialTraits)
{
    {
        using T = sg14::inplace_vector<int, 10>;
        static_assert(std::is_trivially_copyable_v<T>);
        static_assert(std::is_trivially_copy_constructible_v<T>);
        static_assert(std::is_trivially_move_constructible_v<T>);
        static_assert(std::is_trivially_copy_assignable_v<T>);
        static_assert(std::is_trivially_move_assignable_v<T>);
        static_assert(std::is_trivially_destructible_v<T>);
        static_assert(std::is_nothrow_copy_constructible_v<T>);
        static_assert(std::is_nothrow_move_constructible_v<T>);
        static_assert(std::is_nothrow_copy_assignable_v<T>);
        static_assert(std::is_nothrow_move_assignable_v<T>);
        static_assert(std::is_nothrow_destructible_v<T>);
#if defined(__cpp_lib_trivially_relocatable)
        static_assert(std::is_trivially_relocatable_v<T>);
#endif // __cpp_lib_trivially_relocatable
    }
    {
        using T = sg14::inplace_vector<std::vector<int>, 10>;
        static_assert(!std::is_trivially_copyable_v<T>);
        static_assert(!std::is_trivially_copy_constructible_v<T>);
        static_assert(!std::is_trivially_move_constructible_v<T>);
        static_assert(!std::is_trivially_copy_assignable_v<T>);
        static_assert(!std::is_trivially_move_assignable_v<T>);
        static_assert(!std::is_trivially_destructible_v<T>);
        static_assert(!std::is_nothrow_copy_constructible_v<T>);
        static_assert(std::is_nothrow_move_constructible_v<T>);
        static_assert(!std::is_nothrow_copy_assignable_v<T>);
        static_assert(std::is_nothrow_move_assignable_v<T>);
        static_assert(std::is_nothrow_destructible_v<T>);
#if defined(__cpp_lib_trivially_relocatable)
        static_assert(std::is_trivially_relocatable_v<T>);
#endif // __cpp_lib_trivially_relocatable
    }
    {
        constexpr bool msvc = !std::is_nothrow_move_constructible_v<std::list<int>>;
        using T = sg14::inplace_vector<std::list<int>, 10>;
        static_assert(!std::is_trivially_copyable_v<T>);
        static_assert(!std::is_trivially_copy_constructible_v<T>);
        static_assert(!std::is_trivially_move_constructible_v<T>);
        static_assert(!std::is_trivially_copy_assignable_v<T>);
        static_assert(!std::is_trivially_move_assignable_v<T>);
        static_assert(!std::is_trivially_destructible_v<T>);
        static_assert(!std::is_nothrow_copy_constructible_v<T>);
        static_assert(std::is_nothrow_move_constructible_v<T> == !msvc);
        static_assert(!std::is_nothrow_copy_assignable_v<T>);
        static_assert(std::is_nothrow_move_assignable_v<T>);
        static_assert(std::is_nothrow_destructible_v<T>);
#if defined(__cpp_lib_trivially_relocatable)
        static_assert(std::is_trivially_relocatable_v<T> == msvc);
#endif // __cpp_lib_trivially_relocatable
    }
    {
        using T = sg14::inplace_vector<MoveOnly, 10>;
        static_assert(!std::is_trivially_copyable_v<T>);
        static_assert(!std::is_trivially_copy_constructible_v<T>);
        static_assert(!std::is_trivially_move_constructible_v<T>);
        static_assert(!std::is_trivially_copy_assignable_v<T>);
        static_assert(!std::is_trivially_move_assignable_v<T>);
        static_assert(!std::is_trivially_destructible_v<T>);
        static_assert(!std::is_copy_constructible_v<T>);
        static_assert(std::is_nothrow_move_constructible_v<T>);
        static_assert(!std::is_copy_assignable_v<T>);
        static_assert(std::is_nothrow_move_assignable_v<T>);
        static_assert(std::is_nothrow_destructible_v<T>);
#if defined(__cpp_lib_trivially_relocatable)
        static_assert(std::is_trivially_relocatable_v<T>);
#endif // __cpp_lib_trivially_relocatable
    }
    {
        using T = sg14::inplace_vector<MoveOnlyNT, 10>;
        static_assert(!std::is_trivially_copyable_v<T>);
        static_assert(!std::is_trivially_copy_constructible_v<T>);
        static_assert(!std::is_trivially_move_constructible_v<T>);
        static_assert(!std::is_trivially_copy_assignable_v<T>);
        static_assert(!std::is_trivially_move_assignable_v<T>);
        static_assert(!std::is_trivially_destructible_v<T>);
        static_assert(!std::is_copy_constructible_v<T>);
        static_assert(std::is_nothrow_move_constructible_v<T>);
        static_assert(!std::is_copy_assignable_v<T>);
        static_assert(std::is_nothrow_move_assignable_v<T>);
        static_assert(std::is_nothrow_destructible_v<T>);
#if defined(__cpp_lib_trivially_relocatable)
        static_assert(!std::is_trivially_relocatable_v<T>);
#endif // __cpp_lib_trivially_relocatable
    }
    {
        using T = sg14::inplace_vector<int, 0>;
        static_assert(std::is_trivially_copyable_v<T>);
        static_assert(std::is_trivially_copy_constructible_v<T>);
        static_assert(std::is_trivially_move_constructible_v<T>);
        static_assert(std::is_trivially_copy_assignable_v<T>);
        static_assert(std::is_trivially_move_assignable_v<T>);
        static_assert(std::is_trivially_destructible_v<T>);
        static_assert(std::is_nothrow_copy_constructible_v<T>);
        static_assert(std::is_nothrow_move_constructible_v<T>);
        static_assert(std::is_nothrow_copy_assignable_v<T>);
        static_assert(std::is_nothrow_move_assignable_v<T>);
        static_assert(std::is_nothrow_destructible_v<T>);
#if defined(__cpp_lib_trivially_relocatable)
        static_assert(std::is_trivially_relocatable_v<T>);
#endif // __cpp_lib_trivially_relocatable
    }
    {
        using T = sg14::inplace_vector<std::vector<int>, 0>;
        static_assert(std::is_trivially_copyable_v<T>);
        static_assert(std::is_trivially_copy_constructible_v<T>);
        static_assert(std::is_trivially_move_constructible_v<T>);
        static_assert(std::is_trivially_copy_assignable_v<T>);
        static_assert(std::is_trivially_move_assignable_v<T>);
        static_assert(std::is_trivially_destructible_v<T>);
        static_assert(std::is_nothrow_copy_constructible_v<T>);
        static_assert(std::is_nothrow_move_constructible_v<T>);
        static_assert(std::is_nothrow_copy_assignable_v<T>);
        static_assert(std::is_nothrow_move_assignable_v<T>);
        static_assert(std::is_nothrow_destructible_v<T>);
#if defined(__cpp_lib_trivially_relocatable)
        static_assert(std::is_trivially_relocatable_v<T>);
#endif // __cpp_lib_trivially_relocatable
    }
    {
        using T = sg14::inplace_vector<std::list<int>, 0>;
        static_assert(std::is_trivially_copyable_v<T>);
        static_assert(std::is_trivially_copy_constructible_v<T>);
        static_assert(std::is_trivially_move_constructible_v<T>);
        static_assert(std::is_trivially_copy_assignable_v<T>);
        static_assert(std::is_trivially_move_assignable_v<T>);
        static_assert(std::is_trivially_destructible_v<T>);
        static_assert(std::is_nothrow_copy_constructible_v<T>);
        static_assert(std::is_nothrow_move_constructible_v<T>);
        static_assert(std::is_nothrow_copy_assignable_v<T>);
        static_assert(std::is_nothrow_move_assignable_v<T>);
        static_assert(std::is_nothrow_destructible_v<T>);
#if defined(__cpp_lib_trivially_relocatable)
        static_assert(std::is_trivially_relocatable_v<T>);
#endif // __cpp_lib_trivially_relocatable
    }
    {
        using T = sg14::inplace_vector<std::unique_ptr<int>, 0>;
        static_assert(std::is_trivially_copyable_v<T>);
        static_assert(!std::is_trivially_copy_constructible_v<T>);
        static_assert(std::is_trivially_move_constructible_v<T>);
        static_assert(!std::is_trivially_copy_assignable_v<T>);
        static_assert(std::is_trivially_move_assignable_v<T>);
        static_assert(std::is_trivially_destructible_v<T>);
        static_assert(!std::is_copy_constructible_v<T>);
        static_assert(std::is_nothrow_move_constructible_v<T>);
        static_assert(!std::is_copy_assignable_v<T>);
        static_assert(std::is_nothrow_move_assignable_v<T>);
        static_assert(std::is_nothrow_destructible_v<T>);
#if defined(__cpp_lib_trivially_relocatable)
        static_assert(std::is_trivially_relocatable_v<T>);
#endif // __cpp_lib_trivially_relocatable
    }
}

TEST(inplace_vector, ZeroSized)
{
    {
        sg14::inplace_vector<int, 0> v;
        static_assert(std::is_empty_v<decltype(v)>);
        static_assert(std::is_trivial_v<decltype(v)>);
        static_assert(std::is_trivially_copyable_v<decltype(v)>);
        EXPECT_EQ(v.size(), 0u);
        EXPECT_EQ(v.max_size(), 0u);
        EXPECT_EQ(v.capacity(), 0u);
        EXPECT_EQ(v.try_push_back(42), nullptr);
        EXPECT_EQ(v.try_emplace_back(), nullptr);
        ASSERT_THROW(v.push_back(42), std::bad_alloc);
        ASSERT_THROW(v.emplace_back(), std::bad_alloc);
        EXPECT_TRUE(v.empty());
        EXPECT_EQ(v.erase(v.begin(), v.begin()), v.begin());
        int a[1] = {1};
        EXPECT_EQ(v.insert(v.begin(), a, a), v.begin());
    }
    {
        sg14::inplace_vector<std::string, 0> v;
        static_assert(std::is_empty_v<decltype(v)>);
        static_assert(std::is_trivial_v<decltype(v)>);
        static_assert(std::is_trivially_copyable_v<decltype(v)>);
        EXPECT_EQ(v.size(), 0u);
        EXPECT_EQ(v.max_size(), 0u);
        EXPECT_EQ(v.capacity(), 0u);
        EXPECT_EQ(v.try_push_back("abc"), nullptr);
        EXPECT_EQ(v.try_emplace_back("abc", 3), nullptr);
        ASSERT_THROW(v.push_back("abc"), std::bad_alloc);
        ASSERT_THROW(v.emplace_back("abc", 3), std::bad_alloc);
        EXPECT_TRUE(v.empty());
        EXPECT_EQ(v.erase(v.begin(), v.begin()), v.begin());
        std::string a[1] = {"abc"};
        EXPECT_EQ(v.insert(v.begin(), a, a), v.begin());
    }
    {
        sg14::inplace_vector<std::list<int>, 0> v;
        static_assert(std::is_empty_v<decltype(v)>);
        static_assert(std::is_trivial_v<decltype(v)>);
        static_assert(std::is_trivially_copyable_v<decltype(v)>);
        EXPECT_EQ(v.size(), 0u);
        EXPECT_EQ(v.max_size(), 0u);
        EXPECT_EQ(v.capacity(), 0u);
        EXPECT_EQ(v.try_push_back({1,2,3}), nullptr);
        EXPECT_EQ(v.try_emplace_back(), nullptr);
        ASSERT_THROW(v.push_back({1,2,3}), std::bad_alloc);
        ASSERT_THROW(v.emplace_back(), std::bad_alloc);
        EXPECT_TRUE(v.empty());
        EXPECT_EQ(v.erase(v.begin(), v.begin()), v.begin());
        std::list<int> a[1] = {{1,2,3}};
        EXPECT_EQ(v.insert(v.begin(), a, a), v.begin());
    }
    {
        sg14::inplace_vector<std::unique_ptr<int>, 0> v;
        static_assert(std::is_empty_v<decltype(v)>);
        static_assert(std::is_trivial_v<decltype(v)>);
        static_assert(std::is_trivially_copyable_v<decltype(v)>);
        EXPECT_EQ(v.size(), 0u);
        EXPECT_EQ(v.max_size(), 0u);
        EXPECT_EQ(v.capacity(), 0u);
        EXPECT_EQ(v.try_push_back(nullptr), nullptr);
        EXPECT_EQ(v.try_emplace_back(), nullptr);
        ASSERT_THROW(v.push_back(nullptr), std::bad_alloc);
        ASSERT_THROW(v.emplace_back(), std::bad_alloc);
        EXPECT_TRUE(v.empty());
        EXPECT_EQ(v.erase(v.begin(), v.begin()), v.begin());
        std::unique_ptr<int> a[1] = {nullptr};
        EXPECT_EQ(v.insert(v.begin(), std::make_move_iterator(a), std::make_move_iterator(a)), v.begin());
    }
}

TEST(inplace_vector, Iterators)
{
    {
        using V = sg14::inplace_vector<int, 5>;
        using I = typename V::iterator;
        using CI = typename V::const_iterator;
        using RI = typename V::reverse_iterator;
        using CRI = typename V::const_reverse_iterator;
        static_assert(std::is_same_v<I, int*>);
        static_assert(std::is_same_v<CI, const int*>);
        static_assert(std::is_same_v<RI, std::reverse_iterator<I>>);
        static_assert(std::is_same_v<CRI, std::reverse_iterator<CI>>);
        V v(2);
        const V cv(2);
        static_assert(std::is_same_v<decltype(v.begin()), I>);
        static_assert(std::is_same_v<decltype(v.end()), I>);
        static_assert(std::is_same_v<decltype(cv.begin()), CI>);
        static_assert(std::is_same_v<decltype(cv.begin()), CI>);
        static_assert(std::is_same_v<decltype(v.cbegin()), CI>);
        static_assert(std::is_same_v<decltype(v.cbegin()), CI>);
        static_assert(std::is_same_v<decltype(v.rbegin()), RI>);
        static_assert(std::is_same_v<decltype(v.rend()), RI>);
        static_assert(std::is_same_v<decltype(cv.rbegin()), CRI>);
        static_assert(std::is_same_v<decltype(cv.rend()), CRI>);
        static_assert(std::is_same_v<decltype(v.crbegin()), CRI>);
        static_assert(std::is_same_v<decltype(v.crend()), CRI>);
        EXPECT_EQ(v.end(), v.begin() + 2);
        EXPECT_EQ(v.cend(), v.cbegin() + 2);
        EXPECT_EQ(v.rend(), v.rbegin() + 2);
        EXPECT_EQ(v.crend(), v.crbegin() + 2);
        EXPECT_EQ(cv.end(), cv.begin() + 2);
        EXPECT_EQ(cv.cend(), cv.cbegin() + 2);
        EXPECT_EQ(cv.rend(), cv.rbegin() + 2);
        EXPECT_EQ(cv.crend(), cv.crbegin() + 2);
        for (int& i : v) EXPECT_EQ(i, 0);
        for (const int& i : cv) EXPECT_EQ(i, 0);
    }
    {
        using V = sg14::inplace_vector<int, 0>;
        using I = typename V::iterator;
        using CI = typename V::const_iterator;
        using RI = typename V::reverse_iterator;
        using CRI = typename V::const_reverse_iterator;
        static_assert(std::is_same_v<I, int*>);
        static_assert(std::is_same_v<CI, const int*>);
        static_assert(std::is_same_v<RI, std::reverse_iterator<I>>);
        static_assert(std::is_same_v<CRI, std::reverse_iterator<CI>>);
        V v(0);
        const V cv(0);
        static_assert(std::is_same_v<decltype(v.begin()), I>);
        static_assert(std::is_same_v<decltype(v.end()), I>);
        static_assert(std::is_same_v<decltype(cv.begin()), CI>);
        static_assert(std::is_same_v<decltype(cv.begin()), CI>);
        static_assert(std::is_same_v<decltype(v.cbegin()), CI>);
        static_assert(std::is_same_v<decltype(v.cbegin()), CI>);
        static_assert(std::is_same_v<decltype(v.rbegin()), RI>);
        static_assert(std::is_same_v<decltype(v.rend()), RI>);
        static_assert(std::is_same_v<decltype(cv.rbegin()), CRI>);
        static_assert(std::is_same_v<decltype(cv.rend()), CRI>);
        static_assert(std::is_same_v<decltype(v.crbegin()), CRI>);
        static_assert(std::is_same_v<decltype(v.crend()), CRI>);
        EXPECT_EQ(v.end(), v.begin() + 0);
        EXPECT_EQ(v.cend(), v.cbegin() + 0);
        EXPECT_EQ(v.rend(), v.rbegin() + 0);
        EXPECT_EQ(v.crend(), v.crbegin() + 0);
        EXPECT_EQ(cv.end(), cv.begin() + 0);
        EXPECT_EQ(cv.cend(), cv.cbegin() + 0);
        EXPECT_EQ(cv.rend(), cv.rbegin() + 0);
        EXPECT_EQ(cv.crend(), cv.crbegin() + 0);
        for (int& i : v) EXPECT_EQ(i, 0);
        for (const int& i : cv) EXPECT_EQ(i, 0);
    }
}

TEST(inplace_vector, Constructors)
{
    {
        using V = sg14::inplace_vector<int, 5>;
        long a[] = {1,2,3};
        V v1;
        EXPECT_TRUE(v1.empty());
        V v2 = {};
        EXPECT_TRUE(v2.empty());
        V v3 = {1,2,3};
        EXPECT_TRUE(std::equal(v3.begin(), v3.end(), a, a+3));
        auto v4 = V(a, a+3);
        EXPECT_TRUE(std::equal(v4.begin(), v4.end(), a, a+3));
        auto iss = std::istringstream("1 2 3");
        auto v5 = V(std::istream_iterator<int>(iss), std::istream_iterator<int>());
        EXPECT_TRUE(std::equal(v5.begin(), v5.end(), a, a+3));
        auto v6 = V(3);
        EXPECT_EQ(v6, V(3, 0));
        auto v7 = V(3, 42);
        EXPECT_EQ(v7, (V{42, 42, 42}));
    }
    {
        using V = sg14::inplace_vector<std::string, 5>;
        const char *a[] = {"1", "2", "3"};
        V v1;
        EXPECT_TRUE(v1.empty());
        V v2 = {};
        EXPECT_TRUE(v2.empty());
        V v3 = {"1", "2", "3"};
        EXPECT_TRUE(std::equal(v3.begin(), v3.end(), a, a+3));
        auto v4 = V(a, a+3);
        EXPECT_TRUE(std::equal(v4.begin(), v4.end(), a, a+3));
        auto iss = std::istringstream("1 2 3");
        auto v5 = V(std::istream_iterator<std::string>(iss), std::istream_iterator<std::string>());
        EXPECT_TRUE(std::equal(v5.begin(), v5.end(), a, a+3));
        auto v6 = V(3);
        EXPECT_EQ(v6, V(3, ""));
        auto v7 = V(3, "42");
        EXPECT_EQ(v7, (V{"42", "42", "42"}));
    }
#if defined(__cpp_lib_ranges_to_container)
    {
        auto iss = std::istringstream("1 2 3 4");
        auto rg = std::views::istream<int>(iss);
        auto v1 = sg14::inplace_vector<int, 5>(std::from_range, rg);
        EXPECT_EQ(v1, (sg14::inplace_vector<int, 5>{1,2,3,4}));
        auto v2 = v1 | std::ranges::to<sg14::inplace_vector<long, 5>>();
        EXPECT_EQ(v2, (sg14::inplace_vector<long, 5>{1,2,3,4}));
    }
#endif // __cpp_lib_ranges_to_container
}

TEST(inplace_vector, ConstructorsThrow)
{
    {
        using V = sg14::inplace_vector<int, 3>;
        long a[] = {1,2,3,4,5};
        ASSERT_NO_THROW(V(a, a+3));
        ASSERT_THROW(V(a, a+4), std::bad_alloc);
        ASSERT_NO_THROW(V(3));
        ASSERT_THROW(V(4), std::bad_alloc);
        ASSERT_NO_THROW(V(3, 42));
        ASSERT_THROW(V(4, 42), std::bad_alloc);
        ASSERT_NO_THROW(V({1,2,3}));
        ASSERT_THROW(V({1,2,3,4}), std::bad_alloc);
#if defined(__cpp_lib_ranges_to_container)
        auto iss = std::istringstream("1 2 3 4");
        auto rg = std::views::istream<int>(iss);
        ASSERT_THROW(V(std::from_range, rg), std::bad_alloc);
#endif
    }
}

TEST(inplace_vector, TransfersOfOwnership)
{
    {
        // Trivially copyable value_type
        using V = sg14::inplace_vector<int, 10>;
        V source = {1,2,3};
        static_assert(std::is_trivially_move_constructible_v<V>);
        static_assert(std::is_trivially_move_assignable_v<V>);
        V dest = std::move(source);
        EXPECT_EQ(source.size(), 3u);
        EXPECT_EQ(source, (V{1,2,3}));
        EXPECT_EQ(dest.size(), 3u);
        EXPECT_EQ(dest, (V{1,2,3}));
        dest = {1,2};
        dest = std::move(source);
        EXPECT_EQ(source.size(), 3u);
        EXPECT_EQ(source, (V{1,2,3}));
        EXPECT_EQ(dest.size(), 3u);
        EXPECT_EQ(dest, (V{1,2,3}));
    }
    {
        // Trivially relocatable (but not trivially copyable) value_type
        using V = sg14::inplace_vector<std::unique_ptr<int>, 10>;
        V source;
        source.push_back(std::make_unique<int>(1));
        source.push_back(std::make_unique<int>(2));
        source.push_back(std::make_unique<int>(3));
        static_assert(!std::is_trivially_move_constructible_v<V>);
        static_assert(!std::is_trivially_move_assignable_v<V>);
#if defined(__cpp_lib_trivially_relocatable)
        static_assert(std::is_trivially_relocatable_v<V>);
        V dest = std::move(source); // move-construct
        EXPECT_EQ(source.size(), 0u);
        EXPECT_EQ(dest.size(), 3u);
        EXPECT_EQ(*dest.front(), 1);
        EXPECT_EQ(*dest.back(), 3);
        source = V(2);
        source[0] = std::make_unique<int>(42);
        source = std::move(dest); // move-assign 2 := 3
        EXPECT_EQ(source.size(), 3u);
        EXPECT_EQ(dest.size(), 2u);
        EXPECT_EQ(dest.front(), nullptr);
        EXPECT_EQ(*source.front(), 1);
        EXPECT_EQ(*source.back(), 3);
        dest = V(9);
        dest[0] = std::make_unique<int>(42);
        dest[4] = std::make_unique<int>(42);
        dest = std::move(source); // move-assign 9 := 3
        EXPECT_EQ(source.size(), 3u);
        EXPECT_EQ(source.front(), nullptr);
        EXPECT_EQ(dest.size(), 3u);
        EXPECT_EQ(*dest.front(), 1);
        EXPECT_EQ(*dest.back(), 3);
#else
        V dest = std::move(source); // move-construct
        EXPECT_EQ(source.size(), 3u);
        EXPECT_EQ(source.front(), nullptr);
        EXPECT_EQ(source.back(), nullptr);
        EXPECT_EQ(dest.size(), 3u);
        EXPECT_EQ(*dest.front(), 1);
        EXPECT_EQ(*dest.back(), 3);
        source = V(2);
        source[0] = std::make_unique<int>(42);
        source = std::move(dest); // move-assign 2 := 3
        EXPECT_EQ(source.size(), 3u);
        EXPECT_EQ(dest.size(), 3u);
        EXPECT_EQ(*source.front(), 1);
        EXPECT_EQ(*source.back(), 3);
        dest = V(9);
        dest[0] = std::make_unique<int>(42);
        dest[4] = std::make_unique<int>(42);
        dest = std::move(source); // move-assign 9 := 3
        EXPECT_EQ(source.size(), 3u);
        EXPECT_EQ(dest.size(), 3u);
        EXPECT_EQ(*dest.front(), 1);
        EXPECT_EQ(*dest.back(), 3);
#endif
    }
    {
        // Non-trivially relocatable value_type
        using V = sg14::inplace_vector<MoveOnlyNT, 10>;
        V source;
        source.push_back("abc");
        source.push_back("def");
        source.push_back("ghi");
#if defined(__cpp_lib_trivially_relocatable)
        static_assert(!std::is_trivially_relocatable_v<V>);
#endif
        V dest = std::move(source);
        EXPECT_EQ(source.size(), 3u);
        EXPECT_EQ(dest.size(), 3u);
        EXPECT_EQ(dest.front(), "abc");
        EXPECT_EQ(dest.back(), "ghi");
    }
}

TEST(inplace_vector, Clear)
{
    sg14::inplace_vector<int, 3> v = {1,2,3};
    v.clear();
    EXPECT_EQ(v.size(), 0u);
    EXPECT_TRUE(v.empty());
}

TEST(inplace_vector, Noexceptness)
{
    sg14::inplace_vector<std::string, 3> v;
    static_assert(noexcept(v.size()));
    static_assert(noexcept(v.empty()));
    static_assert(noexcept(v.data()));
    static_assert(noexcept(v.begin()));
    static_assert(noexcept(v.end()));
    static_assert(noexcept(v.cbegin()));
    static_assert(noexcept(v.cend()));
    static_assert(noexcept(v.rbegin()));
    static_assert(noexcept(v.rend()));
    static_assert(noexcept(v.crbegin()));
    static_assert(noexcept(v.crend()));
    static_assert(noexcept(v.clear()));
    std::string lvalue = "abc";
    std::initializer_list<std::string> il = {"abc", "def"};
    static_assert(!noexcept(v.insert(v.begin(), lvalue)));
    static_assert(!noexcept(v.insert(v.begin(), il)));
    static_assert(!noexcept(v.insert(v.begin(), il.begin(), il.end())));
    static_assert(!noexcept(v.emplace(v.begin(), "abc", 3)));
    static_assert(!noexcept(v.emplace_back("abc", 3)));
    static_assert(!noexcept(v.push_back(lvalue)));
    static_assert(!noexcept(v.try_emplace_back("abc", 3)));
    static_assert(!noexcept(v.try_push_back(lvalue)));
    static_assert(!noexcept(v.unchecked_emplace_back("abc", 3)));
    static_assert(!noexcept(v.unchecked_push_back(lvalue)));
#if defined(__cpp_lib_ranges_to_container)
    static_assert(!noexcept(v.assign_range(il)));
    static_assert(!noexcept(v.append_range(il)));
    static_assert(!noexcept(v.insert_range(v.begin(), il)));
#endif

    // Lakos rule
    static_assert(!noexcept(v.front()));
    static_assert(!noexcept(v.back()));
    static_assert(!noexcept(v.at(0)));
    static_assert(!noexcept(v[0]));
    static_assert(!noexcept(v.try_push_back(std::move(lvalue))));
    static_assert(!noexcept(v.unchecked_push_back(std::move(lvalue))));
    static_assert(!noexcept(v.erase(v.begin())));
    static_assert(!noexcept(v.erase(v.begin(), v.end())));
}

TEST(inplace_vector, NoexceptnessOfSwap)
{
    using std::swap;
    {
        std::string lvalue;
        sg14::inplace_vector<std::string, 10> v;
        static_assert(noexcept(swap(lvalue, lvalue)));
        static_assert(noexcept(v.swap(v)));
        static_assert(noexcept(swap(v, v)));
    }
    {
        struct ThrowingSwappable {
            explicit ThrowingSwappable() { }
            ThrowingSwappable(const ThrowingSwappable&) { }
            void operator=(const ThrowingSwappable&) { }
            ~ThrowingSwappable() { }
        };
        ThrowingSwappable lvalue;
        sg14::inplace_vector<ThrowingSwappable, 10> v;
        static_assert(!noexcept(swap(lvalue, lvalue)));
        static_assert(!noexcept(v.swap(v)));
        static_assert(!noexcept(swap(v, v)));
    }
}

TEST(inplace_vector, InsertSingle)
{
    {
        using V = sg14::inplace_vector<int, 5>;
        V v = {1, 4, 7};
        int lvalue = 23;
        static_assert(std::is_same_v<decltype(v.insert(v.begin(), lvalue)), V::iterator>);
        static_assert(std::is_same_v<decltype(v.insert(v.begin(), 42)), V::iterator>);
        static_assert(std::is_same_v<decltype(v.insert(v.cbegin(), lvalue)), V::iterator>);
        static_assert(std::is_same_v<decltype(v.insert(v.cbegin(), 42)), V::iterator>);
        auto it = v.insert(v.begin(), 24);
        EXPECT_EQ(it, v.begin());
        EXPECT_EQ(v, (V{24, 1, 4, 7}));
        it = v.insert(v.begin() + 2, lvalue);
        EXPECT_EQ(it, v.begin() + 2);
        EXPECT_EQ(v, (V{24, 1, 23, 4, 7}));
        ASSERT_THROW(v.insert(v.begin() + 2, 23), std::bad_alloc);
        ASSERT_THROW(v.insert(v.end(), lvalue), std::bad_alloc);
        it = v.erase(v.begin() + 1);
        EXPECT_EQ(it, v.begin() + 1);
        EXPECT_EQ(v, (V{24, 23, 4, 7}));
        it = v.insert(v.end(), 10);
        EXPECT_EQ(it, v.end() - 1);
        EXPECT_EQ(v, (V{24, 23, 4, 7, 10}));
        EXPECT_EQ(v.size(), 5u);
    }
    {
        using V = sg14::inplace_vector<std::string, 5>;
        V v = {"abc", "def", "ghi"};
        auto it = v.insert(v.begin(), "xyz");
        EXPECT_EQ(it, v.begin());
        EXPECT_EQ(v, (V{"xyz", "abc", "def", "ghi"}));
        std::string lvalue = "wxy";
        it = v.insert(v.begin() + 2, lvalue);
        EXPECT_EQ(it, v.begin() + 2);
        EXPECT_EQ(v, (V{"xyz", "abc", "wxy", "def", "ghi"}));
        ASSERT_THROW(v.insert(v.begin() + 2, "wxy"), std::bad_alloc);
        ASSERT_THROW(v.insert(v.end(), lvalue), std::bad_alloc);
        it = v.erase(v.begin() + 1);
        EXPECT_EQ(it, v.begin() + 1);
        EXPECT_EQ(v, (V{"xyz", "wxy", "def", "ghi"}));
        it = v.insert(v.end(), "jkl");
        EXPECT_EQ(it, v.end() - 1);
        EXPECT_EQ(v, (V{"xyz", "wxy", "def", "ghi", "jkl"}));
        EXPECT_EQ(v.size(), 5u);
    }
    {
        using V = sg14::inplace_vector<std::list<int>, 5>;
        V v = {{1,2,3}, {4,5,6}, {7,8,9}};
        auto it = v.insert(v.begin(), {24,25,26});
        EXPECT_EQ(it, v.begin());
        EXPECT_EQ(v, (V{{24,25,26}, {1,2,3}, {4,5,6}, {7,8,9}}));
        std::list<int> lvalue = {23,24,25};
        it = v.insert(v.begin() + 2, lvalue);
        EXPECT_EQ(it, v.begin() + 2);
        EXPECT_EQ(v, (V{{24,25,26}, {1,2,3}, {23,24,25}, {4,5,6}, {7,8,9}}));
        ASSERT_THROW(v.insert(v.begin() + 2, {23,24,25}), std::bad_alloc);
        ASSERT_THROW(v.insert(v.end(), lvalue), std::bad_alloc);
        it = v.erase(v.begin() + 1);
        EXPECT_EQ(it, v.begin() + 1);
        EXPECT_EQ(v, (V{{24,25,26}, {23,24,25}, {4,5,6}, {7,8,9}}));
        it = v.insert(v.end(), {10,11,12});
        EXPECT_EQ(it, v.end() - 1);
        EXPECT_EQ(v, (V{{24,25,26}, {23,24,25}, {4,5,6}, {7,8,9}, {10,11,12}}));
        EXPECT_EQ(v.size(), 5u);
    }
    {
        using V = sg14::inplace_vector<MoveOnly, 5>;
        MoveOnly a[3] = {"abc", "def", "ghi"};
        V v = V(std::make_move_iterator(a), std::make_move_iterator(a+3));
        auto it = v.insert(v.begin(), "xyz");
        EXPECT_EQ(it, v.begin());
        EXPECT_EQ(*it, "xyz");
        MoveOnly lvalue = "wxy";
        it = v.insert(v.begin() + 2, std::move(lvalue));
        EXPECT_EQ(it, v.begin() + 2);
        EXPECT_EQ(v, Seq("xyz", "abc", "wxy", "def", "ghi"));
        ASSERT_THROW(v.insert(v.begin() + 2, "wxy"), std::bad_alloc);
        it = v.erase(v.begin() + 1);
        EXPECT_EQ(it, v.begin() + 1);
        EXPECT_EQ(*it, "wxy");
        it = v.insert(v.end(), "jkl");
        EXPECT_EQ(it, v.end() - 1);
        EXPECT_EQ(v, Seq("xyz", "wxy", "def", "ghi", "jkl"));
        EXPECT_EQ(v.size(), 5u);
    }
}

TEST(inplace_vector, InsertMulti)
{
    {
        using V = sg14::inplace_vector<int, 5>;
        V v = {1, 2, 3};
        int a[2] = {4, 5};
        static_assert(std::is_same_v<decltype(v.insert(v.begin(), a, a)), V::iterator>);
        static_assert(std::is_same_v<decltype(v.insert(v.begin(), {1, 2, 3})), V::iterator>);
        static_assert(std::is_same_v<decltype(v.insert(v.cbegin(), a, a)), V::iterator>);
        static_assert(std::is_same_v<decltype(v.insert(v.cbegin(), {1, 2, 3})), V::iterator>);
        auto it = v.insert(v.begin(), a, a+2);
        EXPECT_EQ(it, v.begin());
        EXPECT_EQ(v, (V{4, 5, 1, 2, 3}));
        it = v.insert(v.begin(), a, a);
        EXPECT_EQ(it, v.begin());
        EXPECT_EQ(v, (V{4, 5, 1, 2, 3}));
        ASSERT_THROW(v.insert(v.begin(), a, a+2), std::bad_alloc);
        v.clear();
        it = v.insert(v.begin(), a, a+2);
        EXPECT_EQ(it, v.begin());
        it = v.insert(v.end(), {1, 2}); // insert(initializer_list)
        EXPECT_EQ(it, v.begin() + 2);
        EXPECT_EQ(v, (V{4, 5, 1, 2}));
        ASSERT_THROW(v.insert(v.begin() + 2, a, a+2), std::bad_alloc);
        EXPECT_LE(v.size(), 5u);
    }
    {
        using V = sg14::inplace_vector<std::string, 5>;
        V v = {"1", "2", "3"};
        const char *a[2] = {"4", "5"};
        auto it = v.insert(v.begin(), a, a+2);
        EXPECT_EQ(it, v.begin());
        EXPECT_EQ(v, (V{"4", "5", "1", "2", "3"}));
        it = v.insert(v.begin(), a, a);
        EXPECT_EQ(it, v.begin());
        EXPECT_EQ(v, (V{"4", "5", "1", "2", "3"}));
        ASSERT_THROW(v.insert(v.begin(), a, a+2), std::bad_alloc);
        v.clear();
        it = v.insert(v.begin(), a, a+2);
        EXPECT_EQ(it, v.begin());
        it = v.insert(v.end(), {"1", "2"}); // insert(initializer_list)
        EXPECT_EQ(it, v.begin() + 2);
        EXPECT_EQ(v, (V{"4", "5", "1", "2"}));
        ASSERT_THROW(v.insert(v.begin() + 2, a, a+2), std::bad_alloc);
        EXPECT_LE(v.size(), 5u);
    }
    {
        using V = sg14::inplace_vector<MoveOnly, 5>;
        V v;
        v.emplace_back("1");
        v.emplace_back("2");
        v.emplace_back("3");
        const char *a[2] = {"4", "5"};
        auto it = v.insert(v.begin(), a, a+2);
        EXPECT_EQ(it, v.begin());
        EXPECT_EQ(v, Seq("4", "5", "1", "2", "3"));
        it = v.insert(v.begin(), a, a);
        EXPECT_EQ(it, v.begin());
        EXPECT_EQ(v, Seq("4", "5", "1", "2", "3"));
        ASSERT_THROW(v.insert(v.begin(), a, a+2), std::bad_alloc);
        v.clear();
        it = v.insert(v.begin(), a, a+2);
        EXPECT_EQ(it, v.begin());
        MoveOnly il[2] = {"1", "2"};
        it = v.insert(v.end(), std::make_move_iterator(il), std::make_move_iterator(il + 2));
        EXPECT_EQ(it, v.begin() + 2);
        EXPECT_EQ(v, Seq("4", "5", "1", "2"));
        ASSERT_THROW(v.insert(v.begin() + 2, a, a+2), std::bad_alloc);
        EXPECT_LE(v.size(), 5u);
    }
    {
        using V = sg14::inplace_vector<MoveOnlyNT, 5>;
        V v;
        v.emplace_back("1");
        v.emplace_back("2");
        v.emplace_back("3");
        const char *a[2] = {"4", "5"};
        auto it = v.insert(v.begin(), a, a+2);
        EXPECT_EQ(it, v.begin());
        EXPECT_EQ(v, Seq("4", "5", "1", "2", "3"));
        it = v.insert(v.begin(), a, a);
        EXPECT_EQ(it, v.begin());
        EXPECT_EQ(v, Seq("4", "5", "1", "2", "3"));
        ASSERT_THROW(v.insert(v.begin(), a, a+2), std::bad_alloc);
        v.clear();
        it = v.insert(v.begin(), a, a+2);
        EXPECT_EQ(it, v.begin());
        MoveOnlyNT il[2] = {"1", "2"};
        it = v.insert(v.end(), std::make_move_iterator(il), std::make_move_iterator(il + 2));
        EXPECT_EQ(it, v.begin() + 2);
        EXPECT_EQ(v, Seq("4", "5", "1", "2"));
        ASSERT_THROW(v.insert(v.begin() + 2, a, a+2), std::bad_alloc);
        EXPECT_LE(v.size(), 5u);
    }
}

TEST(inplace_vector, InsertRange)
{
#if defined(__cpp_lib_ranges_to_container)
    {
        using V = sg14::inplace_vector<int, 5>;
        V v = {1, 2};
        auto it = v.insert_range(v.begin() + 1, std::vector<int>{4, 5});
        static_assert(std::is_same_v<decltype(it), V::iterator>);
        EXPECT_EQ(it, v.begin() + 1);
        EXPECT_EQ(v, (V{1, 4, 5, 2}));
        ASSERT_THROW(v.insert_range(v.begin() + 1, std::vector<int>{4, 5}), std::bad_alloc);
        EXPECT_LE(v.size(), 5u);
    }
    {
        using V = sg14::inplace_vector<int, 5>;
        auto iss = std::istringstream("4 5 6 7");
        V v = {1, 2, 3};
        auto it = v.insert_range(v.begin() + 1, std::views::istream<int>(iss) | std::views::take(2));
        static_assert(std::is_same_v<decltype(it), V::iterator>);
        EXPECT_EQ(it, v.begin() + 1);
        EXPECT_EQ(v, (V{1, 4, 5, 2, 3}));
        iss = std::istringstream("6 7");
        ASSERT_THROW(v.insert_range(v.begin() + 1, std::views::istream<int>(iss) | std::views::take(2)), std::bad_alloc);
        EXPECT_LE(v.size(), 5u);
    }
    {
        using V = sg14::inplace_vector<std::string, 5>;
        auto iss = std::istringstream("4 5 6 7");
        V v = {"1", "2"};
        auto it = v.insert_range(v.begin() + 1, std::views::istream<std::string>(iss) | std::views::take(2));
        EXPECT_EQ(it, v.begin() + 1);
        EXPECT_EQ(v, (V{"1", "4", "5", "2"}));
        iss = std::istringstream("6 7");
        ASSERT_THROW(v.insert_range(v.begin() + 1, std::views::istream<std::string>(iss) | std::views::take(2)), std::bad_alloc);
        EXPECT_LE(v.size(), 5u);
    }
#if defined(__cpp_lib_ranges_as_rvalue)
    {
        using V = sg14::inplace_vector<MoveOnly, 5>;
        MoveOnly a[2] = {"abc", "def"};
        V v;
        v.emplace_back("wxy");
        v.emplace_back("xyz");
        auto it = v.insert_range(v.begin() + 1, a | std::views::as_rvalue);
        EXPECT_EQ(it, v.begin() + 1);
        EXPECT_EQ(v, Seq("wxy", "abc", "def", "xyz"));
        ASSERT_THROW(v.insert_range(v.begin() + 1, a | std::views::as_rvalue), std::bad_alloc);
        EXPECT_LE(v.size(), 5u);
    }
#endif // __cpp_lib_ranges_as_rvalue
#endif // __cpp_lib_ranges_to_container
}

TEST(inplace_vector, AppendRange)
{
#if defined(__cpp_lib_ranges_to_container)
    {
        using V = sg14::inplace_vector<int, 5>;
        V v = {1, 2};
        v.append_range(std::vector<int>{4, 5});
        static_assert(std::is_same_v<decltype(v.append_range(std::vector<int>{4, 5})), void>);
        EXPECT_EQ(v, (V{1, 2, 4, 5}));
        ASSERT_THROW(v.append_range(std::vector<int>{4, 5}), std::bad_alloc);
        EXPECT_LE(v.size(), 5u);
    }
    {
        using V = sg14::inplace_vector<int, 5>;
        auto iss = std::istringstream("4 5 6 7");
        V v = {1, 2, 3};
        v.append_range(std::views::istream<int>(iss) | std::views::take(2));
        EXPECT_EQ(v, (V{1, 2, 3, 4, 5}));
        iss = std::istringstream("6 7");
        ASSERT_THROW(v.append_range(std::views::istream<int>(iss) | std::views::take(2)), std::bad_alloc);
        EXPECT_LE(v.size(), 5u);
    }
    {
        using V = sg14::inplace_vector<std::string, 5>;
        auto iss = std::istringstream("4 5 6 7");
        V v = {"1", "2"};
        v.append_range(std::views::istream<std::string>(iss) | std::views::take(2));
        EXPECT_EQ(v, Seq("1", "2", "4", "5"));
        iss = std::istringstream("6 7");
        ASSERT_THROW(v.append_range(std::views::istream<std::string>(iss) | std::views::take(2)), std::bad_alloc);
        EXPECT_LE(v.size(), 5u);
    }
#if defined(__cpp_lib_ranges_as_rvalue)
    {
        using V = sg14::inplace_vector<MoveOnly, 5>;
        MoveOnly a[2] = {"abc", "def"};
        V v;
        v.emplace_back("wxy");
        v.emplace_back("xyz");
        v.append_range(a | std::views::as_rvalue);
        EXPECT_EQ(v, Seq("wxy", "xyz", "abc", "def"));
        ASSERT_THROW(v.append_range(a | std::views::as_rvalue), std::bad_alloc);
        EXPECT_LE(v.size(), 5u);
    }
#endif // __cpp_lib_ranges_as_rvalue
#endif // __cpp_lib_ranges_to_container
}

TEST(inplace_vector, PushBack)
{
    {
        using V = sg14::inplace_vector<int, 5>;
        V v = {1, 4};
        int lvalue = 23;
        static_assert(std::is_same_v<decltype(v.push_back(lvalue)), V::reference>);
        static_assert(std::is_same_v<decltype(v.push_back(42)), V::reference>);
        static_assert(std::is_same_v<decltype(v.emplace_back()), V::reference>);
        EXPECT_EQ(&v.push_back(24), v.data() + 2);
        EXPECT_EQ(&v.push_back(lvalue), v.data() + 3);
        EXPECT_EQ(&v.emplace_back(), v.data() + 4);
        EXPECT_EQ(v, (V{1, 4, 24, 23, 0}));
        EXPECT_THROW(v.push_back(24), std::bad_alloc);
        EXPECT_THROW(v.push_back(lvalue), std::bad_alloc);
        EXPECT_THROW(v.emplace_back(), std::bad_alloc);
        EXPECT_EQ(v, (V{1, 4, 24, 23, 0})); // failed push_back has the strong guarantee

        v = {1, 4};
        static_assert(std::is_same_v<decltype(v.try_push_back(lvalue)), V::value_type*>);
        static_assert(std::is_same_v<decltype(v.try_push_back(42)), V::value_type*>);
        static_assert(std::is_same_v<decltype(v.try_emplace_back()), V::value_type*>);
        EXPECT_EQ(v.try_push_back(24), v.data() + 2);
        EXPECT_EQ(v.try_push_back(lvalue), v.data() + 3);
        EXPECT_EQ(v.try_emplace_back(), v.data() + 4);
        EXPECT_EQ(v, (V{1, 4, 24, 23, 0}));
        EXPECT_EQ(v.try_push_back(24), nullptr);
        EXPECT_EQ(v.try_push_back(lvalue), nullptr);
        EXPECT_EQ(v.try_emplace_back(), nullptr);
        EXPECT_EQ(v, (V{1, 4, 24, 23, 0})); // failed try_push_back has the strong guarantee
    }
    {
        using V = sg14::inplace_vector<std::string, 5>;
        V v = {"1", "4"};
        std::string lvalue = "23";
        static_assert(std::is_same_v<decltype(v.push_back(lvalue)), V::reference>);
        static_assert(std::is_same_v<decltype(v.push_back("42")), V::reference>);
        static_assert(std::is_same_v<decltype(v.emplace_back()), V::reference>);
        EXPECT_EQ(&v.push_back("24"), v.data() + 2);
        EXPECT_EQ(&v.push_back(lvalue), v.data() + 3);
        EXPECT_EQ(&v.emplace_back(), v.data() + 4);
        EXPECT_EQ(v, (V{"1", "4", "24", "23", ""}));
        EXPECT_THROW(v.push_back("24"), std::bad_alloc);
        EXPECT_THROW(v.push_back(lvalue), std::bad_alloc);
        EXPECT_THROW(v.emplace_back(), std::bad_alloc);
        EXPECT_EQ(v, (V{"1", "4", "24", "23", ""})); // failed push_back has the strong guarantee

        v = {"1", "4"};
        static_assert(std::is_same_v<decltype(v.try_push_back(lvalue)), V::value_type*>);
        static_assert(std::is_same_v<decltype(v.try_push_back("42")), V::value_type*>);
        static_assert(std::is_same_v<decltype(v.try_emplace_back()), V::value_type*>);
        EXPECT_EQ(v.try_push_back("24"), v.data() + 2);
        EXPECT_EQ(v.try_push_back(lvalue), v.data() + 3);
        EXPECT_EQ(v.try_emplace_back(), v.data() + 4);
        EXPECT_EQ(v, (V{"1", "4", "24", "23", ""}));
        EXPECT_EQ(v.try_push_back("24"), nullptr);
        EXPECT_EQ(v.try_push_back(lvalue), nullptr);
        EXPECT_EQ(v.try_emplace_back(), nullptr);
        EXPECT_EQ(v, (V{"1", "4", "24", "23", ""})); // failed try_push_back has the strong guarantee
    }
}

TEST(inplace_vector, EraseSingle)
{
    {
        using V = sg14::inplace_vector<int, 5>;
        V v = {1, 2, 3, 4};
        auto it = v.erase(v.begin() + 1);
        static_assert(std::is_same_v<decltype(it), V::iterator>);
        EXPECT_EQ(it, v.begin() + 1);
        EXPECT_EQ(v, (V{1, 3, 4}));
        it = v.erase(v.begin());
        EXPECT_EQ(it, v.begin());
        EXPECT_EQ(v, (V{3, 4}));
        it = v.erase(v.begin() + 1);
        EXPECT_EQ(it, v.end());
        EXPECT_EQ(v, V{3});
        it = v.erase(v.begin());
        EXPECT_EQ(it, v.end());
        EXPECT_TRUE(v.empty());
    }
    {
        using V = sg14::inplace_vector<std::string, 5>;
        V v = {"1", "2", "3", "4"};
        auto it = v.erase(v.begin() + 1);
        static_assert(std::is_same_v<decltype(it), V::iterator>);
        EXPECT_EQ(it, v.begin() + 1);
        EXPECT_EQ(v, (V{"1", "3", "4"}));
        it = v.erase(v.begin());
        EXPECT_EQ(it, v.begin());
        EXPECT_EQ(v, (V{"3", "4"}));
        it = v.erase(v.begin() + 1);
        EXPECT_EQ(it, v.end());
        EXPECT_EQ(v, V{"3"});
        it = v.erase(v.begin());
        EXPECT_EQ(it, v.end());
        EXPECT_TRUE(v.empty());
    }
    {
        using V = sg14::inplace_vector<MoveOnly, 5>;
        const char *a[4] = {"1", "2", "3", "4"};
        V v = V(a, a+4);
        auto it = v.erase(v.begin() + 1);
        static_assert(std::is_same_v<decltype(it), V::iterator>);
        EXPECT_EQ(it, v.begin() + 1);
        EXPECT_EQ(v.size(), 3u);
        it = v.erase(v.begin());
        EXPECT_EQ(it, v.begin());
        EXPECT_EQ(v, V(a+2, a+4));
        it = v.erase(v.begin() + 1);
        EXPECT_EQ(it, v.end());
        EXPECT_EQ(v, V(a+2, a+3));
        it = v.erase(v.begin());
        EXPECT_EQ(it, v.end());
        EXPECT_TRUE(v.empty());
    }
}

TEST(inplace_vector, EraseMulti)
{
    {
        using V = sg14::inplace_vector<int, 5>;
        V v = {1, 2, 3, 4};
        auto it = v.erase(v.begin() + 1, v.begin() + 3);
        static_assert(std::is_same_v<decltype(it), V::iterator>);
        EXPECT_EQ(it, v.begin() + 1);
        EXPECT_EQ(v, (V{1, 4}));
        it = v.erase(v.begin(), v.begin());
        EXPECT_EQ(it, v.begin());
        EXPECT_EQ(v, (V{1, 4}));
        it = v.erase(v.begin() + 1, v.end());
        EXPECT_EQ(it, v.end());
        EXPECT_EQ(v, V{1});
        it = v.erase(v.begin(), v.end());
        EXPECT_EQ(it, v.end());
        EXPECT_TRUE(v.empty());
    }
    {
        using V = sg14::inplace_vector<std::string, 5>;
        V v = {"1", "2", "3", "4"};
        auto it = v.erase(v.begin() + 1, v.begin() + 3);
        static_assert(std::is_same_v<decltype(it), V::iterator>);
        EXPECT_EQ(it, v.begin() + 1);
        EXPECT_EQ(v, (V{"1", "4"}));
        it = v.erase(v.begin(), v.begin());
        EXPECT_EQ(it, v.begin());
        EXPECT_EQ(v, (V{"1", "4"}));
        it = v.erase(v.begin() + 1, v.end());
        EXPECT_EQ(it, v.end());
        EXPECT_EQ(v, V{"1"});
        it = v.erase(v.begin(), v.end());
        EXPECT_EQ(it, v.end());
        EXPECT_TRUE(v.empty());
    }
    {
        using V = sg14::inplace_vector<MoveOnly, 5>;
        const char *a[4] = {"1", "2", "3", "4"};
        V v = V(a, a+4);
        auto it = v.erase(v.begin() + 1, v.begin() + 3);
        static_assert(std::is_same_v<decltype(it), V::iterator>);
        EXPECT_EQ(it, v.begin() + 1);
        EXPECT_EQ(v, Seq("1", "4"));
        it = v.erase(v.begin(), v.begin());
        EXPECT_EQ(it, v.begin());
        EXPECT_EQ(v, Seq("1", "4"));
        it = v.erase(v.begin() + 1, v.end());
        EXPECT_EQ(it, v.end());
        EXPECT_EQ(v, Seq("1"));
        it = v.erase(v.begin(), v.end());
        EXPECT_EQ(it, v.end());
        EXPECT_TRUE(v.empty());
    }
    {
        using V = sg14::inplace_vector<MoveOnlyNT, 5>;
        const char *a[4] = {"1", "2", "3", "4"};
        V v = V(a, a+4);
        auto it = v.erase(v.begin() + 1, v.begin() + 3);
        static_assert(std::is_same_v<decltype(it), V::iterator>);
        EXPECT_EQ(it, v.begin() + 1);
        EXPECT_EQ(v, Seq("1", "4"));
        it = v.erase(v.begin(), v.begin());
        EXPECT_EQ(it, v.begin());
        EXPECT_EQ(v, Seq("1", "4"));
        it = v.erase(v.begin() + 1, v.end());
        EXPECT_EQ(it, v.end());
        EXPECT_EQ(v, Seq("1"));
        it = v.erase(v.begin(), v.end());
        EXPECT_EQ(it, v.end());
        EXPECT_TRUE(v.empty());
    }
}

TEST(inplace_vector, EraseRemoveIdiom)
{
    sg14::inplace_vector<unsigned, 1000> v;
    std::mt19937 g;
    std::generate_n(std::back_inserter(v), 1000, std::ref(g));
    v.erase(std::remove_if(v.begin(), v.end(), [](auto x) { return x % 100 == 0; }), v.end());
    EXPECT_TRUE(std::none_of(v.begin(), v.end(), [](auto x) { return x % 100 == 0; }));
    v.erase(std::remove_if(v.begin(), v.end(), [](auto x) { return x % 100 == 1; }), v.end());
    EXPECT_TRUE(std::none_of(v.begin(), v.end(), [](auto x) { return x % 100 == 1; }));
    for (unsigned i = 0; i < 100; ++i) {
        v.erase(std::remove_if(v.begin(), v.end(), [&](auto x) { return x % 100 == i; }), v.end());
        EXPECT_TRUE(std::none_of(v.begin(), v.end(), [&](auto x) { return x % 100 == i; }));
    }
    EXPECT_TRUE(v.empty());
}

TEST(inplace_vector, AssignFromInitList)
{
    {
        using V = sg14::inplace_vector<std::pair<int, int>, 4>;
        V v = {{1,2}, {3,4}};
        v = {{5,6}};
        EXPECT_EQ(v, (V{{5,6}}));
        v = {{7,8},{9,10},{11,12}};
        EXPECT_EQ(v, (V{{7,8},{9,10},{11,12}}));
        ASSERT_THROW((v = {{}, {}, {}, {}, {}, {}}), std::bad_alloc);
        static_assert(std::is_same_v<decltype(v = {}), V&>);
        static_assert(std::is_same_v<decltype(v = {{1,2}, {3,4}}), V&>);
    }
    {
        struct Counter {
            int i_ = 0;
            Counter(int i): i_(i) {}
            Counter(const Counter& rhs) : i_(rhs.i_ + 1) {}
            void operator=(const Counter& rhs) { i_ = rhs.i_ + 10; }
        };
        sg14::inplace_vector<Counter, 10> v = { 100 };
        EXPECT_EQ(v[0].i_, 101);  // copied from init-list into vector
        v = { 200, 300 };
        EXPECT_EQ(v[0].i_, 210);  // assigned from init-list into vector
        EXPECT_EQ(v[1].i_, 301);  // copied from init-list into vector
    }
}

#endif // __cplusplus >= 201703
