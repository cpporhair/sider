project(example)
add_executable(example.pump pump.cc)

link_directories("/usr/lib")
target_link_libraries (example.pump
        -ltbb
        -pg
        -Wl,--whole-archive  ${SPDK1_LIBRARIES} -Wl,--no-whole-archive
        ${SPDK4_LIBRARIES} ${SPDK2_LIBRARIES}
        boost_program_options pthread isal ssl rt uuid crypto m aio numa dl  )

project(example.ycsb.a)
add_executable(example.ycsb.a ycsb_a.cc)
add_compile_options(-ftemplate-backtrace-limit=0)
link_directories("/usr/lib")
target_link_libraries (example.ycsb.a
        -lstdc++exp
        -ltbb
        -pg
        -Wl,--whole-archive  ${SPDK1_LIBRARIES} -Wl,--no-whole-archive
        ${SPDK4_LIBRARIES} ${SPDK2_LIBRARIES}
        jemalloc
        boost_program_options pthread isal ssl rt uuid crypto m aio numa dl  )


project(example.ycsb.c)
add_executable(example.ycsb.c ycsb_c.cc)
add_compile_options(-ftemplate-backtrace-limit=0)
link_directories("/usr/lib")
target_link_libraries (example.ycsb.c
        -ltbb
        -pg
        -Wl,--whole-archive  ${SPDK1_LIBRARIES} -Wl,--no-whole-archive
        ${SPDK4_LIBRARIES} ${SPDK2_LIBRARIES}
        jemalloc
        boost_program_options pthread isal ssl rt uuid crypto m aio numa dl  )

project(example.ycsb.e)
add_executable(example.ycsb.e ycsb_e.cc)
add_compile_options(-ftemplate-backtrace-limit=0)
link_directories("/usr/lib")
target_link_libraries (example.ycsb.e
        -ltbb
        -pg
        -Wl,--whole-archive  ${SPDK1_LIBRARIES} -Wl,--no-whole-archive
        ${SPDK4_LIBRARIES} ${SPDK2_LIBRARIES}
        jemalloc
        boost_program_options pthread isal ssl rt uuid crypto m aio numa dl  )

project(example.ycsb.l)
add_executable(example.ycsb.l ycsb_l.cc)
add_compile_options(-ftemplate-backtrace-limit=0)
link_directories("/usr/lib")
target_link_libraries (example.ycsb.l
        -ltbb
        -pg
        -Wl,--whole-archive  ${SPDK1_LIBRARIES} -Wl,--no-whole-archive
        ${SPDK4_LIBRARIES} ${SPDK2_LIBRARIES}
        jemalloc
        boost_program_options pthread isal ssl rt uuid crypto m aio numa dl  )

project(example.ycsb.z)
add_executable(example.ycsb.z ycsb_z.cc)
add_compile_options(-ftemplate-backtrace-limit=0)
link_directories("/usr/lib")
target_link_libraries (example.ycsb.z
        -ltbb
        -pg
        -Wl,--whole-archive  ${SPDK1_LIBRARIES} -Wl,--no-whole-archive
        ${SPDK4_LIBRARIES} ${SPDK2_LIBRARIES}
        jemalloc
        boost_program_options pthread isal ssl rt uuid crypto m aio numa dl  )