file(GLOB_RECURSE SRC_CXX_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
file(GLOB_RECURSE CUDA_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cu)

set(INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
                 ${UTIL_DIR})
set(LINK_LIBS utils_lib_cxx utils_lib_cuda)

find_package(CUDA)

if(CUDA_FOUND)
    message("CUDA FOUND")
    message(${CUDA_SOURCES})
    set(CUDA_LINKED ${PROJECT_NAME}_cuda_linked)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    set(CUDA_SEPARABLE_COMPILATION TRUE)
    set(CMAKE_CUDA_FLAGS "-gencode=arch=compute_80,code=sm_80")
    set(CMAKE_CUDA_ARCHITECTURES 80)
    list(APPEND CUDA_NVCC_FLAGS -O3 -std=c++17 --extended-lambda ${CMAKE_CUDA_FLAGS})
    list(APPEND CUDA_NVCC_FLAGS --compiler-options -fPIC)
    cuda_add_library(${CUDA_LINKED} SHARED ${CUDA_SOURCES})
    target_include_directories(${CUDA_LINKED} PUBLIC ${INCLUDE_DIRS})
    set_target_properties(${CUDA_LINKED} PROPERTIES CUDA_SEPARABLE_COMPILATION ON)
endif(CUDA_FOUND)

add_executable(${PROJECT_NAME} ${SRC_CXX_FILES})

set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "-Wl,--whole-archive -rdynamic -lpthread -Wl,--no-whole-archive")

target_include_directories(${PROJECT_NAME} PUBLIC ${INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} ${CUDA_LINKED} ${LINK_LIBS})