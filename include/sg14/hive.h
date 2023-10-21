// Copyright (c) 2022, Matthew Bentley (mattreecebentley@gmail.com) www.plflib.org
// Modified by Arthur O'Dwyer, 2022. Original source:
// https://github.com/mattreecebentley/plf_hive/blob/7b7763f/plf_hive.h

// zLib license (https://www.zlib.net/zlib_license.html):
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
//  claim that you wrote the original software. If you use this software
//  in a product, an acknowledgement in the product documentation would be
//  appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//  misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.

#pragma once

#ifndef SG14_HIVE_RANDOM_ACCESS_ITERATORS
 #define SG14_HIVE_RANDOM_ACCESS_ITERATORS 0
#endif

#if SG14_HIVE_RANDOM_ACCESS_ITERATORS
 #define SG14_HIVE_RELATIONAL_OPERATORS 1  // random access iterators require relational operators
#endif

#ifndef SG14_HIVE_RELATIONAL_OPERATORS
 #define SG14_HIVE_RELATIONAL_OPERATORS 1
#endif

#ifndef SG14_HIVE_DEBUGGING
 #define SG14_HIVE_DEBUGGING 0
#endif

#include <stddef.h>
#include <algorithm>
#include <cassert>
#include <cstring>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <limits>
#include <memory>
#include <type_traits>
#include <utility>

#if __cplusplus >= 202002L
#include <compare>
#include <concepts>
#include <ranges>
#endif // __cplusplus >= 202002L

#ifndef SG14_HIVE_THROW
#include <stdexcept>
#define SG14_HIVE_THROW(x) throw (x)
#endif

namespace sg14 {

// Polyfill std::type_identity_t
template<class T> struct hive_identity { using type = T; };
template<class T> using hive_identity_t = typename hive_identity<T>::type;

// Polyfill std::forward_iterator
#if __cpp_lib_ranges >= 201911L
template<class It>
using hive_fwd_iterator = std::bool_constant<std::forward_iterator<It>>;
#else
template<class It>
using hive_fwd_iterator = std::is_base_of<std::forward_iterator_tag, typename std::iterator_traits<It>::iterator_category>;
#endif

template<class R>
struct hive_txn {
    R& rollback_;
    bool done_ = false;
    explicit hive_txn(R& rollback) : rollback_(rollback) {}
    ~hive_txn() { if (!done_) rollback_(); }
};

template<class F, class R>
static inline void hive_try_rollback(F&& task, R&& rollback) {
    hive_txn<R> txn(rollback);
    task();
    txn.done_ = true;
}

template<class F, class R>
static inline void hive_try_finally(F&& task, R&& finally) {
    hive_txn<R> txn(finally);
    task();
}

namespace hive_priority {
    struct performance {
        using skipfield_type = unsigned short;
    };
    struct memory_use {
        using skipfield_type = unsigned char;
    };
}

template <class T, class allocator_type = std::allocator<T>, class priority = sg14::hive_priority::performance>
class hive {
    template<bool IsConst> class hive_iterator;
    template<bool IsConst> class hive_reverse_iterator;
    friend class hive_iterator<false>;
    friend class hive_iterator<true>;

    using skipfield_type = typename priority::skipfield_type;
    using AllocTraits = std::allocator_traits<allocator_type>;

public:
    using value_type = T;
    using size_type = typename AllocTraits::size_type;
    using difference_type = typename AllocTraits::difference_type;
    using pointer = typename AllocTraits::pointer;
    using const_pointer = typename AllocTraits::const_pointer;
    using reference = T&;
    using const_reference = const T&;
    using iterator = hive_iterator<false>;
    using const_iterator = hive_iterator<true>;
    using reverse_iterator = hive_reverse_iterator<false>;
    using const_reverse_iterator = hive_reverse_iterator<true>;

private:
    inline auto make_value_callback(size_type, const T& value) {
        struct CB {
            const T& value_;
            void construct_and_increment(allocator_type& ea, AlignedEltPtr& p) {
                std::allocator_traits<allocator_type>::construct(ea, p[0].t(), value_);
                ++p;
            }
        };
        return CB{value};
    }

    template<class It, class Sent>
    inline auto make_itpair_callback(It& first, Sent&) {
        struct CB {
            It& first_;
            void construct_and_increment(allocator_type& ea, AlignedEltPtr& p) {
                std::allocator_traits<allocator_type>::construct(ea, p[0].t(), *first_);
                ++p;
                ++first_;
            }
        };
        return CB{first};
    }

    template<class U> using AllocOf = typename std::allocator_traits<allocator_type>::template rebind_alloc<U>;
    template<class U> using PtrOf = typename std::allocator_traits<AllocOf<U>>::pointer;

    struct overaligned_elt {
        struct NextAndPrev {
            skipfield_type nextlink_;
            skipfield_type prevlink_;
        };
        union {
            char dummy_;
            NextAndPrev s_;
            T t_;
        };
        PtrOf<T> t() { return cast_pointer<PtrOf<T>>(std::addressof(t_)); }
        ~overaligned_elt() = delete;
    };

    template <class D, class S>
    static D cast_pointer(S source_pointer) {
#if __cpp_lib_to_address >= 201711L
        return reinterpret_cast<D>(std::to_address(source_pointer));
#else
        return reinterpret_cast<D>(source_pointer); // reject fancy pointer types
#endif
    }

    struct group;
    using AlignedEltPtr = PtrOf<overaligned_elt>;
    using GroupPtr = PtrOf<group>;
    using SkipfieldPtr = PtrOf<skipfield_type>;

    struct group {
        AlignedEltPtr last_endpoint;
            // The address which is one-past the highest cell number that's been used so far in this group - does not change via erasure
            // but may change via insertion/emplacement/assignment (if no previously-erased locations are available to insert to).
            // This variable is necessary because an iterator cannot access the hive's end_. It is probably the most-used variable
            // in general hive usage (being heavily used in operator ++, --), so is first in struct. If all cells in the group have
            // been inserted into at some point, it will be == reinterpret_cast<AlignedEltPtr>(skipfield).
        GroupPtr next_group = nullptr;
        GroupPtr prev_group = nullptr;
        skipfield_type free_list_head = std::numeric_limits<skipfield_type>::max();
            // The index of the last erased element in the group. The last erased element will, in turn, contain
            // the number of the index of the next erased element, and so on. If this is == maximum skipfield_type value
            // then free_list is empty ie. no erasures have occurred in the group.
        skipfield_type capacity;
        skipfield_type size = 0;
            // The total number of active elements in group - changes with insert and erase commands - used to check for empty group in erase function,
            // as an indication to remove the group. Also used in combination with capacity to check if group is full.
        GroupPtr next_erasure_ = nullptr;
            // The next group in the singly-linked list of groups with erasures ie. with active erased-element free lists. nullptr if no next group.
#if SG14_HIVE_RELATIONAL_OPERATORS
        size_type groupno_ = 0;
            // Used for comparison (> < >= <= <=>) iterator operators (used by distance function and user).
#endif

#if SG14_HIVE_RELATIONAL_OPERATORS
        inline size_t group_number() const { return groupno_; }
        inline void set_group_number(size_type x) { groupno_ = x; }
#else
        inline size_t group_number() const { return 42; }
        inline void set_group_number(size_type) { }
#endif

#if SG14_HIVE_DEBUGGING
        void debug_dump() {
            printf(
                "  group #%zu [%zu/%zu used] (last_endpoint=%p, elts=%p, skipfield=%p, freelist=%zu, erasenext=%p)",
                group_number(), size_t(size), size_t(capacity),
                (void*)last_endpoint, (void*)addr_of_element(0), (void*)addr_of_skipfield(0), size_t(free_list_head), (void*)next_erasure_
            );
            if (next_group) {
                printf(" next: #%zu", next_group->group_number());
            } else {
                printf(" next: null");
            }
            if (prev_group) {
                printf(" prev: #%zu\n", prev_group->group_number());
            } else {
                printf(" prev: null\n");
            }
            printf("  skipfield[] =");
            for (int i = 0; i < capacity; ++i) {
                if (skipfield(i) == 0) {
                    printf(" _");
                } else {
                    printf(" %d", int(skipfield(i)));
                }
            }
            if (skipfield(capacity) == 0) {
                printf(" [_]\n");
            } else {
                printf(" [%d]\n", int(skipfield(capacity)));
            }
        }
#endif // SG14_HIVE_DEBUGGING

        explicit group(skipfield_type cap) :
            last_endpoint(addr_of_element(0)), capacity(cap)
        {
            std::fill_n(addr_of_skipfield(0), cap + 1, skipfield_type());
        }

        bool is_packed() const {
            return free_list_head == std::numeric_limits<skipfield_type>::max();
        }

        AlignedEltPtr addr_of_element(size_type n) { return GroupAllocHelper::elements(this) + n; }
        overaligned_elt& element(size_type n) { return GroupAllocHelper::elements(this)[n]; }
        AlignedEltPtr end_of_elements() { return GroupAllocHelper::elements(this) + capacity; }

        SkipfieldPtr addr_of_skipfield(size_type n) { return GroupAllocHelper::skipfield(this) + n; }
        skipfield_type& skipfield(size_type n) { return GroupAllocHelper::skipfield(this)[n]; }

        skipfield_type index_of_last_endpoint() { return last_endpoint - addr_of_element(0); }

        void reset(skipfield_type increment, GroupPtr next, GroupPtr prev, size_type groupno) {
            last_endpoint = addr_of_element(increment);
            next_group = next;
            free_list_head = std::numeric_limits<skipfield_type>::max();
            prev_group = prev;
            size = increment;
            next_erasure_ = nullptr;
            set_group_number(groupno);
            std::fill_n(addr_of_skipfield(0), capacity, skipfield_type());
        }
    };

    template <bool IsConst>
    class hive_iterator {
        GroupPtr group_ = GroupPtr();
        skipfield_type idx_ = 0;

    public:
#if SG14_HIVE_DEBUGGING
        void debug_dump() const {
            printf("iterator(");
            if (group_) printf("#%zu", group_->group_number());
            else printf("null");
            printf(", %zu)\n", size_t(idx_));
        }
#endif // SG14_HIVE_DEBUGGING

#if SG14_HIVE_RANDOM_ACCESS_ITERATORS
        using iterator_category = std::random_access_iterator_tag;
#else
        using iterator_category = std::bidirectional_iterator_tag;
#endif
        using value_type = typename hive::value_type;
        using difference_type = typename hive::difference_type;
        using pointer = std::conditional_t<IsConst, typename hive::const_pointer, typename hive::pointer>;
        using reference = std::conditional_t<IsConst, typename hive::const_reference, typename hive::reference>;

