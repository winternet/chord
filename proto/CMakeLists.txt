
#
# compile the proto idl
#
set(ProtoGenerateTargetPath ${CMAKE_BINARY_DIR}/gen)

include(grpc)
file(GLOB ProtoFiles "*.proto")
string(REPLACE ";" "\n--   " PROTO_FILES_OUT "${ProtoFiles}")
message(STATUS "Found proto files: \n--   ${PROTO_FILES_OUT}")

protobuf_generate_grpc_cpp(ProtoSources ProtoHeaders ${ProtoFiles})

file(GLOB SOURCES_GEN ${ProtoGenerateTargetPath}/*)
string(REPLACE ";" "\n--   " SOURCES_GEN_OUT "${SOURCES_GEN}")
message(STATUS "Found generated source files:\n--   ${SOURCES_GEN_OUT}")

#
# add library containing the compiled sources
#
add_library(${PROJECT_NAME}_grpc ${ProtoSources} ${ProtoHeaders})
target_include_directories(${PROJECT_NAME}_grpc PUBLIC "${ProtoGenerateTargetPath}")
target_link_libraries(${PROJECT_NAME}_grpc gRPC::grpc++)
