set(SUPRAI_MODULES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/modules)

# Domain -----------------------------------------------------------------------
add_library(suprai_domain INTERFACE)
target_include_directories(suprai_domain
    INTERFACE
        ${SUPRAI_MODULES_DIR}/domain/include
)
target_link_libraries(suprai_domain INTERFACE Qt6::Core)

# Provider port ----------------------------------------------------------------
add_library(suprai_provider_api STATIC
    modules/providers/api/include/suprai/providers/Provider.h
    modules/providers/api/src/Provider.cpp
)
target_include_directories(suprai_provider_api
    PUBLIC
        ${SUPRAI_MODULES_DIR}/providers/api/include
)
target_link_libraries(suprai_provider_api
    PUBLIC
        Qt6::Core
        suprai_domain
)

# OpenAI-compatible provider adapter -------------------------------------------
add_library(suprai_provider_openai STATIC
    modules/providers/openai/include/suprai/providers/OpenAIProviderFactory.h
    modules/providers/openai/src/OpenAIChatProvider.h
    modules/providers/openai/src/OpenAIChatProvider.cpp
    modules/providers/openai/src/SseDecoder.h
    modules/providers/openai/src/SseDecoder.cpp
)
target_include_directories(suprai_provider_openai
    PUBLIC
        ${SUPRAI_MODULES_DIR}/providers/openai/include
    PRIVATE
        ${SUPRAI_MODULES_DIR}/providers/openai/src
)
target_link_libraries(suprai_provider_openai
    PUBLIC
        suprai_provider_api
    PRIVATE
        Qt6::Network
)

# Platform ---------------------------------------------------------------------
add_library(suprai_platform STATIC
    modules/platform/include/suprai/platform/AppPaths.h
    modules/platform/include/suprai/platform/Logging.h
    modules/platform/src/AppPaths.cpp
    modules/platform/src/Logging.cpp
)
target_include_directories(suprai_platform
    PUBLIC
        ${SUPRAI_MODULES_DIR}/platform/include
)
target_link_libraries(suprai_platform
    PUBLIC
        Qt6::Core
)

# Persistence ------------------------------------------------------------------
add_library(suprai_persistence STATIC
    modules/persistence/include/suprai/persistence/PersistenceWorker.h
    modules/persistence/src/PersistenceWorker.cpp
)
target_include_directories(suprai_persistence
    PUBLIC
        ${SUPRAI_MODULES_DIR}/persistence/include
)
target_link_libraries(suprai_persistence
    PUBLIC
        Qt6::Core
)

# Runtime ----------------------------------------------------------------------
add_library(suprai_runtime STATIC
    modules/runtime/include/suprai/runtime/AgentRuntime.h
    modules/runtime/include/suprai/runtime/RuntimeConfig.h
    modules/runtime/include/suprai/runtime/RuntimeFactory.h
    modules/runtime/include/suprai/runtime/RuntimeState.h
    modules/runtime/src/AgentEngineEvent.h
    modules/runtime/src/AgentEngine.h
    modules/runtime/src/AgentEngine.cpp
    modules/runtime/src/RuntimeEvent.h
    modules/runtime/src/RuntimeEventAdapter.h
    modules/runtime/src/RuntimeEventAdapter.cpp
    modules/runtime/src/RuntimeOrchestrator.h
    modules/runtime/src/RuntimeOrchestrator.cpp
    modules/runtime/src/MockRuntime.h
    modules/runtime/src/MockRuntime.cpp
    modules/runtime/src/NativeSuprAIRuntime.h
    modules/runtime/src/NativeSuprAIRuntime.cpp
    modules/runtime/src/RuntimeFactory.cpp
)
target_include_directories(suprai_runtime
    PUBLIC
        ${SUPRAI_MODULES_DIR}/runtime/include
    PRIVATE
        ${SUPRAI_MODULES_DIR}/runtime/src
)
target_link_libraries(suprai_runtime
    PUBLIC
        Qt6::Core
        suprai_domain
        suprai_provider_api
)

# UI facade --------------------------------------------------------------------
add_library(suprai_ui STATIC
    modules/ui/include/suprai/ui/ChatController.h
    modules/ui/src/ChatController.cpp
    modules/ui/src/TranscriptModel.h
    modules/ui/src/TranscriptModel.cpp
)
target_include_directories(suprai_ui
    PUBLIC
        ${SUPRAI_MODULES_DIR}/ui/include
    PRIVATE
        ${SUPRAI_MODULES_DIR}/ui/src
)
target_link_libraries(suprai_ui
    PUBLIC
        Qt6::Core
        suprai_runtime
    PRIVATE
        suprai_domain
)

# Composition root / application -----------------------------------------------
qt_add_executable(suprai
    app/main.cpp
    app/AppConfig.h
    app/AppSettings.h
    app/AppSettings.cpp
    app/ApplicationBootstrap.h
    app/ApplicationBootstrap.cpp
)
target_include_directories(suprai
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/app
)

qt_add_qml_module(suprai
    URI SuprAI
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/SettingsPage.qml
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
        suprai_platform
        suprai_persistence
)

install(TARGETS suprai
    RUNTIME DESTINATION bin
)
