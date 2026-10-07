add_library(suprai_domain INTERFACE)
target_include_directories(suprai_domain INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(suprai_domain INTERFACE Qt6::Core)

# Provider contract: runtime depends only on this target. It is a tiny STATIC
# library (rather than INTERFACE) because Provider is a QObject and needs AUTOMOC.
add_library(suprai_provider_api STATIC
    providers/Provider.h
    providers/Provider.cpp
)
target_include_directories(suprai_provider_api PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(suprai_provider_api
    PUBLIC
        Qt6::Core
        suprai_domain
)

# Concrete OpenAI-compatible adapter. Only the composition root/tests link it.
add_library(suprai_provider_openai STATIC
    providers/SseDecoder.h
    providers/SseDecoder.cpp
    providers/OpenAIChatProvider.h
    providers/OpenAIChatProvider.cpp
)
target_include_directories(suprai_provider_openai PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(suprai_provider_openai
    PUBLIC
        Qt6::Core
        Qt6::Network
        suprai_provider_api
)

add_library(suprai_runtime STATIC
    runtime/RuntimeState.h
    runtime/RuntimeConfig.h
    runtime/AgentRuntime.h
    runtime/NativeSuprAIRuntime.h
    runtime/NativeSuprAIRuntime.cpp
    runtime/MockRuntime.h
    runtime/MockRuntime.cpp
)
target_include_directories(suprai_runtime PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(suprai_runtime
    PUBLIC
        Qt6::Core
        suprai_domain
        suprai_provider_api
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
    app/ApplicationBootstrap.h
    app/ApplicationBootstrap.cpp
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
        suprai_provider_openai
)

install(TARGETS suprai
    RUNTIME DESTINATION bin
)
