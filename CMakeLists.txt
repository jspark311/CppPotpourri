#
# ESP-IDF component
#
message(STATUS "==============================================")
message(STATUS "CppPotpourri")
message(STATUS "==============================================")

idf_component_register(
  SRC_DIRS "src" "src/cbor-cpp" "src/Image" "src/Identity" "src/ManuvrLink" "src/TripleAxisPipe"
  INCLUDE_DIRS "src"
  REQUIRES ManuvrPlatform
)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-ignored-qualifiers -Wno-implicit-fallthrough)
