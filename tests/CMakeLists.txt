add_executable(tst_sse_decoder
    tst_sse_decoder.cpp
)
target_link_libraries(tst_sse_decoder
    PRIVATE
        Qt6::Core
        Qt6::Test
        suprai_providers
)
add_test(NAME sse_decoder COMMAND tst_sse_decoder)

add_executable(tst_transcript_model
    tst_transcript_model.cpp
)
target_link_libraries(tst_transcript_model
    PRIVATE
        Qt6::Core
        Qt6::Test
        suprai_ui
)
add_test(NAME transcript_model COMMAND tst_transcript_model)

add_executable(tst_mock_runtime
    tst_mock_runtime.cpp
)
target_link_libraries(tst_mock_runtime
    PRIVATE
        Qt6::Core
        Qt6::Test
        suprai_runtime
)
add_test(NAME mock_runtime COMMAND tst_mock_runtime)

set_tests_properties(
    sse_decoder
    transcript_model
    mock_runtime
    PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)