        friend class hive;
        friend class hive_reverse_iterator<false>;
        friend class hive_reverse_iterator<true>;

        explicit hive_iterator() = default;
        hive_iterator(hive_iterator&&) = default;
        hive_iterator(const hive_iterator&) = default;
        hive_iterator& operator=(hive_iterator&&) = default;
        hive_iterator& operator=(const hive_iterator&) = default;

        template<bool IsConst_ = IsConst, class = std::enable_if_t<IsConst_>>
        hive_iterator(const hive_iterator<false>& rhs) :
            group_(rhs.group_),
            idx_(rhs.idx_)
        {}

        template<bool IsConst_ = IsConst, class = std::enable_if_t<IsConst_>>
        hive_iterator(hive_iterator<false>&& rhs) :
            group_(std::move(rhs.group_)),
            idx_(std::move(rhs.idx_))
        {}

        friend void swap(hive_iterator& a, hive_iterator& b) noexcept {
            using std::swap;
            swap(a.group_, b.group_);
            swap(a.idx_, b.idx_);
        }

#if __cpp_impl_three_way_comparison >= 201907L
        friend bool operator==(const hive_iterator&, const hive_iterator&) = default;
#else
        friend bool operator==(const hive_iterator& a, const hive_iterator& b) { return a.group_ == b.group_ && a.idx_ == b.idx_; }
        friend bool operator!=(const hive_iterator& a, const hive_iterator& b) { return a.group_ != b.group_ || a.idx_ != b.idx_; }
#endif

#if SG14_HIVE_RELATIONAL_OPERATORS
#if __cpp_impl_three_way_comparison >= 201907L
        friend std::strong_ordering operator<=>(const hive_iterator& a, const hive_iterator& b) {
            return a.group_ == b.group_ ?
                a.idx_ <=> b.idx_ :
                a.group_->groupno_ <=> b.group_->groupno_;
        }
#else
        friend bool operator<(const hive_iterator& a, const hive_iterator& b) {
            return a.group_ == b.group_ ?
                a.idx_ < b.idx_ :
                a.group_->groupno_ < b.group_->groupno_;
        }

        friend bool operator>(const hive_iterator& a, const hive_iterator& b) {
            return a.group_ == b.group_ ?
                a.idx_ > b.idx_ :
                a.group_->groupno_ > b.group_->groupno_;
        }

        friend bool operator<=(const hive_iterator& a, const hive_iterator& b) {
            return a.group_ == b.group_ ?
                a.idx_ <= b.idx_ :
                a.group_->groupno_ < b.group_->groupno_;
        }

        friend bool operator>=(const hive_iterator& a, const hive_iterator& b) {
            return a.group_ == b.group_ ?
                a.idx_ >= b.idx_ :
                a.group_->groupno_ > b.group_->groupno_;
        }
#endif
#endif

        inline reference operator*() const noexcept {
            return *group_->element(idx_).t();
        }

        inline pointer operator->() const noexcept {
            return group_->element(idx_).t();
        }

        hive_iterator& operator++() {
            assert(group_ != nullptr);
            auto inc = 1 + group_->skipfield(idx_ + 1);
            idx_ += inc;
            if (idx_ == group_->index_of_last_endpoint() && group_->next_group != nullptr) {
                group_ = group_->next_group;
                idx_ = group_->skipfield(0);
            }
            return *this;
        }

        hive_iterator& operator--() {
            assert(group_ != nullptr);
            if (idx_ != 0) {
                skipfield_type dec = group_->skipfield(idx_ - 1);
                if (dec != idx_) {
                   idx_ -= dec + 1;
                   return *this;
                }
            }
            group_ = group_->prev_group;
            idx_ = group_->capacity - 1 - group_->skipfield(group_->capacity - 1);
            return *this;
        }

        inline hive_iterator operator++(int) { auto copy = *this; ++*this; return copy; }
        inline hive_iterator operator--(int) { auto copy = *this; --*this; return copy; }

    private:
        template<bool IsConst_ = IsConst, class = std::enable_if_t<IsConst_>>
        hive_iterator<false> unconst() const {
            hive_iterator<false> it;
            it.group_ = group_;
            it.idx_ = idx_;
            return it;
        }

        explicit hive_iterator(GroupPtr g, size_t idx) :
            group_(g), idx_(idx) {}

        void advance_forward(difference_type n) {
            assert(n > 0);
            assert(group_ != nullptr);

            if (idx_ != group_->skipfield(0)) {
                skipfield_type endpoint = group_->index_of_last_endpoint();
                while (true) {
                    ++idx_;
                    idx_ += group_->skipfield(idx_);
                    --n;
                    if (idx_ == endpoint) {
                        break;
                    } else if (n == 0) {
                        return;
                    }
                }
                if (group_->next_group == nullptr) {
                    return;
                }
                group_ = group_->next_group;
                if (n == 0) {
                    idx_ = group_->skipfield(0);
                    return;
                }
            }

            while (static_cast<difference_type>(group_->size) <= n) {
                if (group_->next_group == nullptr) {
                    idx_ = group_->index_of_last_endpoint();
                    return;
                } else {
                    n -= group_->size;
                    group_ = group_->next_group;
                    if (n == 0) {
                        idx_ = group_->skipfield(0);
                        return;
                    }
                }
            }

            if (group_->is_packed()) {
                idx_ = n;
            } else {
                idx_ = group_->skipfield(0);
                do {
                    idx_ += 1 + group_->skipfield(idx_ + 1);
                } while (--n != 0);
            }
        }

        void advance_backward(difference_type n) {
            assert(n < 0);
            assert(!(idx_ == group_->skipfield(0) && group_->prev_group == nullptr));

            if (idx_ != group_->index_of_last_endpoint()) {
                if (group_->is_packed()) {
                    difference_type distance_from_beginning = -static_cast<difference_type>(idx_);
                    if (n >= distance_from_beginning) {
                        idx_ += n;
                        return;
                    } else if (group_->prev_group == nullptr) {
                        idx_ = 0;
                        return;
                    } else {
                        n -= distance_from_beginning;
                    }
                } else {
                    skipfield_type beginning_point = group_->skipfield(0);
                    while (idx_ != beginning_point) {
                        --idx_;
                        idx_ -= group_->skipfield(idx_);
                        if (++n == 0) {
                            return;
                        }
                    }
                    if (group_->prev_group == nullptr) {
                        idx_ = group_->skipfield(0);
                        return;
                    }
                }
                group_ = group_->prev_group;
            }

            while (n < -static_cast<difference_type>(group_->size)) {
                if (group_->prev_group == nullptr) {
                    idx_ = group_->skipfield(0);
                    return;
                }
                n += group_->size;
                group_ = group_->prev_group;
            }

            if (n == -static_cast<difference_type>(group_->size)) {
                idx_ = group_->skipfield(0);
            } else if (group_->is_packed()) {
                idx_ = group_->size + n;
            } else {
                idx_ = group_->index_of_last_endpoint();
                do {
                    --idx_;
                    idx_ -= group_->skipfield(idx_);
                } while (++n != 0);
            }
        }

        inline difference_type index_in_group() const { return idx_; }

        difference_type distance_from_start_of_group() const {
            assert(group_ != nullptr);
            if (group_->is_packed() || idx_ == 0) {
                return idx_;
            } else {
                difference_type count = 0;
                skipfield_type endpoint = group_->index_of_last_endpoint();
                for (skipfield_type i = idx_; i != endpoint; ++count) {
                    ++i;
                    i += group_->skipfield(i);
                }
                return group_->size - count;
            }
        }

        difference_type distance_from_end_of_group() const {
            assert(group_ != nullptr);
            if (group_->is_packed() || idx_ == 0) {
                return group_->size - idx_;
            } else {
                difference_type count = 0;
                skipfield_type endpoint = group_->index_of_last_endpoint();
                for (skipfield_type i = idx_; i != endpoint; ++count) {
                    ++i;
                    i += group_->skipfield(i);
                }
                return count;
            }
        }

        difference_type distance_forward(hive_iterator last) const {
            if (last.group_ != group_) {
                difference_type count = last.distance_from_start_of_group();
                for (GroupPtr g = last.group_->prev_group; g != group_; g = g->prev_group) {
                    count += g->size;
                }
                return count + this->distance_from_end_of_group();
            } else if (idx_ == last.idx_) {
                return 0;
            } else if (group_->is_packed()) {
                return last.idx_ - idx_;
            } else {
                difference_type count = 0;
                while (last.idx_ != idx_) {
                    --last.idx_;
                    last.idx_ -= last.group_->skipfield(last.idx_);
                    ++count;
                }
                return count;
            }
        }

    public:
        inline void advance(difference_type n) {
            if (n > 0) {
                advance_forward(n);
            } else if (n < 0) {
                advance_backward(n);
            }
        }

        inline hive_iterator next(difference_type n) const {
            auto copy = *this;
            copy.advance(n);
            return copy;
        }

        inline hive_iterator prev(difference_type n) const {
            auto copy = *this;
            copy.advance(-n);
            return copy;
        }

        difference_type distance(hive_iterator last) const {
#if SG14_HIVE_RELATIONAL_OPERATORS
            if (last < *this) {
                return -last.distance_forward(*this);
            }
#endif
            return distance_forward(last);
        }

#if SG14_HIVE_RANDOM_ACCESS_ITERATORS
        friend hive_iterator& operator+=(hive_iterator& a, difference_type n) { a.advance(n); return a; }
        friend hive_iterator& operator-=(hive_iterator& a, difference_type n) { a.advance(-n); return a; }
        friend hive_iterator operator+(const hive_iterator& a, difference_type n) { return a.next(n); }
        friend hive_iterator operator+(difference_type n, const hive_iterator& a) { return a.next(n); }
        friend hive_iterator operator-(const hive_iterator& a, difference_type n) { return a.prev(n); }
        friend difference_type operator-(const hive_iterator& a, const hive_iterator& b) { return b.distance(a); }
        reference operator[](difference_type n) const { return *next(n); }
#endif
    }; // class hive_iterator

    template <bool IsConst>
    class hive_reverse_iterator {
        hive_iterator<IsConst> it_;

