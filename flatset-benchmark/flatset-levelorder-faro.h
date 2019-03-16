/*
 * Boost Software License - Version 1.0 - August 17th, 2003
 *
 * Permission is hereby granted, free of charge, to any person or organization
 * obtaining a copy of the software and accompanying documentation covered by
 * this license (the "Software") to use, reproduce, display, distribute,
 * execute, and transmit the Software, and to prepare derivative works of the
 * Software, and to permit third-parties to whom the Software is furnished to
 * do so, all subject to the following:
 *
 * The copyright notices in the Software and this entire statement, including
 * the above license grant, this restriction and the following disclaimer,
 * must be included in all copies of the Software, in whole or in part, and
 * all derivative works of the Software, unless such copies or derivative
 * works are solely in the form of machine-executable object code generated by
 * a source language processor.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
 * SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
 * FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

// This is an implementation of the proposed "std::flat_set" as specified in
// http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p1222r1.pdf
// except with its entries laid out in level-order / Eytzinger order instead of sorted.

#include <stddef.h>
#include <algorithm>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <vector>

#include <iostream> // TODO remove this

namespace stdext {

template<class, class, class> class flat_set;

namespace flatset_detail {

#include "eytzinger_utils.h"

    template<class T, class = void> struct qualifies_as_range : std::false_type {};
    template<class T> struct qualifies_as_range<T, decltype(
        std::begin( std::declval<T()>()() ), void(),
        std::end( std::declval<T()>()() ), void()
    )> : std::true_type {};

    template<class It>
    using is_random_access_iterator = std::is_convertible<
        typename std::iterator_traits<It>::iterator_category,
        std::random_access_iterator_tag
    >;

    template<int I> struct priority_tag : priority_tag<I-1> {};
    template<> struct priority_tag<0> {};

    // As proposed in P0591R4. Guaranteed copy elision makes this do the right thing.
    template<class T, class Alloc, class... Args, class = std::enable_if_t<
        std::uses_allocator<T, Alloc>::value && std::is_constructible<T, std::allocator_arg_t, const Alloc&, Args&&...>::value
    >>
    T make_obj_using_allocator_(priority_tag<3>, const Alloc& alloc, Args&&... args) {
        return T(std::allocator_arg, alloc, static_cast<Args&&>(args)...);
    }
    template<class T, class Alloc, class... Args, class = std::enable_if_t<
        std::uses_allocator<T, Alloc>::value && std::is_constructible<T, Args&&..., const Alloc&>::value
    >>
    T make_obj_using_allocator_(priority_tag<2>, const Alloc& alloc, Args&&... args) {
        return T(static_cast<Args&&>(args)..., alloc);
    }
    template<class T, class Alloc, class... Args, class = std::enable_if_t<
        !std::uses_allocator<T, Alloc>::value && std::is_constructible<T, Args&&...>::value
    >>
    T make_obj_using_allocator_(priority_tag<1>, const Alloc&, Args&&... args) {
        return T(static_cast<Args&&>(args)...);
    }
    template<class T, class Alloc, class... Args>
    T make_obj_using_allocator_(priority_tag<0>, const Alloc&, Args&&...) {
        static_assert(sizeof(T)==0, "this request for uses-allocator construction is ill-formed");
    }
    template<class T, class Alloc, class... Args>
    T make_obj_using_allocator(const Alloc& alloc, Args&&... args) {
        return make_obj_using_allocator_<T>(priority_tag<3>(), alloc, static_cast<Args&&>(args)...);
    }

    template<class It, class Compare>
    It unique_helper(It first, It last, Compare& compare) {
        It dfirst = first;
        while (first != last) {
            It next = first;
            ++next;
            if ((next != last) && !bool(compare(*first, *next))) {
                // "next" is a duplicate of "first", so do not preserve "first"
            } else {
                // do preserve "first"
                if (first != dfirst) {
                    *dfirst = std::move(*first);
                }
                ++dfirst;
            }
            first = next;
        }
        return dfirst;
    }

    template<class Container>
    using cont_value_type = typename Container::value_type;

    template<class InputIterator>
    using iter_value_type = std::remove_const_t<typename std::iterator_traits<InputIterator>::value_type>;

    template<class...> using void_t = void;

    template<class A, class = void>
    struct qualifies_as_allocator : std::false_type {};
    template<class A>
    struct qualifies_as_allocator<A, void_t<
        typename A::value_type,
        decltype(std::declval<A&>().allocate(size_t{}))
    >> : std::true_type {};

    template<class It>
    using qualifies_as_input_iterator = std::integral_constant<bool, !std::is_integral<It>::value>;

#if defined(__cpp_lib_is_swappable)
    using std::is_nothrow_swappable;
#else
    template<class, class = std::true_type>
    struct is_nothrow_swappable : std::false_type {};
    template<class T>
    struct is_nothrow_swappable<T, std::integral_constant<bool, noexcept(swap(std::declval<T&>(), std::declval<T&>()))>> : std::true_type {};
#endif

    template<class It>
    class iter {
    public:
        using difference_type = ptrdiff_t;
        using value_type = typename std::iterator_traits<It>::value_type;
        using reference = typename std::iterator_traits<It>::reference;
        using pointer = typename std::iterator_traits<It>::pointer;
        using iterator_category = std::random_access_iterator_tag;

        iter() = default;
        iter(iter&&) = default;
        iter(const iter&) = default;
        iter& operator=(iter&&) = default;
        iter& operator=(const iter&) = default;
        ~iter() = default;

        // This is the iterator-to-const_iterator implicit conversion.
        template<class Jt,
                 class = std::enable_if_t<std::is_convertible<Jt, It>::value>>
        iter(const iter<Jt>& other) :
            begin_(other.private_impl_getbegin()), index_(other.private_impl_getindex()), n_(other.private_impl_getn()) {}

        It base() const noexcept { return begin_ + index_; }

        reference operator*() const {
            return *base();
        }

        template<class It_ = It, class = decltype(std::declval<It_>().operator->())>
        pointer operator->() const {
            return base().operator->();
        }

        template<class It_ = It, std::enable_if_t<std::is_pointer<It_>::value, int> = 0>
        pointer operator->() const {
            return base();
        }

        static int getparent(int i) { return (i - 1) / 2; }
        static int getleft(int i) { return 2*i + 1; }
        static int getright(int i) { return 2*i + 2; }
        static bool isleftchild(int i) { return i % 2 == 1; }
        static bool isrightchild(int i) { return i % 2 == 0; }

        bool has_left_child() const { return getleft(index_) < n_; }
        bool has_right_child() const { return getright(index_) < n_; }
        void step_left() { index_ = getleft(index_); }
        void step_right() { index_ = getright(index_); }

        iter& operator++() {
            if (getright(index_) < n_) {
                index_ = getright(index_);
                while (getleft(index_) < n_) {
                    index_ = getleft(index_);
                }
            } else {
                while (isrightchild(index_)) {
                    if (index_ == 0) {
                        index_ = n_;  // the special "end" index
                        return *this;
                    }
                    index_ = getparent(index_);
                }
                index_ = getparent(index_);
            }
            return *this;
        }

        iter& operator--() {
            if (index_ == n_) {
                index_ = 0;
                while (getright(index_) < n_) {
                    index_ = getright(index_);
                }
            } else if (getleft(index_) < n_) {
                index_ = getleft(index_);
                while (getright(index_) < n_) {
                    index_ = getright(index_);
                }
            } else {
                while (isleftchild(index_)) {
                    index_ = getparent(index_);
                }
                index_ = getparent(index_);
            }
            return *this;
        }

        iter operator++(int) { iter result(*this); ++*this; return result; }
        iter operator--(int) { iter result(*this); --*this; return result; }
        iter& operator+=(ptrdiff_t n) {
            int i = rank_from_eytzinger(index_, n_);
            i += n;
            index_ = eytzinger_from_rank(i, n_);
            return *this;
        }
        iter& operator-=(ptrdiff_t n) {
            int i = rank_from_eytzinger(index_, n_);
            i -= n;
            index_ = eytzinger_from_rank(i, n_);
            return *this;
        }
        reference operator[](ptrdiff_t n) const { return *(*this + n); }
        friend iter operator+(iter it, ptrdiff_t n) { it += n; return it; }
        friend iter operator+(ptrdiff_t n, iter it) { it += n; return it; }
        friend iter operator-(iter it, ptrdiff_t n) { it -= n; return it; }
        friend ptrdiff_t operator-(const iter& it, const iter& jt) {
            int i = rank_from_eytzinger(it.index_, it.n_);
            int j = rank_from_eytzinger(jt.index_, jt.n_);
            return i - j;
        }
        friend bool operator==(const iter& a, const iter& b) { return a.index_ == b.index_; }
        friend bool operator!=(const iter& a, const iter& b) { return !(a.index_ == b.index_); }
        friend bool operator<(const iter& a, const iter& b) { return (a - b) < 0; }
        friend bool operator<=(const iter& a, const iter& b) { return (a - b) <= 0; }
        friend bool operator>(const iter& a, const iter& b) { return (a - b) > 0; }
        friend bool operator>=(const iter& a, const iter& b) { return (a - b) >= 0; }

        friend std::ostream& operator<<(std::ostream& os, const iter& it) {
            // TODO remove this
            os << "(" << it.index_ << "/" << it.n_ << ")";
            return os;
        }

        It private_impl_getbegin() const { return begin_; }
        int private_impl_getindex() const { return index_; }
        int private_impl_getn() const { return n_; }

    private:
        template<class, class, class> friend class stdext::flat_set;

        explicit iter(It&& begin, int idx, int n)
            : begin_(static_cast<It&&>(begin)), index_(idx), n_(n) {}

        It begin_;
        int index_;
        int n_;
    };

    template<class RandomIt>
    void invert_faro_shuffle(RandomIt first, RandomIt last) {
        // Transforms [a0 b0 a1 b1 ... an-1 bn-1 an] to [a0 a1 ... an b0 b1 ... bn-1].
        auto n = last - first;
        // assert(n % 2 == 1);
        if (n >= 3) {
            auto half = (n + 1) >> 1;
            auto quarter = half >> 1;
            invert_faro_shuffle(first, first + (quarter + quarter - 1));
            invert_faro_shuffle(first + (quarter + quarter), last);
            std::rotate(first + quarter, first + (quarter + quarter), first + (half + quarter));
        }
    }

    template<class RandomIt>
    void make_levelorder_from_sorted(RandomIt first, RandomIt last) {
        auto n = last - first;
        using IndexType = decltype(n);
        int height = 1;
        while (IndexType(1u << height) <= n) {
            ++height;
        }
        for (int level = height-1; level != 0; --level) {
            IndexType pow2level = (1u << level);
            IndexType effective_size = std::min(2*pow2level - 1, n);
            IndexType leaf_count = std::min(pow2level, n + 1 - pow2level);
            invert_faro_shuffle(first, first + (2*leaf_count - 1));
            std::rotate(first, first + leaf_count, first + effective_size);
        }
    }

} // namespace flatset_detail

#ifndef STDEXT_HAS_SORTED_UNIQUE
#define STDEXT_HAS_SORTED_UNIQUE

struct sorted_unique_t { explicit sorted_unique_t() = default; };

#if defined(__cpp_inline_variables)
inline
#endif
constexpr sorted_unique_t sorted_unique {};

#endif // STDEXT_HAS_SORTED_UNIQUE

template<
    class Key,
    class Compare = std::less<Key>,
    class KeyContainer = std::vector<Key>
>
class flat_set {
    static_assert(flatset_detail::is_random_access_iterator<typename KeyContainer::iterator>::value, "");
    static_assert(std::is_same<Key, typename KeyContainer::value_type>::value, "");
    static_assert(std::is_convertible<decltype(std::declval<const Compare&>()(std::declval<const Key&>(), std::declval<const Key&>())), bool>::value, "");
#if defined(__cpp_lib_is_swappable)
    static_assert(std::is_nothrow_swappable<KeyContainer>::value, "");
#endif
public:
    using key_type = Key;
    using key_compare = Compare;
    using value_type = Key;
    using value_compare = Compare;
    using reference = Key&;
    using const_reference = const Key&;
    using size_type = size_t; // TODO: this should be KeyContainer::size_type
    using difference_type = ptrdiff_t; // TODO: this should be KeyContainer::difference_type
    using iterator = flatset_detail::iter<typename KeyContainer::iterator>;
    using const_iterator = flatset_detail::iter<typename KeyContainer::const_iterator>;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    using container_type = KeyContainer;

// =========================================================== CONSTRUCTORS
// This is all one massive overload set!

    flat_set() : flat_set(Compare()) {}

    explicit flat_set(KeyContainer ctr)
        : c_(static_cast<KeyContainer&&>(ctr)), compare_()
    {
        this->sort_and_unique_and_levelorder_impl();
    }

    template<class Alloc,
             class = std::enable_if_t<std::uses_allocator<KeyContainer, Alloc>::value>>
    flat_set(KeyContainer ctr, const Alloc& a)
        : c_(flatset_detail::make_obj_using_allocator<KeyContainer>(a, static_cast<KeyContainer&&>(ctr))), compare_()
    {
        this->sort_and_unique_and_levelorder_impl();
    }

    template<class Container,
             std::enable_if_t<flatset_detail::qualifies_as_range<const Container&>::value, int> = 0>
    explicit flat_set(const Container& cont)
        : flat_set(std::begin(cont), std::end(cont), Compare()) {}

    template<class Container,
             class = std::enable_if_t<flatset_detail::qualifies_as_range<const Container&>::value>>
    flat_set(const Container& cont, const Compare& comp)
        : flat_set(std::begin(cont), std::end(cont), comp) {}

    template<class Container, class Alloc,
             class = std::enable_if_t<flatset_detail::qualifies_as_range<const Container&>::value && std::uses_allocator<KeyContainer, Alloc>::value>>
    flat_set(const Container& cont, const Alloc& a)
        : flat_set(std::begin(cont), std::end(cont), Compare(), a) {}

    template<class Container, class Alloc,
             class = std::enable_if_t<flatset_detail::qualifies_as_range<const Container&>::value && std::uses_allocator<KeyContainer, Alloc>::value>>
    flat_set(const Container& cont, const Compare& comp, const Alloc& a)
        : flat_set(std::begin(cont), std::end(cont), comp, a) {}

    flat_set(sorted_unique_t, KeyContainer ctr)
        : c_(static_cast<KeyContainer&&>(ctr)), compare_()
    {
        flatset_detail::make_levelorder_from_sorted(c_.begin(), c_.end());
    }

    template<class Alloc,
             class = std::enable_if_t<std::uses_allocator<KeyContainer, Alloc>::value>>
    flat_set(sorted_unique_t, KeyContainer ctr, const Alloc& a)
        : c_(flatset_detail::make_obj_using_allocator<KeyContainer>(a, static_cast<KeyContainer&&>(ctr))), compare_()
    {
        flatset_detail::make_levelorder_from_sorted(c_.begin(), c_.end());
    }

    template<class Container,
             class = std::enable_if_t<flatset_detail::qualifies_as_range<const Container&>::value>>
    flat_set(sorted_unique_t s, const Container& cont)
        : flat_set(s, std::begin(cont), std::end(cont), Compare()) {}

    template<class Container,
             class = std::enable_if_t<flatset_detail::qualifies_as_range<const Container&>::value>>
    flat_set(sorted_unique_t s, const Container& cont, const Compare& comp)
        : flat_set(s, std::begin(cont), std::end(cont), comp) {}

    template<class Container, class Alloc,
             class = std::enable_if_t<flatset_detail::qualifies_as_range<const Container&>::value && std::uses_allocator<KeyContainer, Alloc>::value>>
    flat_set(sorted_unique_t s, const Container& cont, const Alloc& a)
        : flat_set(s, std::begin(cont), std::end(cont), Compare(), a) {}

    template<class Container, class Alloc,
             class = std::enable_if_t<flatset_detail::qualifies_as_range<const Container&>::value && std::uses_allocator<KeyContainer, Alloc>::value>>
    flat_set(sorted_unique_t s, const Container& cont, const Compare& comp, const Alloc& a)
        : flat_set(s, std::begin(cont), std::end(cont), comp, a) {}

    explicit flat_set(const Compare& comp)
        : c_(), compare_(comp) {}

    template<class Alloc,
             class = std::enable_if_t<std::uses_allocator<KeyContainer, Alloc>::value>>
    flat_set(const Compare& comp, const Alloc& a)
        : c_(flatset_detail::make_obj_using_allocator<KeyContainer>(a)), compare_(comp) {}

    template<class Alloc,
             std::enable_if_t<std::uses_allocator<KeyContainer, Alloc>::value, int> = 0>
    explicit flat_set(const Alloc& a)
        : flat_set(Compare(), a) {}

    // TODO: all templates taking InputIterator probably need to be constrained
    template<class InputIterator>
    flat_set(InputIterator first, InputIterator last, const Compare& comp = Compare())
        : c_(first, last), compare_(comp)
    {
        this->sort_and_unique_and_levelorder_impl();
    }

    // TODO: this constructor could conditionally use KeyContainer's iterator-pair constructor
    template<class InputIterator, class Alloc,
             class = std::enable_if_t<std::uses_allocator<KeyContainer, Alloc>::value>>
    flat_set(InputIterator first, InputIterator last, const Compare& comp, const Alloc& a)
        : c_(flatset_detail::make_obj_using_allocator<KeyContainer>(a)), compare_(comp)
    {
        while (first != last) {
            c_.insert(c_.end(), *first);
            ++first;
        }
        this->sort_and_unique_and_levelorder_impl();
    }

    template<class InputIterator, class Alloc,
             class = std::enable_if_t<std::uses_allocator<KeyContainer, Alloc>::value>>
    flat_set(InputIterator first, InputIterator last, const Alloc& a)
        : flat_set(first, last, Compare(), a) {}

    template<class InputIterator>
    flat_set(sorted_unique_t, InputIterator first, InputIterator last, const Compare& comp = Compare())
        : c_(first, last), compare_(comp)
    {
        flatset_detail::make_levelorder_from_sorted(c_.begin(), c_.end());
    }

    // TODO: this constructor could conditionally use KeyContainer's iterator-pair constructor
    template<class InputIterator, class Alloc,
             class = std::enable_if_t<std::uses_allocator<KeyContainer, Alloc>::value>>
    flat_set(sorted_unique_t, InputIterator first, InputIterator last,
             const Compare& comp, const Alloc& a)
        : c_(flatset_detail::make_obj_using_allocator<KeyContainer>(a)), compare_(comp)
    {
        while (first != last) {
            c_.insert(c_.end(), *first);
            ++first;
        }
        flatset_detail::make_levelorder_from_sorted(c_.begin(), c_.end());
    }

    template<class InputIterator, class Alloc,
             class = std::enable_if_t<std::uses_allocator<KeyContainer, Alloc>::value>>
    flat_set(sorted_unique_t s, InputIterator first, InputIterator last, const Alloc& a)
        : flat_set(s, first, last, Compare(), a) {}

    // TODO: should this be conditionally noexcept?
    template<class Alloc,
             class = std::enable_if_t<std::uses_allocator<KeyContainer, Alloc>::value>>
    flat_set(flat_set&& m, const Alloc& a)
        : c_(flatset_detail::make_obj_using_allocator<KeyContainer>(a, static_cast<KeyContainer&&>(m.c_))), compare_(static_cast<Compare&&>(m.compare_)) {}

    template<class Alloc,
             class = std::enable_if_t<std::uses_allocator<KeyContainer, Alloc>::value>>
    flat_set(const flat_set& m, const Alloc& a)
        : c_(flatset_detail::make_obj_using_allocator<KeyContainer>(a, m.c_)), compare_(m.compare_) {}

    flat_set(std::initializer_list<Key>&& il, const Compare& comp = Compare())
        : flat_set(il, comp) {}

    template<class Alloc,
             class = std::enable_if_t<std::uses_allocator<KeyContainer, Alloc>::value>>
    flat_set(std::initializer_list<Key>&& il, const Compare& comp, const Alloc& a)
        : flat_set(il, comp, a) {}

    template<class Alloc,
             class = std::enable_if_t<std::uses_allocator<KeyContainer, Alloc>::value>>
    flat_set(std::initializer_list<Key>&& il, const Alloc& a)
        : flat_set(il, Compare(), a) {}

    flat_set(sorted_unique_t s, std::initializer_list<Key>&& il, const Compare& comp = Compare())
        : flat_set(s, il, comp) {}

    template<class Alloc,
             class = std::enable_if_t<std::uses_allocator<KeyContainer, Alloc>::value>>
    flat_set(sorted_unique_t s, std::initializer_list<Key>&& il, const Compare& comp, const Alloc& a)
        : flat_set(s, il, comp, a) {}

    template<class Alloc,
             class = std::enable_if_t<std::uses_allocator<KeyContainer, Alloc>::value>>
    flat_set(sorted_unique_t s, std::initializer_list<Key>&& il, const Alloc& a)
        : flat_set(s, il, Compare(), a) {}


// ========================================================== OTHER MEMBERS

    flat_set& operator=(std::initializer_list<Key> il) {
        this->clear();
        this->insert(il);
        return *this;
    }

    iterator begin() noexcept { iterator it(c_.begin(), 0, c_.size()); while (it.has_left_child()) it.step_left(); return it; }
    const_iterator begin() const noexcept { const_iterator it(c_.begin(), 0, c_.size()); while (it.has_left_child()) it.step_left(); return it; }
    iterator end() noexcept { return iterator(c_.begin(), c_.size(), c_.size()); }
    const_iterator end() const noexcept { return const_iterator(c_.begin(), c_.size(), c_.size()); }

    const_iterator cbegin() const noexcept { return begin(); }
    const_iterator cend() const noexcept { return end(); }

    reverse_iterator rbegin() noexcept { return std::make_reverse_iterator(end()); }
    const_reverse_iterator rbegin() const noexcept { return std::make_reverse_iterator(end()); }
    reverse_iterator rend() noexcept { return std::make_reverse_iterator(begin()); }
    const_reverse_iterator rend() const noexcept { return std::make_reverse_iterator(begin()); }

    const_reverse_iterator crbegin() const noexcept { return std::make_reverse_iterator(end()); }
    const_reverse_iterator crend() const noexcept { return std::make_reverse_iterator(begin()); }

#if __cplusplus >= 201703L
    [[nodiscard]]
#endif
    bool empty() const noexcept { return c_.empty(); }
    size_type size() const noexcept { return c_.size(); }
    size_type max_size() const noexcept { return c_.max_size(); }

    template<class F>
    std::pair<iterator, bool> private_impl_emplace(const Key& t, const F& emplacer) {
        auto it = this->lower_bound(t);
        if (it == end() || compare_(t, *it)) {
            auto count = it - begin();
            emplacer();
            auto first = begin() + count;
            auto nfirst = iterator(c_.begin(), c_.size() - 1, c_.size());
            if (first <= nfirst) {
                std::rotate(first, nfirst, std::next(nfirst));
            } else {
                std::rotate(nfirst, std::next(nfirst), std::next(first));
            }
            return {first, true};
        } else {
            return {it, false};
        }
    }

    template<class... Args>
    std::pair<iterator, bool> emplace(Args&&... args) {
        Key t(static_cast<Args&&>(args)...);
        return this->insert(static_cast<Key&&>(t));
    }

    template<class... Args>
    iterator emplace_hint(const_iterator, Args&&... args) {
        return this->emplace(static_cast<Args&&>(args)...).first;
    }

    std::pair<iterator, bool> insert(const Key& t) {
        return this->private_impl_emplace(
            t,
            [&]() { c_.emplace(c_.end(), t); }
        );
    }

    std::pair<iterator, bool> insert(Key&& t) {
        return this->private_impl_emplace(
            t,
            [&]() { c_.emplace(c_.end(), static_cast<Key&&>(t)); }
        );
    }

    iterator insert(const_iterator, const Key& t) {
        return this->insert(t).first;
    }

    iterator insert(const_iterator, Key&& t) {
        return this->insert(static_cast<Key&&>(t)).first;
    }

    template<class InputIterator>
    void insert(InputIterator first, InputIterator last) {
        // TODO: this is horribly slow
        while (first != last) {
            this->insert(*first);
            ++first;
        }
    }

    template<class InputIterator>
    void insert(sorted_unique_t, InputIterator first, InputIterator last) {
        // TODO: this is horribly slow
        while (first != last) {
            this->insert(*first);
            ++first;
        }
    }

    void insert(std::initializer_list<Key> il) {
        this->insert(il.begin(), il.end());
    }

    void insert(sorted_unique_t s, std::initializer_list<Key> il) {
        this->insert(s, il.begin(), il.end());
    }

    // TODO: this is just for debugging
    const KeyContainer& container() const noexcept {
        return c_;
    }

    KeyContainer extract() && {
        KeyContainer result = static_cast<KeyContainer&&>(c_);
        clear();
        return result;
    }

    void replace(KeyContainer&& ctr) {
        c_ = static_cast<KeyContainer&&>(ctr);
    }

    iterator erase(iterator position) {
        return this->erase(const_iterator(position));
    }

    iterator erase(const_iterator position) {
        auto count = position - begin();
        auto first = iterator(c_.begin(), c_.size() - 1, c_.size());
        auto nfirst = begin() + count;
        if (first <= nfirst) {
            std::rotate(first, nfirst, std::next(nfirst));
        } else {
            std::rotate(nfirst, std::next(nfirst), std::next(first));
        }
        c_.erase(std::prev(c_.end()));
        return begin() + count;
    }

    size_type erase(const Key& t) {
        auto it = this->find(t);
        if (it != this->end()) {
            this->erase(it);
            return 1;
        }
        return 0;
    }

    iterator erase(const_iterator first, const_iterator last) {
        int n = last - first;
        for (int i=0; i < n; ++i) {
            first = this->erase(first);
        }
        return begin() + (first - begin());
    }

    void swap(flat_set& m)
        noexcept(flatset_detail::is_nothrow_swappable<Compare>::value)
    {
        using std::swap;
        swap(compare_, m.compare_);
        swap(c_, m.c_);
    }

    void clear() noexcept {
        c_.clear();
    }

    Compare key_comp() const { return compare_; }
    Compare value_comp() const { return compare_; }

    iterator find(const Key& t) {
        auto it = this->lower_bound(t);
        if (it == this->end() || compare_(t, *it)) {
            return this->end();
        }
        return it;
    }

    const_iterator find(const Key& t) const {
        auto it = this->lower_bound(t);
        if (it == this->end() || compare_(t, *it)) {
            return this->end();
        }
        return it;
    }

    template<class K,
             class Compare_ = Compare, class = typename Compare_::is_transparent>
    iterator find(const K& x) {
        auto it = this->lower_bound(x);
        if (it == this->end() || compare_(x, *it)) {
            return this->end();
        }
        return it;
    }

    template<class K,
             class Compare_ = Compare, class = typename Compare_::is_transparent>
    const_iterator find(const K& x) const {
        auto it = this->lower_bound(x);
        if (it == this->end() || compare_(x, *it)) {
            return this->end();
        }
        return it;
    }

    size_type count(const Key& x) const {
        return this->contains(x) ? 1 : 0;
    }

    template<class K,
             class Compare_ = Compare, class = typename Compare_::is_transparent>
    size_type count(const K& x) const {
        return this->contains(x) ? 1 : 0;
    }

    bool contains(const Key& x) const {
        return this->find(x) != this->end();
    }

    template<class K,
             class Compare_ = Compare, class = typename Compare_::is_transparent>
    bool contains(const K& x) const {
        return this->find(x) != this->end();
    }

    iterator lower_bound(const Key& t) {
        if (c_.size() == 0) {
            return end();
        }

        iterator it = iterator(c_.begin(), 0, c_.size());
        while (true) {
            if (compare_(*it, t)) {
                if (it.has_right_child()) {
                    it.step_right();
                } else {
                    return std::next(it);
                }
            } else {
                if (it.has_left_child()) {
                    it.step_left();
                } else {
                    return it;
                }
            }
        }
    }

    const_iterator lower_bound(const Key& t) const {
        return const_cast<flat_set*>(this)->lower_bound(t);
    }

    template<class K,
             class Compare_ = Compare, class = typename Compare_::is_transparent>
    iterator lower_bound(const K& x) {
        if (c_.size() == 0) {
            return end();
        }

        iterator it = iterator(c_.begin(), 0, c_.size());
        while (true) {
            if (compare_(*it, x)) {
                if (it.has_right_child()) {
                    it.step_right();
                } else {
                    return std::next(it);
                }
            } else {
                if (it.has_left_child()) {
                    it.step_left();
                } else {
                    return it;
                }
            }
        }
    }

    template<class K,
             class Compare_ = Compare, class = typename Compare_::is_transparent>
    const_iterator lower_bound(const K& x) const {
        return const_cast<flat_set*>(this)->lower_bound(x);
    }

    iterator upper_bound(const Key& t) {
        if (c_.size() == 0) {
            return end();
        }

        iterator it = iterator(c_.begin(), 0, c_.size());
        while (true) {
            if (compare_(t, *it)) {
                if (it.has_left_child()) {
                    it.step_left();
                } else {
                    return it;
                }
            } else {
                if (it.has_right_child()) {
                    it.step_right();
                } else {
                    return std::next(it);
                }
            }
        }
    }

    const_iterator upper_bound(const Key& t) const {
        return const_cast<flat_set*>(this)->upper_bound(t);
    }

    template<class K,
             class Compare_ = Compare, class = typename Compare_::is_transparent>
    iterator upper_bound(const K& x) {
        if (c_.size() == 0) {
            return end();
        }

        iterator it = iterator(c_.begin(), 0, c_.size());
        while (true) {
            if (compare_(x, *it)) {
                if (it.has_left_child()) {
                    it.step_left();
                } else {
                    return it;
                }
            } else {
                if (it.has_right_child()) {
                    it.step_right();
                } else {
                    return std::next(it);
                }
            }
        }
    }

    template<class K,
             class Compare_ = Compare, class = typename Compare_::is_transparent>
    const_iterator upper_bound(const K& x) const {
        return const_cast<flat_set*>(this)->upper_bound(x);
    }

    std::pair<iterator, iterator> equal_range(const Key& t) {
        return {
            this->lower_bound(t),
            this->upper_bound(t)
        };
    }

    std::pair<const_iterator, const_iterator> equal_range(const Key& t) const {
        return {
            this->lower_bound(t),
            this->upper_bound(t)
        };
    }

    template<class K,
             class Compare_ = Compare, class = typename Compare_::is_transparent>
    std::pair<iterator, iterator> equal_range(const K& x) {
        return {
            this->lower_bound(x),
            this->upper_bound(x)
        };
    }

    template<class K,
             class Compare_ = Compare, class = typename Compare_::is_transparent>
    std::pair<const_iterator, const_iterator> equal_range(const K& x) const {
        return {
            this->lower_bound(x),
            this->upper_bound(x)
        };
    }

private:
    void sort_and_unique_and_levelorder_impl() {
        std::sort(c_.begin(), c_.end(), compare_);
        auto it = flatset_detail::unique_helper(c_.begin(), c_.end(), compare_);
        c_.erase(it, c_.end());
        flatset_detail::make_levelorder_from_sorted(c_.begin(), c_.end());
    }

    void sort_and_levelorder_impl() {
        std::sort(c_.begin(), c_.end(), compare_);
        flatset_detail::make_levelorder_from_sorted(c_.begin(), c_.end());
    }

    KeyContainer c_;
    Compare compare_;
};

// TODO: all six comparison operators should be invisible friends
template<class Key, class Compare, class KeyContainer>
bool operator==(const flat_set<Key, Compare, KeyContainer>& x, const flat_set<Key, Compare, KeyContainer>& y)
{
    return std::equal(x.begin(), x.end(), y.begin(), y.end());
}

template<class Key, class Compare, class KeyContainer>
bool operator!=(const flat_set<Key, Compare, KeyContainer>& x, const flat_set<Key, Compare, KeyContainer>& y)
{
    return !(x == y);
}

template<class Key, class Compare, class KeyContainer>
bool operator<(const flat_set<Key, Compare, KeyContainer>& x, const flat_set<Key, Compare, KeyContainer>& y)
{
    return std::lexicographical_compare(x.begin(), x.end(), y.begin(), y.end());
}

template<class Key, class Compare, class KeyContainer>
bool operator>(const flat_set<Key, Compare, KeyContainer>& x, const flat_set<Key, Compare, KeyContainer>& y)
{
    return (y < x);
}

template<class Key, class Compare, class KeyContainer>
bool operator<=(const flat_set<Key, Compare, KeyContainer>& x, const flat_set<Key, Compare, KeyContainer>& y)
{
    return !(y < x);
}

template<class Key, class Compare, class KeyContainer>
bool operator>=(const flat_set<Key, Compare, KeyContainer>& x, const flat_set<Key, Compare, KeyContainer>& y)
{
    return !(x < y);
}

template<class Key, class Compare, class KeyContainer>
void swap(flat_set<Key, Compare, KeyContainer>& x, flat_set<Key, Compare, KeyContainer>& y) noexcept(noexcept(x.swap(y)))
{
    return x.swap(y);
}

#if defined(__cpp_deduction_guides)

// TODO: this deduction guide should maybe be constrained by qualifies_as_range
template<class Container,
         class = std::enable_if_t<!flatset_detail::qualifies_as_allocator<Container>::value>>
flat_set(Container)
    -> flat_set<flatset_detail::cont_value_type<Container>>;

template<class Container, class Allocator,
         class = std::enable_if_t<!flatset_detail::qualifies_as_allocator<Container>::value &&
                                  flatset_detail::qualifies_as_allocator<Allocator>::value &&
                                  std::uses_allocator<Container, Allocator>::value>>
flat_set(Container, Allocator)
    -> flat_set<flatset_detail::cont_value_type<Container>>;

template<class Container,
         class = std::enable_if_t<!flatset_detail::qualifies_as_allocator<Container>::value>>
flat_set(sorted_unique_t, Container)
    -> flat_set<flatset_detail::cont_value_type<Container>>;

template<class Container, class Allocator,
         class = std::enable_if_t<!flatset_detail::qualifies_as_allocator<Container>::value &&
                                  flatset_detail::qualifies_as_allocator<Allocator>::value &&
                                  std::uses_allocator<Container, Allocator>::value>>
flat_set(sorted_unique_t, Container, Allocator)
    -> flat_set<flatset_detail::cont_value_type<Container>>;

template<class InputIterator, class Compare = std::less<flatset_detail::iter_value_type<InputIterator>>,
         class = std::enable_if_t<flatset_detail::qualifies_as_input_iterator<InputIterator>::value &&
                                  !flatset_detail::qualifies_as_allocator<Compare>::value>>
flat_set(InputIterator, InputIterator, Compare = Compare())
    -> flat_set<flatset_detail::iter_value_type<InputIterator>, Compare>;

template<class InputIterator, class Compare, class Allocator,
         class = std::enable_if_t<flatset_detail::qualifies_as_input_iterator<InputIterator>::value &&
                                  !flatset_detail::qualifies_as_allocator<Compare>::value &&
                                  flatset_detail::qualifies_as_allocator<Allocator>::value>>
flat_set(InputIterator, InputIterator, Compare, Allocator)
    -> flat_set<flatset_detail::iter_value_type<InputIterator>, Compare>;

template<class InputIterator, class Allocator,
         class = std::enable_if_t<flatset_detail::qualifies_as_input_iterator<InputIterator>::value &&
                                  flatset_detail::qualifies_as_allocator<Allocator>::value>>
flat_set(InputIterator, InputIterator, Allocator, int=0/*to please MSVC*/)
    -> flat_set<flatset_detail::iter_value_type<InputIterator>>;

#endif

} // namespace stdext
