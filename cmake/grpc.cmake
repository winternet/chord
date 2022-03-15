find_package(Protobuf REQUIRED)
find_package(gRPC REQUIRED)

message(STATUS "the plugin program: ${GRPC_CPP_PLUGIN_PROGRAM}")
# Get plugin directory
get_filename_component(GRPC_PLUGIN_DIRECTORY ${GRPC_CPP_PLUGIN_PROGRAM} DIRECTORY)

set(_gRPC_PROTO_GENS_DIR ${ProtoGenerateTargetPath})
file(MAKE_DIRECTORY ${_gRPC_PROTO_GENS_DIR})


# protobuf_generate_grpc_cpp is adapted from grpc repos CMakeLists.txt
# protobuf_generate_grpc_cpp
#  --------------------------
#
#   Add custom commands to process ``.proto`` files to C++ using protoc and
#   GRPC plugin::
#
#     protobuf_generate_grpc_cpp [<ARGN>...]
#
#   ``ARGN``
#     ``.proto`` files
#
function(protobuf_generate_grpc_cpp SRCS HDRS)
    if(NOT ARGN)
    message(SEND_ERROR "Error: PROTOBUF_GENERATE_GRPC_CPP() called without any proto files")
    return()
    endif()

    # Clear outputs
    set(${SRCS})
    set(${HDRS})

    # Include directories from protobuf lib and current dir
    set(_gRPC_PROTOBUF_WELLKNOWN_INCLUDE_DIR "${protobuf_INCLUDE_DIR}")
    set(_protobuf_include_path -I . -I ${_gRPC_PROTOBUF_WELLKNOWN_INCLUDE_DIR})

    foreach(FIL ${ARGN})
    get_filename_component(ABS_FIL ${FIL} ABSOLUTE)
    get_filename_component(FIL_WE ${FIL} NAME_WE)
    file(RELATIVE_PATH REL_FIL ${CMAKE_CURRENT_SOURCE_DIR} ${ABS_FIL})
    get_filename_component(REL_DIR ${REL_FIL} DIRECTORY)
    set(RELFIL_WE "${REL_DIR}/${FIL_WE}")

    #if cross-compiling, find host plugin
    if(CMAKE_CROSSCOMPILING)
        find_program(_gRPC_CPP_PLUGIN grpc_cpp_plugin)
    else()
        set(_gRPC_CPP_PLUGIN ${GRPC_CPP_PLUGIN_PROGRAM})
    endif()

    SET(_GRPC_HEADERS 
        "${_gRPC_PROTO_GENS_DIR}/${RELFIL_WE}.grpc.pb.h" 
        "${_gRPC_PROTO_GENS_DIR}/${RELFIL_WE}_mock.grpc.pb.h" 
        "${_gRPC_PROTO_GENS_DIR}/${RELFIL_WE}.pb.h")
    SET(_GRPC_SRCS
        "${_gRPC_PROTO_GENS_DIR}/${RELFIL_WE}.grpc.pb.cc"
        "${_gRPC_PROTO_GENS_DIR}/${RELFIL_WE}.pb.cc")
    list(APPEND ${SRCS} ${_GRPC_SRCS})
    list(APPEND ${HDRS} ${_GRPC_HEADERS})

    add_custom_command(
        OUTPUT ${_GRPC_SRCS} ${_GRPC_HEADERS} 
        COMMAND ${Protobuf_PROTOC_EXECUTABLE}
        ARGS --grpc_out=generate_mock_code=true:${_gRPC_PROTO_GENS_DIR}
            --cpp_out=${_gRPC_PROTO_GENS_DIR}
            --plugin=protoc-gen-grpc=${_gRPC_CPP_PLUGIN}
            ${_protobuf_include_path}
            ${REL_FIL}
        DEPENDS ${ABS_FIL} ${Protobuf_PROTOC_EXECUTABLE} ${_gRPC_CPP_PLUGIN}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Running gRPC C++ protocol buffer compiler on ${FIL}"
        VERBATIM)
    endforeach()

    # Set outputs
    set(${SRCS} ${${SRCS}} PARENT_SCOPE)
    set(${HDRS} ${${HDRS}} PARENT_SCOPE)

endfunction()
