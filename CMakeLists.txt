#
# ESP-IDF component
#
message(STATUS "==============================================")
message(STATUS "CppPotpourri")
message(STATUS "==============================================")

idf_component_register(
  SRC_DIRS
    "src"
    "src/cbor-cpp"
    "src/BusQueue"
    "src/C3PValue"
    "src/Console"
    "src/TimerTools"
    "src/Storage"
    "src/Storage/RecordTypes"
    "src/Image"
    "src/Image/ImageUtils"
    "src/Identity"
    "src/M2MLink"
    "src/Pipes/TripleAxisPipe"
    "src/Pipes/BufferAccepter"
    "src/Pipes/BufferAccepter/GPSWrapper"
    "src/Pipes/BufferAccepter/Base64"
    "src/Pipes/BufferAccepter/LineCoDec"
    "src/Pipes/ScalarPipe"
  INCLUDE_DIRS "src"
  REQUIRES ManuvrPlatform
)

#component_compile_options(-std=c++17)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-ignored-qualifiers -Wno-implicit-fallthrough)
