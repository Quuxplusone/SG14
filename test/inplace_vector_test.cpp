#if __cplusplus >= 201703L

#include <sg14/inplace_vector.h>
#define IPV_TEST_NAME inplace_vector
#include "inplace_vector_common_tests.cpp"

TEST(IPV_TEST_NAME, TrivialTraits)
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
        static_assert(std::is_nothrow_move_assignable_v<T> == !msvc);
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

TEST(IPV_TEST_NAME, PartiallyTrivialTraits)
{
#if (__cpp_concepts >= 202002L)
    constexpr bool ConditionallyTrivial = true;
#else
    constexpr bool ConditionallyTrivial = false;
#endif
    {
        struct S {
            int *p_ = nullptr;
            S(int *p) : p_(p) {}
            S(const S& s) : p_(s.p_) { *p_ += 1; }
            S(S&&) = default;
            S& operator=(const S&) = default;
            S& operator=(S&&) = default;
            ~S() = default;
        };
        using T = sg14::inplace_vector<S, 10>;
        static_assert(!std::is_trivially_copyable_v<T>);
        static_assert(!std::is_trivially_copy_constructible_v<T>);
        static_assert(!std::is_trivially_copy_assignable_v<T>);
        static_assert(std::is_trivially_move_constructible_v<T> == ConditionallyTrivial);
        static_assert(std::is_trivially_move_assignable_v<T> == ConditionallyTrivial);
        static_assert(std::is_trivially_destructible_v<T> == ConditionallyTrivial);
        T v;
        int count = 0;
        v.push_back(S(&count));
        v.push_back(S(&count));
        count = 0;
        T w = v;
        EXPECT_EQ(count, 2); // should have copied two S objects
        T x = std::move(v);
        EXPECT_EQ(count, 2); // moving them is trivial
        EXPECT_EQ(v.size(), 2);
        v.clear(); w = v;
        EXPECT_EQ(count, 2); // destroying them is trivial
        v = x;
        EXPECT_EQ(count, 4); // should have copy-constructed two S objects
        v = x;
        EXPECT_EQ(count, 4); // copy-assigning them is trivial
        v = std::move(x);
        EXPECT_EQ(count, 4); // move-assigning them is trivial
        EXPECT_EQ(x.size(), 2);
        x.clear(); x = std::move(v);
        EXPECT_EQ(count, 4); // move-constructing them is trivial
        EXPECT_EQ(v.size(), 2);
        v.clear(); x = std::move(v);
        EXPECT_EQ(count, 4); // destroying them is trivial
        EXPECT_TRUE(v.empty() && w.empty() && x.empty());
    }
    {
        struct S {
            int *p_ = nullptr;
            S(int *p) : p_(p) {}
            S(const S& s) = default;
            S(S&&) = default;
            S& operator=(const S&) { *p_ += 1; return *this; }
            S& operator=(S&&) = default;
            ~S() = default;
        };
        using T = sg14::inplace_vector<S, 10>;
        static_assert(!std::is_trivially_copyable_v<T>);
        static_assert(std::is_trivially_copy_constructible_v<T> == ConditionallyTrivial);
        static_assert(!std::is_trivially_copy_assignable_v<T>);
        static_assert(std::is_trivially_move_constructible_v<T> == ConditionallyTrivial);
        static_assert(std::is_trivially_move_assignable_v<T> == ConditionallyTrivial);
        static_assert(std::is_trivially_destructible_v<T> == ConditionallyTrivial);
        T v;
        int count = 0;
        v.push_back(S(&count));
        v.push_back(S(&count));
        count = 0;
        T w = v;
        EXPECT_EQ(count, 0); // copying them is trivial
        T x = std::move(v);
        EXPECT_EQ(count, 0); // moving them is trivial
        EXPECT_EQ(v.size(), 2);
        v.clear(); w = v;
        EXPECT_EQ(count, 0); // destroying them is trivial
        v = x;
        EXPECT_EQ(count, 0); // copying them is trivial
        v = x;
        EXPECT_EQ(count, 2); // should have copy-assigned two S objects
        v = std::move(x);
        EXPECT_EQ(count, 2); // move-assigning them is trivial
        EXPECT_EQ(x.size(), 2);
        x.clear(); x = std::move(v);
        EXPECT_EQ(count, 2); // move-constructing them is trivial
        EXPECT_EQ(v.size(), 2);
        v.clear(); x = std::move(v);
        EXPECT_EQ(count, 2); // destroying them is trivial
        EXPECT_TRUE(v.empty() && w.empty() && x.empty());
    }
    {
        struct S {
            int *p_ = nullptr;
            S(int *p) : p_(p) {}
            S(const S& s) = default;
            S(S&&) = default;
            S& operator=(const S&) = default;
            S& operator=(S&&) = default;
            ~S() { *p_ += 1; }
        };
        using T = sg14::inplace_vector<S, 10>;
        static_assert(!std::is_trivially_copyable_v<T>);
        // T's non-trivial dtor prevents `is_trivially_copy_constructible`,
        // even though T's copy constructor itself is trivial.
        static_assert(!std::is_trivially_copy_constructible_v<T>);
        static_assert(!std::is_trivially_copy_assignable_v<T>);
        static_assert(!std::is_trivially_move_constructible_v<T>);
        static_assert(!std::is_trivially_move_assignable_v<T>);
        static_assert(!std::is_trivially_destructible_v<T>);
        T v;
        int count = 0;
        v.push_back(S(&count));
        v.push_back(S(&count));
        count = 0;
        T w = v;
        EXPECT_EQ(count, 0); // copying them is trivial
        T x = std::move(v);
        EXPECT_EQ(count, 0); // moving them is trivial
        EXPECT_EQ(v.size(), 2);
        v.clear();
        EXPECT_EQ(count, 2); // should have destroyed two S objects
        w = v;
        EXPECT_EQ(count, 4); // should have destroyed two S objects
        v = x;
        EXPECT_EQ(count, 4); // copying them is trivial
        v = x;
        EXPECT_EQ(count, 4); // copy-assigning them is trivial
        v = std::move(x);
        EXPECT_EQ(count, 4); // move-assigning them is trivial
        EXPECT_EQ(x.size(), 2);
        x.clear();
        EXPECT_EQ(count, 6); // should have destroyed two S objects
        x = std::move(v);
        EXPECT_EQ(count, 6); // move-constructing them is trivial
        EXPECT_EQ(v.size(), 2);
        v.clear();
        EXPECT_EQ(count, 8); // should have destroyed two S objects
        x = std::move(v);
        EXPECT_EQ(count, 10); // should have destroyed two S objects
        EXPECT_TRUE(v.empty() && w.empty() && x.empty());
    }
}

#endif // __cplusplus >= 201703L
