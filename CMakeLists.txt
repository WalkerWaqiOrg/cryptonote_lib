####################################one cmake list ##########################################################
cmake_minimum_required(VERSION 2.8.7)

project(slow_hash)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
  message(STATUS "Setting default build type: ${CMAKE_BUILD_TYPE}")
endif()
string(TOLOWER ${CMAKE_BUILD_TYPE} CMAKE_BUILD_TYPE_LOWER)

# ARCH defines the target architecture, either by an explicit identifier or
# one of the following two keywords. By default, ARCH a value of 'native':
# target arch = host arch, binary is not portable. When ARCH is set to the
# string 'default', no -march arg is passed, which creates a binary that is
# portable across processors in the same family as host processor.  In cases
# when ARCH is not set to an explicit identifier, cmake's builtin is used
# to identify the target architecture, to direct logic in this cmake script.
# Since ARCH is a cached variable, it will not be set on first cmake invocation.
if (NOT ARCH OR ARCH STREQUAL "" OR ARCH STREQUAL "native" OR ARCH STREQUAL "default")
  set(ARCH_ID "${CMAKE_SYSTEM_PROCESSOR}")
else()
  set(ARCH_ID "${ARCH}")
endif()
string(TOLOWER "${ARCH_ID}" ARM_ID)
string(SUBSTRING "${ARM_ID}" 0 3 ARM_TEST)
if (ARM_TEST STREQUAL "arm")
  set(ARM 1)
  string(SUBSTRING "${ARM_ID}" 0 5 ARM_TEST)
  if (ARM_TEST STREQUAL "armv6")
    set(ARM6 1)
  endif()
  if (ARM_TEST STREQUAL "armv7")
    set(ARM7 1)
  endif()
endif()

if (ARM_ID STREQUAL "aarch64" OR ARM_ID STREQUAL "arm64" OR ARM_ID STREQUAL "armv8-a")
  set(ARM 1)
  set(ARM8 1)
  set(ARCH "armv8-a")
endif()

if(ARCH_ID STREQUAL "ppc64le")
  set(PPC64LE 1)
endif()

if(WIN32 OR ARM)
  set(OPT_FLAGS_RELEASE "-O2")
else()
  set(OPT_FLAGS_RELEASE "-Ofast")
endif()

# BUILD_TAG is used to select the build type to check for a new version
if(BUILD_TAG)
  message(STATUS "Building build tag ${BUILD_TAG}")
  add_definitions("-DBUILD_TAG=${BUILD_TAG}")
else()
  message(STATUS "Building without build tag")
endif()

set(CMAKE_C_FLAGS_RELEASE "-DNDEBUG ${OPT_FLAGS_RELEASE}")
set(CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG ${OPT_FLAGS_RELEASE}")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
enable_testing()


# Check whether we're on a 32-bit or 64-bit system
if(CMAKE_SIZEOF_VOID_P EQUAL "8")
  set(DEFAULT_BUILD_64 ON)
else()
  set(DEFAULT_BUILD_64 OFF)
endif()
option(BUILD_64 "Build for 64-bit? 'OFF' builds for 32-bit." ${DEFAULT_BUILD_64})

if(BUILD_64)
  set(ARCH_WIDTH "64")
else()
  set(ARCH_WIDTH "32")
endif()
message(STATUS "Building for a ${ARCH_WIDTH}-bit system")

# Check if we're on FreeBSD so we can exclude the local miniupnpc (it should be installed from ports instead)
# CMAKE_SYSTEM_NAME checks are commonly known, but specifically taken from libsdl's CMakeLists
if(CMAKE_SYSTEM_NAME MATCHES "kFreeBSD.*|FreeBSD")
  set(FREEBSD TRUE)
endif()

# Check if we're on DragonFly BSD. See the README.md for build instructions.
if(CMAKE_SYSTEM_NAME MATCHES "DragonFly.*")
  set(DRAGONFLY TRUE)
endif()

# Check if we're on OpenBSD. See the README.md for build instructions.
if(CMAKE_SYSTEM_NAME MATCHES "kOpenBSD.*|OpenBSD.*")
  set(OPENBSD TRUE)
endif()

# TODO: check bsdi, NetBSD, to see if they need the same FreeBSD changes
#
# elseif(CMAKE_SYSTEM_NAME MATCHES "kNetBSD.*|NetBSD.*")
#   set(NETBSD TRUE)
# elseif(CMAKE_SYSTEM_NAME MATCHES ".*BSDI.*")
#   set(BSDI TRUE)

include_directories(external/easylogging++ src contrib/epee/include external "${CMAKE_BINARY_DIR}/version")

if(APPLE)
  include_directories(SYSTEM /usr/include/malloc)
  if(POLICY CMP0042)
    cmake_policy(SET CMP0042 NEW)
  endif()
endif()

if(MSVC OR MINGW)
  set(DEFAULT_STATIC true)
else()
  set(DEFAULT_STATIC false)
endif()
option(STATIC "Link libraries statically" ${DEFAULT_STATIC})

# This is a CMake built-in switch that concerns internal libraries
if (NOT DEFINED BUILD_SHARED_LIBS AND NOT STATIC AND CMAKE_BUILD_TYPE_LOWER STREQUAL "debug")
    set(BUILD_SHARED_LIBS ON)
endif()

if (BUILD_SHARED_LIBS)
  message(STATUS "Building internal libraries with position independent code")
  set(PIC_FLAG "-fPIC")
  add_definitions("-DBUILD_SHARED_LIBS")
else()
  message(STATUS "Building internal libraries as static")
endif()

if(MINGW)
  string(REGEX MATCH "^[^/]:/[^/]*" msys2_install_path "${CMAKE_C_COMPILER}")
  message(STATUS "MSYS location: ${msys2_install_path}")
  set(CMAKE_INCLUDE_PATH "${msys2_install_path}/mingw${ARCH_WIDTH}/include")
  # This is necessary because otherwise CMake will make Boost libraries -lfoo
  # rather than a full path. Unfortunately, this makes the shared libraries get
  # linked due to a bug in CMake which misses putting -static flags around the
  # -lfoo arguments.
  set(DEFLIB ${msys2_install_path}/mingw${ARCH_WIDTH}/lib)
  list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_DIRECTORIES ${DEFLIB})
  list(REMOVE_ITEM CMAKE_CXX_IMPLICIT_LINK_DIRECTORIES ${DEFLIB})
