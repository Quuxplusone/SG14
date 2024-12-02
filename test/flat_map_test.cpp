#include <sg14/flat_map.h>

#include <gtest/gtest.h>

#include <deque>
#include <functional>
#include <list>
#if __has_include(<memory_resource>)
#include <memory_resource>
#endif
#include <string>
#include <vector>

#if __cplusplus >= 201702L
#include <sg14/inplace_vector.h>
#endif

#if __cplusplus >= 202002L
#include <ranges>
#endif

template<class T> struct flat_mapt : testing::Test {};

using flat_mapt_types = testing::Types<
    sg14::flat_map<int, const char*>                                                // basic
    , sg14::flat_map<int, const char*, std::greater<int>>                           // custom comparator
#if __cplusplus >= 201402L
    , sg14::flat_map<int, const char*, std::greater<>>                              // transparent comparator
#endif
    , sg14::flat_map<int, const char*, std::less<int>, std::deque<int>>             // custom container
#if __cplusplus >= 201702L
    , sg14::flat_map<int, const char*, std::less<int>, sg14::inplace_vector<int, 100>, sg14::inplace_vector<const char*, 50>>
#endif
#if __cpp_lib_memory_resource >= 201603
    , sg14::flat_map<int, const char*, std::less<int>, std::pmr::vector<int>>       // pmr container
    , sg14::flat_map<int, std::pmr::string, std::less<int>, std::pmr::vector<int>>  // uses-allocator construction
#endif
>;
TYPED_TEST_SUITE(flat_mapt, flat_mapt_types);

namespace {

struct AmbiguousEraseWidget {
    explicit AmbiguousEraseWidget(const char *s) : s_(s) {}

    template<class T>
    AmbiguousEraseWidget(T) : s_("notfound") {}

    friend bool operator<(const AmbiguousEraseWidget& a, const AmbiguousEraseWidget& b) {
        return a.s_ < b.s_;
    }

private:
    std::string s_;
};

} // namespace

TEST(flat_map, AmbiguousErase)
{
    sg14::flat_map<AmbiguousEraseWidget, int> fs;
    fs.emplace("a", 1);
    fs.emplace("b", 2);
    fs.emplace("c", 3);
    EXPECT_TRUE(fs.size() == 3);
    fs.erase(AmbiguousEraseWidget("a"));  // calls erase(const Key&)
    EXPECT_TRUE(fs.size() == 2);
    fs.erase(fs.begin());                 // calls erase(iterator)
    EXPECT_TRUE(fs.size() == 1);
    fs.erase(fs.cbegin());                // calls erase(const_iterator)
    EXPECT_TRUE(fs.size() == 0);
}

TEST(flat_map, ExtractDoesntSwap)
{
#if __cpp_lib_memory_resource >= 201603
    // This test fails if extract() is implemented in terms of swap().
    {
        std::pmr::monotonic_buffer_resource mr;
        std::pmr::polymorphic_allocator<int> a(&mr);
        sg14::flat_map<int, int, std::less<>, std::pmr::vector<int>, std::pmr::vector<int>> fs({{1, 10}, {2, 20}}, a);
        auto ctrs = std::move(fs).extract();
        EXPECT_TRUE(ctrs.keys.get_allocator() == a);
        EXPECT_TRUE(ctrs.values.get_allocator() == a);
    }
#endif

    // Sanity-check with std::allocator, even though this can't fail.
    {
        std::allocator<int> a;
        sg14::flat_map<int, int, std::less<>, std::vector<int>, std::vector<int>> fs({{1, 10}, {2, 20}}, a);
        auto ctrs = std::move(fs).extract();
        EXPECT_TRUE(ctrs.keys.get_allocator() == a);
        EXPECT_TRUE(ctrs.values.get_allocator() == a);
    }
}

namespace {

struct InstrumentedWidget {
    static int move_ctors, copy_ctors;
    InstrumentedWidget() = delete;
    InstrumentedWidget(const char *s) : s_(s) {}
    InstrumentedWidget(InstrumentedWidget&& o) noexcept : s_(std::move(o.s_)) { o.is_moved_from = true; move_ctors += 1; }
    InstrumentedWidget(const InstrumentedWidget& o) : s_(o.s_) { copy_ctors += 1; }
    InstrumentedWidget& operator=(InstrumentedWidget&& o) noexcept {
        s_ = std::move(o.s_);
        o.is_moved_from = true;
        return *this;
    }
    InstrumentedWidget& operator=(const InstrumentedWidget&) = default;

