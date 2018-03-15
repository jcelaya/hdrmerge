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

#add_executable(hdrmerge-test ${test_sources} $<TARGET_OBJECTS:hdrmerge-objects> $<TARGET_OBJECTS:hdrmerge-gui-objects>)
add_executable(hdrmerge-test
    ${test_sources}
)

target_link_libraries(hdrmerge-test ${hdrmerge_libs} ${Boost_LIBRARIES} Qt5::Widgets)
