#if __cplusplus >= 202002L

#include <gtest/gtest.h>
#include <sg14/aa_inplace_vector.h>

#define IPV_TEST_NAME aa_inplace_vector_pmr
#define sg14 sg14::pmr
#include "inplace_vector_common_tests.cpp"
#undef sg14

// Work around old libc++ versions that fail to implement LWG3683.
#define EXPECT_EQ_ALLOC(alloc, mr) EXPECT_EQ(alloc, static_cast<decltype(alloc)>(mr))

struct ScopedSetDefaultResource {
    explicit ScopedSetDefaultResource(std::pmr::memory_resource *mr) : old_(std::pmr::set_default_resource(mr)) {}
    ~ScopedSetDefaultResource() { std::pmr::set_default_resource(old_); }
    std::pmr::memory_resource *old_;
};

TEST(IPV_TEST_NAME, TrivialTraits)
{
    {
        using T = sg14::pmr::inplace_vector<int, 10>;
        static_assert(!std::is_trivially_copyable_v<T>);
        static_assert(!std::is_trivially_copy_constructible_v<T>); // because SOCCC
        static_assert(std::is_trivially_move_constructible_v<T>);
        static_assert(!std::is_trivially_copy_assignable_v<T>);
        static_assert(!std::is_trivially_move_assignable_v<T>);
        static_assert(std::is_trivially_destructible_v<T>);
        static_assert(!std::is_nothrow_copy_constructible_v<T>); // because SOCCC
        static_assert(std::is_nothrow_move_constructible_v<T>);
        static_assert(std::is_nothrow_copy_assignable_v<T>);
        static_assert(std::is_nothrow_move_assignable_v<T>);
        static_assert(std::is_nothrow_destructible_v<T>);
#if defined(__cpp_lib_trivially_relocatable)
        static_assert(!std::is_trivially_relocatable_v<T>);
#endif // __cpp_lib_trivially_relocatable
    }
    {
        using T = sg14::pmr::inplace_vector<std::pmr::vector<int>, 10>;
        static_assert(!std::is_trivially_copyable_v<T>);
        static_assert(!std::is_trivially_copy_constructible_v<T>);
        static_assert(!std::is_trivially_move_constructible_v<T>);
        static_assert(!std::is_trivially_copy_assignable_v<T>);
        static_assert(!std::is_trivially_move_assignable_v<T>);
        static_assert(!std::is_trivially_destructible_v<T>);
        static_assert(!std::is_nothrow_copy_constructible_v<T>);
        static_assert(!std::is_nothrow_move_constructible_v<T>); // because pmr::vector isn't nothrow move-assignable
        static_assert(!std::is_nothrow_copy_assignable_v<T>);
        static_assert(!std::is_nothrow_move_assignable_v<T>);
        static_assert(std::is_nothrow_destructible_v<T>);
#if defined(__cpp_lib_trivially_relocatable)
        static_assert(!std::is_trivially_relocatable_v<T>);
#endif // __cpp_lib_trivially_relocatable
    }
    {
        using T = sg14::pmr::inplace_vector<MoveOnly, 10>;
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
        using T = sg14::pmr::inplace_vector<MoveOnlyNT, 10>;
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
        using T = sg14::pmr::inplace_vector<int, 0>;
        static_assert(!std::is_trivially_copyable_v<T>);
        static_assert(!std::is_trivially_copy_constructible_v<T>); // because SOCCC
        static_assert(std::is_trivially_move_constructible_v<T>);
        static_assert(!std::is_trivially_copy_assignable_v<T>);
        static_assert(!std::is_trivially_move_assignable_v<T>);
        static_assert(std::is_trivially_destructible_v<T>);
        static_assert(!std::is_nothrow_copy_constructible_v<T>); // because SOCCC
        static_assert(std::is_nothrow_move_constructible_v<T>);
        static_assert(std::is_nothrow_copy_assignable_v<T>);
        static_assert(std::is_nothrow_move_assignable_v<T>);
        static_assert(std::is_nothrow_destructible_v<T>);
#if defined(__cpp_lib_trivially_relocatable)
        static_assert(!std::is_trivially_relocatable_v<T>);
#endif // __cpp_lib_trivially_relocatable
    }
    {
        using T = sg14::pmr::inplace_vector<std::vector<int>, 0>;
        static_assert(!std::is_trivially_copyable_v<T>);
        static_assert(!std::is_trivially_copy_constructible_v<T>); // because SOCCC
        static_assert(std::is_trivially_move_constructible_v<T>);
        static_assert(!std::is_trivially_copy_assignable_v<T>);
        static_assert(!std::is_trivially_move_assignable_v<T>);
        static_assert(std::is_trivially_destructible_v<T>);
        static_assert(!std::is_nothrow_copy_constructible_v<T>); // because SOCCC
        static_assert(std::is_nothrow_move_constructible_v<T>);
        static_assert(std::is_nothrow_copy_assignable_v<T>);
        static_assert(std::is_nothrow_move_assignable_v<T>);
        static_assert(std::is_nothrow_destructible_v<T>);
#if defined(__cpp_lib_trivially_relocatable)
        static_assert(!std::is_trivially_relocatable_v<T>);
#endif // __cpp_lib_trivially_relocatable
    }
    {
        using T = sg14::pmr::inplace_vector<std::unique_ptr<int>, 0>;
        static_assert(!std::is_trivially_copyable_v<T>);
        static_assert(!std::is_trivially_copy_constructible_v<T>);
        static_assert(std::is_trivially_move_constructible_v<T>);
        static_assert(!std::is_trivially_copy_assignable_v<T>);
        static_assert(!std::is_trivially_move_assignable_v<T>);
        static_assert(std::is_trivially_destructible_v<T>);
        static_assert(!std::is_copy_constructible_v<T>);
        static_assert(std::is_nothrow_move_constructible_v<T>);
        static_assert(!std::is_copy_assignable_v<T>);
        static_assert(std::is_nothrow_move_assignable_v<T>);
        static_assert(std::is_nothrow_destructible_v<T>);
#if defined(__cpp_lib_trivially_relocatable)
        static_assert(!std::is_trivially_relocatable_v<T>);
#endif // __cpp_lib_trivially_relocatable
    }
}

