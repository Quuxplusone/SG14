#include <sg14/inplace_function.h>

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <type_traits>
#include <vector>

namespace {

static int copied, moved, called_with;
static int expected;

struct Functor {
    Functor() {}
    Functor(const Functor&) { copied += 1; }
    Functor(Functor&&) noexcept { moved += 1; }
    void operator()(int i) { EXPECT_TRUE(i == expected); called_with = i; }
};

struct ConstFunctor {
    ConstFunctor() {}
    ConstFunctor(const ConstFunctor&) { copied += 1; }
    ConstFunctor(ConstFunctor&&) noexcept { moved += 1; }
    void operator()(int i) const { EXPECT_TRUE(i == expected); called_with = i; }
};

void Foo(int i)
{
    EXPECT_TRUE(i == expected);
    called_with = i;
}

} // anonymous namespace

static std::string gLastS;
static int gLastI = 0;
static double gNextReturn = 0.0;

static double GlobalFunction(const std::string& s, int i)
{
    gLastS = s;
    gLastI = i;
    return gNextReturn;
}

TEST(inplace_function, ConstinitDefaultConstructible)
{
    // C++20 introduced `constinit`, which is compatible with
    // non-trivially destructible types like inplace_function.
    // C++20 also relaxed the requirement that `constexpr` variables
    // must have trivial destructors, but we don't take advantage
    // of that yet.
    //
#if __cplusplus >= 202002L
    static constinit sg14::inplace_function<void()> f;
    EXPECT_FALSE(bool(f));
    static constinit sg14::inplace_function<int(int)> g;
    EXPECT_FALSE(bool(g));
#endif
}

TEST(inplace_function, FunctionPointer)
{
    // Even compatible function pointers require an appropriate amount of "storage".
    using CompatibleFunctionType = std::remove_reference_t<decltype(GlobalFunction)>;
    sg14::inplace_function<CompatibleFunctionType, sizeof(CompatibleFunctionType*)> fun(&GlobalFunction);

    EXPECT_TRUE(bool(fun));

    gNextReturn = 7.77;

    double r = fun("hello", 42);

    EXPECT_EQ(gNextReturn, r);
    EXPECT_EQ("hello", gLastS);
    EXPECT_EQ(42, gLastI);
}

TEST(inplace_function, Lambda)
{
    sg14::inplace_function<double(int), 8> fun;
    std::string closure("some closure");
    fun = [&closure](int x) { return GlobalFunction(closure, x); };

    gNextReturn = 7.77;

    double r = fun(42);

    EXPECT_EQ(gNextReturn, r);
    EXPECT_EQ(closure, gLastS);
    EXPECT_EQ(42, gLastI);
}

TEST(inplace_function, StdBind)
{
    sg14::inplace_function<double(int), 64> fun;
    std::string closure("some closure");
    fun = std::bind(GlobalFunction, closure, std::placeholders::_1);

    gNextReturn = 7.77;

    double r = fun(42);

    EXPECT_EQ(gNextReturn, r);
    EXPECT_EQ(closure, gLastS);
    EXPECT_EQ(42, gLastI);
}

namespace {
struct AnotherFunctor
{
    int mTotal = 0;
    static int mDestructorCalls;
    static int mConstructorCalls;
    int operator()(int x) { mTotal += x; return mTotal; }

    AnotherFunctor() { mConstructorCalls++; }
    AnotherFunctor(AnotherFunctor&&) noexcept { mConstructorCalls++; }
    AnotherFunctor(const AnotherFunctor&) { mConstructorCalls++; }
    ~AnotherFunctor() { mDestructorCalls++; }
};
int AnotherFunctor::mDestructorCalls = 0;
int AnotherFunctor::mConstructorCalls = 0;
} // namespace

