project(sider)
add_executable(sider sider.cc)
#set_target_properties(sider PROPERTIES
#        CUDA_SEPARABLE_COMPILATION ON
#        LINKER_LANGUAGE CXX)
set_target_properties(sider PROPERTIES LINKER_LANGUAGE CXX)

link_directories("/usr/lib")
target_link_libraries (sider
        -ltbb
        -pg
        -Wl,--whole-archive  ${SPDK1_LIBRARIES} -Wl,--no-whole-archive
        ${SPDK4_LIBRARIES} ${SPDK2_LIBRARIES}
        boost_program_options pthread isal ssl rt uuid crypto m aio numa dl  )