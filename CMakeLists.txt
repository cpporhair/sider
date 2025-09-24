cmake_minimum_required(VERSION 3.27)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CUDA_STANDARD 23)
set(DCMAKE_GENERATOR_PLATFORM 64)

add_compile_options(-O0 )

find_package(Boost)

set(ENV{PKG_CONFIG_PATH} /home/null/work/kv/spdk/build/lib/pkgconfig)
find_package(PkgConfig)
pkg_search_module(SPDK1 REQUIRED spdk_nvme )
pkg_search_module(SPDK2 REQUIRED spdk_env_dpdk)
pkg_search_module(SPDK3 REQUIRED spdk_syslibs)
pkg_search_module(SPDK4 REQUIRED spdk_vmd )

include_directories("/opt/cuda/include")
include_directories("/home/null/work/kv/spdk/build/include")
include_directories(./)
include_directories(sider/meta)
include_directories(sider)

link_directories("/usr/local/lib/boost")
link_directories("/home/null/work/kv/spdk/build/lib")
link_directories("/home/null/work/kv/spdk/dpdk/build/lib")

set(ENV{PKG_CONFIG_PATH} /home/null/work/kv/spdk/build/lib/pkgconfig)

find_package(PkgConfig)

pkg_search_module(SPDK1 REQUIRED spdk_nvme )
pkg_search_module(SPDK2 REQUIRED spdk_env_dpdk)
pkg_search_module(SPDK3 REQUIRED spdk_syslibs)
pkg_search_module(SPDK4 REQUIRED spdk_vmd )

message(" " ${SPDK5_LIBRARIES})

add_subdirectory(apps/kv)
add_subdirectory(apps/pump)
add_subdirectory(apps/cuda)
add_subdirectory(apps/ycsb)
