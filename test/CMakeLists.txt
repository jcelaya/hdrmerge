include_directories(${Boost_INCLUDE_DIRS})

set(test_sources testMain.cpp testImageStack.cpp testBitmap.cpp testHistogram.cpp testBoxBlur.cpp)

add_executable(hdrmerge-test ${test_sources} $<TARGET_OBJECTS:hdrmerge-objects>)
target_link_libraries(hdrmerge-test ${hdrmerge_libs} ${Boost_LIBRARIES})
