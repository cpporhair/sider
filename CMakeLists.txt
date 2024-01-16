cmake_minimum_required(VERSION 3.23)

set(CMAKE_CXX_STANDARD 23)
set(DCMAKE_GENERATOR_PLATFORM 64)
add_compile_options(-O3)
project(sider)
find_package(Boost)
add_compile_options(-fcontracts)
set(ENV{PKG_CONFIG_PATH} /home/null/work/kv/spdk/build/lib/pkgconfig)
find_package(PkgConfig)
pkg_search_module(SPDK1 REQUIRED spdk_nvme )
pkg_search_module(SPDK2 REQUIRED spdk_env_dpdk)
pkg_search_module(SPDK3 REQUIRED spdk_syslibs)
pkg_search_module(SPDK4 REQUIRED spdk_vmd )

include_directories("/home/null/work/range-v3-0.12.0/include")

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

add_subdirectory(sider)
add_subdirectory(tests/example)