    public:
#if SG14_HIVE_RANDOM_ACCESS_ITERATORS
        using iterator_category = std::random_access_iterator_tag;
#else
        using iterator_category = std::bidirectional_iterator_tag;
#endif
        using value_type = typename hive::value_type;
        using difference_type = typename hive::difference_type;
        using pointer = std::conditional_t<IsConst, typename hive::const_pointer, typename hive::pointer>;
        using reference = std::conditional_t<IsConst, typename hive::const_reference, typename hive::reference>;

        hive_reverse_iterator() = default;
        hive_reverse_iterator(hive_reverse_iterator&&) = default;
        hive_reverse_iterator(const hive_reverse_iterator&) = default;
        hive_reverse_iterator& operator=(hive_reverse_iterator&&) = default;
        hive_reverse_iterator& operator=(const hive_reverse_iterator&) = default;

        template<bool IsConst_ = IsConst, class = std::enable_if_t<IsConst_>>
        hive_reverse_iterator(const hive_reverse_iterator<false>& rhs) :
            it_(rhs.base())
        {}

        explicit hive_reverse_iterator(hive_iterator<IsConst>&& rhs) : it_(std::move(rhs)) {}
        explicit hive_reverse_iterator(const hive_iterator<IsConst>& rhs) : it_(rhs) {}

#if __cpp_impl_three_way_comparison >= 201907L
        friend bool operator==(const hive_reverse_iterator& a, const hive_reverse_iterator& b) = default;
#else
        friend bool operator==(const hive_reverse_iterator& a, const hive_reverse_iterator& b) { return b.it_ == a.it_; }
        friend bool operator!=(const hive_reverse_iterator& a, const hive_reverse_iterator& b) { return b.it_ != a.it_; }
#endif

#if SG14_HIVE_RELATIONAL_OPERATORS
#if __cpp_impl_three_way_comparison >= 201907L
        friend std::strong_ordering operator<=>(const hive_reverse_iterator& a, const hive_reverse_iterator& b) { return (b.it_ <=> a.it_); }
#else
        friend bool operator<(const hive_reverse_iterator& a, const hive_reverse_iterator& b) { return b.it_ < a.it_; }
        friend bool operator>(const hive_reverse_iterator& a, const hive_reverse_iterator& b) { return b.it_ > a.it_; }
        friend bool operator<=(const hive_reverse_iterator& a, const hive_reverse_iterator& b) { return b.it_ <= a.it_; }
        friend bool operator>=(const hive_reverse_iterator& a, const hive_reverse_iterator& b) { return b.it_ >= a.it_; }
#endif
#endif

        inline reference operator*() const noexcept { auto jt = it_; --jt; return *jt; }
        inline pointer operator->() const noexcept { auto jt = it_; --jt; return jt.operator->(); }
        hive_reverse_iterator& operator++() { --it_; return *this; }
        hive_reverse_iterator operator++(int) { auto copy = *this; --it_; return copy; }
        hive_reverse_iterator& operator--() { ++it_; return *this; }
        hive_reverse_iterator operator--(int) { auto copy = *this; ++it_; return copy; }

        hive_iterator<IsConst> base() const noexcept { return it_; }

        hive_reverse_iterator next(difference_type n) const {
            auto copy = *this;
            copy.it_.advance(-n);
            return copy;
        }

        hive_reverse_iterator prev(difference_type n) const {
            auto copy = *this;
            copy.it_.advance(n);
            return copy;
        }

        difference_type distance(const hive_reverse_iterator &last) const {
            return last.it_.distance(it_);
        }

        void advance(difference_type n) {
            it_.advance(-n);
        }

#if SG14_HIVE_RANDOM_ACCESS_ITERATORS
        friend hive_reverse_iterator& operator+=(hive_reverse_iterator& a, difference_type n) { a.advance(n); return a; }
        friend hive_reverse_iterator& operator-=(hive_reverse_iterator& a, difference_type n) { a.advance(-n); return a; }
        friend hive_reverse_iterator operator+(const hive_reverse_iterator& a, difference_type n) { return a.next(n); }
        friend hive_reverse_iterator operator+(difference_type n, const hive_reverse_iterator& a) { return a.next(n); }
        friend hive_reverse_iterator operator-(const hive_reverse_iterator& a, difference_type n) { return a.prev(n); }
        friend difference_type operator-(const hive_reverse_iterator& a, const hive_reverse_iterator& b) { return b.distance(a); }
        reference operator[](difference_type n) const { return *next(n); }
#endif
    }; // hive_reverse_iterator

public:
    void assert_invariants() const {
#if SG14_HIVE_DEBUGGING
        assert(size_ <= capacity_);
        if (size_ == 0) {
            assert(begin_ == end_);
            if (capacity_ == 0) {  // TODO FIXME BUG HACK: this should be `if (true)`
                assert(begin_.group_ == nullptr);
                assert(begin_.idx_ == 0);
                assert(end_.group_ == nullptr);
                assert(end_.idx_ == 0);
            }
            assert(groups_with_erasures_ == nullptr);
            if (capacity_ == 0) {
                assert(unused_groups_ == nullptr);
            } else {
                if (begin_.group_ == nullptr) {
                    assert(unused_groups_ != nullptr);  // the capacity must be somewhere
                }
            }
        } else {
            assert(begin_.group_ != nullptr);
            assert(begin_.group_->prev_group == nullptr);
            assert(end_.group_ != nullptr);
            assert(end_.idx_ == end_.group_->index_of_last_endpoint());
            assert(end_.group_->next_group == nullptr);
            assert(begin_ != end_);
            if (capacity_ == size_) {
                assert(unused_groups_ == nullptr);
            }
        }
        size_type total_size = 0;
        size_type total_cap = 0;
        for (GroupPtr g = begin_.group_; g != nullptr; g = g->next_group) {
            // assert(g->size >= 1); // TODO FIXME BUG HACK
            assert(g->size <= g->capacity);
            total_size += g->size;
            total_cap += g->capacity;
            if (g->is_packed()) {
                assert(g->last_endpoint == g->addr_of_element(g->size));
            } else {
                assert(g->size < g->capacity);
                assert(g->last_endpoint > g->addr_of_element(g->size));
                assert(g->skipfield(g->free_list_head) != 0);
            }
            if (g->last_endpoint != g->end_of_elements()) {
                assert(g == end_.group_);
                assert(g->next_group == nullptr);
            }
            assert(g->skipfield(g->last_endpoint - g->addr_of_element(0)) == 0);
            if (g->size != g->capacity && g->next_group != nullptr) {
                assert(!g->is_packed());
            }
#if SG14_HIVE_RELATIONAL_OPERATORS
            if (g->next_group != nullptr) {
                assert(g->group_number() < g->next_group->group_number());
            }
#endif
            skipfield_type total_skipped = 0;
            for (skipfield_type sb = g->free_list_head; sb != std::numeric_limits<skipfield_type>::max(); sb = g->element(sb).s_.nextlink_) {
                skipfield_type skipblock_length = g->skipfield(sb);
                assert(skipblock_length != 0);
                assert(g->skipfield(sb + skipblock_length - 1) == skipblock_length);
                total_skipped += skipblock_length;
                if (sb == g->free_list_head) {
                    assert(g->element(sb).s_.prevlink_ == std::numeric_limits<skipfield_type>::max());
                }
                if (g->element(sb).s_.nextlink_ != std::numeric_limits<skipfield_type>::max()) {
                    assert(g->element(g->element(sb).s_.nextlink_).s_.prevlink_ == sb);
                }
            }
            if (g == end_.group_) {
                assert(g->capacity == g->size + total_skipped + (g->end_of_elements() - g->last_endpoint));
            } else {
                assert(g->capacity == g->size + total_skipped);
            }
        }
        assert(total_size == size_);
        assert((unused_groups_ != nullptr) == (unused_groups_tail_ != nullptr));
        for (GroupPtr g = unused_groups_; g != nullptr; g = g->next_group) {
            total_cap += g->capacity;
            if (g->next_group == nullptr) {
                assert(unused_groups_tail_ == g);
            }
        }
        assert(total_cap == capacity_);
        if (size_ == capacity_) {
            assert(groups_with_erasures_ == nullptr);
            assert(unused_groups_ == nullptr);
        } else {
            size_type space_in_last_group = end_.group_ != nullptr ? (end_.group_->capacity - end_.idx_) : 0;
            assert(size_ + space_in_last_group == capacity_ || groups_with_erasures_ != nullptr || unused_groups_ != nullptr);
        }
        for (GroupPtr g = groups_with_erasures_; g != nullptr; g = g->next_erasure_) {
            assert(g->size < g->capacity);
        }
#endif
    }

    void debug_dump() const {
#if SG14_HIVE_DEBUGGING
        printf(
            "hive [%zu/%zu used] (erase=%p, unused=%p, mincap=%zu, maxcap=%zu)\n",
            size_t(size_), size_t(capacity_),
            groups_with_erasures_, unused_groups_,
            size_t(impl_min_block_size()), size_t(impl_max_block_size())
        );
        printf("  begin="); begin_.debug_dump();
        size_t total = 0;
        group *prev = nullptr;
        for (auto *g = begin_.group_; g != nullptr; g = g->next_group) {
            g->debug_dump();
            total += g->size;
            assert(g->prev_group == prev);
            prev = g;
        }
        assert(total == size_);
        printf("  end="); end_.debug_dump();
        if (end_.group_) {
            assert(end_.group_->next_group == nullptr);
        }
        printf("UNUSED GROUPS:\n");
        for (auto *g = unused_groups_; g != nullptr; g = g->next_group) {
            g->debug_dump();
            assert(g != begin_.group_);
            assert(g != end_.group_);
        }
        printf("GROUPS WITH ERASURES:");
        for (auto *g = groups_with_erasures_; g != nullptr; g = g->next_erasure_) {
            printf(" %zu", g->group_number());
        }
        printf("\n");
#endif // SG14_HIVE_DEBUGGING
    }

private:
    iterator end_;
    iterator begin_;
    GroupPtr groups_with_erasures_ = GroupPtr();
    GroupPtr unused_groups_ = GroupPtr();
    GroupPtr unused_groups_tail_ = GroupPtr();
    size_type size_ = 0;
    size_type capacity_ = 0;
    allocator_type allocator_;

    size_type trailing_capacity() const {
        if (end_.group_ == nullptr) {
            return 0;
        } else {
            return (end_.group_->capacity - end_.index_in_group());
        }
    }

