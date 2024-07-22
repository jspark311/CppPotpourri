#
# ESP-IDF component
#
message(STATUS "==============================================")
message(STATUS "CppPotpourri")
message(STATUS "==============================================")

set(includes "src")

set(src_dirs)
list(APPEND src_dirs "src")
list(APPEND src_dirs "src/cbor-cpp")
list(APPEND src_dirs "src/BusQueue")
list(APPEND src_dirs "src/C3PValue")
list(APPEND src_dirs "src/Console")
list(APPEND src_dirs "src/TimerTools")
list(APPEND src_dirs "src/TimeSeries")
list(APPEND src_dirs "src/Storage")
list(APPEND src_dirs "src/Storage/RecordTypes")
list(APPEND src_dirs "src/Image")
list(APPEND src_dirs "src/Image/ImageUtils")
list(APPEND src_dirs "src/Identity")
list(APPEND src_dirs "src/M2MLink")
list(APPEND src_dirs "src/Pipes/TripleAxisPipe")
list(APPEND src_dirs "src/Pipes/BufferAccepter")
list(APPEND src_dirs "src/Pipes/BufferAccepter/GPSWrapper")
list(APPEND src_dirs "src/Pipes/BufferAccepter/Base64")
list(APPEND src_dirs "src/Pipes/BufferAccepter/LineCoDec")
list(APPEND src_dirs "src/Pipes/ScalarPipe")


idf_component_register(
  SRC_DIRS "${src_dirs}"
  INCLUDE_DIRS "${includes}"
  PRIV_INCLUDE_DIRS "src"
  REQUIRES ManuvrPlatform
)

target_compile_options(${COMPONENT_LIB} PUBLIC -Wno-ignored-qualifiers -Wno-implicit-fallthrough)
