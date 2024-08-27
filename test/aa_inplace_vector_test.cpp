#if __cplusplus >= 202002L

#include <exception>
#include <gtest/gtest.h>
#include <set>
#include <utility>

struct AssertFail : std::runtime_error {
    using std::runtime_error::runtime_error;
};

#define SG14_INPLACE_VECTOR_ASSERT_PRECONDITION(x, msg) if (!(x)) throw AssertFail(msg);

#include <sg14/aa_inplace_vector.h>

template<class T>
struct CountingAlloc {
    using value_type = T;
    std::multiset<void*> *addresses_ = nullptr;

    explicit CountingAlloc(std::multiset<void*> *addresses) : addresses_(addresses) {}
    friend bool operator==(const CountingAlloc&, const CountingAlloc&) = default;

    template<class U, class... Args>
    void construct(U *p, Args&&... args) {
        if (addresses_->find(p) != addresses_->end()) {
            throw std::runtime_error("Constructing one object over top of another!");
        }
        ::new (p) U(std::forward<Args>(args)...);
        addresses_->insert(p);
    }

    template<class U>
    void destroy(U *p) {
        if (addresses_->find(p) == addresses_->end()) {
            throw std::runtime_error("Constructing one object over top of another!");
        }
        p->~U();
        addresses_->erase(p);
    }
};

template<class T, bool B>
struct NonNoexceptAlloc {
    using value_type = T;
    using propagate_on_container_copy_assignment = std::bool_constant<B>;
    using propagate_on_container_move_assignment = std::bool_constant<B>;
    using propagate_on_container_swap = std::bool_constant<B>;
    using is_always_equal = std::false_type;
    explicit NonNoexceptAlloc(int);
    NonNoexceptAlloc(const NonNoexceptAlloc&); // non-throwing but not noexcept
    NonNoexceptAlloc(NonNoexceptAlloc&&); // non-throwing but not noexcept
    NonNoexceptAlloc& operator=(const NonNoexceptAlloc&); // non-throwing but not noexcept
    NonNoexceptAlloc& operator=(NonNoexceptAlloc&&); // non-throwing but not noexcept
    ~NonNoexceptAlloc() noexcept(false); // non-throwing but not noexcept
    friend bool operator==(NonNoexceptAlloc, NonNoexceptAlloc) { return true; } // non-throwing but not noexcept
    int i_;
};

template<class T>
struct PropagatingAllocator {
    using value_type = T;
    using propagate_on_container_copy_assignment = std::true_type;
    using propagate_on_container_move_assignment = std::true_type;
    using propagate_on_container_swap = std::true_type;
    explicit PropagatingAllocator(int i) : i_(i) {}
    int i_ = 0;
    friend bool operator==(const PropagatingAllocator&, const PropagatingAllocator&) = default;

    template<class U, class... Args>
    void construct(U *p, Args&&... args) {
        ::new (p) U(std::forward<Args>(args)...);
    }

    template<class U>
    void destroy(U *p) {
        p->~U();
    }
};

TEST(aa_inplace_vector, AllocExtendedCopyCtor)
{
    using A = CountingAlloc<int>;
    using V = sg14::inplace_vector<int, 10, A>;
    std::multiset<void*> mr1;
    std::multiset<void*> mr2;
    V v1({1,2,3}, A(&mr1));
    V v2(v1, A(&mr2));
    EXPECT_EQ(v1, v2);
    EXPECT_EQ(v1.get_allocator(), A(&mr1));
    EXPECT_EQ(v2.get_allocator(), A(&mr2));
    EXPECT_EQ(mr1.size(), 3u);
    EXPECT_EQ(mr2.size(), 3u);
}

TEST(aa_inplace_vector, AllocExtendedMoveCtor)
{
    using A = CountingAlloc<int>;
    using V = sg14::inplace_vector<int, 10, A>;
    std::multiset<void*> mr1;
    std::multiset<void*> mr2;
    V v1({1,2,3}, A(&mr1));
    V v2(std::move(v1), A(&mr2));
    EXPECT_EQ(v1.get_allocator(), A(&mr1));
    EXPECT_EQ(v2, V({1,2,3}, A(&mr1)));
    EXPECT_EQ(v2.get_allocator(), A(&mr2));
    EXPECT_EQ(mr1.size(), v1.size());
    EXPECT_EQ(mr2.size(), 3u);
}

