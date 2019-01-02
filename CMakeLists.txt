cmake_minimum_required(VERSION 3.10)
project(libtun2socks)
set(CMAKE_CXX_STANDARD 14)

IF (WIN32)
set(CMAKE_GENERATOR_PLATFORM STRING x64)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc")
set(CMAKE_EXE_LINKER_FLAGS "/machine:X64")
set(CMAKE_GENERATOR_PLATFORM "X64" CACHE STRING "x64" FORCE)
set(CMAKE_EXE_LINKER_FLAGS "/machine:X64" CACHE STRING "x64" FORCE)
set(CMAKE_MODULE_LINKER_FLAGS "/machine:X64" CACHE STRING "x64" FORCE)
set(CMAKE_SKIP_RPATH "/machine:X64" CACHE STRING "x64" FORCE)
set(CMAKE_STATIC_LINKER_FLAGS "/machine:X64" CACHE STRING "x64" FORCE)
set(CMAKE_SHARED_LINKER_FLAGS "/machine:X64" CACHE STRING "x64" FORCE)
set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "Debug;Release" FORCE)
include_directories("C:/Boost/include/boost-1_68")
include_directories("C:/Users/mario/Documents/spdlog-1.2.1/include")
link_directories("C:/Boost/lib")
endif()

function(assign_source_group)
    foreach (_source IN ITEMS ${ARGN})
        if (IS_ABSOLUTE "${_source}")
            file(RELATIVE_PATH _source_rel "${CMAKE_CURRENT_SOURCE_DIR}" "${_source}")
        else ()
            set(_source_rel "${_source}")
        endif ()
        get_filename_component(_source_path "${_source_rel}" PATH)
        string(REPLACE "/" "\\" _source_path_msvc "${_source_path}")
        source_group("${_source_path_msvc}" FILES "${_source}")
    endforeach ()
endfunction(assign_source_group)

function(my_add_executable)
    foreach (_source IN ITEMS ${ARGN})
        assign_source_group(${_source})
    endforeach ()
    add_executable(${ARGV})
endfunction(my_add_executable)

file(GLOB_RECURSE LWIP_INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/lwip/include/*.*")
file(GLOB_RECURSE LWIP_CORE "${CMAKE_CURRENT_SOURCE_DIR}/lwip/core/*.*")
file(GLOB_RECURSE LWIP_NETIF "${CMAKE_CURRENT_SOURCE_DIR}/lwip/netif/*.*")
file(GLOB_RECURSE LWIP_PLATFORM "${CMAKE_CURRENT_SOURCE_DIR}/lwip/platform/*.*")

file(GLOB_RECURSE LIB "${CMAKE_CURRENT_SOURCE_DIR}/lib/*.*")
file(GLOB_RECURSE NET "${CMAKE_CURRENT_SOURCE_DIR}/net/*.*")
file(GLOB_RECURSE TUNTAP "${CMAKE_CURRENT_SOURCE_DIR}/tuntap/*.*")
file(GLOB_RECURSE UTILS "${CMAKE_CURRENT_SOURCE_DIR}/utils/*.*")

include_directories(lwip/include)

my_add_executable(libtun2socks
        
		main.cpp
		${LWIP_CORE}
		${LWIP_NETIF}
		${LWIP_PLATFORM}
		${LIB}
		${NET}
		${TUNTAP}
		${UTILS}
		)
		
add_definitions(-DBOOST_COROUTINES_NO_DEPRECATION_WARNING)
add_definitions(-DLWIP_DONT_PROVIDE_BYTEORDER_FUNCTIONS)
add_definitions(-DSINGLETON_RAW_POINTER)


IF (WIN32)
	
 
  add_definitions(-D_WIN32_WINNT=0x0A00)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
  add_definitions(-DWIN32_LEAN_AND_MEAN)
  add_definitions(-D_WINSOCK_DEPRECATED_NO_WARNINGS)

ENDIF()
set_target_properties(libtun2socks PROPERTIES PREFIX "")

IF (CMAKE_SYSTEM_NAME MATCHES "Darwin")
include_directories("/Users/mario/Documents/spdlog-1.2.1/include")
ENDIF()

IF (CMAKE_SYSTEM_NAME MATCHES "Linux")
    include_directories("/home/spdlog-1.2.1/include")
ENDIF()

IF (NOT WIN32)
    set(Boost_USE_STATIC_LIBS ON)
    set(Boost_USE_MULTITHREADED ON)
    set(Boost_USE_STATIC_RUNTIME OFF)
    find_package(Boost 1.68.0 REQUIRED system  coroutine context regex thread)

    IF (Boost_FOUND)
        include_directories(${Boost_INCLUDE_DIRS})
        target_link_libraries(libtun2socks ${Boost_LIBRARIES})
    ENDIF ()
ENDIF()
