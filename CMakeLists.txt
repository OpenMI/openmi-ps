cmake_minimum_required(VERSION 2.4)

project(openmi_ps)

set(CMAKE_VERBOSE_MAKEFILE off)

set(BASIC_CXXFLAGS "-std=c++11 -g -O3 -Wall -fPIC")
set(BASIC_CPPDEFINES "-DHAVE_NETINET_IN_H") 
set(CMAKE_CXX_FLAGS "${BASIC_CXXFLAGS} ${BASIC_CPPDEFINES} -DEIGEN_USE_THREADS")

include_directories(
  $ENV{DEPS_ENV_HOME}/include  
  $ENV{DEPS_ENV_HOME}/include/eigen3
  ${PROJECT_SOURCE_DIR}/../openmi-base
  ${PROJECT_SOURCE_DIR}/../openmi-base/base
  ${PROJECT_SOURCE_DIR}/../openmi-idl
  ${PROJECT_SOURCE_DIR}/../openmi
  ${PROJECT_SOURCE_DIR}/../openmi/core/framework
  ${PROJECT_SOURCE_DIR}/../openmi/core/ops
  ${PROJECT_SOURCE_DIR}/../openmi/core/graph
  ${PROJECT_SOURCE_DIR}/../openmi/core/engine
  ${PROJECT_SOURCE_DIR}
)

link_directories(
  $ENV{DEPS_ENV_HOME}/lib 
  ${PROJECT_SOURCE_DIR}/../openmi-base/lib
  ${PROJECT_SOURCE_DIR}/../openmi-idl/lib
  ${PROJECT_SOURCE_DIR}/../openmi/lib
  ${PROJECT_SOURCE_DIR}/lib
  /usr/lib64
)
if (CMAKE_SYSTEM_NAME MATCHES "Linux")
  link_directories($ENV{DEPS_ENV_HOME}/lib64)
endif()

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib) 

set(STATIC_LINKER_FLAGS "-Wl,-Bstatic ${DEPS_WHOLE_ARCHIVE_LIBRARIES} ${DEPS_STATIC_LIBRARIES}")
set(SHARED_LINKER_FLAGS "-Wl,-Bdynamic ${DEPS_SHARED_LIBRARIES}")
set(DYNAMIC_LINKER "-Wl,--dynamic-linker=${DEPS_DYNAMIC_LINKER}")
set(summary_linker_flags "${STATIC_LINKER_FLAGS} ${SHARED_LINKER_FLAGS} ${DYNAMIC_LINKER}")

aux_source_directory(ps srcs_ps)
aux_source_directory(ps/internal srcs_ps_internal)
set(srcs ${srcs_ps})
list(APPEND srcs ${srcs_ps_internal})

set(libname openmi_ps)
#add_library(${libname} SHARED ${srcs})
#target_link_libraries(${libname} ${summary_linker_flags})

add_library(${libname}_static STATIC ${srcs})
target_link_libraries(${libname}_static ${link_libs})
set_target_properties(${libname}_static PROPERTIES OUTPUT_NAME ${libname})

add_subdirectory(test)
