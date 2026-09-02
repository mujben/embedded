get_filename_component(PIO_ENV_NAME "${CMAKE_BINARY_DIR}" NAME)

if("${PIO_ENV_NAME}" STREQUAL "")
    message(FATAL_ERROR "Could not determine the PlatformIO environment name from CMAKE_BINARY_DIR (${CMAKE_BINARY_DIR}).")
endif()

file(GLOB app_sources "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

list(FILTER app_sources EXCLUDE REGEX ".*_main\\.c$")

list(APPEND app_sources "${CMAKE_CURRENT_SOURCE_DIR}/${PIO_ENV_NAME}_main.c")

idf_component_register(
    SRCS ${app_sources}
    INCLUDE_DIRS "."
)