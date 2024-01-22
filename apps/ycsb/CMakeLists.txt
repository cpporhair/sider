
project(ycsb)

add_executable(ycsb ycsb.cc)
set_target_properties(ycsb PROPERTIES LINKER_LANGUAGE CXX)
add_compile_options(-ftemplate-backtrace-limit=0)
link_directories("/usr/lib")
target_link_libraries (ycsb
        -ltbb
        -pg
        -Wl,--whole-archive  ${SPDK1_LIBRARIES} -Wl,--no-whole-archive
        ${SPDK4_LIBRARIES} ${SPDK2_LIBRARIES}
        jemalloc
        boost_program_options pthread isal ssl rt uuid crypto m aio numa dl  )