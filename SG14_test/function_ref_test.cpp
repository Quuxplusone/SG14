#include "SG14_test.h"
#include "function_ref.h"
#include <cassert>
#include <memory>
#include <string>
#include <type_traits>

namespace {

static int copied, moved, called_with;
static int expected;

struct Functor {
    Functor() {}
    Functor(const Functor&) { copied += 1; }
    Functor(Functor&&) { moved += 1; }
    void operator()(int i) { assert(i == expected); called_with = i; }
};

struct ConstFunctor {
    ConstFunctor() {}
    ConstFunctor(const ConstFunctor&) { copied += 1; }
    ConstFunctor(ConstFunctor&&) { moved += 1; }
    void operator()(int i) const { assert(i == expected); called_with = i; }
};

void Foo(int i)
{
    assert(i == expected);
    called_with = i;
}

} // anonymous namespace

#define EXPECT_EQ(val1, val2) assert(val1 == val2)
#define EXPECT_TRUE(val) assert(val)
#define EXPECT_FALSE(val) assert(!val)

std::string gLastS;
int gLastI = 0;
double gNextReturn = 0.0;

double GlobalFunction(const std::string& s, int i)
{
    gLastS = s;
    gLastI = i;
    return gNextReturn;
}

void FunctionPointer()
{
    // Even compatible function pointers require an appropriate amount of "storage".
    using CompatibleFunctionType = std::remove_reference_t<decltype(GlobalFunction)>;
    stdext::function_ref<CompatibleFunctionType> fun(&GlobalFunction);

    EXPECT_TRUE(bool(fun));

    gNextReturn = 7.77;

    double r = fun("hello", 42);

    EXPECT_EQ(gNextReturn, r);
    EXPECT_EQ("hello", gLastS);
    EXPECT_EQ(42, gLastI);
}

void Lambda()
{
    stdext::function_ref<double(int)> fun;
    std::string closure("some closure");
    fun = [&closure](int x) { return GlobalFunction(closure, x); };

    gNextReturn = 7.77;

    double r = fun(42);

    EXPECT_EQ(gNextReturn, r);
    EXPECT_EQ(closure, gLastS);
    EXPECT_EQ(42, gLastI);
}

void Bind()
{
    stdext::function_ref<double(int)> fun;
    std::string closure("some closure");
    auto bound = std::bind(GlobalFunction, closure, std::placeholders::_1);
    fun = std::ref(bound);

    gNextReturn = 7.77;

    double r = fun(42);

    EXPECT_EQ(gNextReturn, r);
    EXPECT_EQ(closure, gLastS);
    EXPECT_EQ(42, gLastI);
}

void Copying()
{
    auto sptr = std::make_shared<int>(42);
    EXPECT_EQ(1, sptr.use_count());

    auto lambda = [sptr]() { return *sptr; };
    EXPECT_EQ(2, sptr.use_count());

    stdext::function_ref<int()> fun1 = lambda;
    stdext::function_ref<int()> fun2;

    EXPECT_EQ(2, sptr.use_count());
    EXPECT_TRUE(bool(fun1));
    EXPECT_FALSE(bool(fun2));

    fun2 = fun1;
    EXPECT_EQ(2, sptr.use_count());
    EXPECT_TRUE(bool(fun1));
    EXPECT_TRUE(bool(fun2));

    fun1 = nullptr;
    EXPECT_EQ(2, sptr.use_count());
    EXPECT_FALSE(bool(fun1));
    EXPECT_TRUE(bool(fun2));
}

void ContainingStdFunction()
{
    // build a big closure, bigger than 32 bytes
    uint64_t offset1 = 1234;
    uint64_t offset2 = 77;
    uint64_t offset3 = 666;
    std::string str1 = "12345";

    std::function<int(const std::string&)> stdfun
                    = [offset1, offset2, offset3, str1](const std::string& str)
    {
        return int(offset1 + offset2 + offset3 + str1.length() + str.length());
    };

    stdext::function_ref<int(const std::string&)> fun = stdfun;

    int r = fun("123");
    EXPECT_EQ(r, int(offset1+offset2+offset3+str1.length()+3));
}

void AssignmentDifferentFunctor()
{
    int calls = 0;
    stdext::function_ref<int(int,int)> add = [&calls] (int a, int b) { ++calls; return a+b; };
    stdext::function_ref<int(int,int)> mul = [&calls] (int a, int b) { ++calls; return a*b; };

    int r1 = add(3, 5);
    EXPECT_EQ(8, r1);

    int r2 = mul(2, 5);
    EXPECT_EQ(10, r2);

    EXPECT_EQ(2, calls);

    add = mul;

    int r3 = add(3, 5);
    EXPECT_EQ(15, r3);

    int r4 = mul(2, 5);
    EXPECT_EQ(10, r4);

    EXPECT_EQ(4, calls);
}