    friend bool operator<(const InstrumentedWidget& a, const InstrumentedWidget& b) {
        return a.s_ < b.s_;
    }
    std::string str() const { return s_; }

    bool is_moved_from = false;
private:
    std::string s_;
};
int InstrumentedWidget::move_ctors = 0;
int InstrumentedWidget::copy_ctors = 0;

} // namespace

TEST(flat_map, MoveOperationsPilferOwnership)
{
    using FS = sg14::flat_map<InstrumentedWidget, int>;
    InstrumentedWidget::move_ctors = 0;
    InstrumentedWidget::copy_ctors = 0;
    FS fs;
    fs.insert(std::make_pair(InstrumentedWidget("abc"), 1));
    EXPECT_TRUE(InstrumentedWidget::move_ctors == 3);
    EXPECT_TRUE(InstrumentedWidget::copy_ctors == 0);

    fs.emplace(InstrumentedWidget("def"), 1); fs.erase("def");  // poor man's reserve()
    InstrumentedWidget::copy_ctors = 0;
    InstrumentedWidget::move_ctors = 0;

    fs.emplace("def", 1);  // is still not directly emplaced; a temporary is created to find()
    EXPECT_TRUE(InstrumentedWidget::move_ctors == 1);
    EXPECT_TRUE(InstrumentedWidget::copy_ctors == 0);
    InstrumentedWidget::move_ctors = 0;

    FS fs2 = std::move(fs);  // should just transfer buffer ownership
    EXPECT_TRUE(InstrumentedWidget::move_ctors == 0);
    EXPECT_TRUE(InstrumentedWidget::copy_ctors == 0);

    fs = std::move(fs2);  // should just transfer buffer ownership
    EXPECT_TRUE(InstrumentedWidget::move_ctors == 0);
    EXPECT_TRUE(InstrumentedWidget::copy_ctors == 0);

    FS fs3(fs, std::allocator<InstrumentedWidget>());
    EXPECT_TRUE(InstrumentedWidget::move_ctors == 0);
    EXPECT_TRUE(InstrumentedWidget::copy_ctors == 2);
    InstrumentedWidget::copy_ctors = 0;

    FS fs4(std::move(fs), std::allocator<InstrumentedWidget>());  // should just transfer buffer ownership
    EXPECT_TRUE(InstrumentedWidget::move_ctors == 0);
    EXPECT_TRUE(InstrumentedWidget::copy_ctors == 0);
}

TEST(flat_map, SortedUniqueConstruction)
{
    auto a = sg14::sorted_unique;
    sg14::sorted_unique_t b;
    sg14::sorted_unique_t c{};
    (void)a; (void)b; (void)c;

#if 0 // TODO: GCC cannot compile this
    struct explicitness_tester {
        bool test(std::vector<int>) { return true; }
        bool test(sg14::sorted_unique_t) { return false; }
    };
    explicitness_tester tester;
    EXPECT_TRUE(tester.test({}) == true);
#endif
}

TEST(flat_map, MoveOnly)
{
    using T = std::unique_ptr<int>;
    sg14::flat_map<int, T> fm;
#if __cpp_lib_ranges >= 201911L && __cpp_lib_ranges_to_container >= 202202L && __cpp_lib_ranges_as_rvalue >= 202207L
    std::vector<std::pair<int, T>> pairs;
    fm = sg14::flat_map<int, T>(std::from_range, pairs | std::views::as_rvalue);
    fm = sg14::flat_map<int, T>(std::from_range, pairs | std::views::as_rvalue, std::less<int>());
#endif
    std::vector<int> keys;
    std::vector<T> values;
    fm.replace(keys, std::move(values));
    fm.replace(std::move(keys), std::move(values));
    fm.replace(sg14::sorted_unique, keys, std::move(values));
    fm.replace(sg14::sorted_unique, std::move(keys), std::move(values));
}

