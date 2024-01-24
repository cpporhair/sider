project(sider)

find_package(protobuf CONFIG REQUIRED)

add_executable(sider ./proto/command.pb.cc sider.cc)
set_target_properties(sider PROPERTIES LINKER_LANGUAGE CXX)
message("a" ${PROTOBUF_LIBRARIES})
link_directories("/usr/lib")
target_link_libraries (sider
        -ltbb
        -pg
        protobuf::libprotobuf
        -Wl,--whole-archive  ${SPDK1_LIBRARIES} -Wl,--no-whole-archive
        ${SPDK4_LIBRARIES} ${SPDK2_LIBRARIES}
        boost_program_options pthread isal ssl rt uuid crypto m aio numa dl  )