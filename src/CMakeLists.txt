add_library(suprai_domain INTERFACE)
target_include_directories(suprai_domain INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(suprai_domain INTERFACE Qt6::Core)

add_library(suprai_providers STATIC
    providers/Provider.h
    providers/SseDecoder.h
    providers/SseDecoder.cpp
    providers/OpenAIChatProvider.h
    providers/OpenAIChatProvider.cpp
)
target_include_directories(suprai_providers PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(suprai_providers
    PUBLIC
        Qt6::Core
        Qt6::Network
        suprai_domain
)

add_library(suprai_runtime STATIC
    runtime/RuntimeState.h
    runtime/RuntimeConfig.h
    runtime/AgentRuntime.h
    runtime/NativeSuprAIRuntime.h
    runtime/NativeSuprAIRuntime.cpp
    runtime/MockRuntime.h
    runtime/MockRuntime.cpp
    runtime/RuntimeFactory.h
    runtime/RuntimeFactory.cpp
)
target_include_directories(suprai_runtime PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(suprai_runtime
    PUBLIC
        Qt6::Core
        suprai_domain
        suprai_providers
)

add_library(suprai_ui STATIC
    ui/TranscriptModel.h
    ui/TranscriptModel.cpp
    ui/ChatController.h
    ui/ChatController.cpp
)
target_include_directories(suprai_ui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(suprai_ui
    PUBLIC
        Qt6::Core
        suprai_domain
        suprai_runtime
)

qt_add_executable(suprai
    app/main.cpp
    app/AppSettings.h
    app/AppSettings.cpp
)

qt_add_qml_module(suprai
    URI SuprAI
    VERSION 1.0
    QML_FILES
        qml/Main.qml
)

target_link_libraries(suprai
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Qml
        Qt6::Quick
        Qt6::QuickControls2
        suprai_ui
        suprai_runtime
)

install(TARGETS suprai
    RUNTIME DESTINATION bin
)
