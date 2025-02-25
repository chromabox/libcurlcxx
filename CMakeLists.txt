cmake_minimum_required(VERSION 3.22)

project(
	curlcxx
	VERSION 0.1.0
	DESCRIPTION "modern C++ cURL wrapper library"
	LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(DEFAULT_BUILD_TYPE "Release")
set(CMAKE_C_FLAGS_DEBUG "-g3 -Og")
set(CMAKE_C_FLAGS_RELEASE "-g -O2")

# libcurl build option
#set(BUILD_CURL_EXE ON)
set(BUILD_CURL_EXE OFF)
set(BUILD_SHARED_LIBS OFF)
set(BUILD_STATIC_LIBS ON)
set(BUILD_STATIC_CURL ON)
set(BUILD_LIBCURL_DOCS OFF)
set(CURL_DISABLE_INSTALL OFF)
set(ENABLE_WEBSOCKETS ON)
set(ENABLE_CURL_MANUAL OFF)

option(BUILD_SAMPLE "Build Samples" OFF)

add_subdirectory(extlibs/curl)

set(LIBCURLCXX_INC_DIRS
  extlibs/curl/include
  include
  include/base
  include/ext
)

set(LIBCURLCXX_SRC_FILES
  src/base/curlcxx_cdtor.cpp
  src/base/curlcxx_easy.cpp
  src/base/curlcxx_error.cpp
  src/base/curlcxx_mime.cpp
  src/base/curlcxx_multi.cpp
  src/base/curlcxx_slist.cpp
  src/base/curlcxx_stream.cpp
  src/base/curlcxx_utility.cpp
  src/ext/curlcxx_http_req.cpp
  src/ext/curlcxx_websocket.cpp
)


add_library(${PROJECT_NAME} STATIC ${LIBCURLCXX_SRC_FILES})
set_property(TARGET ${PROJECT_NAME} PROPERTY PUBLIC_HEADER ${LIBCURLCXX_INC_DIRS})

target_include_directories(${PROJECT_NAME} PUBLIC ${LIBCURLCXX_INC_DIRS})

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)
target_compile_options(${PROJECT_NAME} PUBLIC -Wall)

target_link_libraries(${PROJECT_NAME} libcurl)

# add samples directory.
if(BUILD_SAMPLE)
  add_subdirectory(sample)
endif()