TEST(flat_map, TryEmplace)
{
    sg14::flat_map<int, InstrumentedWidget> fm;
    std::pair<sg14::flat_map<int, InstrumentedWidget>::iterator, bool> pair;
    if (true) {
        // try_emplace for a non-existent key does move-from.
        InstrumentedWidget w("abc");
        pair = fm.try_emplace(1, std::move(w));
        EXPECT_TRUE(w.is_moved_from);
        EXPECT_TRUE(pair.second);
    }
    if (true) {
        // try_emplace over an existing key is a no-op.
        InstrumentedWidget w("def");
        pair = fm.try_emplace(1, std::move(w));
        EXPECT_TRUE(!w.is_moved_from);
        EXPECT_TRUE(!pair.second);
        EXPECT_TRUE(pair.first->first == 1);
        EXPECT_TRUE(pair.first->second.str() == "abc");
    }
    if (true) {
        // emplace for a non-existent key does move-from.
        InstrumentedWidget w("abc");
        pair = fm.emplace(2, std::move(w));
        EXPECT_TRUE(w.is_moved_from);
        EXPECT_TRUE(pair.second);
        EXPECT_TRUE(pair.first->first == 2);
        EXPECT_TRUE(pair.first->second.str() == "abc");
    }
    if (true) {
        // emplace over an existing key is a no-op, but does move-from in order to construct the pair.
        InstrumentedWidget w("def");
        pair = fm.emplace(2, std::move(w));
        EXPECT_TRUE(w.is_moved_from);
        EXPECT_TRUE(!pair.second);
        EXPECT_TRUE(pair.first->first == 2);
        EXPECT_TRUE(pair.first->second.str() == "abc");
    }
    if (true) {
        // insert-or-assign for a non-existent key does move-construct.
        InstrumentedWidget w("abc");
        pair = fm.insert_or_assign(3, std::move(w));
        EXPECT_TRUE(w.is_moved_from);
        EXPECT_TRUE(pair.second);
        EXPECT_TRUE(pair.first->first == 3);
        EXPECT_TRUE(pair.first->second.str() == "abc");
    }
    if (true) {
        // insert-or-assign over an existing key does a move-assign.
        InstrumentedWidget w("def");
        pair = fm.insert_or_assign(3, std::move(w));
        EXPECT_TRUE(w.is_moved_from);
        EXPECT_TRUE(!pair.second);
        EXPECT_TRUE(pair.first->first == 3);
        EXPECT_TRUE(pair.first->second.str() == "def");
    }
}


TEST(flat_map, TryEmplaceHint)
{
    sg14::flat_map<int, InstrumentedWidget> fm;
    sg14::flat_map<int, InstrumentedWidget>::iterator iterator;
    if (true) {
        // try_emplace for a non-existent key does move-from.
        InstrumentedWidget w("abc");
        iterator = fm.try_emplace(fm.end(), 1, std::move(w));
        assert(w.is_moved_from);
        assert(iterator == fm.begin());
    }
    if (true) {
        // try_emplace over an existing key is a no-op.
        InstrumentedWidget w("def");
        iterator = fm.try_emplace(fm.end(), 1, std::move(w));
        assert(!w.is_moved_from);
        assert(fm.size() == 1);
        assert(iterator->first == 1);
        assert(iterator->second.str() == "abc");
    }
    if (true) {
        // try_emplace to non-empty with correct hint.
        InstrumentedWidget w("def");
        iterator = fm.try_emplace(fm.end(), 2, std::move(w));
        assert(w.is_moved_from);
        assert(iterator == (fm.end() - 1));
        assert(fm.size() == 2);
        assert(iterator->first == 2);
        assert(iterator->second.str() == "def");
    }
    if (true) {
        // try_emplace to non-empty with wrong hint.
        InstrumentedWidget w("qqq");
        iterator = fm.try_emplace(fm.end(), 0, std::move(w));
        assert(w.is_moved_from);
        assert(iterator == fm.begin());
        assert(fm.size() == 3);
        assert(iterator->first == 0);
        assert(iterator->second.str() == "qqq");
    }
    if (true) {
        // try_emplace to non-empty with wrong hint (not end()).
        InstrumentedWidget w("qqq2");
        iterator = fm.try_emplace(fm.end() - 1, -1, std::move(w));
        assert(w.is_moved_from);
        assert(iterator == fm.begin());
        assert(fm.size() == 4);
        assert(iterator->first == -1);
        assert(iterator->second.str() == "qqq2");
    }
}

