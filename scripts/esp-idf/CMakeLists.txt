#
# ESP-IDF component
#

idf_build_get_property(target IDF_TARGET)


message(STATUS "==============================================")
message(STATUS "CppPotpourri")
message(STATUS "==============================================")

file(GLOB_RECURSE srcs
  src/*.c
  src/*.cpp
)

idf_component_register(
  SRCS src/CppPotpourri.cpp
  INCLUDE_DIRS src
  PRIV_INCLUDE_DIRS .
)
