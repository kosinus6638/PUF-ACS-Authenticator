set(NAME_EXE "au" CACHE STRING "Name of the binary")

# Find all source files automatically
FILE(GLOB SOURCES *.cpp *.c)

add_executable(
    ${NAME_EXE}
    ${SOURCES}
)

find_package(
    Boost
    1.74 REQUIRED
    COMPONENTS program_options REQUIRED
)

target_link_libraries(
    ${NAME_EXE}
    PUBLIC Boost::program_options
)

target_include_directories(
    ${NAME_EXE}
    PRIVATE ../lib/PUF-ACS
)

target_link_libraries(
    ${NAME_EXE}
    PRIVATE
    ${PUF_ACS_NAME}
)