TEST(flat_map, TryEmplaceHintConstRef)
{
    sg14::flat_map<int, InstrumentedWidget> fm;
    sg14::flat_map<int, InstrumentedWidget>::iterator iterator;
    int key;
    if (true) {
        // try_emplace for a non-existent key does move-from.
        InstrumentedWidget w("abc");
        key = 1;
        iterator = fm.try_emplace(fm.end(), key, std::move(w));
        assert(w.is_moved_from);
        assert(iterator == fm.begin());
    }
    if (true) {
        // try_emplace over an existing key is a no-op.
        InstrumentedWidget w("def");
        iterator = fm.try_emplace(fm.end(), key, std::move(w));
        assert(!w.is_moved_from);
        assert(fm.size() == 1);
        assert(iterator->first == 1);
        assert(iterator->second.str() == "abc");
    }
    if (true) {
        // try_emplace to non-empty with correct hint.
        InstrumentedWidget w("def");
        key = 2;
        iterator = fm.try_emplace(fm.end(), key, std::move(w));
        assert(w.is_moved_from);
        assert(iterator == (fm.end() - 1));
        assert(fm.size() == 2);
        assert(iterator->first == 2);
        assert(iterator->second.str() == "def");
    }
    if (true) {
        // try_emplace to non-empty with wrong hint.
        InstrumentedWidget w("qqq");
        key = 0;
        iterator = fm.try_emplace(fm.end(), key, std::move(w));
        assert(w.is_moved_from);
        assert(iterator == fm.begin());
        assert(fm.size() == 3);
        assert(iterator->first == 0);
        assert(iterator->second.str() == "qqq");
    }
    if (true) {
        // try_emplace to non-empty with wrong hint (not end()).
        InstrumentedWidget w("qqq2");
        key = -1;
        iterator = fm.try_emplace(fm.end() - 1, key, std::move(w));
        assert(w.is_moved_from);
        assert(iterator == fm.begin());
        assert(fm.size() == 4);
        assert(iterator->first == -1);
        assert(iterator->second.str() == "qqq2");
    }
}


TEST(flat_map, VectorBool)
{
    using FM = sg14::flat_map<bool, bool>;
    FM fm;
    auto it_inserted = fm.emplace(true, false);
    EXPECT_TRUE(it_inserted.second);
    auto it = it_inserted.first;
    EXPECT_TRUE(it == fm.begin());
    EXPECT_TRUE(it->first == true); EXPECT_TRUE(it->first);
    EXPECT_TRUE(it->second == false); EXPECT_TRUE(!it->second);
    it->second = false;
    EXPECT_TRUE(fm.size() == 1);
    it = fm.emplace_hint(it, false, true);
    EXPECT_TRUE(it == fm.begin());
    EXPECT_TRUE(it->first == false); EXPECT_TRUE(!it->first);
    EXPECT_TRUE(it->second == true); EXPECT_TRUE(it->second);
    it->second = true;
    EXPECT_TRUE(fm.size() == 2);
    auto count = fm.erase(false);
    EXPECT_EQ(count, 1u);
    EXPECT_EQ(fm.size(), 1u);
    it = fm.erase(fm.begin());
    EXPECT_TRUE(fm.empty());
    EXPECT_EQ(it, fm.begin());
    EXPECT_EQ(it, fm.end());

    EXPECT_EQ(fm.find(true), fm.end());
    fm.try_emplace(true, true);
    EXPECT_NE(fm.find(true), fm.end());
    EXPECT_EQ(fm[true], true);
    fm[true] = false;
    EXPECT_NE(fm.find(true), fm.end());
    EXPECT_EQ(fm[true], false);
    fm.clear();
}

#if defined(__cpp_deduction_guides)
static bool free_function_less(const int& a, const int& b) {
    return (a < b);
}

template<class... Args>
static auto flatmap_is_ctadable_from(int, Args&&... args)
    -> decltype(flat_map(std::forward<Args>(args)...), std::true_type{})
{
    return {};
}

template<class... Args>
static auto flatmap_is_ctadable_from(long, Args&&...)
    -> std::false_type
{
    return {};
}
#endif // defined(__cpp_deduction_guides)


