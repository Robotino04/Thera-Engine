cmake_minimum_required(VERSION 3.0)

file(GLOB_RECURSE THERA_SRC "*.cpp" "*.hpp" "*.tpp")



set(VERSION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/include/Thera/Utils/GitInfo.hpp")
set(VERSION_FILE_TMP "${CMAKE_CURRENT_BINARY_DIR}/GitInfo.hpp.tmp")
set(VERSION_FILE_TEMPLATE "${CMAKE_SOURCE_DIR}/tools/gitHashExtract_template.ipp")
set(VERSION_PYTHON "${CMAKE_SOURCE_DIR}/tools/gitHashExtract.py")
list(APPEND THERA_SRC ${VERSION_FILE})

add_custom_command(
    OUTPUT always_rebuild this_file_should_never_exist_and_ensures_rebuilding
    COMMAND cmake -E touch always_rebuild
)

add_custom_command(OUTPUT ${VERSION_FILE}
    COMMAND python ${VERSION_PYTHON} ${VERSION_FILE_TEMPLATE} ${VERSION_FILE_TMP}
    # only change the output file if the contents are actually different (avoids rebuilding if not needed)
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${VERSION_FILE_TMP} ${VERSION_FILE}
    COMMAND ${CMAKE_COMMAND} -E remove ${VERSION_FILE_TMP}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Creating ${VERSION_FILE}"
    DEPENDS always_rebuild
)

add_library(Thera STATIC ${THERA_SRC})

target_include_directories(Thera PUBLIC "include/")
target_compile_features(Thera PUBLIC cxx_std_20)

target_link_libraries(Thera PUBLIC ANSI)