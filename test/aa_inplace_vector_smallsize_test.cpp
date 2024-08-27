#if __cplusplus >= 202002L

#include <sg14/aa_inplace_vector.h>
#include <type_traits>

template<class T, class SizeType>
struct SmallSizeAllocator {
    using value_type = T;
    using size_type = std::make_unsigned_t<SizeType>;
    using difference_type = std::make_signed_t<SizeType>;

    // Because inplace_vector never allocates memory, the `allocate` and
    // `deallocate` members can be omitted. This makes `SmallSizeAllocator`
    // not-an-allocator from the Standard's point of view. SG14 is happy
    // with it, though.
};
static_assert(std::allocator_traits<SmallSizeAllocator<int, int>>::is_always_equal::value);

namespace smallsize {
  template<class T, size_t N> using inplace_vector = sg14::inplace_vector<T, N, SmallSizeAllocator<T, unsigned char>>;
}

#define IPV_TEST_NAME aa_inplace_vector_smallsize
#define sg14 smallsize
#include "inplace_vector_common_tests.cpp"
#undef sg14

TEST(IPV_TEST_NAME, TrivialTraits)
{
    {
        using T = smallsize::inplace_vector<int, 10>;
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
        static_assert(std::is_nothrow_swappable_v<T>);
#if defined(__cpp_lib_trivially_relocatable)
        static_assert(std::is_trivially_relocatable_v<T>);
#endif // __cpp_lib_trivially_relocatable
    }
    {
        using T = smallsize::inplace_vector<std::vector<int>, 10>;
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
        static_assert(std::is_nothrow_swappable_v<T>);
#if defined(__cpp_lib_trivially_relocatable)
        static_assert(std::is_trivially_relocatable_v<T>);
#endif // __cpp_lib_trivially_relocatable
    }
    {
        constexpr bool msvc = !std::is_nothrow_move_constructible_v<std::list<int>>;
        using T = smallsize::inplace_vector<std::list<int>, 10>;
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
        static_assert(std::is_nothrow_swappable_v<T> == !msvc);
#if defined(__cpp_lib_trivially_relocatable)
        static_assert(std::is_trivially_relocatable_v<T> == msvc);
#endif // __cpp_lib_trivially_relocatable
    }
    {
        using T = smallsize::inplace_vector<MoveOnly, 10>;
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
        static_assert(std::is_nothrow_swappable_v<T>);
#if defined(__cpp_lib_trivially_relocatable)
        static_assert(std::is_trivially_relocatable_v<T>);
#endif // __cpp_lib_trivially_relocatable
    }
    {
        using T = smallsize::inplace_vector<MoveOnlyNT, 10>;
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
        static_assert(std::is_nothrow_swappable_v<T>);
#if defined(__cpp_lib_trivially_relocatable)
        static_assert(!std::is_trivially_relocatable_v<T>);
#endif // __cpp_lib_trivially_relocatable
    }
    {
        using T = smallsize::inplace_vector<int, 0>;
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
        static_assert(std::is_nothrow_swappable_v<T>);
#if defined(__cpp_lib_trivially_relocatable)
        static_assert(std::is_trivially_relocatable_v<T>);
#endif // __cpp_lib_trivially_relocatable
    }
    {
        using T = smallsize::inplace_vector<std::vector<int>, 0>;
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
        static_assert(std::is_nothrow_swappable_v<T>);
#if defined(__cpp_lib_trivially_relocatable)
        static_assert(std::is_trivially_relocatable_v<T>);
#endif // __cpp_lib_trivially_relocatable
    }
    {
        using T = smallsize::inplace_vector<std::list<int>, 0>;
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
        static_assert(std::is_nothrow_swappable_v<T>);
#if defined(__cpp_lib_trivially_relocatable)
        static_assert(std::is_trivially_relocatable_v<T>);
#endif // __cpp_lib_trivially_relocatable
    }
    {
        using T = smallsize::inplace_vector<std::unique_ptr<int>, 0>;
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
        static_assert(std::is_nothrow_swappable_v<T>);
#if defined(__cpp_lib_trivially_relocatable)
        static_assert(std::is_trivially_relocatable_v<T>);
#endif // __cpp_lib_trivially_relocatable
    }
}

TEST(IPV_TEST_NAME, PartiallyTrivialTraits)
{
    constexpr bool ConditionallyTrivial = true;
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
        using T = smallsize::inplace_vector<S, 10>;
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
        using T = smallsize::inplace_vector<S, 10>;
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
        using T = smallsize::inplace_vector<S, 10>;
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

TEST(IPV_TEST_NAME, SizeAndAlignment)
{
#ifdef _MSC_VER
    // Our `ipv_alloc_holder` has a single [[no_unique_address]] member.
    // Even using [[msvc::no_unique_address]], MSVC fails to treat
    // `ipv_alloc_holder` as an empty base for the purposes of optimization,
    // which means it occupies an extra byte compared to the Itanium ABI.
    //
    static constexpr bool msvc = true;
#else
    static constexpr bool msvc = false;
#endif

    static_assert(sizeof(sg14::inplace_vector<char, 22>) == 32);
    static_assert(alignof(sg14::inplace_vector<char, 22>) == 8);
    static_assert(sizeof(sg14::pmr::inplace_vector<char, 22>) == 40);
    static_assert(alignof(sg14::pmr::inplace_vector<char, 22>) == 8);
    static_assert(sizeof(smallsize::inplace_vector<char, 22>) == (msvc ? 24 : 23));
    static_assert(alignof(smallsize::inplace_vector<char, 22>) == 1);

    static_assert(sizeof(smallsize::inplace_vector<char, 2>) == (msvc ? 4 : 3));
    static_assert(alignof(smallsize::inplace_vector<char, 2>) == 1);

    static_assert(sizeof(sg14::inplace_vector<char, 2, SmallSizeAllocator<char, short>>) == (msvc ? 6 : 4));
    static_assert(alignof(sg14::inplace_vector<char, 2, SmallSizeAllocator<char, short>>) == 2);
}

TEST(IPV_TEST_NAME, Capacity)
{
    using T = smallsize::inplace_vector<char, 2>;
    static_assert(std::is_same_v<decltype(T().capacity()), unsigned char>);
    static_assert(std::is_same_v<decltype(T().max_size()), unsigned char>);
    static_assert(std::is_same_v<decltype(T().size()), unsigned char>);
    static_assert(T::capacity() == 2);
    static_assert(T::max_size() == 2);
    static_assert(T().capacity() == 2);
    static_assert(T().max_size() == 2);
}

TEST(IPV_TEST_NAME, Smoke)
{
    smallsize::inplace_vector<char, 2> a = {1, 2};
    smallsize::inplace_vector<char, 2> b = {3};
    EXPECT_TRUE(a < b);
    EXPECT_FALSE(a == b);
    EXPECT_FALSE(a < a);
    EXPECT_TRUE(a == a);
    EXPECT_EQ(a.size(), 2);
    EXPECT_EQ(b.size(), 1);
    b.erase(b.begin());
    EXPECT_TRUE(b.empty());
    EXPECT_THROW(a.push_back(3), std::bad_alloc);
}

#endif // __cplusplus >= 202002L
