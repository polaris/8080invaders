add_executable(test main.cpp)
target_include_directories(test SYSTEM PRIVATE ../emulator ${SDL2_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS})
target_link_libraries(test PRIVATE emu ${SDL2_LIBRARIES})
set_target_properties(test
        PROPERTIES
        CXX_STANDARD 17
        CXX_EXTENSIONS OFF
        CXX_STANDARD_REQUIRED ON)
