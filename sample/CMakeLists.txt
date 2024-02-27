cmake_minimum_required(VERSION 3.22)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_FLAGS_DEBUG "-g3 -Og")
set(CMAKE_C_FLAGS_RELEASE "-g -O2")

add_compile_definitions(PICOJSON_USE_INT64)

project(test)


set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(get_sample get_sample.cpp)
add_executable(multi_sample multi_sample.cpp)
add_executable(http_get_sample http_get_sample.cpp)
add_executable(http_download_sample http_download_sample.cpp)
add_executable(http_multi_sample http_multi_sample.cpp)
add_executable(http_post_sample http_post_sample.cpp)
add_executable(mastodon_public_read mastodon_public_read.cpp)
add_executable(misskey_public_read misskey_public_read.cpp)
add_executable(chatgpt_api_sample chatgpt_api_sample.cpp)
add_executable(bluesky_timeline_read bluesky_timeline_read.cpp)


target_link_libraries(get_sample curlcxx)
target_link_libraries(multi_sample curlcxx)
target_link_libraries(http_get_sample curlcxx)
target_link_libraries(http_download_sample curlcxx)
target_link_libraries(http_multi_sample curlcxx)
target_link_libraries(http_post_sample curlcxx)
target_link_libraries(mastodon_public_read curlcxx)
target_link_libraries(misskey_public_read curlcxx)
target_link_libraries(chatgpt_api_sample curlcxx)
target_link_libraries(bluesky_timeline_read curlcxx)
