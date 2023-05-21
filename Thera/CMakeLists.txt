cmake_minimum_required(VERSION 3.0)

file(GLOB_RECURSE THERA_SRC "*.cpp" "*.hpp" "*.tpp")



set(VERSION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/include/Thera/Utils/GitInfo.hpp")
set(VERSION_FILE_TEMPLATE "${CMAKE_SOURCE_DIR}/tools/gitHashExtract_template.ipp")
set(VERSION_PYTHON "${CMAKE_SOURCE_DIR}/tools/gitHashExtract.py")
list(APPEND THERA_SRC ${VERSION_FILE})

add_custom_command(OUTPUT ${VERSION_FILE}
    COMMAND python ${VERSION_PYTHON} ${VERSION_FILE_TEMPLATE} ${VERSION_FILE}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Creating ${VERSION_FILE}"
)

add_custom_target(GitHashExtractor ALL DEPENDS ${VERSION_FILE} ${VERSION_FILE_TEMPLATE} ${VERSION_PYTHON})


add_library(Thera STATIC ${THERA_SRC})

target_include_directories(Thera PUBLIC "include/")
target_compile_features(Thera PUBLIC cxx_std_20)

target_link_libraries(Thera PUBLIC ANSI)