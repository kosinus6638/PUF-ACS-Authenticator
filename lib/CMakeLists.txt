set(PUF_ACS_NAME "puf-acs" CACHE STRING "Name of the PUF-ACS library")

add_subdirectory(mbedtls)

set(PUF_ACS_SOURCES
    PUF-ACS/math.cpp
    PUF-ACS/packets.cpp
    PUF-ACS/statics.cpp
)


if(${BUILD_AUTHENTICATOR})
    list(APPEND PUF_ACS_SOURCES PUF-ACS/authenticator.cpp)
endif()

if(${BUILD_SUPPLICANT})
    list(APPEND PUF_ACS_SOURCES PUF-ACS/supplicant.cpp)
endif()


add_library(
    ${PUF_ACS_NAME}
    STATIC
    ${PUF_ACS_SOURCES}
)


#target_include_directories(
#    ${PUF_ACS_NAME}
#    PRIVATE lib/mbedtls/include/mbedtls
#)
#
#
#target_link_libraries(
#    ${PUF_ACS_NAME}
#    PRIVATE MbedTLS::mbedtls
#)
