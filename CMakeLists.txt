cmake_minimum_required(VERSION 3.16.0)


project(
    PUF-ACS-Authenticator
    VERSION 0.1.0
    DESCRIPTION "Implementation of a PUF-ACS Authenticator"
    LANGUAGES CXX
)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Debug)


# Set this to True if authenticator functionality is needed
set(BUILD_AUTHENTICATOR TRUE
    CACHE BOOL
    "Include authenticator functionality"
)

# Set this to True if supplicant functionality is needed
set(BUILD_SUPPLICANT FALSE
    CACHE BOOL
    "Include supplicant functionality"
)


add_subdirectory(lib)
add_subdirectory(src)
