find_package(GTest REQUIRED)
file(GLOB test_sources CONFIGURE_DEPENDS ./*.cpp)
add_executable(
  armv8_a_study_test
  ${test_sources}
)

target_link_libraries(
  armv8_a_study_test
  GTest::gtest_main
)

gtest_discover_tests(armv8_a_study_test)