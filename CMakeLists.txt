cmake_minimum_required(VERSION 3.19)
project(PSU)

set(CMAKE_CXX_STANDARD 20)
#set(CMAKE_CXX_COMPILER /usr/bin/clang)
#set(CMAKE_C_FLAGS "-ffunction-sections -Wall -Wno-strict-aliasing  -maes -msse2 -msse4.1 -mpclmul -Wno-sign-compare -Wfatal-errors -pthread")
SET(CMAKE_CXX_FLAGS_RELEASE "-g -O3 -DNDEBUG")
set(MY_LIBS volePSI secureJoin libOTe cryptoTools relic_s macoro coproto sodium bitpolymul)
set(BOOST_COMPOENTS filesystem system thread atomic regex)

add_compile_options(-Wno-everything -mavx2 -msse4.2 -maes -msse2 -msse3 -mssse3 -msse4.1 -mpclmul ) 
include_directories(libs/volepsi/include)
link_directories(libs/volepsi/lib)
include_directories(libs/securejoin/include)
link_directories(libs/securejoin/lib)
set(BOOST_INCLUDEDIR libs/volepsi/include)
include_directories(${BOOST_INCLUDEDIR})

find_package(Boost COMPONENTS REQUIRED ${BOOST_COMPOENTS})
include_directories(mp_oprf)
include_directories(common)
include_directories(oprf_cuckoo_translate)

add_library(mp_oprf STATIC
        mp_oprf/MPOPRFReceiver.cpp
        mp_oprf/MPOPRFReceiver.h
        mp_oprf/MPOPRFSender.cpp
        mp_oprf/MPOPRFSender.h
        )

add_library(common STATIC
        common/cmdline.h
        common/PSUSender.h
        common/PSUReceiver.h
        common/utils.cpp
        common/utils.h
        common/MP-OPRF-Parameters.h
        common/SimpleIndexParameters.h
        common/SimpleIndex.cpp
        common/SimpleIndex.h
        common/sha1.cpp
        common/sha1.h)

add_library(oct STATIC
        oprf_cuckoo_translate/OCTSender.cpp
        oprf_cuckoo_translate/OCTSender.h
        oprf_cuckoo_translate/OCTReceiver.cpp
        oprf_cuckoo_translate/OCTReceiver.h)



add_executable(test_psu
        test/test_psu.cpp)

target_link_libraries(test_psu oct common mp_oprf ${MY_LIBS} ${Boost_LIBRARIES})