endif()

if(STATIC)
  if(MSVC)
    set(CMAKE_FIND_LIBRARY_SUFFIXES .lib .dll.a .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
  else()
    set(CMAKE_FIND_LIBRARY_SUFFIXES .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
  endif()
endif()

option(STACK_TRACE "Install a hook that dumps stack on exception" ${DEFAULT_STACK_TRACE})

if(STACK_TRACE)
  message(STATUS "Stack trace on exception enabled")
else()
  message(STATUS "Stack trace on exception disabled")
endif()

# Handle OpenSSL, used for sha256sum on binary updates
if (APPLE AND NOT IOS)
  if (NOT OpenSSL_DIR)
      EXECUTE_PROCESS(COMMAND brew --prefix openssl
        OUTPUT_VARIABLE OPENSSL_ROOT_DIR
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    message(STATUS "Using OpenSSL found at ${OPENSSL_ROOT_DIR}")
  endif()
endif()

find_package(OpenSSL REQUIRED)
if(STATIC AND NOT IOS)
  if(UNIX)
    set(OPENSSL_LIBRARIES "${OPENSSL_LIBRARIES};${CMAKE_DL_LIBS}")
  endif()
endif()

if (UNIX AND NOT APPLE)
  # Note that at the time of this writing the -Wstrict-prototypes flag added below will make this fail
  set(THREADS_PREFER_PTHREAD_FLAG ON)
  find_package(Threads)
endif()


if(MSVC)
  add_definitions("/bigobj /MP /W3 /GS- /D_CRT_SECURE_NO_WARNINGS /wd4996 /wd4345 /D_WIN32_WINNT=0x0600 /DWIN32_LEAN_AND_MEAN /DGTEST_HAS_TR1_TUPLE=0 /FIinline_c.h /D__SSE4_1__")
  # set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /Dinline=__inline")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /STACK:10485760")
  if(STATIC)
    foreach(VAR CMAKE_C_FLAGS_DEBUG CMAKE_CXX_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE CMAKE_CXX_FLAGS_RELEASE)
      string(REPLACE "/MD" "/MT" ${VAR} "${${VAR}}")
    endforeach()
  endif()
  include_directories(SYSTEM src/platform/msc)
else()
  include(TestCXXAcceptsFlag)
  set(ARCH native CACHE STRING "CPU to build for: -march value or 'default' to not pass -march at all")
  message(STATUS "Building on ${CMAKE_SYSTEM_PROCESSOR} for ${ARCH}")
  if(ARCH STREQUAL "default")
    set(ARCH_FLAG "")
  elseif(PPC64LE)
    set(ARCH_FLAG "-mcpu=${ARCH}")
  elseif(IOS AND ARCH STREQUAL "arm64")
    message(STATUS "IOS: Changing arch from arm64 to armv8")
    set(ARCH_FLAG "-march=armv8")
  else()
    set(ARCH_FLAG "-march=${ARCH}")
  endif()
  set(WARNINGS "-Wall -Wextra -Wpointer-arith -Wundef -Wvla -Wwrite-strings -Wno-error=extra -Wno-error=deprecated-declarations -Wno-unused-parameter -Wno-unused-variable -Wno-error=unused-variable -Wno-error=undef -Wno-error=uninitialized")
  if(NOT MINGW)
    set(WARNINGS_AS_ERRORS_FLAG "-Werror")
  endif()
  if(CMAKE_C_COMPILER_ID STREQUAL "Clang")
    if(ARM)
      set(WARNINGS "${WARNINGS} -Wno-error=inline-asm")
    endif()
  else()
    set(WARNINGS "${WARNINGS} -Wlogical-op -Wno-error=maybe-uninitialized")
  endif()
  if(MINGW)
    set(WARNINGS "${WARNINGS} -Wno-error=unused-value -Wno-error=unused-but-set-variable")
    set(MINGW_FLAG "${MINGW_FLAG} -DWIN32_LEAN_AND_MEAN")
    set(Boost_THREADAPI win32)
    include_directories(SYSTEM src/platform/mingw)
    # mingw doesn't support LTO (multiple definition errors at link time)
    set(USE_LTO_DEFAULT false)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--stack,10485760")
    if(NOT BUILD_64)
      add_definitions(-DWINVER=0x0501 -D_WIN32_WINNT=0x0501)
    endif()
  endif()
  set(C_WARNINGS "-Waggregate-return -Wnested-externs -Wold-style-definition -Wstrict-prototypes")
  set(CXX_WARNINGS "-Wno-reorder -Wno-missing-field-initializers")

  if(COVERAGE)
    message(STATUS "Building with profiling for test coverage report")
    set(COVERAGE_FLAGS "-fprofile-arcs -ftest-coverage --coverage")
  endif()

  # With GCC 6.1.1 the compiled binary malfunctions due to aliasing. Until that
  # is fixed in the code (Issue #847), force compiler to be conservative.
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-strict-aliasing")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-strict-aliasing")

  option(NO_AES "Explicitly disable AES support" ${NO_AES})

  if(NOT NO_AES AND NOT ARM AND NOT PPC64LE)
    message(STATUS "AES support enabled")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -maes")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -maes")
  elseif(PPC64LE)
    message(STATUS "AES support not available on ppc64le")
  elseif(ARM6)
    message(STATUS "AES support not available on ARMv6")
  elseif(ARM7)
    message(STATUS "AES support not available on ARMv7")
  elseif(ARM8)
    CHECK_CXX_ACCEPTS_FLAG("-march=${ARCH}+crypto" ARCH_PLUS_CRYPTO)
    if(ARCH_PLUS_CRYPTO)
      message(STATUS "Crypto extensions enabled for ARMv8")
      set(ARCH_FLAG "-march=${ARCH}+crypto")
    else()
      message(STATUS "Crypto extensions unavailable on your ARMv8 device")
    endif()
  else()
    message(STATUS "AES support disabled")
  endif()

  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11 -D_GNU_SOURCE ${MINGW_FLAG} ${STATIC_ASSERT_FLAG} ${WARNINGS} ${C_WARNINGS} ${ARCH_FLAG} ${COVERAGE_FLAGS} ${PIC_FLAG}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -D_GNU_SOURCE ${MINGW_FLAG} ${STATIC_ASSERT_CPP_FLAG} ${WARNINGS} ${CXX_WARNINGS} ${ARCH_FLAG} ${COVERAGE_FLAGS} ${PIC_FLAG}")

  # With GCC 6.1.1 the compiled binary malfunctions due to aliasing. Until that
  # is fixed in the code (Issue #847), force compiler to be conservative.
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-strict-aliasing")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-strict-aliasing")

  if(ARM)
    message(STATUS "Setting FPU Flags for ARM Processors")

    #NB NEON hardware does not fully implement the IEEE 754 standard for floating-point arithmetic
    #Need custom assembly code to take full advantage of NEON SIMD

    #Cortex-A5/9  -mfpu=neon-fp16
    #Cortex-A7/15 -mfpu=neon-vfpv4
    #Cortex-A8    -mfpu=neon
    #ARMv8      -FP and SIMD on by default for all ARM8v-A series, NO -mfpu setting needed

    #For custom -mtune, processor IDs for ARMv8-A series:
    #0xd04 - Cortex-A35
    #0xd07 - Cortex-A57
    #0xd08 - Cortex-A72
    #0xd03 - Cortex-A73

    if(NOT ARM8)
      CHECK_CXX_ACCEPTS_FLAG(-mfpu=vfp3-d16 CXX_ACCEPTS_VFP3_D16)
      CHECK_CXX_ACCEPTS_FLAG(-mfpu=vfp4 CXX_ACCEPTS_VFP4)
      CHECK_CXX_ACCEPTS_FLAG(-mfloat-abi=hard CXX_ACCEPTS_MFLOAT_HARD)
      CHECK_CXX_ACCEPTS_FLAG(-mfloat-abi=softfp CXX_ACCEPTS_MFLOAT_SOFTFP)
    endif()

    if(ARM8)
      CHECK_CXX_ACCEPTS_FLAG(-mfix-cortex-a53-835769 CXX_ACCEPTS_MFIX_CORTEX_A53_835769)
      CHECK_CXX_ACCEPTS_FLAG(-mfix-cortex-a53-843419 CXX_ACCEPTS_MFIX_CORTEX_A53_843419)
    endif()

    if(ARM6)
      message(STATUS "Selecting VFP for ARMv6")
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mfpu=vfp")
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mfpu=vfp")
    endif(ARM6)

    if(ARM7)
      if(CXX_ACCEPTS_VFP3_D16 AND NOT CXX_ACCEPTS_VFP4)
        message(STATUS "Selecting VFP3 for ARMv7")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mfpu=vfp3-d16")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mfpu=vfp3-d16")
      endif()

      if(CXX_ACCEPTS_VFP4)
        message(STATUS "Selecting VFP4 for ARMv7")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mfpu=vfp4")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mfpu=vfp4")
      endif()

      if(CXX_ACCEPTS_MFLOAT_HARD)
        message(STATUS "Setting Hardware ABI for Floating Point")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mfloat-abi=hard")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mfloat-abi=hard")
      endif()

      if(CXX_ACCEPTS_MFLOAT_SOFTFP AND NOT CXX_ACCEPTS_MFLOAT_HARD)
        message(STATUS "Setting Software ABI for Floating Point")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mfloat-abi=softfp")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mfloat-abi=softfp")
      endif()
    endif(ARM7)

    if(ARM8)
      if(CXX_ACCEPTS_MFIX_CORTEX_A53_835769)
        message(STATUS "Enabling Cortex-A53 workaround 835769")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mfix-cortex-a53-835769")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mfix-cortex-a53-835769")
      endif()

      if(CXX_ACCEPTS_MFIX_CORTEX_A53_843419)
        message(STATUS "Enabling Cortex-A53 workaround 843419")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mfix-cortex-a53-843419")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mfix-cortex-a53-843419")
      endif()
    endif(ARM8)

  endif(ARM)

  if(ANDROID AND NOT BUILD_GUI_DEPS STREQUAL "ON" OR IOS)
    #From Android 5: "only position independent executables (PIE) are supported" 
    message(STATUS "Enabling PIE executable")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIE")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIE")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_CXX_FLAGS} -fPIE -pie")
  endif()

  if(APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGTEST_HAS_TR1_TUPLE=0")
  endif()

  set(DEBUG_FLAGS "-g3")
  if(CMAKE_C_COMPILER_ID STREQUAL "GNU" AND NOT (CMAKE_C_COMPILER_VERSION VERSION_LESS 4.8))
    set(DEBUG_FLAGS "${DEBUG_FLAGS} -Og ")
  else()
    set(DEBUG_FLAGS "${DEBUG_FLAGS} -O0 ")
  endif()

  if(NOT DEFINED USE_LTO_DEFAULT)
    set(USE_LTO_DEFAULT false)
  endif()
  set(USE_LTO ${USE_LTO_DEFAULT} CACHE BOOL "Use Link-Time Optimization (Release mode only)")

  if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # There is a clang bug that does not allow to compile code that uses AES-NI intrinsics if -flto is enabled, so explicitly disable
    set(USE_LTO false)
  endif()


  if(USE_LTO)
    set(RELEASE_FLAGS "${RELEASE_FLAGS} -flto")
    if(STATIC)
      set(RELEASE_FLAGS "${RELEASE_FLAGS} -ffat-lto-objects")
    endif()
    # Since gcc 4.9 the LTO format is non-standard (slim), so we need the gcc-specific ar and ranlib binaries
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND NOT (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.9.0) AND NOT OPENBSD AND NOT DRAGONFLY)
      # When invoking cmake on distributions on which gcc's binaries are prefixed
      # with an arch-specific triplet, the user must specify -DCHOST=<prefix>
      if (DEFINED CHOST)
        set(CMAKE_AR "${CHOST}-gcc-ar")
        set(CMAKE_RANLIB "${CHOST}-gcc-ranlib")
      else()
        set(CMAKE_AR "gcc-ar")
        set(CMAKE_RANLIB "gcc-ranlib")
      endif()
    endif()
  endif()

  set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} ${DEBUG_FLAGS}")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${DEBUG_FLAGS}")
  set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} ${RELEASE_FLAGS}")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${RELEASE_FLAGS}")

  if(STATIC)
    # STATIC already configures most deps to be linked in statically,
    # here we make more deps static if the platform permits it
    if (MINGW)
      # On Windows, this is as close to fully-static as we get:
      # this leaves only deps on /c/Windows/system32/*.dll
      set(STATIC_FLAGS "-static")
    elseif (NOT (APPLE OR FREEBSD OR OPENBSD OR DRAGONFLY))
      # On Linux, we don't support fully static build, but these can be static
      set(STATIC_FLAGS "-static-libgcc -static-libstdc++")
    endif()
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${STATIC_FLAGS} ")
  endif()