TEST(flat_map, DeductionGuides)
{
    using sg14::flat_map;
#if defined(__cpp_deduction_guides)
#if __cpp_lib_ranges >= 201911L && __cpp_lib_ranges_to_container >= 202202L
    if (true) {
        // flat_map(std::from_range_t, R&&)
        std::vector<std::pair<std::string, int>> v;
        flat_map fm1(std::from_range, v);
        static_assert(std::is_same_v<decltype(fm1), flat_map<std::string, int>>);
        flat_map fm2 = flat_map(std::from_range, std::deque<std::pair<std::string, int>>());
        static_assert(std::is_same_v<decltype(fm2), flat_map<std::string, int>>);
        std::list<std::pair<const int* const, const int*>> lst;
        flat_map fm3(std::from_range, lst);
        static_assert(std::is_same_v<decltype(fm3), flat_map<const int*, const int*>>);
#if __cpp_lib_memory_resource >= 201603
        std::pmr::vector<std::pair<std::pmr::string, int>> pv;
        flat_map fm4(std::from_range, pv);
        static_assert(std::is_same_v<decltype(fm4), flat_map<std::pmr::string, int>>);
#endif
    }
#endif // __cpp_lib_ranges >= 201911L && __cpp_lib_ranges_to_container >= 202202L
    if (true) {
        // flat_map(InputIterator, InputIterator)
        std::vector<std::pair<std::string, int>> v;
        flat_map fm1(v.begin(), v.end());
        static_assert(std::is_same_v<decltype(fm1), flat_map<std::string, int>>);
        flat_map fm2(sg14::sorted_unique, v.begin(), v.end());
        static_assert(std::is_same_v<decltype(fm2), flat_map<std::string, int>>);
        std::list<std::pair<const int* const, const int*>> lst;
        flat_map fm3(lst.begin(), lst.end());
        static_assert(std::is_same_v<decltype(fm3), flat_map<const int*, const int*>>);
#if __cpp_lib_memory_resource >= 201603
        std::pmr::vector<std::pair<std::pmr::string, int>> pv;
        flat_map fm4(pv.begin(), pv.end());
        static_assert(std::is_same_v<decltype(fm4), flat_map<std::pmr::string, int>>);
#endif
        std::initializer_list<std::pair<int, std::string>> il = {{1,"c"}, {5,"b"}, {3,"a"}};
        flat_map fm5(il.begin(), il.end());
        static_assert(std::is_same_v<decltype(fm5), flat_map<int, std::string>>);
        EXPECT_EQ(fm5, decltype(fm5)(sg14::sorted_unique, {{1,"c"}, {3,"a"}, {5,"b"}}));
    }
    if (true) {
        // flat_map(InputIterator, InputIterator, Compare)
        std::vector<std::pair<std::string, int>> v;
        flat_map fm1(v.begin(), v.end(), std::less<>());
        static_assert(std::is_same_v<decltype(fm1), flat_map<std::string, int, std::less<>>>);
        flat_map fm2(sg14::sorted_unique, v.begin(), v.end(), std::less<>());
        static_assert(std::is_same_v<decltype(fm2), flat_map<std::string, int, std::less<>>>);
        std::list<std::pair<const int* const, const int*>> lst;
        flat_map fm3(lst.begin(), lst.end(), std::greater<>());
        static_assert(std::is_same_v<decltype(fm3), flat_map<const int*, const int*, std::greater<>>>);
#if __cpp_lib_memory_resource >= 201603
        std::pmr::vector<std::pair<std::pmr::string, int>> pv;
        flat_map fm4(pv.begin(), pv.end(), std::greater<std::pmr::string>());
        static_assert(std::is_same_v<decltype(fm4), flat_map<std::pmr::string, int, std::greater<std::pmr::string>>>);
#endif
        std::initializer_list<std::pair<int, std::string>> il = {{1,"c"}, {5,"b"}, {3,"a"}};
        flat_map fm5(il.begin(), il.end(), std::less<>());
        static_assert(std::is_same_v<decltype(fm5), flat_map<int, std::string, std::less<>>>);
        EXPECT_EQ(fm5, decltype(fm5)(sg14::sorted_unique, {{1,"c"}, {3,"a"}, {5,"b"}}));

        std::pair<int, int> arr[] = {{1,2}, {3,4}, {2,3}, {4,5}};
        flat_map fm6(arr, arr + 4, free_function_less);
        static_assert(std::is_same_v<decltype(fm6), flat_map<int, int, bool(*)(const int&, const int&)>>);
        EXPECT_EQ(fm6.key_comp(), free_function_less);
        EXPECT_EQ(fm6, decltype(fm6)(sg14::sorted_unique, {{1,2}, {2,3}, {3,4}, {4,5}}, free_function_less));
    }
#endif // defined(__cpp_deduction_guides)
}

