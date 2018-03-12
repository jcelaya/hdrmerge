include_directories(${Boost_INCLUDE_DIRS})

set(
    test_sources
    testMain.cpp
    testImageStack.cpp
    testBitmap.cpp
    testHistogram.cpp
    testBoxBlur.cpp
    testArray2D.cpp
    testDngFloatWriter.cpp
)


if(APPLE)
    add_executable(hdrmerge-test ${test_sources} $<TARGET_OBJECTS:alglib-objects> $<TARGET_OBJECTS:hdrmerge-objects> $<TARGET_OBJECTS:hdrmerge-gui-objects>)
else()
    add_executable(hdrmerge-test ${test_sources} $<TARGET_OBJECTS:hdrmerge-objects> $<TARGET_OBJECTS:hdrmerge-gui-objects>)
endif()

target_link_libraries(hdrmerge-test ${hdrmerge_libs} ${Boost_LIBRARIES})