endif()

####################################two cmake list   src/cmakelist.txt##########################################################

if (WIN32 OR STATIC)
  add_definitions(-DSTATICLIB)
  # miniupnp changed their static define
  add_definitions(-DMINIUPNP_STATICLIB)
endif ()

function (monero_private_headers group)
  source_group("${group}\\Private"
    FILES
      ${ARGN})
endfunction ()


function (monero_add_library name)
  source_group("${name}"
    FILES
      ${ARGN})

  # Define a ("virtual") object library and an actual library that links those
  # objects together. The virtual libraries can be arbitrarily combined to link
  # any subset of objects into one library archive. This is used for releasing
  # libwallet, which combines multiple components.
  set(objlib obj_${name})
  add_library(${objlib} OBJECT ${ARGN})
  add_library("${name}" $<TARGET_OBJECTS:${objlib}>)
  set_property(TARGET "${name}"
    PROPERTY
      FOLDER "libs")
  target_compile_definitions(${objlib}
    PRIVATE $<TARGET_PROPERTY:${name},INTERFACE_COMPILE_DEFINITIONS>)
endfunction ()

####################################three cmake list  src/crypto/cmakelist.txt##########################################################
set(crypto_sources
  aesb.c
  blake256.c
  groestl.c
  hash-extra-blake.c
  hash-extra-groestl.c
  hash-extra-jh.c
  hash-extra-skein.c
  hash.c
  jh.c
  keccak.c
  oaes_lib.c
  skein.c
  slow-hash.c)

