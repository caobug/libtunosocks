cmake_minimum_required(VERSION 3.10)
project(libtun2socks)
set(CMAKE_CXX_STANDARD 14)

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
        message(${ARGV})
    add_library(${ARGV})
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
        SHARED
        lib/tunosocks.cpp
        #lib/tunosocks_wrapper.cpp
		#main.cpp
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

IF (WIN32)
    include_directories("C:/spdlog-1.3.1/include")
ENDIF()

IF(NOT BUILD_IOS_LIBRARY)
    #    set(Boost_USE_STATIC_LIBS ON)
    #    set(Boost_USE_MULTITHREADED ON)
    #    set(Boost_USE_STATIC_RUNTIME OFF)
    #    find_package(Boost 1.70.0 REQUIRED system filesystem coroutine context regex thread)
    #
    #    if (Boost_FOUND)
    #        include_directories(${Boost_INCLUDE_DIRS})
    #        target_link_libraries(libtun2socks ${Boost_LIBRARIES})
    #    endif ()

    IF(WIN32)
        include_directories("C:/Boost/include/boost-1_70")

        #target_link_libraries(libtun2socks "C:/Boost/lib/libboost_regex-vc142-mt-x64-1_70.lib")
        #target_link_libraries(libtun2socks "C:/Boost/lib/libboost_context-vc142-mt-x64-1_70.lib")
        #target_link_libraries(libtun2socks "C:/Boost/lib/libboost_coroutine-vc142-mt-x64-1_70.lib")
        #target_link_libraries(libtun2socks "C:/Boost/lib/libboost_context-vc142-mt-x64-1_70.lib")
        #target_link_libraries(libtun2socks "C:/Boost/lib/libboost_date_time-vc142-mt-x64-1_70.lib")
        #target_link_libraries(libtun2socks "C:/Boost/lib/libboost_thread-vc142-mt-x64-1_70.lib")
        #target_link_libraries(libtun2socks "C:/Boost/lib/libboost_chrono-vc142-mt-x64-1_70.lib")
        #target_link_libraries(libtun2socks "C:/Boost/lib/libboost_filesystem-vc142-mt-gd-x64-1_70.lib")
        #target_link_libraries(libtun2socks "C:/Boost/lib/libboost_program_options-vc142-mt-gd-x64-1_70.lib")
		
		
		
        target_link_libraries(libtun2socks "C:/Boost/lib/libboost_regex-vc142-mt-gd-x64-1_70.lib")
        target_link_libraries(libtun2socks "C:/Boost/lib/libboost_context-vc142-mt-gd-x64-1_70.lib")
        target_link_libraries(libtun2socks "C:/Boost/lib/libboost_coroutine-vc142-mt-gd-x64-1_70.lib")
        target_link_libraries(libtun2socks "C:/Boost/lib/libboost_context-vc142-mt-gd-x64-1_70.lib")
        target_link_libraries(libtun2socks "C:/Boost/lib/libboost_date_time-vc142-mt-gd-x64-1_70.lib")
        target_link_libraries(libtun2socks "C:/Boost/lib/libboost_thread-vc142-mt-gd-x64-1_70.lib")
        target_link_libraries(libtun2socks "C:/Boost/lib/libboost_chrono-vc142-mt-gd-x64-1_70.lib")
        target_link_libraries(libtun2socks "C:/Boost/lib/libboost_filesystem-vc142-mt-gd-x64-1_70.lib")
		target_link_libraries(libtun2socks "C:/Boost/lib/libboost_program_options-vc142-mt-gd-x64-1_70.lib")


    ELSE()
        include_directories("/usr/local/include")

        target_link_libraries(libtun2socks "/usr/local/lib/libboost_thread.a")
        target_link_libraries(libtun2socks "/usr/local/lib/libboost_regex.a")
        target_link_libraries(libtun2socks "/usr/local/lib/libboost_coroutine.a")
        target_link_libraries(libtun2socks "/usr/local/lib/libboost_context.a")
        target_link_libraries(libtun2socks "/usr/local/lib/libboost_filesystem.a")
        target_link_libraries(libtun2socks "/usr/local/lib/libboost_program_options.a")
    ENDIF()
ENDIF()