TEST(aa_inplace_vector, IgnoreNoexceptnessOfAllocator)
{
    {
        using T = sg14::inplace_vector<int, 10, NonNoexceptAlloc<int, true>>;
        static_assert(std::is_nothrow_copy_constructible_v<T>);
        static_assert(std::is_nothrow_move_constructible_v<T>);
        static_assert(!std::is_nothrow_copy_assignable_v<T>); // because Lakos rule
        static_assert(!std::is_nothrow_move_assignable_v<T>); // because Lakos rule
        static_assert(std::is_nothrow_destructible_v<T>);
    }
    {
        using T = sg14::inplace_vector<int, 10, NonNoexceptAlloc<int, false>>;
        static_assert(std::is_nothrow_copy_constructible_v<T>);
        static_assert(std::is_nothrow_move_constructible_v<T>);
        static_assert(std::is_nothrow_copy_assignable_v<T>);
        static_assert(std::is_nothrow_move_assignable_v<T>);
        static_assert(std::is_nothrow_destructible_v<T>);
    }
}

TEST(aa_inplace_vector, SwapAllocators)
{
    using A = PropagatingAllocator<int>;
    static_assert(std::allocator_traits<A>::propagate_on_container_swap::value);
    {
        using T = sg14::inplace_vector<int, 4, A>;
        T a = T(A(1));
        T b = T(A(1));
        a.swap(b);
        swap(a, b);
        T c = T(A(2));
        try { a.swap(c); EXPECT_TRUE(false); } catch (const AssertFail& ex) { EXPECT_STREQ(ex.what(), "swap tried to swap unequal allocators; this is UB"); }
        try { swap(a, c); EXPECT_TRUE(false); } catch (const AssertFail& ex) { EXPECT_STREQ(ex.what(), "swap tried to swap unequal allocators; this is UB"); }
    }
    {
        using T = sg14::inplace_vector<int, 0, A>;
        T a = T(A(1));
        T b = T(A(1));
        a.swap(b);
        swap(a, b);
        T c = T(A(2));
        try { a.swap(c); EXPECT_TRUE(false); } catch (const AssertFail& ex) { EXPECT_STREQ(ex.what(), "swap tried to swap unequal allocators; this is UB"); }
        try { swap(a, c); EXPECT_TRUE(false); } catch (const AssertFail& ex) { EXPECT_STREQ(ex.what(), "swap tried to swap unequal allocators; this is UB"); }
    }
}

TEST(aa_inplace_vector, CopyAssignAllocators)
{
    using A = PropagatingAllocator<int>;
    static_assert(std::allocator_traits<A>::propagate_on_container_copy_assignment::value);
    {
        using T = sg14::inplace_vector<int, 4, A>;
        static_assert(!std::is_trivially_copy_assignable_v<T>);
        T a = T(A(1));
        T b = T(A(1));
        a = b;
        T c = T(A(2));
        try { a = c; EXPECT_TRUE(false); } catch (const AssertFail& ex) { EXPECT_STREQ(ex.what(), "operator= tried to propagate an unequal allocator; this is UB"); }
    }
    {
        using T = sg14::inplace_vector<int, 0, A>;
        static_assert(!std::is_trivially_copy_assignable_v<T>);
        T a = T(A(1));
        T b = T(A(1));
        a = b;
        T c = T(A(2));
        try { a = c; EXPECT_TRUE(false); } catch (const AssertFail& ex) { EXPECT_STREQ(ex.what(), "operator= tried to propagate an unequal allocator; this is UB"); }
    }
}

TEST(aa_inplace_vector, MoveAssignAllocators)
{
    using A = PropagatingAllocator<int>;
    static_assert(std::allocator_traits<A>::propagate_on_container_move_assignment::value);
    {
        using T = sg14::inplace_vector<int, 4, A>;
        static_assert(!std::is_trivially_move_assignable_v<T>);
        T a = T(A(1));
        T b = T(A(1));
        a = std::move(b);
        T c = T(A(2));
        try { a = std::move(c); EXPECT_TRUE(false); } catch (const AssertFail& ex) { EXPECT_STREQ(ex.what(), "operator= tried to propagate an unequal allocator; this is UB"); }
    }
    {
        using T = sg14::inplace_vector<int, 0, A>;
        static_assert(!std::is_trivially_move_assignable_v<T>);
        T a = T(A(1));
        T b = T(A(1));
        a = std::move(b);
        T c = T(A(2));
        try { a = std::move(c); EXPECT_TRUE(false); } catch (const AssertFail& ex) { EXPECT_STREQ(ex.what(), "operator= tried to propagate an unequal allocator; this is UB"); }
    }
}

#endif // __cplusplus >= 202002L