TEST(inplace_function, FunctorDestruction)
{
    AnotherFunctor::mDestructorCalls = 0;
    AnotherFunctor::mConstructorCalls = 0;
    {
        AnotherFunctor ftor;
        sg14::inplace_function<int(int), 4> fun(ftor);

        int r1 = fun(1);
        int r2 = fun(7);

        EXPECT_EQ(1, r1);
        EXPECT_EQ(8, r2);
    }
    EXPECT_EQ(AnotherFunctor::mDestructorCalls, AnotherFunctor::mConstructorCalls);

    AnotherFunctor::mDestructorCalls = 0;
    AnotherFunctor::mConstructorCalls = 0;
    {
        AnotherFunctor ftor;
        sg14::inplace_function<int(int), 4> fun(ftor);
        sg14::inplace_function<int(int), 4> fun2(fun);  // copy-ctor
        sg14::inplace_function<int(int), 4> fun3(std::move(fun));  // move-ctor
        fun3 = fun2;  // copy-asgn
        fun3 = std::move(fun2);  // move-asgn
    }
    EXPECT_EQ(AnotherFunctor::mDestructorCalls, AnotherFunctor::mConstructorCalls);
}

TEST(inplace_function, Swapping)
{
    AnotherFunctor::mDestructorCalls = 0;
    AnotherFunctor::mConstructorCalls = 0;
    {
        AnotherFunctor ftor;
        auto lambda = [](int x){ return x + 10; };
        sg14::inplace_function<int(int), 4> fun(ftor);
        sg14::inplace_function<int(int), 4> fun2(lambda);

        fun.swap(fun2);  // swap...
        fun2.swap(fun);  // ...and swap back

        int r1 = fun(1);
        int r2 = fun(7);
        EXPECT_EQ(1, r1);
        EXPECT_EQ(8, r2);

        int r3 = fun2(1);
        int r4 = fun2(7);
        EXPECT_EQ(11, r3);
        EXPECT_EQ(17, r4);
    }
    EXPECT_EQ(AnotherFunctor::mDestructorCalls, AnotherFunctor::mConstructorCalls);
}

TEST(inplace_function, Copying)
{
    auto sptr = std::make_shared<int>(42);
    EXPECT_EQ(1, sptr.use_count());

    sg14::inplace_function<int(), 16> fun1 = [sptr]() { return *sptr; };
    sg14::inplace_function<int(), 16> fun2;

    EXPECT_EQ(2, sptr.use_count());
    EXPECT_TRUE(bool(fun1));
    EXPECT_FALSE(bool(fun2));

    fun2 = fun1;
    EXPECT_EQ(3, sptr.use_count());
    EXPECT_TRUE(bool(fun1));
    EXPECT_TRUE(bool(fun2));

    // this should call destructor on existing functor
    fun1 = nullptr;
    EXPECT_EQ(2, sptr.use_count());
    EXPECT_FALSE(bool(fun1));
    EXPECT_TRUE(bool(fun2));
}

TEST(inplace_function, ContainingStdFunction)
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

    sg14::inplace_function<int(const std::string&), 64> fun = stdfun;

    int r = fun("123");
    EXPECT_EQ(r, int(offset1 + offset2 + offset3 + str1.length() + 3));
}

TEST(inplace_function, SimilarTypeCopy)
{
    auto sptr = std::make_shared<int>(42);
    EXPECT_EQ(1, sptr.use_count());

    sg14::inplace_function<int(), 16> fun1 = [sptr]() { return *sptr; };
    sg14::inplace_function<int(), 17> fun2(fun1); // fun1 is bigger than 17, but we should be smart about it
    sg14::inplace_function<int(), 18> fun3;

    EXPECT_EQ(3, sptr.use_count());
    EXPECT_FALSE(fun3);
    fun3 = fun2;
    EXPECT_EQ(4, sptr.use_count());
    EXPECT_TRUE(bool(fun2));
    EXPECT_TRUE(bool(fun3));

    fun1 = nullptr;
    fun2 = nullptr;
    EXPECT_EQ(2, sptr.use_count());
    fun3 = nullptr;
    EXPECT_EQ(1, sptr.use_count());

    sg14::inplace_function<int(), 17> fun4;
    fun4 = fun1; // fun1 is bigger than 17, but we should be smart about it
}