TYPED_TEST(flat_mapt, Construction)
{
    using FS = TypeParam;

    static_assert(std::is_same<int, typename FS::key_type>::value, "");
    static_assert(std::is_convertible<const char*, typename FS::mapped_type>::value, "");
    using Mapped = typename FS::mapped_type;
    using Str = std::conditional_t<std::is_same<Mapped, const char *>::value, std::string, Mapped>;
    using Compare = typename FS::key_compare;
    std::vector<int> keys = {1, 3, 5};
    std::vector<const char*> values = {"a", "c", "b"};
    std::vector<std::pair<int, const char*>> pairs = {
        {1, "a"},
        {3, "c"},
        {5, "b"},
    };
    if (true) {
        FS fs;  // default constructor
        fs = {
            {1, "a"},
            {3, "c"},
            {5, "b"},
        };
        EXPECT_TRUE(std::is_sorted(fs.keys().begin(), fs.keys().end(), fs.key_comp()));
        EXPECT_TRUE(std::is_sorted(fs.begin(), fs.end(), fs.value_comp()));
        EXPECT_TRUE(fs[1] == Str("a"));
        EXPECT_TRUE(fs[3] == Str("c"));
        EXPECT_TRUE(fs[5] == Str("b"));
    }
    for (auto&& fs : {
#if __cpp_lib_ranges >= 201911L && __cpp_lib_ranges_to_container >= 202202L
        FS(std::from_range, pairs),
        FS(std::from_range, pairs, Compare()),
#endif
        FS({{1, "a"}, {3, "c"}, {5, "b"}}),
        FS(pairs.begin(), pairs.end()),
        FS(pairs.rbegin(), pairs.rend()),
        FS({{1, "a"}, {3, "c"}, {5, "b"}}, Compare()),
        FS(pairs.begin(), pairs.end(), Compare()),
        FS(pairs.rbegin(), pairs.rend(), Compare()),
    }) {
        EXPECT_TRUE(std::is_sorted(fs.keys().begin(), fs.keys().end(), fs.key_comp()));
        EXPECT_TRUE(std::is_sorted(fs.begin(), fs.end(), fs.value_comp()));
        EXPECT_EQ(fs.size(), 3u);
        EXPECT_TRUE(fs.find(0) == fs.end());
        EXPECT_TRUE(fs.find(1) != fs.end());
        EXPECT_TRUE(fs.find(2) == fs.end());
        EXPECT_TRUE(fs.find(3) != fs.end());
        EXPECT_TRUE(fs.find(4) == fs.end());
        EXPECT_TRUE(fs.find(5) != fs.end());
        EXPECT_TRUE(fs.find(6) == fs.end());
        EXPECT_TRUE(fs.at(1) == Str("a"));
        EXPECT_TRUE(fs.at(3) == Str("c"));
        EXPECT_TRUE(fs.find(5)->second == Str("b"));
    }
    if (std::is_sorted(keys.begin(), keys.end(), Compare())) {
        for (auto&& fs : {
            FS(sg14::sorted_unique, pairs.begin(), pairs.end()),
            FS(sg14::sorted_unique, {{1, "a"}, {3, "c"}, {5, "b"}}),
            FS(sg14::sorted_unique, pairs.begin(), pairs.end(), Compare()),
            FS(sg14::sorted_unique, {{1, "a"}, {3, "c"}, {5, "b"}}, Compare()),
        }) {
            EXPECT_TRUE(std::is_sorted(fs.keys().begin(), fs.keys().end(), fs.key_comp()));
            EXPECT_TRUE(std::is_sorted(fs.begin(), fs.end(), fs.value_comp()));
            EXPECT_TRUE(fs.at(1) == Str("a"));
            EXPECT_TRUE(fs.at(3) == Str("c"));
            EXPECT_TRUE(fs.find(5)->second == Str("b"));
        }
    }
}

TYPED_TEST(flat_mapt, InsertOrAssign)
{
    using FM = TypeParam;

    FM fm;
    const char *str = "a";
    using Mapped = typename FM::mapped_type;
    using Str = std::conditional_t<std::is_same<Mapped, const char *>::value, std::string, Mapped>;

    fm.insert_or_assign(1, str);
    EXPECT_TRUE(fm.at(1) == Str("a"));
    EXPECT_TRUE(fm[1] == Str("a"));
    fm.insert_or_assign(2, std::move(str));
    EXPECT_TRUE(fm.at(2) == Str("a"));
    EXPECT_TRUE(fm[2] == Str("a"));
    fm.insert_or_assign(2, "b");
    EXPECT_TRUE(fm.at(2) == Str("b"));
    EXPECT_TRUE(fm[2] == Str("b"));
    fm.insert_or_assign(3, "c");
    EXPECT_TRUE(fm.at(3) == Str("c"));
    EXPECT_TRUE(fm[3] == Str("c"));

    // With hints.
    fm.insert_or_assign(fm.begin(), 1, str);
    EXPECT_TRUE(fm.at(1) == Str("a"));
    EXPECT_TRUE(fm[1] == Str("a"));
    fm.insert_or_assign(fm.begin()+2, 2, std::move(str));
    EXPECT_TRUE(fm.at(2) == Str("a"));
    EXPECT_TRUE(fm[2] == Str("a"));
    fm.insert_or_assign(fm.end(), 2, "c");
    EXPECT_TRUE(fm.at(2) == Str("c"));
    EXPECT_TRUE(fm[2] == Str("c"));
    fm.insert_or_assign(fm.end() - 1, 3, "b");
    EXPECT_TRUE(fm.at(3) == Str("b"));
    EXPECT_TRUE(fm[3] == Str("b"));
}