    void blank() {
        end_ = iterator();
        begin_ = iterator();
        groups_with_erasures_ = nullptr;
        unused_groups_ = nullptr;
        unused_groups_tail_ = nullptr;
        size_ = 0;
        capacity_ = 0;
    }

#if SG14_HIVE_RELATIONAL_OPERATORS
    void renumber_all_groups() {
        size_type groupno = 0;
        for (GroupPtr g = begin_.group_; g != nullptr; g = g->next_group) {
            g->set_group_number(groupno++);
        }
    }
#else
    inline void renumber_all_groups() { }
#endif

public:
    hive() = default;
    explicit hive(const allocator_type &alloc) : allocator_(alloc) {}
    hive(const hive& h) : hive(h, std::allocator_traits<allocator_type>::select_on_container_copy_construction(h.allocator_)) {}

    hive(const hive& h, const hive_identity_t<allocator_type>& alloc) :
        allocator_(alloc)
    {
        reserve(h.size());
        range_assign_impl(h.begin(), h.end());
    }

    hive(hive&& source) noexcept :
        end_(std::move(source.end_)),
        begin_(std::move(source.begin_)),
        groups_with_erasures_(std::move(source.groups_with_erasures_)),
        unused_groups_(std::move(source.unused_groups_)),
        size_(source.size_),
        capacity_(source.capacity_),
        allocator_(source.get_allocator())
    {
        assert(&source != this);
        source.blank();
    }

    hive(hive&& source, const hive_identity_t<allocator_type>& alloc) : hive(alloc)
    {
        assert(&source != this);
        bool should_use_source_allocator = (
            std::allocator_traits<allocator_type>::is_always_equal::value ||
            alloc == source.get_allocator()
        );
        if (should_use_source_allocator) {
            *this = std::move(source);
        } else {
            reserve(source.size());
            range_assign_impl(std::make_move_iterator(source.begin()), std::make_move_iterator(source.end()));
        }
    }

    hive(size_type n, const T& value, const allocator_type &alloc = allocator_type()) :
        allocator_(alloc)
    {
        assign(n, value);
    }

    explicit hive(size_type n) { assign(n, T()); }

    hive(size_type n, const allocator_type &alloc) :
        allocator_(alloc)
    {
        assign(n, T());
    }

    template<class It, class = std::enable_if_t<!std::is_integral<It>::value>>
    hive(It first, It last)
    {
        assign(std::move(first), std::move(last));
    }

    template<class It, class = std::enable_if_t<!std::is_integral<It>::value>>
    hive(It first, It last, const allocator_type &alloc) :
        allocator_(alloc)
    {
        assign(std::move(first), std::move(last));
    }

    hive(std::initializer_list<T> il, const hive_identity_t<allocator_type>& alloc = allocator_type()) :
        allocator_(alloc)
    {
        assign(il.begin(), il.end());
    }

#if __cpp_lib_ranges >= 201911L && __cpp_lib_ranges_to_container >= 202202L
    template<std::ranges::input_range R>
        requires std::convertible_to<std::ranges::range_reference_t<R>, T>
    hive(std::from_range_t, R&& rg)
    {
        assign_range(std::forward<R>(rg));
    }

    template<std::ranges::input_range R>
        requires std::convertible_to<std::ranges::range_reference_t<R>, T>
    hive(std::from_range_t, R&& rg, const allocator_type &alloc) :
        allocator_(alloc)
    {
        assign_range(std::forward<R>(rg));
    }
#endif // __cpp_lib_ranges >= 201911L && __cpp_lib_ranges_to_container >= 202202L

    ~hive() {
        assert_invariants();
        destroy_all_data();
    }

private:
    struct GroupAllocHelper {
        union U {
            group g_;
            overaligned_elt elt_;
            ~U() = delete;
        };
        struct type {
            alignas(U) char dummy;
        };

        static inline AlignedEltPtr elements(GroupPtr g) {
            auto p = PtrOf<char>(g);
            p += sizeof(U);
            return AlignedEltPtr(p);
        }

        static inline SkipfieldPtr skipfield(GroupPtr g) {
            return SkipfieldPtr(elements(g) + g->capacity);
        }

        static GroupPtr allocate_group(allocator_type a, size_t cap) {
            auto ta = AllocOf<type>(a);
            size_t bytes_for_group = sizeof(U);
            size_t bytes_for_elts = sizeof(overaligned_elt) * cap;
            size_t bytes_for_skipfield = sizeof(skipfield_type) * (cap + 1);
            size_t n = (bytes_for_group + bytes_for_elts + bytes_for_skipfield + sizeof(type) - 1) / sizeof(type);
            PtrOf<type> p = std::allocator_traits<AllocOf<type>>::allocate(ta, n);
            GroupPtr g = PtrOf<group>(p);
            ::new (cast_pointer<void*>(g)) group(cap);
            return g;
        }
        static void deallocate_group(allocator_type a, GroupPtr g) {
            size_t cap = g->capacity;
            auto ta = AllocOf<type>(a);
            size_t bytes_for_group = sizeof(U);
            size_t bytes_for_elts = sizeof(overaligned_elt) * cap;
            size_t bytes_for_skipfield = sizeof(skipfield_type) * (cap + 1);
            size_t n = (bytes_for_group + bytes_for_elts + bytes_for_skipfield + sizeof(type) - 1) / sizeof(type);
            std::allocator_traits<AllocOf<type>>::deallocate(ta, PtrOf<type>(g), n);
        }
    };

    void allocate_unused_group(size_type cap) {
        GroupPtr g = GroupAllocHelper::allocate_group(get_allocator(), cap);
        unused_groups_push_front(g);
        capacity_ += cap;
    }

    inline void deallocate_group(GroupPtr g) {
        GroupAllocHelper::deallocate_group(get_allocator(), g);
    }

    void unspecialcase_end_group(GroupPtr g) {
        assert(g == end_.group_);
        size_type trailing = g->end_of_elements() - g->last_endpoint;
        size_type sb = g->capacity - trailing;
        if (trailing != 0) {
            g->skipfield(sb) = trailing;
            g->skipfield(g->capacity - 1) = trailing;
            if (g->free_list_head == std::numeric_limits<skipfield_type>::max()) {
                g->next_erasure_ = std::exchange(groups_with_erasures_, g);
            }
            g->element(sb).s_.nextlink_ = std::exchange(g->free_list_head, sb);
            g->element(sb).s_.prevlink_ = std::numeric_limits<skipfield_type>::max();
            g->last_endpoint = g->end_of_elements();
        }
    }

    void destroy_all_data() {
        if (GroupPtr g = begin_.group_) {
            end_.group_->next_group = unused_groups_;

            if constexpr (!std::is_trivially_destructible<T>::value) {
                if (size_ != 0) {
                    while (true) {
                        // Erase elements without bothering to update skipfield - much faster:
                        skipfield_type end_pointer = g->index_of_last_endpoint();
                        do {
                            std::allocator_traits<allocator_type>::destroy(allocator_, begin_.operator->());
                            begin_.idx_ += 1 + begin_.group_->skipfield(begin_.idx_ + 1);
                        } while (begin_.idx_ != end_pointer); // ie. beyond end of available data

                        GroupPtr next = g->next_group;
                        deallocate_group(g);
                        g = next;

                        if (g == unused_groups_) {
                            break;
                        }
                        begin_ = iterator(g, g->skipfield(0));
                    }
                }
            }

            while (g != nullptr) {
                GroupPtr next = g->next_group;
                deallocate_group(g);
                g = next;
            }
        }
    }

public:
    template<class... Args>
    iterator emplace(Args&&... args) {
        allocator_type ea = get_allocator();
        if (trailing_capacity() != 0) {
            iterator result = end_;
            GroupPtr g = result.group_;
            std::allocator_traits<allocator_type>::construct(ea, result.operator->(), static_cast<Args&&>(args)...);
            assert(end_.group_->skipfield(end_.idx_) == 0);
            ++end_.idx_;
            g->last_endpoint += 1;
            g->size += 1;
            ++size_;
            assert_invariants();
            return result;
        } else if (groups_with_erasures_ != nullptr) {
            GroupPtr g = groups_with_erasures_;
            skipfield_type sb = g->free_list_head;
            assert(sb < g->capacity);
            auto result = iterator(g, sb);
            skipfield_type nextsb = g->element(sb).s_.nextlink_;
            assert(g->element(sb).s_.prevlink_ == std::numeric_limits<skipfield_type>::max());
            hive_try_rollback([&]() {
                std::allocator_traits<allocator_type>::construct(ea, result.operator->(), static_cast<Args&&>(args)...);
            }, [&]() {
                g->element(sb).s_.prevlink_ = std::numeric_limits<skipfield_type>::max();
                g->element(sb).s_.nextlink_ = nextsb;
            });
            g->size += 1;
            size_ += 1;
            if (g == begin_.group_ && sb == 0) {
                begin_ = result;
            }
            skipfield_type old_skipblock_length = std::exchange(g->skipfield(sb), 0);
            assert(1 <= old_skipblock_length && old_skipblock_length <= g->capacity);
            skipfield_type new_skipblock_length = (old_skipblock_length - 1);
            if (new_skipblock_length == 0) {
                g->free_list_head = nextsb;
                if (nextsb == std::numeric_limits<skipfield_type>::max()) {
                    groups_with_erasures_ = g->next_erasure_;
                } else {
                    g->element(nextsb).s_.prevlink_ = std::numeric_limits<skipfield_type>::max();
                }
            } else {
                g->skipfield(sb + 1) = new_skipblock_length;
                g->skipfield(sb + old_skipblock_length - 1) = new_skipblock_length;
                g->free_list_head = sb + 1;
                g->element(sb + 1).s_.prevlink_ = std::numeric_limits<skipfield_type>::max();
                g->element(sb + 1).s_.nextlink_ = nextsb;
                if (nextsb != std::numeric_limits<skipfield_type>::max()) {
                    g->element(nextsb).s_.prevlink_ = sb + 1;
                }
            }
            assert_invariants();
            return result;
        } else {
            if (unused_groups_ == nullptr) {
                allocate_unused_group(recommend_block_size());
            }
            GroupPtr g = unused_groups_;
            std::allocator_traits<allocator_type>::construct(ea, g->element(0).t(), static_cast<Args&&>(args)...);
            (void)unused_groups_pop_front();
            std::fill_n(g->addr_of_skipfield(0), g->capacity, skipfield_type());
            g->size = 1;
            g->last_endpoint = g->addr_of_element(1);
            g->free_list_head = std::numeric_limits<skipfield_type>::max();
            g->next_group = nullptr;
            g->prev_group = end_.group_;
            auto result = iterator(g, 0);
            if (end_.group_ != nullptr) {
                if (end_.group_->group_number() == std::numeric_limits<size_type>::max()) {
                    renumber_all_groups();
                }
                end_.group_->next_group = g;
                g->set_group_number(end_.group_->group_number() + 1);
            } else {
                begin_ = result;
                g->set_group_number(0);
            }
            end_ = iterator(g, 1);
            ++size_;
            return result;
        }
    }

