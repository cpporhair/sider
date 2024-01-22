project(pump.example)
add_executable(pump.example pump.cc)
set_target_properties(pump.example PROPERTIES LINKER_LANGUAGE CXX)
link_directories("/usr/lib")
target_link_libraries (pump.example
        -ltbb
        -pg
        -Wl,--whole-archive  ${SPDK1_LIBRARIES} -Wl,--no-whole-archive
        ${SPDK4_LIBRARIES} ${SPDK2_LIBRARIES}
        boost_program_options pthread isal ssl rt uuid crypto m aio numa dl  )
