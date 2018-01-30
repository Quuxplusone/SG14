#include "SG14_test.h"
#include "any_ref.h"
#include <cassert>
#include <memory>
#include <string>
#include <type_traits>

static void test_struct_layout()
{
    static_assert(std::alignment_of< stdext::any_ref >::value == alignof(void*), "");
    static_assert(sizeof( stdext::any_ref ) == 2 * sizeof(void*), "");
}

template<class T>
T& as_lvalue(T&& t) { return static_cast<T&>(t); }

template<class ExpectedT>
static void test_passing_to_function(stdext::any_cref a, ExpectedT expectedValue)
{
    static_assert(std::is_same<decltype(stdext::any_cast<ExpectedT>(&a)), const ExpectedT*>::value, "");
    static_assert(std::is_same<decltype(stdext::any_cast<const ExpectedT>(&a)), const ExpectedT*>::value, "");
    static_assert(std::is_same<decltype(stdext::any_cast<const ExpectedT&>(a)), const ExpectedT&>::value, "");
    static_assert(std::is_same<decltype(stdext::any_cast<const ExpectedT&&>(a)), const ExpectedT&&>::value, "");

    assert(a.has_value());
    assert(a.type() == typeid(ExpectedT));
    assert(*stdext::any_cast<ExpectedT>(&a) == expectedValue);
    assert(stdext::any_cast<ExpectedT*>(&a) == nullptr);
    assert(stdext::any_cast<ExpectedT>(a) == expectedValue);
    bool caught = false;
    try {
        ExpectedT *p = stdext::any_cast<ExpectedT*>(a);
        (void)p;
    } catch (const stdext::bad_any_cast&) {
        caught = true;
    }
    assert(caught);
}

template<class ExpectedT>
static void test2_passing_to_function(stdext::any_ref a, ExpectedT expectedValue)
{
    static_assert(std::is_same<decltype(stdext::any_cast<ExpectedT>(&a)), ExpectedT*>::value, "");
    static_assert(std::is_same<decltype(stdext::any_cast<const ExpectedT>(&a)), const ExpectedT*>::value, "");
    static_assert(std::is_same<decltype(stdext::any_cast<ExpectedT&>(a)), ExpectedT&>::value, "");
    static_assert(std::is_same<decltype(stdext::any_cast<const ExpectedT&>(a)), const ExpectedT&>::value, "");
    static_assert(std::is_same<decltype(stdext::any_cast<ExpectedT&&>(a)), ExpectedT&&>::value, "");
    static_assert(std::is_same<decltype(stdext::any_cast<const ExpectedT&&>(a)), const ExpectedT&&>::value, "");

    assert(a.has_value());
    assert(a.type() == typeid(ExpectedT));
    assert(*stdext::any_cast<ExpectedT>(&a) == expectedValue);
    assert(stdext::any_cast<ExpectedT*>(&a) == nullptr);
    assert(stdext::any_cast<ExpectedT>(a) == expectedValue);
    bool caught = false;
    try {
        ExpectedT *p = stdext::any_cast<ExpectedT*>(a);
        (void)p;
    } catch (const stdext::bad_any_cast&) {
        caught = true;
    }
    assert(caught);
}

static void test_empty()
{
    stdext::any_ref a;
    assert(!a.has_value());
    assert(a.type() == typeid(void));
}

static void test_reseating()
{
    int i = 42;
    stdext::any_cref ca, cb;
    stdext::any_ref a;
    cb = std::cref(i);
    assert(cb.type() == typeid(int));
    assert(&stdext::any_cast<const int&>(cb) == &i);
    ca = cb;
    assert(ca.type() == typeid(int));
    assert(&stdext::any_cast<const int&>(ca) == &i);
    a = std::ref(i);
    assert(a.type() == typeid(int));
    assert(&stdext::any_cast<int&>(a) == &i);
    ca = a;
    assert(ca.type() == typeid(int));
    assert(&stdext::any_cast<const int&>(ca) == &i);
}

static void test_constness_example()
{
    using T = int;
    T t = 42;
    const T ct = 43;
    T& rt1 = stdext::any_cast<T&>(stdext::any_ref(std::ref(t)));
    const T& rct1 = stdext::any_cast<const T&>(std::ref(ct));
    const T& rct2 = stdext::any_cast<const T&>(std::cref(ct));
    assert(&rt1 == &t);
    assert(&rct1 == &ct);
    assert(&rct2 == &ct);
}

static void test_volatile_misuse()
{
    // Laundering through any_ref to remove `volatile` is permitted.
    volatile int v;
    stdext::any_ref r = std::ref(v);
    int *nvp = stdext::any_cast<int>(&r);
    assert(nvp == (int*)&v);

    // Laundering through any_ref to add `volatile` is permitted.
    int nv;
    r = std::ref(nv);
    volatile int *vp = stdext::any_cast<volatile int>(&r);
    assert(vp == (int*)&nv);
}

void sg14_test::any_ref_test()
{
    static_assert(std::is_nothrow_default_constructible< stdext::any_ref >::value, "");
    static_assert(std::is_nothrow_copy_constructible< stdext::any_ref >::value, "");
    static_assert(std::is_nothrow_move_constructible< stdext::any_ref >::value, "");
    static_assert(std::is_nothrow_copy_assignable< stdext::any_ref >::value, "");
    static_assert(std::is_nothrow_move_assignable< stdext::any_ref >::value, "");
    static_assert(std::is_nothrow_destructible< stdext::any_ref >::value, "");
    static_assert(std::is_trivially_copy_constructible< stdext::any_ref >::value, "");
    static_assert(std::is_trivially_move_constructible< stdext::any_ref >::value, "");
    static_assert(std::is_trivially_copy_assignable< stdext::any_ref >::value, "");
    static_assert(std::is_trivially_move_assignable< stdext::any_ref >::value, "");
    static_assert(std::is_trivially_destructible< stdext::any_ref >::value, "");
#if __cplusplus >= 201703L
    static_assert(std::is_nothrow_swappable< stdext::any_ref& >::value, "");
#endif
    static_assert(std::is_trivially_copyable< stdext::any_ref >::value, "");

    test_struct_layout();

    assert(stdext::any_cast<int>((stdext::any_ref*)nullptr) == nullptr);

    int i = 42;
    const int ci = 43;
    test_passing_to_function(std::ref(i), 42);
    test_passing_to_function(std::ref(ci), 43);
    test_passing_to_function(std::cref(i), 42);
    test_passing_to_function(std::cref(ci), 43);
    test_passing_to_function(std::cref(as_lvalue((long)i)), 42L);
    test_passing_to_function(std::cref(as_lvalue(nullptr)), nullptr);

    test2_passing_to_function(std::ref(i), 42);
    test2_passing_to_function(std::ref(as_lvalue((long)i)), 42L);
    test2_passing_to_function(std::ref(as_lvalue(nullptr)), nullptr);

    test_empty();
    test_reseating();
    test_constness_example();
    test_volatile_misuse();
}

#ifdef TEST_MAIN
int main()
{
    sg14_test::any_ref_test();
}
#endif
