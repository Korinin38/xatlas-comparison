cmake_minimum_required(VERSION 3.25)
if (GPU_CUDA_SUPPORT)
    project(xatlas_comparison LANGUAGES CUDA C CXX)
#    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer -g -O3")
#    set(CMAKE_CUDA_FLAGS ${CMAKE_CUDA_FLAGS} "-g -G")
else()
    project(xatlas_comparison LANGUAGES C CXX)
endif()

set(CMAKE_CXX_STANDARD 17)

#option(GPU_CUDA_SUPPORT "CUDA support." ON)

find_package(OpenMP)
if (OpenMP_CXX_FOUND)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
else()
    message(FATAL_ERROR "OpenMP not found! You need OpenMP for speedup on multicore CPUs!")
endif()

add_subdirectory(libs/gpu)
add_subdirectory(libs/clew)
add_subdirectory(libs/utils)

if (GPU_CUDA_SUPPORT)
    find_package(CUDA REQUIRED)
    add_definitions(-DCUDA_SUPPORT)
    set(CUDA_NVCC_FLAGS ${CUDA_NVCC_FLAGS} -lineinfo)
    set(GPU_SOURCES libs/xatlas/cl/blit_cl.h libs/xatlas/cu/blit.cu)
else()
    message(NOTICE "CUDA disabled.")
    set(GPU_SOURCES ${GPU_SOURCES} libs/xatlas/cl/blit.cl libs/xatlas/cl/blit_cl.h)
    convertIntoHeader(libs/xatlas/cl/blit.cl libs/xatlas/cl/blit_cl.h blit_kernel)
endif()
#convertIntoHeader(libs/xatlas/cl/blit.cl libs/xatlas/cl/blit_cl.h blit_kernel)

add_library(xatlas
        libs/xatlas/xatlas.h
        libs/xatlas/xatlas.cpp
        ${GPU_SOURCES})

add_library(xatlas_old
        libs/xatlas-old/xatlas.h
        libs/xatlas-old/xatlas.cpp)

find_package(X11 REQUIRED)
add_library(CImg
        libs/xatlas/CImg.h)
target_link_libraries(CImg X11)
set_target_properties(CImg PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(xatlas CImg)

target_link_libraries(xatlas libclew libgpu libutils)

add_library(stb_image
        libs/stb_image/stb_image_write.h
        libs/stb_image/stb_image_write.c)

add_library(tiny_obj_loader
        libs/tiny_obj_loader/tiny_obj_loader.h
        libs/tiny_obj_loader/tiny_obj_loader.cpp)

add_library(ply_shape
        libs/ply_shape/ply_shape.h
        libs/ply_shape/ply_shape.cpp)

include_directories(libs)
target_include_directories(xatlas PUBLIC libs/xatlas)
target_include_directories(stb_image PUBLIC libs/stb_image)
target_include_directories(tiny_obj_loader PUBLIC libs/tiny_obj_loader)

add_library(xatlas_example src/x_example.cpp)
add_library(xatlas_old_example src/x_old_example.cpp)

if (GPU_CUDA_SUPPORT)
    cuda_add_executable(xatlas_comparison src/main.cpp)
else()
    add_executable(xatlas_comparison src/main.cpp)
endif()

include_directories(/opt/include)

target_link_libraries(xatlas_example xatlas stb_image tiny_obj_loader)
target_link_libraries(xatlas_old_example xatlas_old stb_image tiny_obj_loader)
target_link_libraries(xatlas_comparison xatlas_example xatlas_old_example ply_shape)

convertIntoHeader(src/cl/blit.cl src/cl/blit_cl.h blit_kernel)
add_executable(gpu src/scratch_gpu.cpp src/cl/blit_cl.h)
target_link_libraries(gpu libclew libgpu libutils xatlas_example)
target_link_options(gpu PUBLIC "-Mcuda")