    inline iterator insert(const T& value) { return emplace(value); }
    inline iterator insert(T&& value) { return emplace(std::move(value)); }

    void insert(size_type n, const T& value) {
        if (n == 0) {
            // do nothing
        } else if (n == 1) {
            insert(value);
        } else {
            callback_insert_impl(n, make_value_callback(n, value));
        }
    }

    template<class It, std::enable_if_t<!std::is_integral<It>::value>* = nullptr>
    inline void insert(It first, It last) {
        range_insert_impl(std::move(first), std::move(last));
    }

#if __cpp_lib_ranges >= 201911L
    template<std::ranges::input_range R>
        requires std::convertible_to<std::ranges::range_reference_t<R>, T>
    inline void insert_range(R&& rg) {
        if constexpr (std::ranges::sized_range<R&>) {
            reserve(size() + std::ranges::size(rg));
        }
        range_insert_impl(std::ranges::begin(rg), std::ranges::end(rg));
    }
#endif

    inline void insert(std::initializer_list<T> il) {
        range_insert_impl(il.begin(), il.end());
    }

    iterator erase(const_iterator it) {
        assert(size_ != 0);
        assert(it.group_ != nullptr);
        assert(it.idx_ != it.group_->index_of_last_endpoint());
        assert(it.group_->skipfield(it.idx_) == 0);

        if constexpr (!std::is_trivially_destructible<T>::value) {
            allocator_type ea = get_allocator();
            std::allocator_traits<allocator_type>::destroy(ea, it.operator->());
        }

        GroupPtr g = it.group_;

        --size_;
        --(g->size);

        if (g->size != 0) {
            const char prev_skipfield = (g->skipfield(it.idx_ - (it.idx_ != 0)) != 0);
            const char after_skipfield = (g->skipfield(it.idx_ + 1) != 0);
            skipfield_type update_value = 1;

            if (!(prev_skipfield | after_skipfield)) {
                g->skipfield(it.idx_) = 1;
                if (g->is_packed()) {
                    g->next_erasure_ = std::exchange(groups_with_erasures_, g);
                } else {
                    g->element(g->free_list_head).s_.prevlink_ = it.idx_;
                }
                g->element(it.idx_).s_.nextlink_ = g->free_list_head;
                g->element(it.idx_).s_.prevlink_ = std::numeric_limits<skipfield_type>::max();
                g->free_list_head = it.idx_;
            } else if (prev_skipfield & (!after_skipfield)) {
                // previous erased consecutive elements, none following
                skipfield_type new_skipblock_length = g->skipfield(it.idx_ - 1) + 1;
                g->skipfield(it.idx_) = new_skipblock_length;
                g->skipfield(it.idx_ - (new_skipblock_length - 1)) = new_skipblock_length;
            } else if ((!prev_skipfield) & after_skipfield) {
                // following erased consecutive elements, none preceding
                skipfield_type new_skipblock_length = g->skipfield(it.idx_ + 1) + 1;
                g->skipfield(it.idx_) = new_skipblock_length;
                g->skipfield(it.idx_ + (new_skipblock_length - 1)) = new_skipblock_length;

                const skipfield_type following_previous = g->element(it.idx_ + 1).s_.nextlink_;
                const skipfield_type following_next = g->element(it.idx_ + 1).s_.prevlink_;
                g->element(it.idx_).s_.nextlink_ = following_previous;
                g->element(it.idx_).s_.prevlink_ = following_next;

                const skipfield_type index = static_cast<skipfield_type>(it.index_in_group());

                if (following_previous != std::numeric_limits<skipfield_type>::max()) {
                    g->element(following_previous).s_.prevlink_ = index;
                }

                if (following_next != std::numeric_limits<skipfield_type>::max()) {
                    g->element(following_next).s_.nextlink_ = index;
                } else {
                    g->free_list_head = index;
                }
                update_value = new_skipblock_length;
            } else {
                // both preceding and following consecutive erased elements - erased element is between two skipblocks
                skipfield_type preceding_value = g->skipfield(it.idx_ - 1);
                skipfield_type following_value = g->skipfield(it.idx_ + 1);
                skipfield_type new_skipblock_length = preceding_value + following_value + 1;

                // Join the skipblocks
                g->skipfield(it.idx_ - preceding_value) = new_skipblock_length;
                g->skipfield(it.idx_ + following_value) = new_skipblock_length;

                // Remove the following skipblock's entry from the free list
                const skipfield_type following_previous = g->element(it.idx_ + 1).s_.nextlink_;
                const skipfield_type following_next = g->element(it.idx_ + 1).s_.prevlink_;

                if (following_previous != std::numeric_limits<skipfield_type>::max()) {
                    it.group_->element(following_previous).s_.prevlink_ = following_next;
                }

                if (following_next != std::numeric_limits<skipfield_type>::max()) {
                    it.group_->element(following_next).s_.nextlink_ = following_previous;
                } else {
                    it.group_->free_list_head = following_previous;
                }
                update_value = following_value + 1;
            }

            iterator result = it.unconst();
            result.idx_ += update_value;

            if (result.idx_ == g->index_of_last_endpoint() && g->next_group != nullptr) {
                result = iterator(g->next_group, g->next_group->skipfield(0));
            }

            if (it == begin_) {
                begin_ = result;
            }
            // assert_invariants();
            return result;
        } else {
            iterator result;
            if (g->next_group != nullptr && g->prev_group != nullptr) {
                g->next_group->prev_group = g->prev_group;
                g->prev_group->next_group = g->next_group;
                result = iterator(g->next_group, g->next_group->skipfield(0));
            } else if (g->next_group != nullptr) {
                g->next_group->prev_group = nullptr;
                begin_ = iterator(g->next_group, g->next_group->skipfield(0));
                result = begin_;
            } else if (g->prev_group != nullptr) {
                g->prev_group->next_group = nullptr;
                end_ = iterator(g->prev_group, g->prev_group->capacity);
                result = end_;
            } else {
                begin_ = iterator();
                end_ = iterator();
                result = end_;
            }
            if (!g->is_packed()) {
                remove_from_groups_with_erasures_list(g);
            }
            unused_groups_push_front(g);
            assert_invariants();
            return result;
        }
    }

