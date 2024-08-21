#if __cplusplus >= 201703L

#include <sg14/inplace_vector.h>
#define IPV_TEST_NAME inplace_vector
#define IPV_HAS_CONDITIONALLY_TRIVIAL_SMFS (__cpp_concepts >= 202002L)
#include "inplace_vector_common_tests.cpp"

#endif // __cplusplus >= 201703L