TEST(IPV_TEST_NAME, PartiallyTrivialTraits)
{
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
        using T = sg14::pmr::inplace_vector<S, 10>;
        static_assert(!std::is_trivially_copyable_v<T>);
        static_assert(!std::is_trivially_copy_constructible_v<T>);
        static_assert(!std::is_trivially_copy_assignable_v<T>);
        static_assert(std::is_trivially_move_constructible_v<T>);
        static_assert(!std::is_trivially_move_assignable_v<T>); // because Lakos rule
        static_assert(std::is_trivially_destructible_v<T>);
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
        using T = sg14::pmr::inplace_vector<S, 10>;
        static_assert(!std::is_trivially_copyable_v<T>);
        static_assert(!std::is_trivially_copy_constructible_v<T>); // because SOCCC
        static_assert(!std::is_trivially_copy_assignable_v<T>);
        static_assert(std::is_trivially_move_constructible_v<T>);
        static_assert(!std::is_trivially_move_assignable_v<T>); // because Lakos rule
        static_assert(std::is_trivially_destructible_v<T>);
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
        using T = sg14::pmr::inplace_vector<S, 10>;
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

TEST(IPV_TEST_NAME, AllocConstructors)
{
    std::pmr::monotonic_buffer_resource mr;
    {
        using V = sg14::pmr::inplace_vector<int, 5>;
        long a[] = {1,2,3};
        V v1 = V(&mr);
        EXPECT_TRUE(v1.empty());
        EXPECT_EQ_ALLOC(v1.get_allocator(), &mr);
        // `V v2 = V({}, &mr)` would be ambiguous with the iterator-pair ctor.
        // Therefore don't test it.
        //
        V v3 = V({1,2,3}, &mr);
        EXPECT_TRUE(std::equal(v3.begin(), v3.end(), a, a+3));
        EXPECT_EQ_ALLOC(v3.get_allocator(), &mr);
        auto v4 = V(a, a+3, &mr);
        EXPECT_TRUE(std::equal(v4.begin(), v4.end(), a, a+3));
        EXPECT_EQ_ALLOC(v4.get_allocator(), &mr);
        auto iss = std::istringstream("1 2 3");
        auto v5 = V(std::istream_iterator<int>(iss), std::istream_iterator<int>(), &mr);
        EXPECT_TRUE(std::equal(v5.begin(), v5.end(), a, a+3));
        EXPECT_EQ_ALLOC(v5.get_allocator(), &mr);
        auto v6 = V(3, &mr);
        EXPECT_EQ(v6, V(3, 0));
        EXPECT_EQ_ALLOC(v6.get_allocator(), &mr);
        auto v7 = V(3, 42, &mr);
        EXPECT_EQ(v7, (V{42, 42, 42}));
        EXPECT_EQ_ALLOC(v7.get_allocator(), &mr);
    }
    {
        using V = sg14::pmr::inplace_vector<std::pmr::string, 5>;
        const char *a[] = {"1", "2", "3"};
        V v1(&mr);
        EXPECT_TRUE(v1.empty());
        EXPECT_EQ_ALLOC(v1.get_allocator(), &mr);
        V v3 = V({"1", "2", "3"}, &mr);
        EXPECT_TRUE(std::equal(v3.begin(), v3.end(), a, a+3));
        EXPECT_EQ_ALLOC(v3.get_allocator(), &mr);
        EXPECT_EQ_ALLOC(v3[0].get_allocator(), &mr);
        auto v4 = V(a, a+3, &mr);
        EXPECT_TRUE(std::equal(v4.begin(), v4.end(), a, a+3));
        EXPECT_EQ_ALLOC(v4.get_allocator(), &mr);
        EXPECT_EQ_ALLOC(v4[0].get_allocator(), &mr);
        auto iss = std::istringstream("1 2 3");
        auto v5 = V(std::istream_iterator<std::pmr::string>(iss), std::istream_iterator<std::pmr::string>(), &mr);
        EXPECT_TRUE(std::equal(v5.begin(), v5.end(), a, a+3));
        EXPECT_EQ_ALLOC(v5.get_allocator(), &mr);
        EXPECT_EQ_ALLOC(v5[0].get_allocator(), &mr);
        auto v6 = V(3, &mr);
        EXPECT_EQ(v6, V(3, ""));
        EXPECT_EQ_ALLOC(v6.get_allocator(), &mr);
        EXPECT_EQ_ALLOC(v6[0].get_allocator(), &mr);
        auto v7 = V(3, "42", &mr);
        EXPECT_EQ(v7, (V{"42", "42", "42"}));
        EXPECT_EQ_ALLOC(v7.get_allocator(), &mr);
        EXPECT_EQ_ALLOC(v7[0].get_allocator(), &mr);
    }
#if __cpp_lib_ranges >= 201911L && __cpp_lib_ranges_to_container >= 202202L
    {
        auto iss = std::istringstream("1 2 3 4");
        auto rg = std::views::istream<int>(iss);
        auto v1 = sg14::pmr::inplace_vector<int, 5>(std::from_range, rg, &mr);
        EXPECT_EQ(v1, (sg14::pmr::inplace_vector<int, 5>{1,2,3,4}));
        EXPECT_EQ_ALLOC(v1.get_allocator(), &mr);
        auto v2 = v1 | std::ranges::to<sg14::pmr::inplace_vector<long, 5>>(&mr);
        EXPECT_EQ(v2, (sg14::pmr::inplace_vector<long, 5>{1,2,3,4}));
        EXPECT_EQ_ALLOC(v2.get_allocator(), &mr);
    }
    {
        auto iss = std::istringstream("1 2 3 4");
        auto rg = std::views::istream<std::pmr::string>(iss);
        auto v1 = sg14::pmr::inplace_vector<std::pmr::string, 5>(std::from_range, rg, &mr);
        EXPECT_EQ(v1, Seq("1", "2", "3", "4"));
        EXPECT_EQ_ALLOC(v1.get_allocator(), &mr);
        EXPECT_EQ_ALLOC(v1[0].get_allocator(), &mr);
        auto v2 = v1 | std::ranges::to<sg14::pmr::inplace_vector<std::pmr::string, 5>>(&mr);
        EXPECT_EQ(v1, Seq("1", "2", "3", "4"));
        EXPECT_EQ_ALLOC(v2.get_allocator(), &mr);
        EXPECT_EQ_ALLOC(v2[0].get_allocator(), &mr);
    }
#endif // __cpp_lib_ranges >= 201911L && __cpp_lib_ranges_to_container >= 202202L
}

TEST(IPV_TEST_NAME, AllocConstructorsThrow)
{
    std::pmr::monotonic_buffer_resource mr;
    {
        using V = sg14::pmr::inplace_vector<int, 3>;
        long a[] = {1,2,3,4,5};
        ASSERT_NO_THROW(V(a, a+3, &mr));
        ASSERT_THROW(V(a, a+4), std::bad_alloc);
        ASSERT_NO_THROW(V(3, &mr));
        ASSERT_THROW(V(4, &mr), std::bad_alloc);
        ASSERT_NO_THROW(V(3, 42, &mr));
        ASSERT_THROW(V(4, 42, &mr), std::bad_alloc);
        ASSERT_NO_THROW(V({1,2,3}, &mr));
        ASSERT_THROW(V({1,2,3,4}, &mr), std::bad_alloc);
#if __cpp_lib_ranges >= 201911L && __cpp_lib_ranges_to_container >= 202202L
        auto iss = std::istringstream("1 2 3 4");
        auto rg = std::views::istream<int>(iss);
        ASSERT_THROW(V(std::from_range, rg, &mr), std::bad_alloc);
#endif // __cpp_lib_ranges >= 201911L && __cpp_lib_ranges_to_container >= 202202L
    }
}


TEST(IPV_TEST_NAME, AllocCopying)
{
    std::pmr::monotonic_buffer_resource mr1;
    std::pmr::monotonic_buffer_resource mr2;
    std::pmr::monotonic_buffer_resource mr3;
    ScopedSetDefaultResource guard(&mr2);
    {
        using V = sg14::pmr::inplace_vector<int, 5>;
        V source({1,2,3}, &mr1);
        V dest = source;
        EXPECT_EQ(dest, (V{1,2,3}));
        EXPECT_EQ_ALLOC(dest.get_allocator(), &mr2);
        ScopedSetDefaultResource guard2(&mr3);
        dest = {4,5};
        EXPECT_EQ(dest, (V{4,5}));
        EXPECT_EQ_ALLOC(dest.get_allocator(), &mr2);
        dest = source;
        EXPECT_EQ(dest, (V{1,2,3}));
        EXPECT_EQ_ALLOC(dest.get_allocator(), &mr2);
        EXPECT_EQ(source, (V{1,2,3}));
        EXPECT_EQ_ALLOC(source.get_allocator(), &mr1);
    }
    {
        using V = sg14::pmr::inplace_vector<std::pmr::string, 5>;
        V source({"1", "2", "3"}, &mr1);
        V dest = source;
        EXPECT_EQ(dest, Seq("1", "2", "3"));
        EXPECT_EQ_ALLOC(dest.get_allocator(), &mr2);
        EXPECT_EQ_ALLOC(dest[0].get_allocator(), &mr2);
        EXPECT_EQ_ALLOC(dest[1].get_allocator(), &mr2);
        EXPECT_EQ_ALLOC(dest[2].get_allocator(), &mr2);
        ScopedSetDefaultResource guard2(&mr3);
        dest = {"4", "5"};
        EXPECT_EQ(dest, Seq("4", "5"));
        EXPECT_EQ_ALLOC(dest.get_allocator(), &mr2);
        EXPECT_EQ_ALLOC(dest[0].get_allocator(), &mr2);
        EXPECT_EQ_ALLOC(dest[1].get_allocator(), &mr2);
        dest = source;
        EXPECT_EQ(dest, Seq("1", "2", "3"));
        EXPECT_EQ_ALLOC(dest.get_allocator(), &mr2);
        EXPECT_EQ_ALLOC(dest[0].get_allocator(), &mr2);
        EXPECT_EQ_ALLOC(dest[1].get_allocator(), &mr2);
        EXPECT_EQ_ALLOC(dest[2].get_allocator(), &mr2);
        EXPECT_EQ(source, Seq("1", "2", "3"));
        EXPECT_EQ_ALLOC(source.get_allocator(), &mr1);
    }
}

TEST(IPV_TEST_NAME, AllocMoveConstruction)
{
    std::pmr::monotonic_buffer_resource mr1;
    std::pmr::monotonic_buffer_resource mr2;
    std::pmr::monotonic_buffer_resource mr3;
    ScopedSetDefaultResource guard(&mr2);
    {
        // Trivially copyable value_type
        using V = sg14::pmr::inplace_vector<int, 10>;
        V source({1, 2, 3}, &mr1);
        V dest = std::move(source);
        EXPECT_EQ(dest, (V{1,2,3}));
        EXPECT_EQ_ALLOC(dest.get_allocator(), &mr1); // allocator is propagated
        EXPECT_EQ_ALLOC(source.get_allocator(), &mr1); // source allocator is unchanged
        // source elements can be trivially copied-from
        EXPECT_EQ(source, (V{1,2,3}));
    }
    {
        // Move-only, trivially relocatable (not trivially copyable) value_type
        using V = sg14::pmr::inplace_vector<std::unique_ptr<int>, 10>;
        V source(3, &mr1);
        source[0] = std::make_unique<int>(1);
        source[1] = std::make_unique<int>(2);
        source[2] = std::make_unique<int>(3);
        V dest = std::move(source);
        EXPECT_EQ(dest.size(), 3u);
        EXPECT_EQ(*dest[0], 1);
        EXPECT_EQ(*dest[1], 2);
        EXPECT_EQ(*dest[2], 3);
        EXPECT_EQ_ALLOC(dest.get_allocator(), &mr1); // allocator is propagated
        EXPECT_EQ_ALLOC(source.get_allocator(), &mr1); // source allocator is unchanged
#if defined(__cpp_lib_trivially_relocatable)
        // source elements can be relocated-from
        EXPECT_TRUE(std::is_trivially_relocatable_v<std::unique_ptr<int>>);
        EXPECT_TRUE(source.empty());
#else
        // source is moved-from
        EXPECT_EQ(source.size(), 3u);
        EXPECT_EQ(source[0], nullptr);
        EXPECT_EQ(source[1], nullptr);
        EXPECT_EQ(source[2], nullptr);
#endif
    }
    {
        // Variously trivially relocatable (but never trivially copyable) value_type
        using V = sg14::pmr::inplace_vector<std::string, 10>;
        V source({"1", "2", "3"}, &mr1);
        V dest = std::move(source);
        EXPECT_EQ(dest, Seq("1", "2", "3"));
        EXPECT_EQ_ALLOC(dest.get_allocator(), &mr1); // allocator is propagated
        EXPECT_EQ_ALLOC(source.get_allocator(), &mr1); // source allocator is unchanged
    }
    {
        // PMR (non-trivially-relocatable) value_type
        using V = sg14::pmr::inplace_vector<std::pmr::string, 10>;
        V source({"1", "2", "3"}, &mr1);
        V dest = std::move(source);
        EXPECT_EQ(dest, Seq("1", "2", "3"));
        EXPECT_EQ_ALLOC(dest.get_allocator(), &mr1); // allocator is propagated
        EXPECT_EQ_ALLOC(dest[0].get_allocator(), &mr1); // allocator is propagated
        EXPECT_EQ_ALLOC(source.get_allocator(), &mr1); // source allocator is unchanged
        if (!source.empty()) {
            EXPECT_EQ(source.size(), 3u);
            EXPECT_EQ_ALLOC(source[0].get_allocator(), &mr1);
        }
    }
}

TEST(IPV_TEST_NAME, AllocNoexceptnessOfSwap)
{
    using std::swap;
    {
        sg14::pmr::inplace_vector<std::string, 10> v;
        static_assert(std::is_nothrow_move_assignable_v<std::string>);
        static_assert(std::is_nothrow_swappable_v<std::string>);
        static_assert(noexcept(v.swap(v))); // because the allocator doesn't propagate
        static_assert(noexcept(swap(v, v)));
    }
    {
        sg14::pmr::inplace_vector<std::pmr::string, 10> v;
        static_assert(!std::is_nothrow_move_assignable_v<std::pmr::string>);
        static_assert(std::is_nothrow_swappable_v<std::pmr::string>);
        static_assert(!noexcept(v.swap(v))); // because allocator-extended move-construction might throw
        static_assert(!noexcept(swap(v, v)));
    }
    {
        sg14::pmr::inplace_vector<std::pmr::string, 0> v;
        static_assert(noexcept(v.swap(v)));
        static_assert(noexcept(swap(v, v)));
    }
}

TEST(IPV_TEST_NAME, AllocInsertSingle)
{
    std::pmr::monotonic_buffer_resource mr1;
    std::pmr::monotonic_buffer_resource mr2;
    {
        using V = sg14::pmr::inplace_vector<std::pmr::string, 5>;
        V v({"abc", "def", "ghi"}, &mr1);
        ScopedSetDefaultResource(nullptr);
        auto it = v.insert(v.begin(), std::pmr::string("xyz", &mr2));
        EXPECT_EQ(it, v.begin());
        EXPECT_EQ(v, Seq("xyz", "abc", "def", "ghi"));
        std::pmr::string lvalue("wxy", &mr2);
        it = v.insert(v.begin() + 2, lvalue);
        EXPECT_EQ(it, v.begin() + 2);
        EXPECT_EQ(v, Seq("xyz", "abc", "wxy", "def", "ghi"));
        ASSERT_THROW(v.insert(v.begin() + 2, std::pmr::string("wxy", &mr2)), std::bad_alloc);
        ASSERT_THROW(v.insert(v.end(), lvalue), std::bad_alloc);
        EXPECT_EQ_ALLOC(v.get_allocator(), &mr1);
        EXPECT_EQ_ALLOC(v[0].get_allocator(), &mr1);
        EXPECT_EQ_ALLOC(v[1].get_allocator(), &mr1);
        EXPECT_EQ_ALLOC(v[2].get_allocator(), &mr1);
        EXPECT_EQ_ALLOC(v[3].get_allocator(), &mr1);
        EXPECT_EQ_ALLOC(v[4].get_allocator(), &mr1);
        it = v.erase(v.begin() + 1);
        EXPECT_EQ(it, v.begin() + 1);
        EXPECT_EQ(v, Seq("xyz", "wxy", "def", "ghi"));
        it = v.insert(v.end(), std::pmr::string("jkl", &mr2));
        EXPECT_EQ(it, v.end() - 1);
        EXPECT_EQ_ALLOC(it->get_allocator(), &mr1);
        EXPECT_EQ(v, Seq("xyz", "wxy", "def", "ghi", "jkl"));
        EXPECT_EQ(v.size(), 5u);
    }
}

TEST(IPV_TEST_NAME, AllocInsertMulti)
{
    std::pmr::monotonic_buffer_resource mr1;
    std::pmr::monotonic_buffer_resource mr2;
    {
        using V = sg14::pmr::inplace_vector<std::pmr::string, 5>;
        V v({"1", "2", "3"}, &mr1);
        ScopedSetDefaultResource(nullptr);
        const char *a[2] = {"4", "5"};
        auto it = v.insert(v.begin(), a, a+2);
        EXPECT_EQ(it, v.begin());
        EXPECT_EQ(v, Seq("4", "5", "1", "2", "3"));
        it = v.insert(v.begin(), a, a);
        EXPECT_EQ(it, v.begin());
        EXPECT_EQ(v, Seq("4", "5", "1", "2", "3"));
        EXPECT_EQ_ALLOC(v.get_allocator(), &mr1);
        EXPECT_EQ_ALLOC(v[0].get_allocator(), &mr1);
        EXPECT_EQ_ALLOC(v[1].get_allocator(), &mr1);
        EXPECT_EQ_ALLOC(v[2].get_allocator(), &mr1);
        EXPECT_EQ_ALLOC(v[3].get_allocator(), &mr1);
        EXPECT_EQ_ALLOC(v[4].get_allocator(), &mr1);
        ASSERT_THROW(v.insert(v.begin(), a, a+2), std::bad_alloc);
        v.clear();
        it = v.insert(v.begin(), a, a+2);
        EXPECT_EQ(it, v.begin());
        it = v.insert(v.end(), {std::pmr::string("1", &mr2), std::pmr::string("2", &mr2)}); // insert(initializer_list)
        EXPECT_EQ(it, v.begin() + 2);
        EXPECT_EQ(v, Seq("4", "5", "1", "2"));
        EXPECT_EQ_ALLOC(v.get_allocator(), &mr1);
        EXPECT_EQ_ALLOC(v[0].get_allocator(), &mr1);
        EXPECT_EQ_ALLOC(v[1].get_allocator(), &mr1);
        EXPECT_EQ_ALLOC(v[2].get_allocator(), &mr1);
        EXPECT_EQ_ALLOC(v[3].get_allocator(), &mr1);
        ASSERT_THROW(v.insert(v.begin() + 2, a, a+2), std::bad_alloc);
        EXPECT_LE(v.size(), 5u);
    }
}

TEST(IPV_TEST_NAME, AllocAssign)
{
    std::pmr::monotonic_buffer_resource mr1;
    std::pmr::monotonic_buffer_resource mr2;
    ScopedSetDefaultResource guard(&mr2);
    {
        using V = sg14::pmr::inplace_vector<std::pmr::string, 5>;
        ScopedSetDefaultResource guard2(nullptr);
        V v(&mr1);
        const char *a[] = {"1", "2", "3", "4", "5", "6"};
        v.assign(a, a+2);
        EXPECT_EQ(v, Seq("1", "2"));
        v.assign(a+2, a+6);
        EXPECT_EQ(v, Seq("3", "4", "5", "6"));
        EXPECT_EQ_ALLOC(v.get_allocator(), &mr1);
        EXPECT_EQ_ALLOC(v[0].get_allocator(), &mr1);
        EXPECT_EQ_ALLOC(v[1].get_allocator(), &mr1);
        EXPECT_EQ_ALLOC(v[2].get_allocator(), &mr1);
        EXPECT_EQ_ALLOC(v[3].get_allocator(), &mr1);
        v.assign(a, a+2);
        EXPECT_EQ(v, Seq("1", "2"));
        ASSERT_THROW(v.assign(a, a+6), std::bad_alloc);
        EXPECT_LE(v.size(), 5u);
    }
    {
        using V = sg14::pmr::inplace_vector<std::pmr::string, 5>;
        V v(&mr1);
        auto iss = std::istringstream("1 2");
        v.assign(std::istream_iterator<std::pmr::string>(iss), {});
        EXPECT_EQ(v, Seq("1", "2"));
        iss = std::istringstream("4 5 6 7");
        v.assign(std::istream_iterator<std::pmr::string>(iss), {});
        EXPECT_EQ(v, Seq("4", "5", "6", "7"));
        EXPECT_EQ_ALLOC(v.get_allocator(), &mr1);
        EXPECT_EQ_ALLOC(v[0].get_allocator(), &mr1);
        EXPECT_EQ_ALLOC(v[1].get_allocator(), &mr1);
        EXPECT_EQ_ALLOC(v[2].get_allocator(), &mr1);
        EXPECT_EQ_ALLOC(v[3].get_allocator(), &mr1);
        iss = std::istringstream("1 2");
        v.assign(std::istream_iterator<std::pmr::string>(iss), {});
        EXPECT_EQ(v, Seq("1", "2"));
        iss = std::istringstream("1 2 3 4 5 6");
        ASSERT_THROW(v.assign(std::istream_iterator<std::pmr::string>(iss), {}), std::bad_alloc);
        EXPECT_LE(v.size(), 5u);
    }
}

TEST(IPV_TEST_NAME, AllocAssignRange)
{
    std::pmr::monotonic_buffer_resource mr1;
    std::pmr::monotonic_buffer_resource mr2;
    ScopedSetDefaultResource guard(&mr2);
#if __cpp_lib_ranges >= 201911L && __cpp_lib_ranges_to_container >= 202202L
    {
        using V = sg14::pmr::inplace_vector<std::pmr::string, 5>;
        V v(&mr1);
        v.assign_range(std::vector<const char*>{"1", "2"});
        EXPECT_EQ(v, Seq("1", "2"));
        v.assign_range(std::vector<std::pmr::string>{"4", "5", "6", "7"});
        EXPECT_EQ(v, Seq("4", "5", "6", "7"));
        EXPECT_EQ_ALLOC(v.get_allocator(), &mr1);
        EXPECT_EQ_ALLOC(v[0].get_allocator(), &mr1); // v uses its own allocator
        EXPECT_EQ_ALLOC(v[1].get_allocator(), &mr1); // v uses its own allocator
        EXPECT_EQ_ALLOC(v[2].get_allocator(), &mr1); // v uses its own allocator
        EXPECT_EQ_ALLOC(v[3].get_allocator(), &mr1); // v uses its own allocator
        v.assign_range(std::pmr::vector<std::pmr::string>{"1", "2"});
        EXPECT_EQ(v, Seq("1", "2"));
        ASSERT_THROW(v.assign_range(std::pmr::vector<std::pmr::string>(6)), std::bad_alloc);
        EXPECT_LE(v.size(), 5u);
    }
    {
        using V = sg14::pmr::inplace_vector<std::pmr::string, 5>;
        auto iss = std::istringstream("4 5 6 7");
        V v({"1", "2"}, &mr1);
        v.assign_range(std::views::istream<std::pmr::string>(iss) | std::views::take(3));
        EXPECT_EQ(v, Seq("4", "5", "6"));
        EXPECT_EQ_ALLOC(v.get_allocator(), &mr1);
        EXPECT_EQ_ALLOC(v[0].get_allocator(), &mr1); // v uses its own allocator
        EXPECT_EQ_ALLOC(v[1].get_allocator(), &mr1); // v uses its own allocator
        EXPECT_EQ_ALLOC(v[2].get_allocator(), &mr1); // v uses its own allocator
        iss = std::istringstream("6 7");
        v.assign_range(std::views::istream<std::pmr::string>(iss) | std::views::take(2));
        EXPECT_EQ(v, Seq("6", "7"));
        iss = std::istringstream("6 7 8 9 10 11");
        ASSERT_THROW(v.assign_range(std::views::istream<std::pmr::string>(iss) | std::views::take(6)), std::bad_alloc);
        EXPECT_LE(v.size(), 5u);
    }
#endif // __cpp_lib_ranges >= 201911L && __cpp_lib_ranges_to_container >= 202202L
}

TEST(IPV_TEST_NAME, AllocInsertRange)
{
    std::pmr::monotonic_buffer_resource mr1;
    std::pmr::monotonic_buffer_resource mr2;
    ScopedSetDefaultResource guard(&mr2);
#if __cpp_lib_ranges >= 201911L && __cpp_lib_ranges_to_container >= 202202L
    {
        using V = sg14::pmr::inplace_vector<std::pmr::string, 5>;
        auto iss = std::istringstream("4 5 6 7");
        V v = V({"1", "2"}, &mr1);
        auto it = v.insert_range(v.begin() + 1, std::views::istream<std::pmr::string>(iss) | std::views::take(2));
        EXPECT_EQ(it, v.begin() + 1);
        EXPECT_EQ(v, Seq("1", "4", "5", "2"));
        EXPECT_EQ_ALLOC(v.get_allocator(), &mr1);
        EXPECT_EQ_ALLOC(v[0].get_allocator(), &mr1); // v uses its own allocator
        EXPECT_EQ_ALLOC(v[1].get_allocator(), &mr1); // v uses its own allocator
        EXPECT_EQ_ALLOC(v[2].get_allocator(), &mr1); // v uses its own allocator
        EXPECT_EQ_ALLOC(v[3].get_allocator(), &mr1); // v uses its own allocator
        iss = std::istringstream("6 7");
        ASSERT_THROW(v.insert_range(v.begin() + 1, std::views::istream<std::pmr::string>(iss) | std::views::take(2)), std::bad_alloc);
        EXPECT_EQ(v, Seq("1", "4", "5", "2"));
    }
#endif // __cpp_lib_ranges >= 201911L && __cpp_lib_ranges_to_container >= 202202L
}

TEST(IPV_TEST_NAME, AllocAppendRange)
{
    std::pmr::monotonic_buffer_resource mr1;
    std::pmr::monotonic_buffer_resource mr2;
    ScopedSetDefaultResource guard(&mr2);
#if __cpp_lib_ranges >= 201911L && __cpp_lib_ranges_to_container >= 202202L
    {
        using V = sg14::pmr::inplace_vector<std::pmr::string, 5>;
        auto iss = std::istringstream("4 5 6 7");
        V v = V({"1", "2"}, &mr1);
        v.append_range(std::views::istream<std::pmr::string>(iss) | std::views::take(2));
        EXPECT_EQ(v, Seq("1", "2", "4", "5"));
        EXPECT_EQ_ALLOC(v.get_allocator(), &mr1);
        EXPECT_EQ_ALLOC(v[0].get_allocator(), &mr1); // v uses its own allocator
        EXPECT_EQ_ALLOC(v[1].get_allocator(), &mr1); // v uses its own allocator
        EXPECT_EQ_ALLOC(v[2].get_allocator(), &mr1); // v uses its own allocator
        EXPECT_EQ_ALLOC(v[3].get_allocator(), &mr1); // v uses its own allocator
        iss = std::istringstream("6 7");
        ASSERT_THROW(v.append_range(std::views::istream<std::pmr::string>(iss) | std::views::take(2)), std::bad_alloc);
        EXPECT_LE(v.size(), 5u);
    }
#endif // __cpp_lib_ranges >= 201911L && __cpp_lib_ranges_to_container >= 202202L
}

TEST(IPV_TEST_NAME, AllocAssignFromInitList)
{
    std::pmr::monotonic_buffer_resource mr1;
    std::pmr::monotonic_buffer_resource mr2;
    ScopedSetDefaultResource guard(&mr2);
    {
        using V = sg14::pmr::inplace_vector<std::pmr::string, 4>;
        V v = V({"a", "b"}, &mr1);
        v = {"c"};
        EXPECT_EQ(v, Seq("c"));
        EXPECT_EQ_ALLOC(v.get_allocator(), &mr1);
        EXPECT_EQ_ALLOC(v[0].get_allocator(), &mr1);
        v = {"d", "e", "f", "g"};
        EXPECT_EQ(v, Seq("d", "e", "f", "g"));
        EXPECT_EQ_ALLOC(v.get_allocator(), &mr1);
        EXPECT_EQ_ALLOC(v[0].get_allocator(), &mr1);
        EXPECT_EQ_ALLOC(v[1].get_allocator(), &mr1);
        EXPECT_EQ_ALLOC(v[2].get_allocator(), &mr1);
        EXPECT_EQ_ALLOC(v[3].get_allocator(), &mr1);
        ASSERT_THROW((v = {{}, {}, {}, {}, {}, {}}), std::bad_alloc);
    }
}

TEST(IPV_TEST_NAME, AllocatorDoesntAffectComparison)
{
    std::pmr::monotonic_buffer_resource mr1;
    std::pmr::monotonic_buffer_resource mr2;
    sg14::pmr::inplace_vector<int, 4> a(&mr1);
    sg14::pmr::inplace_vector<int, 4> b(&mr2);
    EXPECT_TRUE((a == b) && (a <= b) && (a >= b));
    EXPECT_FALSE((a != b) || (a < b) || (a > b));
#if __cpp_impl_three_way_comparison >= 201907L
    EXPECT_EQ(a <=> b, std::strong_ordering::equal);
#endif
    a = {1,2,3};
    b = {1,2,3};
    EXPECT_TRUE((a == b) && (a <= b) && (a >= b));
    EXPECT_FALSE((a != b) || (a < b) || (a > b));
#if __cpp_impl_three_way_comparison >= 201907L
    EXPECT_EQ(a <=> b, std::strong_ordering::equal);
#endif
    a = {1,2,3};
    b = {1};
    EXPECT_TRUE((a != b) && (a > b) && (a >= b));
    EXPECT_TRUE((b != a) && (b < a) && (b <= a));
    EXPECT_FALSE((a == b) || (a < b) || (a <= b));
    EXPECT_FALSE((b == a) || (b > a) || (b >= a));
#if __cpp_impl_three_way_comparison >= 201907L
    EXPECT_EQ(a <=> b, std::strong_ordering::greater);
    EXPECT_EQ(b <=> a, std::strong_ordering::less);
#endif
    a = {1,3};
    b = {1,2,3};
    EXPECT_TRUE((a != b) && (a > b) && (a >= b));
    EXPECT_TRUE((b != a) && (b < a) && (b <= a));
    EXPECT_FALSE((a == b) || (a < b) || (a <= b));
    EXPECT_FALSE((b == a) || (b > a) || (b >= a));
#if __cpp_impl_three_way_comparison >= 201907L
    EXPECT_EQ(a <=> b, std::strong_ordering::greater);
    EXPECT_EQ(b <=> a, std::strong_ordering::less);
#endif
    a = {1,2,4};
    b = {1,2,3};
    EXPECT_TRUE((a != b) && (a > b) && (a >= b));
    EXPECT_TRUE((b != a) && (b < a) && (b <= a));
    EXPECT_FALSE((a == b) || (a < b) || (a <= b));
    EXPECT_FALSE((b == a) || (b > a) || (b >= a));
#if __cpp_impl_three_way_comparison >= 201907L
    EXPECT_EQ(a <=> b, std::strong_ordering::greater);
    EXPECT_EQ(b <=> a, std::strong_ordering::less);
#endif
}

TEST(IPV_TEST_NAME, AllocResize)
{
    {
        std::pmr::monotonic_buffer_resource mr1;
        std::pmr::monotonic_buffer_resource mr2;
        ScopedSetDefaultResource guard(nullptr);
        sg14::pmr::inplace_vector<std::pmr::string, 4> v(&mr1);
        v.resize(2);
        EXPECT_EQ(v, Seq("", ""));
        EXPECT_EQ_ALLOC(v.get_allocator(), &mr1);
        EXPECT_EQ_ALLOC(v[0].get_allocator(), &mr1);
        EXPECT_EQ_ALLOC(v[1].get_allocator(), &mr1);
        v.resize(1, std::pmr::string("a", &mr2));
        EXPECT_EQ(v, Seq(""));
        EXPECT_EQ_ALLOC(v[0].get_allocator(), &mr1);
        v.resize(3, std::pmr::string("b", &mr2));
        EXPECT_EQ(v, Seq("", "b", "b"));
        EXPECT_EQ_ALLOC(v[0].get_allocator(), &mr1);
        EXPECT_EQ_ALLOC(v[1].get_allocator(), &mr1);
        EXPECT_EQ_ALLOC(v[2].get_allocator(), &mr1);
        v.resize(4);
        EXPECT_EQ(v, Seq("", "b", "b", ""));
        v.resize(2, "c");
        EXPECT_EQ(v, Seq("", "b"));
        ASSERT_THROW(v.resize(5), std::bad_alloc);
        ASSERT_THROW(v.resize(6, std::pmr::string("d", &mr2)), std::bad_alloc);
        EXPECT_EQ(v, Seq("", "b"));
    }
}

#endif // __cplusplus >= 202002L