    iterator erase(const_iterator first, const_iterator last) {
        allocator_type ea = get_allocator();
        const_iterator current = first;
        if (current.group_ != last.group_) {
            if (current.idx_ != current.group_->skipfield(0)) {
                size_type number_of_group_erasures = 0;
                skipfield_type end = first.group_->index_of_last_endpoint();
                if (std::is_trivially_destructible<T>::value && current.group_->is_packed()) {
                    number_of_group_erasures += static_cast<size_type>(current.group_->size - current.idx_);
                } else {
                    while (current.idx_ != end) {
                        if (current.group_->skipfield(current.idx_) == 0) {
                            if constexpr (!std::is_trivially_destructible<T>::value) {
                                std::allocator_traits<allocator_type>::destroy(ea, current.operator->());
                            }
                            ++number_of_group_erasures;
                            ++current.idx_;
                        } else {
                            const skipfield_type prev_free_list_index = current.group_->element(current.idx_).s_.nextlink_;
                            const skipfield_type next_free_list_index = current.group_->element(current.idx_).s_.prevlink_;
                            current.idx_ += current.group_->skipfield(current.idx_);
                            if (next_free_list_index == std::numeric_limits<skipfield_type>::max() && prev_free_list_index == std::numeric_limits<skipfield_type>::max()) {
                                remove_from_groups_with_erasures_list(first.group_);
                                first.group_->free_list_head = std::numeric_limits<skipfield_type>::max();
                                number_of_group_erasures += (end - current.idx_);
                                if constexpr (!std::is_trivially_destructible<T>::value) {
                                    while (current.idx_ != end) {
                                        std::allocator_traits<allocator_type>::destroy(ea, current.operator->());
                                        ++current.idx_;
                                    }
                                }
                                break; // end overall while loop
                            } else if (next_free_list_index == std::numeric_limits<skipfield_type>::max()) {
                                current.group_->free_list_head = prev_free_list_index; // make free list head equal to next free list node
                                current.group_->element(prev_free_list_index).s_.prevlink_ = std::numeric_limits<skipfield_type>::max();
                            } else {
                                current.group_->element(next_free_list_index).s_.nextlink_ = prev_free_list_index;
                                if (prev_free_list_index != std::numeric_limits<skipfield_type>::max()) {
                                    current.group_->element(prev_free_list_index).s_.prevlink_ = next_free_list_index;
                                }
                            }
                        }
                    }
                }

                const skipfield_type previous_node_value = first.group_->skipfield(first.idx_ - 1);
                const skipfield_type distance_to_end = end - first.idx_;

                if (previous_node_value == 0) {
                    first.group_->skipfield(first.idx_) = distance_to_end;
                    first.group_->skipfield(first.idx_ + (distance_to_end - 1)) = distance_to_end;
                    if (first.group_->is_packed()) {
                        first.group_->next_erasure_ = std::exchange(groups_with_erasures_, first.group_);
                    } else {
                        first.group_->element(first.group_->free_list_head).s_.prevlink_ = first.idx_;
                    }

                    first.group_->element(first.idx_).s_.nextlink_ = first.group_->free_list_head;
                    first.group_->element(first.idx_).s_.prevlink_ = std::numeric_limits<skipfield_type>::max();
                    first.group_->free_list_head = first.idx_;
                } else {
                    skipfield_type new_skipblock_length = previous_node_value + distance_to_end;
                    first.group_->skipfield(first.idx_ - previous_node_value) = new_skipblock_length;
                    first.group_->skipfield(first.idx_ + (distance_to_end - 1)) = new_skipblock_length;
                }
                first.group_->size -= number_of_group_erasures;
                size_ -= number_of_group_erasures;
                current.group_ = current.group_->next_group;
            }

            // Intermediate groups:
            const GroupPtr prev = current.group_->prev_group;
            while (current.group_ != last.group_) {
                if constexpr (!std::is_trivially_destructible<T>::value) {
                    current.idx_ = current.group_->skipfield(0);
                    skipfield_type end = current.group_->index_of_last_endpoint();
                    do {
                        std::allocator_traits<allocator_type>::destroy(ea, current.operator->());
                        current.idx_ += 1 + current.group_->skipfield(current.idx_ + 1);
                    } while (current.idx_ != end);
                }
                if (!current.group_->is_packed()) {
                    remove_from_groups_with_erasures_list(current.group_);
                }
                size_ -= current.group_->size;
                GroupPtr current_group = std::exchange(current.group_, current.group_->next_group);
                unused_groups_push_front(current_group);
            }

            current.idx_ = current.group_->skipfield(0);
            current.group_->prev_group = prev;

            if (prev != nullptr) {
                prev->next_group = current.group_;
            } else {
                begin_ = last.unconst();
            }
        }

        assert(current.group_ == last.group_);
        if (current == last) {
            assert_invariants();
            return last.unconst();
        }

        if (last != end_ || current.idx_ != current.group_->skipfield(0)) {
            size_type number_of_group_erasures = 0;
            const_iterator current_saved = current;

            if (std::is_trivially_destructible<T>::value && current.group_->is_packed()) {
                number_of_group_erasures += (last.idx_ - current.idx_);
            } else {
                while (current.idx_ != last.idx_) {
                    if (current.group_->skipfield(current.idx_) == 0) {
                        if constexpr (!std::is_trivially_destructible<T>::value) {
                            std::allocator_traits<allocator_type>::destroy(ea, current.operator->());
                        }
                        ++number_of_group_erasures;
                        ++current.idx_;
                    } else {
                        const skipfield_type prev_free_list_index = current.group_->element(current.idx_).s_.nextlink_;
                        const skipfield_type next_free_list_index = current.group_->element(current.idx_).s_.prevlink_;

                        current.idx_ += current.group_->skipfield(current.idx_);

                        if (next_free_list_index == std::numeric_limits<skipfield_type>::max() && prev_free_list_index == std::numeric_limits<skipfield_type>::max()) {
                            remove_from_groups_with_erasures_list(last.group_);
                            last.group_->free_list_head = std::numeric_limits<skipfield_type>::max();
                            number_of_group_erasures += (last.idx_ - current.idx_);
                            if constexpr (!std::is_trivially_destructible<T>::value) {
                                while (current.idx_ != last.idx_) {
                                    std::allocator_traits<allocator_type>::destroy(ea, current.operator->());
                                    ++current.idx_;
                                }
                            }
                            break; // end overall while loop
                        } else if (next_free_list_index == std::numeric_limits<skipfield_type>::max()) {
                            current.group_->free_list_head = prev_free_list_index;
                            current.group_->element(prev_free_list_index).s_.prevlink_ = std::numeric_limits<skipfield_type>::max();
                        } else {
                            current.group_->element(next_free_list_index).s_.nextlink_ = prev_free_list_index;
                            if (prev_free_list_index != std::numeric_limits<skipfield_type>::max()) {
                                current.group_->element(prev_free_list_index).s_.prevlink_ = next_free_list_index;
                            }
                        }
                    }
                }
            }

            const skipfield_type distance_to_last = (last.idx_ - current_saved.idx_);
            const skipfield_type index = current_saved.idx_;

            if (current_saved.idx_ == 0 || current_saved.group_->skipfield(current_saved.idx_ - 1) == 0) {
                current_saved.group_->skipfield(current_saved.idx_) = distance_to_last;
                last.group_->skipfield(last.idx_ - 1) = distance_to_last;

                if (last.group_->is_packed()) {
                    last.group_->next_erasure_ = std::exchange(groups_with_erasures_, last.group_);
                } else {
                    last.group_->element(last.group_->free_list_head).s_.prevlink_ = index;
                }

                current_saved.group_->element(current_saved.idx_).s_.nextlink_ = last.group_->free_list_head;
                current_saved.group_->element(current_saved.idx_).s_.prevlink_ = std::numeric_limits<skipfield_type>::max();
                last.group_->free_list_head = index;
            } else {
                skipfield_type prev_node_value = current_saved.group_->skipfield(current_saved.idx_ - 1);
                skipfield_type new_skipblock_length = prev_node_value + distance_to_last;
                current_saved.group_->skipfield(current_saved.idx_ - prev_node_value) = new_skipblock_length;
                last.group_->skipfield(last.idx_ - 1) = new_skipblock_length;
            }

            if (first == begin_) {
                begin_ = last.unconst();
            }
            last.group_->size -= number_of_group_erasures;
            size_ -= number_of_group_erasures;
        } else {
            if constexpr (!std::is_trivially_destructible<T>::value) {
                while (current.idx_ != last.idx_) {
                    std::allocator_traits<allocator_type>::destroy(ea, current.operator->());
                    current.idx_ += 1 + current.group_->skipfield(current.idx_ + 1);
                }
            }

            size_ -= current.group_->size;
            if (size_ != 0) {
                if (!current.group_->is_packed()) {
                    remove_from_groups_with_erasures_list(current.group_);
                }

                current.group_->prev_group->next_group = current.group_->next_group;

                if (current.group_ == end_.group_) {
                    GroupPtr prev = current.group_->prev_group;
                    end_ = iterator(prev, prev->index_of_last_endpoint());
                    unused_groups_push_front(current.group_);
                    return end_;
                } else if (current.group_ == begin_.group_) {
                    GroupPtr next = current.group_->next_group;
                    begin_ = iterator(next, next->skipfield(0));
                }

                if (current.group_->next_group != end_.group_) {
                    capacity_ -= current.group_->capacity;
                } else {
                    unused_groups_push_front(current.group_);
                    return last.unconst();
                }
            } else {
                reset_only_group_left(current.group_);
                return end_;
            }

            deallocate_group(current.group_);  // TODO FIXME BUG HACK: don't do this
        }

        return last.unconst();
    }

    void swap(hive &source)
        noexcept(std::allocator_traits<allocator_type>::propagate_on_container_swap::value || std::allocator_traits<allocator_type>::is_always_equal::value)
    {
        using std::swap;
        swap(end_, source.end_);
        swap(begin_, source.begin_);
        swap(groups_with_erasures_, source.groups_with_erasures_);
        swap(unused_groups_, source.unused_groups_);
        swap(unused_groups_tail_, source.unused_groups_tail_);
        swap(size_, source.size_);
        swap(capacity_, source.capacity_);
        if constexpr (std::allocator_traits<allocator_type>::propagate_on_container_swap::value && !std::allocator_traits<allocator_type>::is_always_equal::value) {
            swap(allocator_, source.allocator_);
        }
    }

    friend void swap(hive& a, hive& b) noexcept(noexcept(a.swap(b))) { a.swap(b); }

    void clear() noexcept {
        if (size_ != 0) {
            if constexpr (!std::is_trivially_destructible<T>::value) {
                allocator_type ea = get_allocator();
                for (iterator it = begin_; it != end_; ++it) {
                    std::allocator_traits<allocator_type>::destroy(ea, it.operator->());
                }
            }
            if (begin_.group_ != end_.group_) {
                // Move all other groups onto the unused_groups list
                end_.group_->next_group = unused_groups_;
                if (unused_groups_ == nullptr) {
                    unused_groups_tail_ = end_.group_;
                }
                unused_groups_ = begin_.group_->next_group;
            }
            reset_only_group_left(begin_.group_);
            groups_with_erasures_ = nullptr;
            size_ = 0;
        }
    }

    void splice(hive& source) {
        assert_invariants();
        source.assert_invariants();
        assert(&source != this);

        if (capacity_ + source.capacity_ > max_size()) {
            SG14_HIVE_THROW(std::length_error("Result of splice would exceed max_size()"));
        }

        size_type trailing = trailing_capacity();
        if (trailing > source.trailing_capacity()) {
            source.splice(*this);
            swap(source);
        } else {
            if (source.groups_with_erasures_ != nullptr) {
                if (groups_with_erasures_ == nullptr) {
                    groups_with_erasures_ = source.groups_with_erasures_;
                } else {
                    GroupPtr tail = groups_with_erasures_;
                    while (tail->next_erasure_ != nullptr) {
                        tail = tail->next_erasure_;
                    }
                    tail->next_erasure_ = source.groups_with_erasures_;
                }
            }
            if (source.unused_groups_ != nullptr) {
                if (unused_groups_ == nullptr) {
                    unused_groups_ = std::exchange(source.unused_groups_, nullptr);
                } else {
                    unused_groups_tail_->next_group = std::exchange(source.unused_groups_, nullptr);
                }
                unused_groups_tail_ = std::exchange(source.unused_groups_tail_, nullptr);
            }
            if (trailing != 0 && source.begin_.group_ != nullptr) {
                unspecialcase_end_group(end_.group_);
            }
            if (source.begin_.group_ != nullptr) {
                source.begin_.group_->prev_group = end_.group_;
                if (end_.group_ != nullptr) {
                    end_.group_->next_group = source.begin_.group_;
#if SG14_HIVE_RELATIONAL_OPERATORS
                    if (source.begin_.group_->group_number() <= end_.group_->group_number()) {
                        renumber_all_groups();
                    }
#endif
                } else {
                    assert(begin_.group_ == nullptr);
                    begin_ = std::move(source.begin_);
                }
                end_ = std::move(source.end_);
            }
            size_ += std::exchange(source.size_, 0);
            capacity_ += std::exchange(source.capacity_, 0);

            source.begin_ = iterator();
            source.end_ = iterator();
            source.groups_with_erasures_ = nullptr;
        }

        assert_invariants();
        source.assert_invariants();
    }