set(crypto_headers)

set(crypto_private_headers
  blake256.h
  chacha8.h
  crypto-ops.h
  crypto.h
  generic-ops.h
  groestl.h
  groestl_tables.h
  hash-ops.h
  hash.h
  initializer.h
  jh.h
  keccak.h
  oaes_config.h
  oaes_lib.h
  random.h
  skein.h
  skein_port.h
  warnings.h)

monero_private_headers(cncrypto
  ${crypto_private_headers})
monero_add_library(cncrypto
  ${crypto_sources}
  ${crypto_headers}
  ${crypto_private_headers})
target_link_libraries(cncrypto
  PUBLIC
    ${Boost_SYSTEM_LIBRARY}
  PRIVATE
    ${EXTRA_LIBRARIES})

if (ARM)
  option(NO_OPTIMIZED_MULTIPLY_ON_ARM
     "Compute multiply using generic C implementation instead of ARM ASM" OFF)
  if(NO_OPTIMIZED_MULTIPLY_ON_ARM)
    message(STATUS "Using generic C implementation for multiply")
    set_property(SOURCE slow-hash.c
      PROPERTY COMPILE_DEFINITIONS "NO_OPTIMIZED_MULTIPLY_ON_ARM")
  endif()
endif()

# Because of the way Qt works on android with JNI, the code does not live in the main android thread
# So this code runs with a 1 MB default stack size. 
# This will force the use of the heap for the allocation of the scratchpad
if (ANDROID OR IOS)
  if( BUILD_GUI_DEPS )
    add_definitions(-DFORCE_USE_HEAP=1)
  endif()
endif()