TYPED_TEST(flat_mapt, SpecialMembers)
{
    using FS = TypeParam;

    static_assert(std::is_default_constructible<FS>::value, "");
    static_assert(std::is_nothrow_move_constructible<FS>::value == std::is_nothrow_move_constructible<typename FS::key_container_type>::value && std::is_nothrow_move_constructible<typename FS::mapped_container_type>::value && std::is_nothrow_move_constructible<typename FS::key_compare>::value, "");
    static_assert(std::is_copy_constructible<FS>::value, "");
    static_assert(std::is_copy_assignable<FS>::value, "");
    static_assert(std::is_move_assignable<FS>::value, "");
    static_assert(std::is_nothrow_destructible<FS>::value, "");

    static_assert(std::is_default_constructible<typename FS::containers>::value, "");
    static_assert(std::is_nothrow_move_constructible<typename FS::containers>::value == std::is_nothrow_move_constructible<typename FS::key_container_type>::value && std::is_nothrow_move_constructible<typename FS::mapped_container_type>::value, "");
    static_assert(std::is_copy_constructible<typename FS::containers>::value, "");
    static_assert(std::is_copy_assignable<typename FS::containers>::value, "");
    static_assert(std::is_move_assignable<typename FS::containers>::value, "");
    static_assert(std::is_nothrow_destructible<typename FS::containers>::value, "");
}

TYPED_TEST(flat_mapt, ComparisonOperators)
{
    using FM = TypeParam;

    // abc[] is "", "a", "b", "c", but the char pointers themselves are also comparable.
    const char abc_buffer[] = "\0a\0b\0c";
    const char *abc[] = {abc_buffer+0, abc_buffer+1, abc_buffer+3, abc_buffer+5};

    FM fm1 = {
        {1, abc[2]}, {2, abc[3]},
    };
    FM fm2 = {
        {1, abc[2]}, {2, abc[3]},
    };
    // {1b, 2c} is equal to {1b, 2c}.
    EXPECT_TRUE(fm1 == fm2);
    EXPECT_TRUE(!(fm1 != fm2));
    EXPECT_TRUE(!(fm1 < fm2));
    EXPECT_TRUE(!(fm1 > fm2));
    EXPECT_TRUE(fm1 <= fm2);
    EXPECT_TRUE(fm1 >= fm2);

    fm2[2] = abc[1];
    // {1b, 2c} is greater than {1b, 2a}.
    EXPECT_TRUE(!(fm1 == fm2));
    EXPECT_TRUE(fm1 != fm2);
    EXPECT_TRUE(!(fm1 < fm2));
    EXPECT_TRUE(fm1 > fm2);
    EXPECT_TRUE(!(fm1 <= fm2));
    EXPECT_TRUE(fm1 >= fm2);

    fm2.erase(2);
    fm2.insert({0, abc[3]});
    // {1b, 2c} is greater than {0c, 1b}.
    EXPECT_TRUE(!(fm1 == fm2));
    EXPECT_TRUE(fm1 != fm2);
    EXPECT_TRUE(!(fm1 < fm2));
    EXPECT_TRUE(fm1 > fm2);
    EXPECT_TRUE(!(fm1 <= fm2));
    EXPECT_TRUE(fm1 >= fm2);
}

TYPED_TEST(flat_mapt, Search)
{
    using FM = TypeParam;

    FM fm{{1, "a"}, {2, "b"}, {3, "c"}};
    auto it = fm.lower_bound(2);
    auto cit = const_cast<const FM&>(fm).lower_bound(2);
    EXPECT_TRUE(it == fm.begin() + 1);
    EXPECT_TRUE(cit == it);

    it = fm.upper_bound(2);
    cit = const_cast<const FM&>(fm).upper_bound(2);
    EXPECT_TRUE(it == fm.begin() + 2);
    EXPECT_TRUE(cit == it);

    auto itpair = fm.equal_range(2);
    auto citpair = const_cast<const FM&>(fm).equal_range(2);
    EXPECT_TRUE(itpair.first == fm.begin() + 1);
    EXPECT_TRUE(itpair.second == fm.begin() + 2);
    EXPECT_TRUE(citpair == decltype(citpair)(itpair));

    static_assert(std::is_same<decltype(it), typename FM::iterator>::value, "");
    static_assert(std::is_same<decltype(cit), typename FM::const_iterator>::value, "");
}

