set(PUF_ACS_NAME "puf-acs" CACHE STRING "Name of the PUF-ACS library")


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

find_package(MbedTLS QUIET)
if(${MbedTLS_FOUND})
    message("Using MBedTLS cmake component")
else()
    message("Linking against mbedtls")
    target_link_libraries( ${PUF_ACS_NAME} mbedtls )
    target_link_libraries( ${PUF_ACS_NAME} mbedx509 )
    target_link_libraries( ${PUF_ACS_NAME} mbedcrypto )
endif()