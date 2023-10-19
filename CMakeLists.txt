#
# ESP-IDF component
#
message(STATUS "==============================================")
message(STATUS "CppPotpourri")
message(STATUS "==============================================")

idf_component_register(
  SRC_DIRS "src" "src/cbor-cpp" "src/BusQueue" "src/C3PValue" "src/Storage" "src/Image" "src/Image/ImageUtils" "src/Identity" "src/M2MLink" "src/TripleAxisPipe"
  INCLUDE_DIRS "src"
  REQUIRES ManuvrPlatform
)

#component_compile_options(-std=c++17)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-ignored-qualifiers -Wno-implicit-fallthrough)
