cmake_minimum_required(VERSION 3.20)
project(recs)

add_subdirectory(ext)

add_library(recs INTERFACE)

target_include_directories(recs
    INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/include
)

add_subdirectory(tests)

add_executable(test ${TESTS_SOURCES})
target_compile_features(test
    PUBLIC
    cxx_std_20
)

if(MSVC)
    # From cppbestpractices
    target_compile_options(test
        PRIVATE
        /permissive-
        /W4
        /w14242
        /w14254
        /w14263
        /w14265
        /w14287
        /we4289
        /w14296
        /w14311
        /w14545
        /w14546
        /w14547
        /w14549
        /w14555
        /w14619
        /w14640
        /w14826
        /w14905
        /w14906
        /w14928
    )
    target_compile_definitions(test PRIVATE "_CRTDBG_MAP_ALLOC")
else()
    target_compile_options(test
        PRIVATE
        -pedantic
        -Wall
        -Wextra
        -Wunused
        -Wno-missing-field-initializers
        -Wno-self-assign-overloaded
    )
endif() # NOT MSVC

target_link_libraries(test PRIVATE recs Catch2WithMain)
