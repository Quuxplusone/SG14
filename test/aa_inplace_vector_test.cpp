#if __cplusplus >= 202002L

#include <exception>
#include <gtest/gtest.h>
#include <set>
#include <sg14/aa_inplace_vector.h>
#include <utility>

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

#endif // __cplusplus >= 202002L
