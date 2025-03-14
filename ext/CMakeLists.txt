# Pull submodules
execute_process(COMMAND git submodule update --init --depth 1)

add_subdirectory(catch2)

set(CELERO_COMPILE_DYNAMIC_LIBRARIES OFF "Set to ON to build Celero for dynamic linking.  Use OFF for static.")
add_subdirectory(celero)
