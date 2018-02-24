cmake_minimum_required(VERSION 3.0.1)
project(fastmono-performance)
# set(CMAKE_BUILD_TYPE Release) # first need to figure out how to stop compiler optimising out the triangulation calls
set(CMAKE_CXX_STANDARD 11)

set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/bin)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
set(SOURCE_FILES ${PROJECT_SOURCE_DIR}/bench.cpp ${PROJECT_SOURCE_DIR}/polypartition/polypartition.cpp)

find_package(Boost COMPONENTS thread REQUIRED)
if (NOT Boost_FOUND)
	message(FATAL_ERROR "Boost not found.\nBoost is required for CGAL, which provides the random polygon generator.")
endif()

find_package(CGAL REQUIRED)
if (NOT CGAL_FOUND)
	message(WARNING "Could not find CGAL using FindCGAL.cmake. Attempting to locate CGAL from environment variables.")
	if (NOT CGAL_INCLUDE)
		if (DEFINED ENV{CGAL_DIR})
			message(STATUS "Using CGAL from environment variable CGAL_DIR: $ENV{CGAL_DIR}")
			set(CGAL_INCLUDE $ENV{CGAL_DIR}/include $ENV{CGAL_DIR}/build/include)# may not work if CGAL compiler_options.h is not in CGAL_DIR/build/CGA
			set(CGAL_LIB $ENV{CGAL_DIR}/build/lib)# again, may not work
		else()
			message(FATAL_ERROR "CGAL_INCLUDE is not set. Please set it to the include path of CGAL or specify the environment variable CGAL_DIR.")
		endif()
	endif()

	if (NOT CGAL_LIB)
		message(FATA_ERROR "CGAL_LIB is not set. Please set it to the lib path of CGAL or specify the environment variable CGAL_DIR.")
	endif()
endif()

if (MSVC)
	set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MD /EHsc")# Set MD runtime for MSVC.
	set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Ox /GL")
elseif (CMAKE_COMPILER_IS_GNUCC)
	set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -O3")
endif (MSVC)

# Attempt to add support for colors on gcc or clang with ninja
include(CheckCXXCompilerFlag)
macro(AddCXXFlagIfSupported flag test)
  CHECK_CXX_COMPILER_FLAG(${flag} ${test})
  if( ${${test}} )
    message("adding ${flag}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${flag}")
  endif()
endmacro()

if("Ninja" STREQUAL ${CMAKE_GENERATOR})
  AddCXXFlagIfSupported(-fcolor-diagnostics COMPILER_SUPPORTS_fcolor-diagnostics)
endif()

AddCXXFlagIfSupported(-ferror-limit=2 COMPILER_SUPPORTS_ferror-limit)
AddCXXFlagIfSupported(-fmax-errors=2 COMPILER_SUPPORTS_fmax-errors)

include_directories(${PROJECT_SOURCE_DIR} ${CGAL_INCLUDE} ${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIRS} ${CGAL_LIB})

add_executable(benchmark ${SOURCE_FILES})
