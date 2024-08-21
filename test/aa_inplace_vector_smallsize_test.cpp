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

namespace smallsize {
  template<class T, size_t N> using inplace_vector = sg14::inplace_vector<T, N, SmallSizeAllocator<T, unsigned char>>;
}

#define IPV_TEST_NAME aa_inplace_vector_smallsize
#define IPV_HAS_CONDITIONALLY_TRIVIAL_SMFS 1
#define sg14 smallsize
#include "inplace_vector_common_tests.cpp"
#undef sg14

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
