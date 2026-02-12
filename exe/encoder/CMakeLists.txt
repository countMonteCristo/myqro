project(encoder)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror -pedantic")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

include_directories(${CMAKE_HOME_DIRECTORY}/myqro/include/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
file(GLOB_RECURSE SOURCES LIST_DIRECTORIES true ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
set(SOURCES ${SOURCES})

add_executable(encoder
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
    ${SOURCES}
)

target_link_libraries(encoder
    myqro_static
)