TEST(inplace_function, AssignmentFromDifferentFunctor)
{
    int calls = 0;
    sg14::inplace_function<int(int,int), 16> add = [&calls] (int a, int b) { ++calls; return a+b; };
    sg14::inplace_function<int(int,int), 16> mul = [&calls] (int a, int b) { ++calls; return a*b; };

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

namespace {
struct ThrowingFunctor {
    static int countdown;
    static int constructed;
    static int destructed;
    static int called;
    static void reset(int k) {
        countdown = k;
        constructed = 0;
        destructed = 0;
        called = 0;
    }
    static void check_countdown() {
        if (countdown > 0 && --countdown == 0) {
            throw 42;
        }
    }
    ThrowingFunctor() { check_countdown(); ++constructed; }
    ThrowingFunctor(const ThrowingFunctor&) {
        check_countdown();
        ++constructed;
    }
    ThrowingFunctor(ThrowingFunctor&&) noexcept { ++constructed; }
    ThrowingFunctor& operator=(const ThrowingFunctor&) = delete;
    ThrowingFunctor& operator=(ThrowingFunctor&&) = delete;
    ~ThrowingFunctor() noexcept { ++destructed; }
    void operator()() const { ++called; }
};
int ThrowingFunctor::countdown = 0;
int ThrowingFunctor::constructed = 0;
int ThrowingFunctor::destructed = 0;
int ThrowingFunctor::called = 0;
} // namespace

TEST(inplace_function, ExceptionSafety)
{
    using IPF = sg14::inplace_function<void(), sizeof(ThrowingFunctor)>;
    ThrowingFunctor tf;
    EXPECT_EQ(ThrowingFunctor::constructed, 1);
    EXPECT_EQ(ThrowingFunctor::destructed, 0);
    EXPECT_EQ(ThrowingFunctor::called, 0);
    bool caught;

    // Copy-construction from a ThrowingFunctor might throw.
    try { tf.reset(1); caught = false; IPF a(tf); } catch (int) { caught = true; }
    EXPECT_TRUE((caught) && (tf.countdown == 0) && (tf.constructed == 0) && (tf.destructed == 0));
    try { tf.reset(2); caught = false; IPF a(tf); } catch (int) { caught = true; }
    EXPECT_TRUE((!caught) && (tf.countdown == 1) && (tf.constructed == 1) && (tf.destructed == 1));

    // Copy-construction from an IPF (containing a ThrowingFunctor) might throw.
    try { tf.reset(2); caught = false; IPF a(tf); IPF b(a); } catch (int) { caught = true; }
    EXPECT_TRUE((caught) && (tf.countdown == 0) && (tf.constructed == 1) && (tf.destructed == 1));
    try { tf.reset(3); caught = false; IPF a(tf); IPF b(a); } catch (int) { caught = true; }
    EXPECT_TRUE((!caught) && (tf.countdown == 1) && (tf.constructed == 2) && (tf.destructed == 2));

    // Move-construction and destruction should be assumed not to throw.
    try { tf.reset(1); caught = false; IPF a(std::move(tf)); IPF b(std::move(a)); } catch (int) { caught = true; }
    EXPECT_TRUE((!caught) && (tf.countdown == 1) && (tf.constructed == 2) && (tf.destructed == 2));

    // The assignment operators are implemented as "construct an IPF, then move from it"; so, one copy and one move of the ThrowingFunctor.
    try { tf.reset(1); caught = false; IPF a; a = tf; } catch (int) { caught = true; }
    EXPECT_TRUE((caught) && (tf.countdown == 0) && (tf.constructed == 0) && (tf.destructed == 0));
    try { tf.reset(2); caught = false; IPF a; a = tf; } catch (int) { caught = true; }
    EXPECT_TRUE((!caught) && (tf.countdown == 1) && (tf.constructed == 2) && (tf.destructed == 2));

    // The assignment operators are implemented as "construct an IPF, then move from it"; so, two moves of the ThrowingFunctor.
    try { tf.reset(1); caught = false; IPF a; a = std::move(tf); } catch (int) { caught = true; }
    EXPECT_TRUE((!caught) && (tf.countdown == 1) && (tf.constructed == 2) && (tf.destructed == 2));

    try { tf.reset(1); caught = false; IPF a; IPF b(tf); a = b; } catch (int) { caught = true; }
    EXPECT_TRUE((caught) && (tf.countdown == 0) && (tf.constructed == 0) && (tf.destructed == 0));
    try { tf.reset(2); caught = false; IPF a; IPF b(tf); a = b; } catch (int) { caught = true; }
    EXPECT_TRUE((caught) && (tf.countdown == 0) && (tf.constructed == 1) && (tf.destructed == 1));
}

TEST(inplace_function, StructLayout)
{
    constexpr bool linux = (alignof(long double) == 16);
    static_assert(alignof(sg14::inplace_function<void(int), 1>) == alignof(void*), "");
    static_assert(alignof(sg14::inplace_function<void(int), 2>) == alignof(void*), "");
    static_assert(alignof(sg14::inplace_function<void(int), 4>) == alignof(void*), "");
    static_assert(alignof(sg14::inplace_function<void(int), 8>) == 8, "");
    static_assert(alignof(sg14::inplace_function<void(int), 16>) == (linux ? 16 : alignof(void*)), "");
    static_assert(alignof(sg14::inplace_function<void(int), 32>) == (linux ? 16 : alignof(void*)), "");
    static_assert(sizeof( sg14::inplace_function<void(int), sizeof(void*)> ) == 2 * sizeof(void*), "");
}

TEST(inplace_function, ConvertibleFromNullptr)
{
    using IPF = sg14::inplace_function<void(int)>;
    auto nil = nullptr;
    const auto cnil = nullptr;

    IPF f;                    EXPECT_TRUE(! bool(f));
    f = nullptr;              EXPECT_TRUE(! bool(f));
    f = IPF(nullptr);         EXPECT_TRUE(! bool(f));
    f = IPF();                EXPECT_TRUE(! bool(f));
    f = IPF{};                EXPECT_TRUE(! bool(f));
    f = {};                   EXPECT_TRUE(! bool(f));
    f = nil;                  EXPECT_TRUE(! bool(f));
    f = IPF(nil);             EXPECT_TRUE(! bool(f));
    f = IPF(std::move(nil));  EXPECT_TRUE(! bool(f));
    f = cnil;                 EXPECT_TRUE(! bool(f));
    f = IPF(cnil);            EXPECT_TRUE(! bool(f));
    f = IPF(std::move(cnil)); EXPECT_TRUE(! bool(f));

    EXPECT_TRUE(!f);
    EXPECT_TRUE(f == nullptr);
    EXPECT_TRUE(!(f != nullptr));
    expected = 0; try { f(42); } catch (const std::bad_function_call&) { expected = 1; } EXPECT_TRUE(expected == 1);
}

namespace {
struct oon_functor {
    int dummy;

    oon_functor(int i) : dummy(i) {}
    int operator()(int i) { return i + dummy; }

    void *operator new(size_t, void *p) {
        EXPECT_TRUE(false);  // class-specific "new" should not be called
        return p;
    }
};
} // namespace

TEST(inplace_function, ADLProofOperatorNew)
{
    using IPF = sg14::inplace_function<int(int), 8>;
    oon_functor oon(42);
    IPF fun = oon;
    IPF fun2;
    fun2 = oon;
    fun = fun2;
    EXPECT_EQ(43, fun(1));
}

TEST(inplace_function, MoveConstructionIsNoexcept)
{
    using IPF = sg14::inplace_function<void(int), sizeof(Functor)>;
    std::vector<IPF> vec;
    vec.push_back(Functor());
    copied = 0;
    moved = 0;
    vec.reserve(vec.capacity() + 1);
    EXPECT_EQ(0, copied);
    EXPECT_EQ(1, moved);
}

TEST(inplace_function, MoveConstructionFromSmallerIsNoexcept)
{
    using IPF32 = sg14::inplace_function<void(int), 32>;
    using IPF40 = sg14::inplace_function<void(int), 40>;
    static_assert(std::is_nothrow_constructible<IPF32, IPF32&&>::value, "");
    static_assert(std::is_nothrow_assignable<IPF32, IPF32&&>::value, "");
    static_assert(std::is_nothrow_constructible<IPF40, IPF32&&>::value, "");
    static_assert(std::is_nothrow_assignable<IPF40, IPF32&&>::value, "");
}

// https://bugs.llvm.org/show_bug.cgi?id=32072
struct test_bug_32072_C;
struct test_bug_32072 {
    sg14::inplace_function<test_bug_32072_C()> m;
};
static_assert(std::is_copy_constructible<test_bug_32072>::value, "");
static_assert(std::is_nothrow_move_constructible<test_bug_32072>::value, "");

TEST(inplace_function, RvalueRefParameter)
{
    sg14::inplace_function<void(std::unique_ptr<int>&&)> f;
    f = [](std::unique_ptr<int>) {};
    f = [](std::unique_ptr<int>&&) {};
    f = [](const std::unique_ptr<int>&) {};
    f(std::make_unique<int>(42));
    sg14::inplace_function<void(std::unique_ptr<int>)> g;
    g = [](std::unique_ptr<int>) {};
    g = [](std::unique_ptr<int>&&) {};
    g = [](const std::unique_ptr<int>&) {};
    g(std::make_unique<int>(42));
}

TEST(inplace_function, IsConvertibleTrait)
{
    static_assert(std::is_convertible<int(&)(), sg14::inplace_function<int()>>::value, "");
    static_assert(std::is_convertible<int(*)(), sg14::inplace_function<int()>>::value, "");
    static_assert(std::is_convertible<int(*&)(), sg14::inplace_function<int()>>::value, "");
    static_assert(std::is_convertible<int(*&&)(), sg14::inplace_function<int()>>::value, "");
}

TEST(inplace_function, ConvertibleFromQualifiedCallOperator)
{
    struct Callable { void operator()() {} };
    struct LvalueOnlyCallable { void operator()() & {} };
    struct RvalueOnlyCallable { void operator()() && {} };
    struct ConstCallable { void operator()() const {} };
    struct ConstOnlyCallable { void operator()() const {} void operator()() = delete; };
    struct NonconstOnlyCallable { void operator()() {} void operator()() const = delete; };
    struct LvalueConstCallable { void operator()() const & {} };
    struct NoexceptCallable { void operator()() noexcept {} };
    static_assert(std::is_convertible<Callable, sg14::inplace_function<void()>>::value, "");
    static_assert(std::is_convertible<LvalueOnlyCallable, sg14::inplace_function<void()>>::value, "");
    static_assert(!std::is_convertible<RvalueOnlyCallable, sg14::inplace_function<void()>>::value, "");
    static_assert(std::is_convertible<ConstCallable, sg14::inplace_function<void()>>::value, "");
    static_assert(!std::is_convertible<ConstOnlyCallable, sg14::inplace_function<void()>>::value, "");
    static_assert(std::is_convertible<NonconstOnlyCallable, sg14::inplace_function<void()>>::value, "");
    static_assert(std::is_convertible<LvalueConstCallable, sg14::inplace_function<void()>>::value, "");
    static_assert(std::is_convertible<NoexceptCallable, sg14::inplace_function<void()>>::value, "");
}

TEST(inplace_function, ConvertibleFromLambda)
{
    struct NoDefaultCtor {
        int val;
        explicit NoDefaultCtor(int v) : val{v} {}
    };

    const auto a = []() -> int { return 3; };
    static_assert(std::is_convertible<decltype(a), sg14::inplace_function<int()>>::value, "");
    static_assert(!std::is_convertible<decltype(a), sg14::inplace_function<int(int)>>::value, "");
    static_assert(!std::is_convertible<decltype(a), sg14::inplace_function<void(int&)>>::value, "");

    const auto b = [](int&) -> void {};
    static_assert(std::is_convertible<decltype(b), sg14::inplace_function<void(int&)>>::value, "");
    static_assert(!std::is_convertible<decltype(b), sg14::inplace_function<int()>>::value, "");
    static_assert(!std::is_convertible<decltype(b), sg14::inplace_function<int(int)>>::value, "");

    const auto c = [](int, NoDefaultCtor) -> int { return 3; };
    static_assert(std::is_convertible<decltype(c), sg14::inplace_function<void(int, NoDefaultCtor)>>::value, "");
    static_assert(!std::is_convertible<decltype(c), sg14::inplace_function<int()>>::value, "");
    static_assert(!std::is_convertible<decltype(c), sg14::inplace_function<int(int)>>::value, "");

    const auto d = []() -> void {};
    static_assert(std::is_convertible<decltype(d), sg14::inplace_function<void()>>::value, "");
    static_assert(!std::is_convertible<decltype(d), sg14::inplace_function<int()>>::value, "");
    static_assert(!std::is_convertible<decltype(d), sg14::inplace_function<int(int)>>::value, "");

    static_assert(std::is_convertible<int(), sg14::inplace_function<const int&()>>::value, "");
    static_assert(std::is_convertible<int(*)(), sg14::inplace_function<const int&()>>::value, "");

    // Same as a, but not const.
    auto e = []() -> int { return 3; };
    static_assert(std::is_convertible<decltype(e), sg14::inplace_function<int()>>::value, "");
    static_assert(!std::is_convertible<decltype(e), sg14::inplace_function<int(int)>>::value, "");
    static_assert(!std::is_convertible<decltype(e), sg14::inplace_function<void(int&)>>::value, "");

    // Same as a, but not const and mutable.
    auto f = []() mutable -> int { return 3; };
    static_assert(std::is_convertible<decltype(f), sg14::inplace_function<int()>>::value, "");
    static_assert(!std::is_convertible<decltype(f), sg14::inplace_function<int(int)>>::value, "");
    static_assert(!std::is_convertible<decltype(f), sg14::inplace_function<void(int&)>>::value, "");
}

namespace {
struct InstrumentedCopyConstructor {
    static int copies;
    static int moves;
    InstrumentedCopyConstructor() = default;
    InstrumentedCopyConstructor(const InstrumentedCopyConstructor&) {
        copies += 1;
    }
    InstrumentedCopyConstructor(InstrumentedCopyConstructor&&) noexcept {
        moves += 1;
    }
};
int InstrumentedCopyConstructor::copies = 0;
int InstrumentedCopyConstructor::moves = 0;
} // anonymous namespace

TEST(inplace_function, ReturnByMove)
{
    using IPF20 = sg14::inplace_function<void(), 20>;
    using IPF40 = sg14::inplace_function<void(), 40>;
    static_assert(std::is_convertible<IPF20, IPF40>::value, "");
    static_assert(std::is_convertible<IPF20&, IPF40>::value, "");
    static_assert(std::is_convertible<IPF20&&, IPF40>::value, "");
    static_assert(std::is_convertible<const IPF20&, IPF40>::value, "");
    static_assert(std::is_convertible<const IPF20&&, IPF40>::value, "");

    auto foo = []() -> IPF40 {
        InstrumentedCopyConstructor cc;
        InstrumentedCopyConstructor::copies = 0;
        InstrumentedCopyConstructor::moves = 0;
        IPF20 f = [cc]() { };
        EXPECT_TRUE(InstrumentedCopyConstructor::copies == 1);
        EXPECT_TRUE(InstrumentedCopyConstructor::moves == 1);
        InstrumentedCopyConstructor::copies = 0;
        InstrumentedCopyConstructor::moves = 0;
        return f;
    };
    IPF40 f = foo();
    EXPECT_TRUE(InstrumentedCopyConstructor::copies == 0);
    EXPECT_TRUE(InstrumentedCopyConstructor::moves == 1);
}

TEST(inplace_function, IsInvocableTrait)
{
    using C_Int1 = int();
    using C_Int2 = int(int);
    using C_Void = void(int&);

    using sg14::inplace_function_detail::is_invocable_r;

    static_assert(is_invocable_r<int, C_Int1>::value, "");
    static_assert(! is_invocable_r<int, C_Int2>::value, "");
    static_assert(! is_invocable_r<int, C_Void>::value, "");

    static_assert(is_invocable_r<int, C_Int2, int>::value, "");
    static_assert(! is_invocable_r<int, C_Int1, int>::value, "");
    static_assert(! is_invocable_r<int, C_Void, int>::value, "");

    static_assert(is_invocable_r<void, C_Void, int&>::value, "");
    static_assert(! is_invocable_r<void, C_Int1, int&>::value, "");

    // Testing widening and narrowing conversions, and the "conversion" to void.
    static_assert(is_invocable_r<void, C_Int1>::value, "");
    static_assert(is_invocable_r<long, C_Int1>::value, "");
    static_assert(is_invocable_r<char, C_Int1>::value, "");

    // Testing the conversion from void to int, which should definitely not be allowed.
    static_assert(! is_invocable_r<int, C_Void, int&>::value, "");

    // cppreference:
    // > Determines whether Fn can be invoked with the arguments ArgTypes...
    // > to yield a result that is convertible to R.
    //
    // void is treated specially because a function's return value can be ignored.
    static_assert(is_invocable_r<void, C_Int2, int&>::value, "");
    static_assert(is_invocable_r<const void, C_Int2, int&>::value, "");

    // Regression tests for both is_invocable and is_convertible.
    static_assert(is_invocable_r<const int&, int()>::value, "");
    static_assert(is_invocable_r<const int&, int(*)()>::value, "");
}

static int overloaded_function(sg14::inplace_function<int()>) { return 1; }
static int overloaded_function(sg14::inplace_function<int(int)>) { return 2; }
TEST(inplace_function, OverloadingOnArity)
{
    EXPECT_EQ(overloaded_function([]() { return 0; }), 1);
    EXPECT_EQ(overloaded_function([](int) { return 0; }), 2);
}

static int overloaded_function2(sg14::inplace_function<int(int)>) { return 1; }
static int overloaded_function2(sg14::inplace_function<int(int*)>) { return 2; }
TEST(inplace_function, OverloadingOnParameterType)
{
    EXPECT_EQ(overloaded_function2([](int) { return 0; }), 1);
    EXPECT_EQ(overloaded_function2([](int*) { return 0; }), 2);
}

static int overloaded_function3(sg14::inplace_function<int(int)>) { return 1; }
static int overloaded_function3(sg14::inplace_function<int*(int)>) { return 2; }
TEST(inplace_function, OverloadingOnReturnType)
{
    EXPECT_EQ(overloaded_function3([](int) { return 0; }), 1);
    EXPECT_EQ(overloaded_function3([](int) { return nullptr; }), 2);
}

TEST(inplace_function, Traits)
{
    using IPF = sg14::inplace_function<void(int)>;
    static_assert(std::is_nothrow_default_constructible<IPF>::value, "");
    static_assert(std::is_copy_constructible<IPF>::value, "");
    static_assert(std::is_move_constructible<IPF>::value, "");
    static_assert(std::is_copy_assignable<IPF>::value, "");
    static_assert(std::is_move_assignable<IPF>::value, "");
#if __cplusplus >= 201703L
    static_assert(std::is_swappable<IPF&>::value, "");
    static_assert(std::is_invocable<const IPF&, int>::value, "");
    static_assert(std::is_invocable_r<void, const IPF&, int>::value, "");
#endif
    static_assert(std::is_nothrow_destructible<IPF>::value, "");

    using IPF40 = sg14::inplace_function<void(int), 40>; // the default is 32
    static_assert(std::is_constructible<IPF40, const IPF&>::value, "");
    static_assert(std::is_constructible<IPF40, IPF&&>::value, "");  // TODO: nothrow
    static_assert(std::is_assignable<IPF40&, const IPF&>::value, "");
    static_assert(std::is_assignable<IPF40&, IPF&&>::value, "");  // TODO: nothrow
    //static_assert(!std::is_assignable<IPF&, const IPF40&>::value, "");
    //static_assert(!std::is_assignable<IPF&, IPF40&&>::value, "");
#if __cplusplus >= 201703L
    static_assert(!std::is_swappable_with<IPF40&, IPF&>::value, "");
    static_assert(!std::is_swappable_with<IPF&, IPF40&>::value, "");
#endif
    static_assert(std::is_nothrow_destructible<IPF40>::value, "");
}

TEST(inplace_function, DefaultConstructor)
{
    using IPF = sg14::inplace_function<void(int)>;

    IPF func;
    EXPECT_TRUE(!func);
    EXPECT_TRUE(!bool(func));
    EXPECT_TRUE(func == nullptr);
    EXPECT_TRUE(!(func != nullptr));
    expected = 0; try { func(42); } catch (std::bad_function_call&) { expected = 1; } EXPECT_TRUE(expected == 1);
}

TEST(inplace_function, Assignment)
{
    using IPF = sg14::inplace_function<void(int)>;

    IPF func;

    func = Foo;
    EXPECT_TRUE(!!func);
    EXPECT_TRUE(func);
    EXPECT_TRUE(!(func == nullptr));
    EXPECT_TRUE(func != nullptr);
    called_with = 0; expected = 42; func(42); EXPECT_TRUE(called_with == 42);
}
