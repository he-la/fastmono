cmake_minimum_required(VERSION 3.0.1)
project(fastmono-performance CXX)
set(CMAKE_CXX_STANDARD 11)

if ("Debug" STREQUAL CMAKE_BUILD_TYPE)
	message(WARNING "Running benchmarks as debug builds is not recommended.")
	add_definitions(-DDEBUG)
endif()

# FIXME: Remove
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS_DEBUG} -O0")
# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS_DEBUG} -O0")

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
set(SOURCE_FILES ${PROJECT_SOURCE_DIR}/bench.cpp ${PROJECT_SOURCE_DIR}/polypartition/src/polypartition.cpp)

find_package(Boost COMPONENTS thread REQUIRED)
if (NOT Boost_FOUND)
	message(FATAL_ERROR "Boost not found.\nBoost is required for CGAL, which provides the random polygon generator.")
endif()

find_package(CGAL REQUIRED)
if (NOT CGAL_FOUND)
	message(WARNING "Could not find CGAL using FindCGAL.cmake. Attempting to locate CGAL from environment variables.")
	if (NOT CGAL_INCLUDE_DIR)
		if (DEFINED ENV{CGAL_DIR})
			message(STATUS "Using CGAL from environment variable CGAL_DIR: $ENV{CGAL_DIR}")
			set(CGAL_INCLUDE_DIR $ENV{CGAL_DIR}/include $ENV{CGAL_DIR}/build/include)# may not work if CGAL compiler_options.h is not in CGAL_DIR/build/CGA
			set(CGAL_LIBRARIES $ENV{CGAL_DIR}/build/lib)# again, may not work
		else()
			message(FATAL_ERROR "CGAL_INCLUDE_DIR is not set. Please set it to the include path of CGAL or specify the environment variable CGAL_DIR.")
		endif()
	endif()

	if (NOT CGAL_LIBRARIES)
		message(FATA_ERROR "CGAL_LIBRARIES is not set. Please set it to the lib path of CGAL or specify the environment variable CGAL_DIR.")
	endif()
endif()

# if (MSVC)
# 	set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MD /EHsc")# Set MD runtime for MSVC.
# 	set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Ox /GL")
# elseif (CMAKE_COMPILER_IS_GNUCC)
# 	set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -O3")
# endif (MSVC)

# Attempt to add support for colors on gcc or clang with ninja
include(CheckCXXCompilerFlag)
macro(AddCXXFlagIfSupported flag test)
  CHECK_CXX_COMPILER_FLAG(${flag} ${test})
  if( ${${test}} )
    message(STATUS "Compiller supports ${flag}; adding.")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${flag}")
  endif()
endmacro()

if("Ninja" STREQUAL ${CMAKE_GENERATOR})
  AddCXXFlagIfSupported("-fcolor-diagnostics" COMPILER_SUPPORTS_fcolor-diagnostics)
endif()

AddCXXFlagIfSupported("-ferror-limit=2" COMPILER_SUPPORTS_ferror-limit)
AddCXXFlagIfSupported("-fmax-errors=2" COMPILER_SUPPORTS_fmax-errors)

if ("Release" STREQUAL CMAKE_BUILD_TYPE)
  CHECK_CXX_COMPILER_FLAG("-O3 -mllvm -polly" COMPILER_SUPPORTS_polly)
  if (${COMPILER_SUPPORTS_polly} AND NOT DEFINED FMT_NO_POLLY)
    message(STATUS "Compiler supports LLVM polyhedral optimsiation - adding according commands.")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -mllvm -polly -mllvm -polly-parallel -mllvm -polly-vectorizer=stripmine -mllvm -polly-dot")
  else()
    message(STATUS "No LLVM polyhedral optimsiation support detected - skipping.")
  endif()
endif()

include_directories(${PROJECT_SOURCE_DIR} ${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIRS})

# Support mutliple versions of FindCGAL
if (CGAL_LIBRARIES)
	link_libraries(${CGAL_LIBRARIES})
endif()

if (CGAL_INCLUDE_DIR)
	include_directories(${CGAL_INCLUDE_DIR})
endif()

if (CGAL_USE_FILE)
	include(${CGAL_USE_FILE})
endif()

add_executable(benchmark ${SOURCE_FILES})