    inline void splice(hive&& source) { this->splice(source); }

private:
    template<bool MightFillIt, class CB>
    void callback_fill_skipblock(skipfield_type n, CB cb, GroupPtr g) {
        assert(g == groups_with_erasures_);
        allocator_type ea = get_allocator();
        skipfield_type sb = g->free_list_head;
        AlignedEltPtr d_first = g->addr_of_element(sb);
        AlignedEltPtr d_last = d_first + n;
        skipfield_type nextsb = d_first[0].s_.nextlink_;
        skipfield_type old_skipblock_length = g->skipfield(sb);
        assert(1 <= n && n <= old_skipblock_length);
        assert(g->skipfield(sb + old_skipblock_length - 1) == old_skipblock_length);
        AlignedEltPtr p = d_first;
        hive_try_finally([&]() {
            while (p != d_last) {
                cb.construct_and_increment(ea, p);
            }
        }, [&]() {
            skipfield_type nadded = p - d_first;
            g->size += nadded;
            size_ += nadded;
            if (nadded != 0 && d_first == begin_.group_->addr_of_element(0)) {
                begin_ = iterator(g, 0);
            }
            std::fill_n(g->addr_of_skipfield(sb), nadded, skipfield_type());
            skipfield_type new_skipblock_length = (old_skipblock_length - nadded);
            if (MightFillIt && new_skipblock_length == 0) {
                g->free_list_head = nextsb;
                if (nextsb == std::numeric_limits<skipfield_type>::max()) {
                    groups_with_erasures_ = g->next_erasure_;
                }
            } else {
                g->skipfield(sb + nadded) = new_skipblock_length;
                g->skipfield(sb + old_skipblock_length - 1) = new_skipblock_length;
                g->free_list_head = sb + nadded;
                g->element(sb + nadded).s_.prevlink_ = std::numeric_limits<skipfield_type>::max();
                g->element(sb + nadded).s_.nextlink_ = nextsb;
                if (nextsb != std::numeric_limits<skipfield_type>::max()) {
                    g->element(nextsb).s_.prevlink_ = sb + nadded;
                }
            }
        });
    }

    template<class CB>
    void callback_fill_trailing_capacity(skipfield_type n, CB cb, GroupPtr g) {
        allocator_type ea = get_allocator();
        assert(g == end_.group_);
        assert(g->is_packed());
        assert(1 <= n && n <= g->capacity - g->size);
        assert(g->next_group == nullptr);
        AlignedEltPtr d_first = g->last_endpoint;
        AlignedEltPtr d_last = g->last_endpoint + n;
        AlignedEltPtr p = d_first;
        hive_try_finally([&]() {
            while (p != d_last) {
                cb.construct_and_increment(ea, p);
            }
        }, [&]() {
            skipfield_type nadded = p - d_first;
            g->last_endpoint = p;
            g->size += nadded;
            size_ += nadded;
            end_ = iterator(g, p - g->addr_of_element(0));
        });
    }

    template<class CB>
    void callback_fill_unused_group(skipfield_type n, CB cb, GroupPtr g) {
        if (end_.group_ != nullptr && end_.group_->group_number() == std::numeric_limits<size_type>::max()) {
            renumber_all_groups();
        }
        allocator_type ea = get_allocator();
        assert(g == unused_groups_);
        assert(1 <= n && n <= g->capacity);
        AlignedEltPtr d_first = g->addr_of_element(0);
        AlignedEltPtr d_last = g->addr_of_element(n);
        AlignedEltPtr p = d_first;
        hive_try_finally([&]() {
            while (p != d_last) {
                cb.construct_and_increment(ea, p);
            }
        }, [&]() {
            skipfield_type nadded = p - d_first;
            if (nadded != 0) {
                (void)unused_groups_pop_front();
                std::fill_n(g->addr_of_skipfield(0), g->capacity, skipfield_type());
                g->free_list_head = std::numeric_limits<skipfield_type>::max();
                g->last_endpoint = p;
                g->size = nadded;
                size_ += nadded;
                g->next_group = nullptr;
                if (end_.group_ != nullptr) {
                    if (end_.group_->group_number() == std::numeric_limits<size_type>::max()) {
                        renumber_all_groups();
                    }
                    end_.group_->next_group = g;
                    g->prev_group = end_.group_;
                    g->set_group_number(end_.group_->group_number() + 1);
                } else {
                    g->prev_group = nullptr;
                    g->set_group_number(0);
                }
                end_ = iterator(g, nadded);
                if (begin_.group_ == nullptr) {
                    begin_ = iterator(g, 0);
                }
            } else {
                assert(g == unused_groups_);
                assert(g != end_.group_);
            }
        });
    }

    template<class CB>
    void callback_insert_impl(size_type n, CB cb) {
        reserve(size_ + n);
        assert_invariants();
        while (groups_with_erasures_ != nullptr) {
            GroupPtr g = groups_with_erasures_;
            assert(g->free_list_head != std::numeric_limits<skipfield_type>::max());
            skipfield_type skipblock_length = g->skipfield(g->free_list_head);
            if (skipblock_length > n) {
                callback_fill_skipblock<false>(n, cb, g);
                assert_invariants();
                return;
            } else {
                callback_fill_skipblock<true>(skipblock_length, cb, g);
                n -= skipblock_length;
                if (n == 0) {
                    return;
                }
            }
        }
        assert_invariants();
        if (n != 0 && end_.group_ != nullptr) {
            GroupPtr g = end_.group_;
            assert(g->is_packed());
            size_type space = g->capacity - g->size;
            if (space >= n) {
                callback_fill_trailing_capacity(n, cb, g);
                assert_invariants();
                return;
            } else if (space != 0) {
                callback_fill_trailing_capacity(space, cb, g);
                n -= space;
            }
        }
        assert_invariants();
        while (n != 0) {
            GroupPtr g = unused_groups_;
            if (g->capacity >= n) {
                callback_fill_unused_group(n, cb, g);
                assert_invariants();
                return;
            } else {
                callback_fill_unused_group(g->capacity, cb, g);
                n -= g->capacity;
            }
        }
        assert_invariants();
    }

    template<class It, class Sent>
    inline void range_assign_impl(It first, Sent last) {
        if constexpr (!hive_fwd_iterator<It>::value) {
            clear();
            for ( ; first != last; ++first) {
                emplace(*first);
            }
        } else if (first == last) {
            clear();
        } else {
#if __cpp_lib_ranges >= 201911L
            size_type n = std::ranges::distance(first, last);
#else
            size_type n = std::distance(first, last);
#endif
            clear();
            callback_insert_impl(n, make_itpair_callback(first, last));
            assert_invariants();
        }
    }

    template<class It, class Sent>
    void range_insert_impl(It first, Sent last) {
        if constexpr (!hive_fwd_iterator<It>::value) {
            for ( ; first != last; ++first) {
                emplace(*first);
            }
        } else if (first == last) {
            return;
        } else {
#if __cpp_lib_ranges >= 201911L
            size_type n = std::ranges::distance(first, last);
#else
            size_type n = std::distance(first, last);
#endif
            callback_insert_impl(n, make_itpair_callback(first, last));
        }
    }

    inline void update_subsequent_group_numbers(GroupPtr g) {
#if SG14_HIVE_RELATIONAL_OPERATORS
        do {
            g->groupno_ -= 1;
            g = g->next_group;
        } while (g != nullptr);
#endif
        (void)g;
    }

    void remove_from_groups_with_erasures_list(GroupPtr g) {
        assert(groups_with_erasures_ != nullptr);
        if (g == groups_with_erasures_) {
            groups_with_erasures_ = groups_with_erasures_->next_erasure_;
        } else {
            GroupPtr prev = groups_with_erasures_;
            GroupPtr curr = groups_with_erasures_->next_erasure_;
            while (g != curr) {
                prev = curr;
                curr = curr->next_erasure_;
            }
            prev->next_erasure_ = curr->next_erasure_;
        }
    }

    inline void reset_only_group_left(GroupPtr g) {
        groups_with_erasures_ = nullptr;
        g->reset(0, nullptr, nullptr, 0);
        begin_ = iterator(g, 0);
        end_ = begin_;
    }

    inline void unused_groups_push_front(GroupPtr g) {
        g->next_group = std::exchange(unused_groups_, g);
        if (unused_groups_tail_ == nullptr) {
            unused_groups_tail_ = g;
        }
    }

    inline GroupPtr unused_groups_pop_front() {
        GroupPtr g = std::exchange(unused_groups_, unused_groups_->next_group);
        assert(g != nullptr);
        if (unused_groups_tail_ == g) {
            unused_groups_tail_ = nullptr;
        }
        return g;
    }

public:
    template <class It, class = std::enable_if_t<!std::is_integral<It>::value>>
    inline void assign(It first, It last) {
        range_assign_impl(std::move(first), std::move(last));
    }

#if __cpp_lib_ranges >= 201911L
    template<std::ranges::input_range R>
        requires std::convertible_to<std::ranges::range_reference_t<R>, T> &&
                 std::assignable_from<T&, std::ranges::range_reference_t<R>>
    inline void assign_range(R&& rg) {
        range_assign_impl(std::ranges::begin(rg), std::ranges::end(rg));
    }
#endif

    inline void assign(size_type n, const T& value) {
        clear();
        insert(n, value);
    }

    inline void assign(std::initializer_list<T> il) {
        range_assign_impl(il.begin(), il.end());
    }

    inline allocator_type get_allocator() const noexcept { return allocator_; }

    inline iterator begin() noexcept { return begin_; }
    inline const_iterator begin() const noexcept { return begin_; }
    inline iterator end() noexcept { return end_; }
    inline const_iterator end() const noexcept { return end_; }
    inline reverse_iterator rbegin() noexcept { return reverse_iterator(end_); }
    inline const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end_); }
    inline reverse_iterator rend() noexcept { return reverse_iterator(begin_); }
    inline const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin_); }

    inline const_iterator cbegin() const noexcept { return begin_; }
    inline const_iterator cend() const noexcept { return end_; }
    inline const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(end_); }
    inline const_reverse_iterator crend() const noexcept { return const_reverse_iterator(begin_); }

    [[nodiscard]] inline bool empty() const noexcept { return size_ == 0; }
    inline size_type size() const noexcept { return size_; }
    inline size_type max_size() const noexcept { return std::allocator_traits<allocator_type>::max_size(get_allocator()); }
    inline size_type capacity() const noexcept { return capacity_; }

    inline size_type max_block_size() const noexcept {
        size_type a = impl_max_block_size();
        size_type b = max_size();
        return a < b ? a : b;
    }