static void test_struct_layout()
{
    static_assert(std::alignment_of< stdext::function_ref<void(int)> >::value == alignof(void*), "");
    static_assert(sizeof( stdext::function_ref<void(int)> ) == 2 * sizeof(void*), "");
}

static void test_nullptr()
{
    using IPF = stdext::function_ref<void()>;
    auto nil = nullptr;
    const auto cnil = nullptr;

    IPF f;                    assert(not bool(f));
    f = nullptr;              assert(not bool(f));
    f = IPF(nullptr);         assert(not bool(f));
    f = IPF();                assert(not bool(f));
    f = IPF{};                assert(not bool(f));
    f = {};                   assert(not bool(f));
    f = nil;                  assert(not bool(f));
    f = IPF(nil);             assert(not bool(f));
    f = IPF(std::move(nil));  assert(not bool(f));
    f = cnil;                 assert(not bool(f));
    f = IPF(cnil);            assert(not bool(f));
    f = IPF(std::move(cnil)); assert(not bool(f));
}

void set_called(bool& called) { called = true; }

void test_passing_to_function(stdext::function_ref<void(bool&)> ref)
{
    bool called = false;
    ref(called);
    assert(called);
}

template<bool shouldConst>
struct CaresAboutConst {
    void operator()(bool& called) { called = !shouldConst; }
    void operator()(bool& called) const { called = shouldConst; }
};

template<class T>
decltype(auto) as_const(T&& t) { return static_cast<const T&&>(t); }

void sg14_test::function_ref_test()
{
    // first set of tests (copied from inplace_function)
    AssignmentDifferentFunctor();
    FunctionPointer();
    Lambda();
    Bind();
    Copying();
    ContainingStdFunction();

    // second set of tests
    using IPF = stdext::function_ref<void(int)>;
    static_assert(std::is_nothrow_default_constructible<IPF>::value, "");
    static_assert(std::is_nothrow_copy_constructible<IPF>::value, "");
    static_assert(std::is_nothrow_move_constructible<IPF>::value, "");
    static_assert(std::is_nothrow_copy_assignable<IPF>::value, "");
    static_assert(std::is_nothrow_move_assignable<IPF>::value, "");
    static_assert(std::is_trivially_copy_constructible<IPF>::value, "");
    static_assert(std::is_trivially_move_constructible<IPF>::value, "");
    static_assert(std::is_trivially_copy_assignable<IPF>::value, "");
    static_assert(std::is_trivially_move_assignable<IPF>::value, "");
#if __cplusplus >= 201703L
    static_assert(std::is_nothrow_swappable<IPF&>::value, "");
    static_assert(std::is_invocable<const IPF&, int>::value, "");
    static_assert(std::is_invocable_r<void, const IPF&, int>::value, "");
#endif
    static_assert(std::is_nothrow_destructible<IPF>::value, "");
    static_assert(std::is_trivially_destructible<IPF>::value, "");
    static_assert(std::is_trivially_copyable<IPF>::value, "");

    test_struct_layout();

    IPF func;
    assert(!func);
    assert(!bool(func));
    assert(func == nullptr);
    assert(!(func != nullptr));
    expected = 0; try { func(42); } catch (std::bad_function_call&) { expected = 1; } assert(expected == 1);

    func = Foo;
    assert(!!func);
    assert(func);
    assert(!(func == nullptr));
    assert(func != nullptr);
    called_with = 0; expected = 42; func(42); assert(called_with == 42);

    func = nullptr;
    assert(!func);
    assert(!bool(func));
    assert(func == nullptr);
    assert(!(func != nullptr));
    expected = 0; try { func(42); } catch (std::bad_function_call&) { expected = 1; } assert(expected == 1);

    test_nullptr();
    test_passing_to_function([](bool& called) { called = true; });
    test_passing_to_function([](bool& called) mutable { called = true; });
    test_passing_to_function(set_called);
    test_passing_to_function(&set_called);
    test_passing_to_function(std::ref(set_called));
    auto set_called_lambda = [](bool& called) { called = true; };
    test_passing_to_function(set_called_lambda);
    test_passing_to_function(std::ref(set_called_lambda));
    auto set_called_ptr = &set_called;
    test_passing_to_function(set_called_ptr);
    test_passing_to_function(std::ref(set_called_ptr));

    test_passing_to_function(as_const(CaresAboutConst<true>{}));
    test_passing_to_function(CaresAboutConst<false>{});
    const CaresAboutConst<true> cctrue{};
    CaresAboutConst<true> cctrue2;
    CaresAboutConst<false> ccfalse;
    test_passing_to_function(std::ref(cctrue));
    test_passing_to_function(std::cref(cctrue));
    test_passing_to_function(std::cref(cctrue2));
    test_passing_to_function(std::ref(ccfalse));
}

#ifdef TEST_MAIN
int main()
{
    sg14_test::function_ref_test();
}
#endif
