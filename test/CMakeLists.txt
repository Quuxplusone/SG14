
find_package(GTest REQUIRED)
include(GoogleTest)

add_executable(utest
  aa_inplace_vector_pmr_test.cpp
  aa_inplace_vector_smallsize_test.cpp
  aa_inplace_vector_stdallocator_test.cpp
  aa_inplace_vector_test.cpp
  flat_map_test.cpp
  flat_set_test.cpp
  hive_test.cpp
  inplace_function_test.cpp
  inplace_vector_test.cpp
  ring_span_test.cpp
  slot_map_test.cpp
  uninitialized_test.cpp
  unstable_remove_test.cpp
)
target_include_directories(utest PRIVATE ${SG14_INCLUDE_DIRECTORY})
target_link_libraries(utest PRIVATE GTest::gtest GTest::gtest_main)

if (DEFINED ENV{GITHUB_ACTIONS})
  if (WIN32)
    add_custom_command(
      TARGET utest POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_RUNTIME_DLLS:utest> $<TARGET_FILE_DIR:utest>
      COMMAND_EXPAND_LISTS
    )
  endif()
  gtest_discover_tests(utest)
endif()

## Ad-hoc compiler options ##

if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
  set_property(TARGET utest APPEND PROPERTY COMPILE_OPTIONS "-Wall;-Wextra;-Werror")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  set_property(TARGET utest APPEND PROPERTY COMPILE_OPTIONS "-Wall;-Wextra;-Werror")
  # GCC 11 gives bogus warning "iteration 3 invokes undefined behavior"
  set_property(SOURCE aa_inplace_vector_smallsize_test.cpp APPEND PROPERTY COMPILE_OPTIONS "-Wno-aggressive-loop-optimizations")
  # GCC 13 gives bogus warnings on local types with typedefs
  set_property(SOURCE hive_test.cpp APPEND PROPERTY COMPILE_OPTIONS "-Wno-unused-local-typedefs")
  if (CMAKE_CXX_COMPILER_VERSION MATCHES "^7.*")
    # GCC 7 gives bogus warnings on structured bindings
    set_property(SOURCE slot_map_test.cpp APPEND PROPERTY COMPILE_OPTIONS "-Wno-unused-variable;-Wno-unused-but-set-variable")
  endif()
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  set_property(TARGET utest APPEND PROPERTY COMPILE_OPTIONS "-W4;-WX;-DNOMINMAX;-D_SCL_SECURE_NO_WARNINGS")
  # MSVC gives bogus "conditional expression is constant" warnings
  set_property(SOURCE aa_inplace_vector_pmr_test.cpp APPEND PROPERTY COMPILE_OPTIONS "-wd4127")
  set_property(SOURCE aa_inplace_vector_smallsize_test.cpp APPEND PROPERTY COMPILE_OPTIONS "-wd4127")
  set_property(SOURCE aa_inplace_vector_stdallocator_test.cpp APPEND PROPERTY COMPILE_OPTIONS "-wd4127")
  set_property(SOURCE aa_inplace_vector_test.cpp APPEND PROPERTY COMPILE_OPTIONS "-wd4127")
  set_property(SOURCE hive_test.cpp APPEND PROPERTY COMPILE_OPTIONS "-wd4127")
  set_property(SOURCE inplace_vector_test.cpp APPEND PROPERTY COMPILE_OPTIONS "-wd4127")
  # MSVC gives bogus "possible loss of data" warnings
  set_property(SOURCE hive_test.cpp APPEND PROPERTY COMPILE_OPTIONS "-wd4244;-wd4267")
  # MSVC gives bogus "unreachable code" warnings
  set_property(SOURCE hive_test.cpp APPEND PROPERTY COMPILE_OPTIONS "-wd4702")
  # std::aligned_storage_t is deprecated in C++23
  set_property(SOURCE inplace_function_test.cpp APPEND PROPERTY COMPILE_OPTIONS "-D_SILENCE_ALL_CXX23_DEPRECATION_WARNINGS")
endif()
