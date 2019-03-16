constexpr void constexpr_assert(bool b)
{
    (void)b;
//    if (!b) __builtin_unreachable();
}

constexpr int depth_of_complete_tree(int n)
{
#if 0
    int depth = 0;
    while ((1 << depth) <= n) {
        ++depth;
    }
    return depth;
#else
    return (n == 0) ? 0 : (32 - __builtin_clz(unsigned(n)));
#endif
}
static_assert(depth_of_complete_tree(0) == 0, "");
static_assert(depth_of_complete_tree(1) == 1, "");
static_assert(depth_of_complete_tree(2) == 2, "");
static_assert(depth_of_complete_tree(3) == 2, "");
static_assert(depth_of_complete_tree(4) == 3, "");
static_assert(depth_of_complete_tree(5) == 3, "");
static_assert(depth_of_complete_tree(6) == 3, "");
static_assert(depth_of_complete_tree(7) == 3, "");
static_assert(depth_of_complete_tree(8) == 4, "");

constexpr int nodes_in_complete_tree(int depth)
{
    return (1 << depth) - 1;
}
static_assert(nodes_in_complete_tree(0) == 0, "");
static_assert(nodes_in_complete_tree(1) == 1, "");
static_assert(nodes_in_complete_tree(2) == 3, "");
static_assert(nodes_in_complete_tree(3) == 7, "");
static_assert(nodes_in_complete_tree(4) == 15, "");

constexpr int eytzinger_from_rank_in_complete_tree(int i, int depth)
{
    constexpr_assert(0 <= i && i < nodes_in_complete_tree(depth));
#if 0
    int D = depth - 1;
    while (i & 1) {
        --D;
        i >>= 1;
    }
    return (1 << D) - 1 + i/2;
#else
    int D = 1 + __builtin_ctz(~i);
    return (1 << (depth - D)) - 1 + (i >> D);
#endif
}
static_assert(eytzinger_from_rank_in_complete_tree(0, 1) == 0, "");
static_assert(eytzinger_from_rank_in_complete_tree(0, 2) == 1, "");
static_assert(eytzinger_from_rank_in_complete_tree(1, 2) == 0, "");
static_assert(eytzinger_from_rank_in_complete_tree(2, 2) == 2, "");
static_assert(eytzinger_from_rank_in_complete_tree(0, 3) == 3, "");
static_assert(eytzinger_from_rank_in_complete_tree(1, 3) == 1, "");
static_assert(eytzinger_from_rank_in_complete_tree(2, 3) == 4, "");
static_assert(eytzinger_from_rank_in_complete_tree(3, 3) == 0, "");
static_assert(eytzinger_from_rank_in_complete_tree(4, 3) == 5, "");
static_assert(eytzinger_from_rank_in_complete_tree(5, 3) == 2, "");
static_assert(eytzinger_from_rank_in_complete_tree(6, 3) == 6, "");

constexpr int rank_from_eytzinger_in_complete_tree(int i, int depth)
{
    constexpr_assert(0 <= i && i < nodes_in_complete_tree(depth));
    int D = depth_of_complete_tree(i + 1);
    int nodes_above = nodes_in_complete_tree(D - 1);
    int T = i - nodes_above;  // number of subtrees to our left
    int SD = (depth - D) + 1;  // depth of subtrees at our node's level
    return (T << SD) + nodes_in_complete_tree(SD - 1);
}
static_assert(rank_from_eytzinger_in_complete_tree(0, 1) == 0, "");
static_assert(rank_from_eytzinger_in_complete_tree(0, 2) == 1, "");
static_assert(rank_from_eytzinger_in_complete_tree(1, 2) == 0, "");
static_assert(rank_from_eytzinger_in_complete_tree(2, 2) == 2, "");
static_assert(rank_from_eytzinger_in_complete_tree(0, 3) == 3, "");
static_assert(rank_from_eytzinger_in_complete_tree(1, 3) == 1, "");
static_assert(rank_from_eytzinger_in_complete_tree(2, 3) == 5, "");
static_assert(rank_from_eytzinger_in_complete_tree(3, 3) == 0, "");
static_assert(rank_from_eytzinger_in_complete_tree(4, 3) == 2, "");
static_assert(rank_from_eytzinger_in_complete_tree(5, 3) == 4, "");
static_assert(rank_from_eytzinger_in_complete_tree(6, 3) == 6, "");
static_assert(rank_from_eytzinger_in_complete_tree(0, 4) == 7, "");
static_assert(rank_from_eytzinger_in_complete_tree(1, 4) == 3, "");
static_assert(rank_from_eytzinger_in_complete_tree(2, 4) == 11, "");
static_assert(rank_from_eytzinger_in_complete_tree(3, 4) == 1, "");
static_assert(rank_from_eytzinger_in_complete_tree(4, 4) == 5, "");
static_assert(rank_from_eytzinger_in_complete_tree(5, 4) == 9, "");
static_assert(rank_from_eytzinger_in_complete_tree(6, 4) == 13, "");
static_assert(rank_from_eytzinger_in_complete_tree(7, 4) == 0, "");
static_assert(rank_from_eytzinger_in_complete_tree(8, 4) == 2, "");
static_assert(rank_from_eytzinger_in_complete_tree(9, 4) == 4, "");
static_assert(rank_from_eytzinger_in_complete_tree(10, 4) == 6, "");
static_assert(rank_from_eytzinger_in_complete_tree(11, 4) == 8, "");
static_assert(rank_from_eytzinger_in_complete_tree(12, 4) == 10, "");
static_assert(rank_from_eytzinger_in_complete_tree(13, 4) == 12, "");
static_assert(rank_from_eytzinger_in_complete_tree(14, 4) == 14, "");