private:
    static inline size_type impl_min_block_size() { return 3; }
    static inline size_type impl_max_block_size() { return std::numeric_limits<skipfield_type>::max(); }

    inline size_type recommend_block_size() const {
        size_type r = size_;
        if (r < 8) r = 8;
        if (r > impl_max_block_size()) r = impl_max_block_size();
        return r;
    }

    void reserve_impl(size_type n, size_type min, size_type max) {
        if (n <= capacity_) {
            return;
        } else if (n > max_size()) {
            SG14_HIVE_THROW(std::length_error("n must be at most max_size()"));
        }
        size_type needed = n - capacity_;
        while (needed >= max) {
            allocate_unused_group(max);
            needed -= max;
        }
        if (needed != 0) {
            if (needed < min) {
                needed = min;
            }
            bool should_move_to_back_of_list = (unused_groups_ != nullptr && unused_groups_->capacity > needed);
            allocate_unused_group(needed);
            if (should_move_to_back_of_list) {
                GroupPtr g = unused_groups_pop_front();
                std::exchange(unused_groups_tail_, g)->next_group = g;
                g->next_group = nullptr;
            }
        }
        assert(capacity_ >= n);
        assert_invariants();
    }

    void reshape_impl_deallocate_unused_groups(size_t min, size_t max) {
        GroupPtr g = unused_groups_;
        GroupPtr prev = nullptr;
        while (g != nullptr) {
            if (g->capacity < min || g->capacity > max) {
                if (prev != nullptr) {
                    prev->next_group = g->next_group;
                } else {
                    unused_groups_ = g->next_group;
                }
                if (g->next_group == nullptr) {
                    unused_groups_tail_ = prev;
                }
                capacity_ -= g->capacity;
                deallocate_group(std::exchange(g, g->next_group));
            } else {
                prev = std::exchange(g, g->next_group);
            }
        }
    }

    void transfer_group_impl(hive& h, GroupPtr g) {
        if (g->prev_group != nullptr && g->next_group != nullptr) {
            g->prev_group->next_group = g->next_group;
            g->next_group->prev_group = g->prev_group;
        } else if (g->prev_group != nullptr) {
            g->prev_group->next_group = g->next_group;
            h.end_ = iterator(g->prev_group, g->prev_group->capacity);
        } else if (g->next_group != nullptr) {
            g->next_group->prev_group = g->prev_group;
            h.begin_ = iterator(g->next_group, g->next_group->skipfield(0));
        } else {
            h.begin_ = iterator();
            h.end_ = iterator();
        }
        if (!g->is_packed()) {
            h.remove_from_groups_with_erasures_list(g);
            g->next_erasure_ = std::exchange(groups_with_erasures_, g);
        }
        h.capacity_ -= g->capacity;
        h.size_ -= g->size;
        g->next_group = nullptr;
        g->prev_group = end_.group_;
        if (end_.group_ != nullptr) {
            end_.group_->next_group = g;
        } else {
            begin_ = iterator(g, g->skipfield(0));
        }
        end_ = iterator(g, g->index_of_last_endpoint());
        capacity_ += g->capacity;
        size_ += g->size;
    }

public:
    bool reshape(size_type min, size_type n = 0) {
        if (n > max_size()) {
            SG14_HIVE_THROW(std::length_error("n must be at most max_size()"));
        }
        if (min > max_block_size()) {
            SG14_HIVE_THROW(std::length_error("min must be at most max_block_size()"));
        }
        reshape_impl_deallocate_unused_groups(min, std::numeric_limits<size_type>::max());
        size_type oldsize = size();
        hive other(get_allocator());
        for (GroupPtr g = begin_.group_; g != nullptr; ) {
            GroupPtr next = g->next_group;
            assert(g->size >= 1);
            if (g->capacity < min) {
                other.transfer_group_impl(*this, g);
            }
            g = next;
        }
        assert_invariants();
        other.assert_invariants();
        if (other.empty()) {
            return false;
        } else {
            hive_try_rollback([&]() {
                reserve_impl(oldsize, min, max_block_size());
                while (!other.empty()) {
                    emplace(std::move(*other.begin()));
                    other.erase(other.begin());
                }
            }, [&]() {
                this->splice(other);
            });
            return true;
        }
    }

    hive& operator=(const hive& source) {
        if constexpr (std::allocator_traits<allocator_type>::propagate_on_container_copy_assignment::value) {
            allocator_type source_allocator(source);
            if (!std::allocator_traits<allocator_type>::is_always_equal::value && get_allocator() != source.get_allocator()) {
                // Deallocate existing blocks as source allocator is not necessarily able to do so
                destroy_all_data();
                blank();
            }
            allocator_ = source.get_allocator();
        }
        range_assign_impl(source.begin(), source.end());
        return *this;
    }

    hive& operator=(hive&& source)
        noexcept(std::allocator_traits<allocator_type>::propagate_on_container_move_assignment::value ||
                 std::allocator_traits<allocator_type>::is_always_equal::value)
    {
        assert(&source != this);
        destroy_all_data();

        bool should_use_source_allocator = (
            std::allocator_traits<allocator_type>::propagate_on_container_move_assignment::value ||
            std::allocator_traits<allocator_type>::is_always_equal::value ||
            this->get_allocator() == source.get_allocator()
        );
        if (should_use_source_allocator) {
            constexpr bool can_just_memcpy = (
                std::is_trivially_copyable<allocator_type>::value &&
                std::is_trivial<GroupPtr>::value &&
                std::is_trivial<AlignedEltPtr>::value &&
                std::is_trivial<SkipfieldPtr>::value
            );
            if constexpr (can_just_memcpy) {
                std::memcpy(static_cast<void *>(this), &source, sizeof(hive));
            } else {
                end_ = std::move(source.end_);
                begin_ = std::move(source.begin_);
                groups_with_erasures_ = std::move(source.groups_with_erasures_);
                unused_groups_ = std::move(source.unused_groups_);
                unused_groups_tail_ = std::move(source.unused_groups_tail_);
                size_ = source.size_;
                capacity_ = source.capacity_;
                if constexpr (std::allocator_traits<allocator_type>::propagate_on_container_move_assignment::value) {
                    allocator_ = std::move(source.allocator_);
                }
            }
        } else {
            reserve(source.size());
            range_assign_impl(std::make_move_iterator(source.begin()), std::make_move_iterator(source.end()));
            source.destroy_all_data();
        }

        source.blank();
        assert_invariants();
        return *this;
    }

    inline hive& operator=(std::initializer_list<T> il) {
        range_assign_impl(il.begin(), il.end());
        assert_invariants();
        return *this;
    }

    void shrink_to_fit() {
        const size_type min = impl_min_block_size();
        const size_type max = impl_max_block_size();
        trim_capacity();
        size_type oldsize = size();
        hive other(get_allocator());
        for (GroupPtr g = begin_.group_; g != nullptr; ) {
            GroupPtr next = g->next_group;
            if (next != nullptr) {
                if (g->size != max) {
                    other.transfer_group_impl(*this, g);
                }
            } else {
                if (g->capacity > min && g->capacity > g->size) {
                    other.transfer_group_impl(*this, g);
                }
            }
            g = next;
        }
        assert_invariants();
        other.assert_invariants();
        if (!other.empty()) {
            hive_try_rollback([&]() {
                reserve(oldsize);
                while (!other.empty()) {
                    emplace(std::move(*other.begin()));
                    other.erase(other.begin());
                }
            }, [&]() {
                this->splice(other);
            });
        }
    }

    void trim_capacity() noexcept {
        for (GroupPtr g = unused_groups_; g != nullptr; ) {
            GroupPtr next = g->next_group;
            capacity_ -= g->capacity;
            deallocate_group(g);
            g = next;
        }
        unused_groups_ = nullptr;
        unused_groups_tail_ = nullptr;
        assert_invariants();
    }

    void reserve(size_type n) {
        reserve_impl(n, impl_min_block_size(), impl_max_block_size());
    }

private:
    struct item_index_tuple {
        pointer original_location;
        size_type original_index;

        explicit item_index_tuple(pointer _item, size_type _index) :
            original_location(_item),
            original_index(_index)
        {}
    };

public:
    template <class Comp>
    void sort(Comp less) {
        if (size_ <= 1) {
            return;
        }

        struct ItemT {
            T *ptr_;
            size_type idx_;
        };

        std::unique_ptr<ItemT[]> a = std::make_unique<ItemT[]>(size_);
        auto it = begin_;
        for (size_type i = 0; i < size_; ++i) {
            a[i] = ItemT{std::addressof(*it), i};
            ++it;
        }
        assert(it == end_);
        std::sort(a.get(), a.get() + size_, [&](const ItemT& a, const ItemT& b) { return less(*a.ptr_, *b.ptr_); });

        for (size_type i = 0; i < size_; ++i) {
            size_type src = a[i].idx_;
            size_type dest = i;
            if (src != dest) {
                T temp = std::move(*a[i].ptr_);
                do {
                    *a[dest].ptr_ = std::move(*a[src].ptr_);
                    dest = src;
                    src = a[dest].idx_;
                    a[dest].idx_ = dest;
                } while (src != i);
                *a[dest].ptr_ = std::move(temp);
            }
        }
        assert_invariants();
    }

    inline void sort() { sort(std::less<T>()); }

    template<class Comp>
    size_type unique(Comp eq) {
        size_type count = 0;
        auto end = cend();
        for (auto it = cbegin(); it != end; ) {
            auto previous = it++;
            if (it == end) {
                break;
            }
            if (eq(*it, *previous)) {
                auto orig = ++count;
                auto last = it;
                while (++last != end && eq(*last, *previous)) {
                    ++count;
                }
                if (count != orig) {
                    it = erase(it, last);
                } else {
                    it = erase(it);
                }
                end = cend();
            }
        }
        assert_invariants();
        return count;
    }

    inline size_type unique() { return unique(std::equal_to<T>()); }
};

} // namespace sg14

namespace std {

    template<class T, class A, class P, class Pred>
    typename sg14::hive<T, A, P>::size_type erase_if(sg14::hive<T, A, P>& h, Pred pred) {
        typename sg14::hive<T, A, P>::size_type count = 0;
        auto end = h.end();
        for (auto it = h.begin(); it != end; ++it) {
            if (pred(*it)) {
                auto orig = ++count;
                auto last = it;
                while (++last != end && pred(*last)) {
                    ++count;
                }
                if (count != orig) {
                    it = h.erase(it, last);
                } else {
                    it = h.erase(it);
                }
                end = h.end();
                if (it == end) {
                    break;
                }
            }
        }
        return count;
    }

    template<class T, class A, class P>
    inline typename sg14::hive<T, A, P>::size_type erase(sg14::hive<T, A, P>& h, const T& value) {
        return std::erase_if(h, [&](const T &x) { return x == value; });
    }
} // namespace std
