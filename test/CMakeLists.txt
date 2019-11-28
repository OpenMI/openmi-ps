cmake_minimum_required(VERSION 2.4) 

set(PREFIX_LINKER_FLAGS "-static -Wl,--whole-archive ${DEPS_WHOLE_ARCHIVE_LIBRARIES} -Wl,--no-whole-archive -Wl,--eh-frame-hdr")

if (CMAKE_SYSTEM_NAME MATCHES "Linux")
  set(summary_linker_flags "${PREFIX_LINKER_FLAGS} ${STATIC_LINKER_FLAGS} ${SHARED_LINKER_FLAGS} ${DYNAMIC_LINKER}")
else()
  set(summary_linker_flags "${DEPS_WHOLE_ARCHIVE_LIBRARIES} ${DEPS_STATIC_LIBRARIES} -lopenmi_ps ${DEPS_SHARED_LIBRARIES}")
endif()

function (add_program arg)
  list(LENGTH ARGV  argv_len)
  if (NOT (${argv_len} STREQUAL "1")) 
    message(FATAL_ERROR "argment number should be equals to 1")
  endif()

  set(prog_name ${ARGV0})
  message(STATUS "exec_name[${prog_name}], src_name[${prog_name}.cc], link_libs[${link_libs}]")
  add_executable(${prog_name} "${prog_name}.cc")
  target_link_libraries(${prog_name} ${summary_linker_flags})
endfunction()

add_program("client")
add_program("core_engine_model_parser")
add_program("kv_storer_test")
add_program("optimizer_test")
add_program("ps_client_test")
add_program("server")