constexpr int rank_from_eytzinger(int i, int n)
{
    if (i == n) return n;
    constexpr_assert(0 <= i && i < n);
    int D = depth_of_complete_tree(n);
    int R = rank_from_eytzinger_in_complete_tree(i, D);
    int CT_leaf_nodes_present = n - nodes_in_complete_tree(D-1);
    int CT_leaf_nodes_less_than_i = (R+1) >> 1;
    if (CT_leaf_nodes_present < CT_leaf_nodes_less_than_i) {
        R -= (CT_leaf_nodes_less_than_i - CT_leaf_nodes_present);
    }
    return R;
}
static_assert(rank_from_eytzinger(0, 1) == 0, "");
static_assert(rank_from_eytzinger(0, 2) == 1, "");
static_assert(rank_from_eytzinger(1, 2) == 0, "");
static_assert(rank_from_eytzinger(0, 3) == 1, "");
static_assert(rank_from_eytzinger(1, 3) == 0, "");
static_assert(rank_from_eytzinger(2, 3) == 2, "");
static_assert(rank_from_eytzinger(0, 4) == 2, "");
static_assert(rank_from_eytzinger(1, 4) == 1, "");
static_assert(rank_from_eytzinger(2, 4) == 3, "");
static_assert(rank_from_eytzinger(3, 4) == 0, "");
static_assert(rank_from_eytzinger(0, 5) == 3, "");
static_assert(rank_from_eytzinger(1, 5) == 1, "");
static_assert(rank_from_eytzinger(2, 5) == 4, "");
static_assert(rank_from_eytzinger(3, 5) == 0, "");
static_assert(rank_from_eytzinger(4, 5) == 2, "");

constexpr int eytzinger_from_rank(int i, int n)
{
    if (i == n) return n;
    constexpr_assert(0 <= i && i < n);
    int D = depth_of_complete_tree(n);
    int C = (1 << D) - 1;  // number of nodes in the complete tree
    int CT_leaf_nodes_present = n - (C >> 1);
    int CT_leaf_nodes_less_than_i = i >> 1;
    if (CT_leaf_nodes_present > CT_leaf_nodes_less_than_i) {
        return eytzinger_from_rank_in_complete_tree(i, D);
    } else {
        int j = C - ((n - i) << 1);
        int E = eytzinger_from_rank_in_complete_tree(j, D);
        constexpr_assert(rank_from_eytzinger(E, n) == i);
        return E;
    }
}
static_assert(eytzinger_from_rank(0, 1) == 0, "");
static_assert(eytzinger_from_rank(0, 2) == 1, "");
static_assert(eytzinger_from_rank(1, 2) == 0, "");
static_assert(eytzinger_from_rank(0, 3) == 1, "");
static_assert(eytzinger_from_rank(1, 3) == 0, "");
static_assert(eytzinger_from_rank(2, 3) == 2, "");
static_assert(eytzinger_from_rank(0, 4) == 3, "");
static_assert(eytzinger_from_rank(1, 4) == 1, "");
static_assert(eytzinger_from_rank(2, 4) == 0, "");
static_assert(eytzinger_from_rank(3, 4) == 2, "");
static_assert(eytzinger_from_rank(0, 5) == 3, "");
static_assert(eytzinger_from_rank(1, 5) == 1, "");
static_assert(eytzinger_from_rank(2, 5) == 4, "");
static_assert(eytzinger_from_rank(3, 5) == 0, "");
static_assert(eytzinger_from_rank(4, 5) == 2, "");
static_assert(eytzinger_from_rank(0, 8) == 7, "");
static_assert(eytzinger_from_rank(1, 8) == 3, "");
static_assert(eytzinger_from_rank(2, 8) == 1, "");
static_assert(eytzinger_from_rank(3, 8) == 4, "");
static_assert(eytzinger_from_rank(4, 8) == 0, "");
static_assert(eytzinger_from_rank(5, 8) == 5, "");
static_assert(eytzinger_from_rank(6, 8) == 2, "");
static_assert(eytzinger_from_rank(7, 8) == 6, "");
static_assert(eytzinger_from_rank(0, 9) == 7, "");
static_assert(eytzinger_from_rank(1, 9) == 3, "");
static_assert(eytzinger_from_rank(2, 9) == 8, "");
static_assert(eytzinger_from_rank(3, 9) == 1, "");
static_assert(eytzinger_from_rank(4, 9) == 4, "");
static_assert(eytzinger_from_rank(5, 9) == 0, "");
static_assert(eytzinger_from_rank(6, 9) == 5, "");
static_assert(eytzinger_from_rank(7, 9) == 2, "");
static_assert(eytzinger_from_rank(8, 9) == 6, "");
