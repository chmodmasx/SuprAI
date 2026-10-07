add_executable(tst_sse_decoder
    tst_sse_decoder.cpp
)
target_include_directories(tst_sse_decoder
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/modules/providers/openai/src
)
target_link_libraries(tst_sse_decoder
    PRIVATE
        Qt6::Core
        Qt6::Test
        suprai_provider_openai
)
add_test(NAME sse_decoder COMMAND tst_sse_decoder)

add_executable(tst_transcript_model
    tst_transcript_model.cpp
)
target_include_directories(tst_transcript_model
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/modules/ui/src
)
target_link_libraries(tst_transcript_model
    PRIVATE
        Qt6::Core
        Qt6::Test
        suprai_ui
        suprai_domain
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

add_executable(tst_native_runtime
    tst_native_runtime.cpp
)
target_link_libraries(tst_native_runtime
    PRIVATE
        Qt6::Core
        Qt6::Network
        Qt6::Test
        suprai_runtime
        suprai_provider_openai
)
add_test(NAME native_runtime COMMAND tst_native_runtime)


add_executable(tst_runtime_layering
    tst_runtime_layering.cpp
)
target_include_directories(tst_runtime_layering
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/modules/runtime/src
)
target_link_libraries(tst_runtime_layering
    PRIVATE
        Qt6::Core
        Qt6::Test
        suprai_runtime
        suprai_provider_api
)
add_test(NAME runtime_layering COMMAND tst_runtime_layering)

set_tests_properties(
    sse_decoder
    transcript_model
    mock_runtime
    native_runtime
    runtime_layering
    PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)
