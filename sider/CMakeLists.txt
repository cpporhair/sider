add_executable(sider sider.cc)

link_directories("/usr/lib")
target_link_libraries (sider
        -ltbb
        -pg
        -Wl,--whole-archive  ${SPDK1_LIBRARIES} -Wl,--no-whole-archive
        ${SPDK4_LIBRARIES} ${SPDK2_LIBRARIES}
        boost_program_options pthread isal ssl rt uuid crypto m aio numa dl  )