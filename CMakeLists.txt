#
# ESP-IDF component
#
message(STATUS "==============================================")
message(STATUS "CppPotpourri")
message(STATUS "==============================================")

# idf_component_set_property(CppPotpourri COMPONENT_NAME "CppPotpourri")
# idf_component_set_property(CppPotpourri COMPONENT_TYPE LIBRARY)

idf_component_register(
  SRC_DIRS "src" "src/cbor-cpp" "src/Image" "src/Identity"
  INCLUDE_DIRS "src"
)
