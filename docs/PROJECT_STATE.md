# SuprAI Project State

```yaml
milestone: M0
status: complete
last_verified_commit: 9c0bd0b4a7e10b60af957a970969537a24683377

working:
  - repository exists and is writable
  - README establishes Linux-native Qt/QML direction
  - AGENTS.md establishes AI continuation contract
  - architecture v0 defines UI/core/native-agent/platform boundaries
  - proof-driven roadmap exists
  - NativeSuprAIRuntime is the canonical production agent runtime
  - MockRuntime is test-only
  - Hermes/OpenClaw are research references only
  - AppImage selected as first portable artifact
  - native Qt stack decision recorded in ADR-0001
  - native SuprAI runtime decision recorded in ADR-0002
  - upstream Hermes/OpenClaw/Qt/AppImage reference notes recorded

broken: []

decisions:
  - Linux first
  - Qt Quick/QML presentation
  - C++20 application core and agent runtime
  - CMake/Ninja
  - no Electron or Node runtime in shipped core app
  - NativeSuprAIRuntime is built from the start
  - no planned Hermes/OpenClaw runtime dependency or adapter
  - provider abstraction targets model inference only
  - first provider is OpenAI-compatible HTTP
  - local endpoints such as llama.cpp/vLLM/NInfer-compatible APIs are first-class targets
  - AgentLoop, tools, approvals, MCP, persistence and context management are SuprAI-owned
  - Wayland first, X11 compatibility where practical
  - Qt WebEngine is optional and requires explicit architectural decision
  - SQLite for SuprAI-owned durable state
  - secure desktop storage for secrets
  - initial process model is one modular native application, not a premature client/gateway split

open_questions:
  - exact visual language and component system
  - exact OpenAI-compatible transport API shape (Chat Completions vs Responses support strategy)
  - exact secure-secret implementation
  - exact AppImage tooling after proof build
  - minimum supported distro / ABI floor
  - first built-in tool set and default approval policy
  - initial MCP transport scope
  - memory design after core agent loop is proven

next_milestone: M1
next_exact_steps:
  - create CMake/Qt source skeleton
  - create C++ application bootstrap
  - create QML shell with left navigation, chat area and inspector
  - add XDG path helper and structured logging
  - add basic tests and CI build
  - verify Wayland and X11 launch paths
  - then implement AgentRuntime + MockRuntime
  - then implement NativeSuprAIRuntime and OpenAI-compatible provider

verification_commands:
  - none_yet
```

Update this file at every milestone handoff.