TEST(flat_map, Iterators)
{
    sg14::flat_map<int, char> fm = {{3,'3'}, {1,'1'}, {4,'4'}, {2,'2'}};
    EXPECT_EQ(fm.begin(), fm.cbegin());
    EXPECT_EQ(fm.end(), fm.cend());
    EXPECT_EQ(fm.rbegin(), fm.crbegin());
    EXPECT_EQ(fm.rend(), fm.crend());
    std::vector<std::pair<int, char>> expected = {{1,'1'}, {2,'2'}, {3,'3'}, {4,'4'}};
    auto eq = [](auto&& p1, auto&& p2) {
        return (p1.first == p2.first) && (p1.second == p2.second);
    };
    EXPECT_TRUE(std::equal(fm.begin(), fm.end(), expected.begin(), expected.end(), eq));
    EXPECT_TRUE(std::equal(fm.cbegin(), fm.cend(), expected.cbegin(), expected.cend(), eq));
    EXPECT_TRUE(std::equal(fm.rbegin(), fm.rend(), expected.rbegin(), expected.rend(), eq));
    EXPECT_TRUE(std::equal(fm.crbegin(), fm.crend(), expected.crbegin(), expected.crend(), eq));
}

TEST(flat_map, ContainerAccessors)
{
    // The C++23 flat_map provides only `ctrs = fm.extract()` and `fm.replace(ctrs)`,
    // but the SG14 version provides direct access to the containers in place.
    sg14::flat_map<int, char> fm = {{3,'3'}, {1,'1'}, {4,'4'}, {2,'2'}};
    const auto& cfm = fm;
    static_assert(std::is_same<decltype(fm.keys()), const std::vector<int>&>::value, "");
    static_assert(std::is_same<decltype(fm.values()), std::vector<char>&>::value, "");
    static_assert(std::is_same<decltype(cfm.keys()), const std::vector<int>&>::value, "");
    static_assert(std::is_same<decltype(cfm.values()), const std::vector<char>&>::value, "");
    std::vector<int> expected_keys = {1, 2, 3, 4};
    std::vector<char> expected_values = {'1', '2', '3', '4'};
    EXPECT_EQ(fm.keys(), expected_keys);
    EXPECT_EQ(fm.values(), expected_values);
    EXPECT_EQ(cfm.keys(), expected_keys);
    EXPECT_EQ(cfm.values(), expected_values);
}

TEST(flat_map, ExtractReplace)
{
    std::vector<int> expected_keys = {1, 2, 7, 8};
    std::vector<char> expected_values = {'1', '2', '7', '8'};
    std::vector<int> rvalue_keys = {2, 7, 1, 8};
    std::vector<char> rvalue_values = {'2', '7', '1', '8'};

    sg14::flat_map<int, char> fm;
    fm.replace(std::move(rvalue_keys), std::move(rvalue_values));
    EXPECT_TRUE(rvalue_keys.empty());
    EXPECT_TRUE(rvalue_values.empty());
    EXPECT_EQ(fm.keys(), expected_keys);
    EXPECT_EQ(fm.values(), expected_values);
    fm.replace(expected_keys, expected_values); // lvalues
    EXPECT_EQ(fm.keys(), expected_keys);
    EXPECT_EQ(fm.values(), expected_values);

    rvalue_keys = {1, 2, 7, 8};
    rvalue_values = {'1', '2', '7', '8'};
    fm.replace(sg14::sorted_unique, std::move(rvalue_keys), std::move(rvalue_values));
    EXPECT_TRUE(rvalue_keys.empty());
    EXPECT_TRUE(rvalue_values.empty());
    EXPECT_EQ(fm.keys(), expected_keys);
    EXPECT_EQ(fm.values(), expected_values);
    fm.replace(sg14::sorted_unique, expected_keys, expected_values); // lvalues
    EXPECT_EQ(fm.keys(), expected_keys);
    EXPECT_EQ(fm.values(), expected_values);
}
