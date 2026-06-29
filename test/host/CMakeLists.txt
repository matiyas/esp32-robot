cmake_minimum_required(VERSION 3.16)
project(esp32_robot_host_test C)

set(CMAKE_C_STANDARD 17)

# Unity test framework
include(FetchContent)
FetchContent_Declare(
    unity
    GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git
    GIT_TAG v2.5.2
)
FetchContent_MakeAvailable(unity)

# Mock implementations
add_library(mocks STATIC
    mocks/mock_esp.c
)
target_include_directories(mocks PUBLIC
    mocks
    ${CMAKE_SOURCE_DIR}/../components/hal/include
    ${CMAKE_SOURCE_DIR}/../components/robot_core/include
    ${CMAKE_SOURCE_DIR}/../components/motor_control/include
    ${CMAKE_SOURCE_DIR}/../components/servo_control/include
    ${CMAKE_SOURCE_DIR}/../components/safety_handler/include
)

# Test executable. The crsf_input parser/mapping are pure C (no ESP headers),
# so we compile the real production sources directly. Note the ../../ depth:
# CMAKE_SOURCE_DIR is test/host, so components live two levels up.
add_executable(robot_test
    test_safety_validate.c
    test_crsf_parser.c
    test_rc_map.c
    ${CMAKE_SOURCE_DIR}/../../components/crsf_input/src/crsf_protocol.c
    ${CMAKE_SOURCE_DIR}/../../components/crsf_input/src/rc_map.c
)

target_link_libraries(robot_test
    unity
    mocks
)

target_include_directories(robot_test PRIVATE
    ${CMAKE_SOURCE_DIR}/../components/hal/include
    ${CMAKE_SOURCE_DIR}/../components/robot_core/include
    ${CMAKE_SOURCE_DIR}/../components/motor_control/include
    ${CMAKE_SOURCE_DIR}/../components/servo_control/include
    ${CMAKE_SOURCE_DIR}/../components/safety_handler/include
    ${CMAKE_SOURCE_DIR}/../../components/crsf_input/include
)

enable_testing()
add_test(NAME robot_test COMMAND robot_test)
