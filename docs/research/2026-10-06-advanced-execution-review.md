# Advanced Execution Architecture Review — 2026-10-06

Status: research baseline for future runtime milestones.
Scope: turn/run/task identity, steering, subagents, detached work, cancellation, durable background tasks, scheduling, and restart recovery.

## Executive conclusions

1. Session, Turn, Run, ProviderAttempt, ToolInvocation and Task must be distinct entities.
2. User-input acceptance must be distinct from foreground-turn completion.
3. A session can be foreground-idle while background tasks continue.
4. Steering is not cancellation. Steering is queued guidance consumed only at safe runtime boundaries.
5. Accepted steering is not equivalent to delivered steering; delivery outcome must be observable.
6. A running tool is not preempted merely to apply steering. Sequential unstarted tool tails may be skipped, but structural tool-call/result pairing must remain valid.
7. Subagents should be modeled as child sessions executed by Tasks, not as a special second agent runtime.
8. Subagent completion should be push/event driven. The parent should never poll in a model loop just to discover completion.
9. Background Task status and result-delivery status are separate.
10. Cancellation is a request until the executor confirms termination. Remote/external executors may decline or fail cancellation.
11. Restart recovery needs ownership generations/leases. Persisted "running" metadata alone is never proof an executor is alive.
12. Interrupted mutating work with ambiguous external outcome must not be automatically replayed.
13. Schedules/automations are trigger definitions; each firing creates execution work. A schedule is not a Task.
14. MCP Tasks map into SuprAI TaskManager but do not define SuprAI's internal task model.
15. Linux systemd transient user services are a promising optional backend for process work that should survive the GUI process, but require a proof before adoption.

## 1. Entity model

Recommended hierarchy:

```text
Session
  durable conversation context
  contains accepted Inputs and logical Turns

Input
  durable admitted user/internal input
  may:
    - start a new Turn
    - steer an active Turn
    - queue for a later Turn
    - request interruption

Turn
  logical foreground work episode
  stable across recovery/resume segments

Run
  one executable generation/segment of a Turn
  exactly one live owner at a time
  may end because:
    - completed
    - yielded/waiting
    - cancelled
    - failed
    - interrupted/recoverable

ProviderAttempt
  one inference HTTP/request attempt inside a Run

ToolInvocation
  one durable side-effect/read operation inside a Run

Task
  durable registry record for detached/asynchronous work
  examples:
    - subagent
    - background process
    - MCP durable task
    - scheduled execution
    - future remote worker
```

This is intentionally more explicit than a simple Session -> Messages model.

### Why Turn and Run are separate

A single logical Turn may require multiple execution generations:
- initial execution;
- resume after waiting for child Tasks;
- restart recovery after the process dies;
- explicit continuation after a recoverable infrastructure interruption.

The user should still perceive one logical piece of work even when the runtime used multiple Runs.

A Run therefore has:
- run_id;
- turn_id;
- generation;
- recovery_of_run_id?;
- owner_instance_id;
- started_at;
- terminal state.

A Turn has:
- turn_id;
- session_id;
- root input IDs;
- semantic status;
- lineage/branch metadata.

## 2. Input admission is not turn completion

Agent Client Protocol v2 made a useful semantic correction: prompt RPC acceptance acknowledges that the user message was inserted, while foreground work progresses independently through state notifications.

SuprAI should use the same internal principle even though its first UI is in-process.

```text
submitInput()
  -> durable Input accepted
  -> immediate admission result with input_id
  -> later foreground state/events
  -> eventual Turn terminal event
```

Do not make the command that accepts input block until the agent finishes.

Benefits:
- steering;
- queued follow-ups;
- reconnect/replay;
- multi-surface observation later;
- background events;
- clear ambiguity handling when a client disconnects.

## 3. Session foreground state vs background work

A session has a small foreground state:

```text
idle
running
requires_action
cancelling
degraded
```

Background Task activity is orthogonal.

A session may be:
- foreground `idle`;
- with 3 subagent Tasks still `running`;
- with one completed child result queued for a future continuation.

Background notifications do not automatically make the session foreground-running.

This separation appears explicitly in ACP v2 and solves a major UI/state-model ambiguity.

## 4. Queued input and steering

Recommended input modes:

```text
steer
followup
collect
interrupt
```

### steer
Try to expose the new input to the active Turn at its next safe boundary.

If the runtime cannot accept steering, preserve it as a queued follow-up.

### followup
Never change the active Turn. Start later.

### collect
Coalesce compatible queued inputs into a later Turn after a debounce/quiet window.

### interrupt
Request cancellation of the active Turn, then admit the new input for a new Turn after cancellation/reconciliation.

The initial UI does not need to expose all four modes immediately. The runtime model should support them without schema changes.

## 5. Steering boundaries

Steering must not pretend arbitrary preemption is safe.

Recommended boundary policy:

1. Never terminate an already-running tool merely because steering arrived.
2. A sequential tool call already started is allowed to settle.
3. Before each later unstarted sequential call, check pending steering.
4. If steering is consumed, unstarted calls from the old assistant plan can be marked `skipped_by_steering`.
5. Every skipped tool call receives a terminal synthetic ToolResult so the transcript remains structurally paired.
6. Parallel tool batches that have already been admitted are treated as one committed batch and settle before steering.
7. Consume steering before the next model request.

Canonical ordering:

```text
assistant tool calls
real/synthetic tool results
steering input(s)
next inference request
```

This mirrors a robust pattern in OpenClaw.

## 6. Steering delivery states

A common mistake is treating queue acceptance as model delivery.

Use:

```text
SteerDisposition
  accepted
  delivered
  missed
  rejected
  converted_to_followup
```

`accepted` means the runtime took custody.

`delivered` means the steering input crossed a model-visible/runtime boundary.

`missed` means the target Run completed before the accepted steer could land.

Hermes currently documents an equivalent important distinction for subagent steering: queued steering can miss because the child finishes first.

This should be visible in diagnostics and, where relevant, UI.

## 7. Input priority

Do not use one undifferentiated queue.

Recommended priority classes:

1. cancellation / explicit operator control;
2. approval or clarification responses blocking current work;
3. user steering/corrections;
4. task/subagent completion events required by a waiting parent;
5. ordinary user follow-ups;
6. low-priority progress/internal telemetry.

Within a priority class preserve FIFO order unless a specific control semantic says otherwise.

Avoid starvation by aging lower-priority inputs.

## 8. Subagent model

A subagent is:

```text
Task(source=subagent)
  + child Session
  + child Turn/Run(s)
  + parent/requester binding
```

It is not a second agent implementation.

### Default context

Prefer isolated explicit context over implicit full-parent cloning.

Child receives a `TaskBrief`:
- goal;
- selected context;
- selected canonical item references;
- project/workspace identity;
- attachments;
- allowed capability envelope;
- model/profile choice;
- expected output contract.

Optional future context modes:
- isolated (default);
- fork selected parent history;
- explicit snapshot.

### Result

Store the child transcript durably.

Inject into parent context by default only:
- final summary;
- structured result;
- artifact references;
- important failure metadata.

Child output is agent-generated evidence, not higher-authority user instruction.

## 9. Subagent authority

Effective child authority:

```text
parent/requester capability
INTERSECT
configured child profile
INTERSECT
task-specific restrictions
INTERSECT
current system policy
```

A child can never widen permissions by requesting a larger toolset.

Default deny for child control-plane capabilities such as:
- changing global config;
- persistent memory writes;
- creating schedules;
- sending external messages;
- privilege escalation;
- spawning grandchildren unless explicitly enabled.

Nested spawning:
- default max depth: 1;
- configurable later;
- always bounded;
- global + per-session concurrency caps.

## 10. Attached vs detached child work

Need two lifecycle modes.

### attached
The parent Turn cannot logically finish without the child.
- cancellation of parent cascades;
- parent may yield while waiting;
- completion resumes the same logical Turn.

### detached
The child may outlive the spawning Run/Turn.
- Task remains registered;
- result is delivered to requester Session later;
- parent Turn may already be terminal;
- explicit cancellation still possible.

A spawn operation must declare which semantics it uses. Do not infer this from UI wording.

## 11. Push completion, not model polling

OpenClaw's current `sessions_yield` pattern is sound: spawn returns immediately; completion is pushed back; the model does not waste turns polling status.

SuprAI should support a control primitive conceptually equivalent to:

```text
yieldUntil(tasks)
```

Behavior:
- current Run exits into a waiting/suspended Turn state;
- no model token is spent polling;
- Task completion event wakes the Turn;
- a new Run generation continues it.

The runtime, not the model, watches task completion.

Task inspection remains available for debugging/user inspection but is not the waiting mechanism.

## 12. Parent/child completion binding

Completion routing binds to exact ownership:
- requester_session_id;
- requester_turn_id;
- spawning_run_id;
- task_id;
- child_session_id;
- child_run identity/generation.

A stale result must not enter a different/replaced conversation merely because a display key matches.

Simplest SuprAI policy:
- a reset/new conversation creates a new Session ID;
- do not reuse a Session ID for semantically new conversation state.

This eliminates an entire class of stale-result adoption problems seen in more complex systems.

## 13. Cancellation model

Cancellation has at least two phases:

```text
cancel_requested
cancelled
```

Never report `cancelled` simply because SuprAI sent a signal.

Different executors:
- provider stream: abort local transport; ignore later events for superseded attempt;
- QProcess: SIGTERM/process-group stop then bounded SIGKILL policy;
- subagent: propagate to active child Runs;
- MCP Task: call tasks/cancel, but cancellation may not be honored;
- remote/external worker: await authoritative acknowledgement or mark uncertain/lost.

An uncooperative external tool may not be forcibly cancellable.

## 14. Cancellation scope

Separate commands:

### cancelRun(run_id)
Stops only that exact execution generation and its attached descendants.

### cancelTurn(turn_id)
Stops current Run(s) implementing that logical Turn and retires its waiting continuations.

### cancelSessionForeground(session_id)
Stops foreground work but does not necessarily destroy detached Tasks.

### closeSession(session_id)
Stops foreground work and applies configured policy to attached/detached Tasks, then frees active resources.

### cancelTask(task_id)
Targets one detached/background unit.

Do not make a generic "Stop" silently mean every scope.

UI can still expose one primary Stop button; its exact scope is defined and visible.

## 15. Task registry

Task is an activity/execution record, not a replacement for Session or Turn.

Suggested states:

```text
queued
running
waiting_input
cancel_requested
succeeded
failed
timed_out
cancelled
lost
outcome_unknown
```

Terminal:
- succeeded;
- failed;
- timed_out;
- cancelled;
- lost;
- outcome_unknown.

`lost`:
The authoritative executor disappeared and outcome cannot be discovered after the configured grace/reconciliation path.

`outcome_unknown`:
The external operation may have happened, and replay is unsafe. This is especially important for mutating ToolInvocation/process work.

OpenClaw's current durable task registry similarly distinguishes `lost` from normal failure.

## 16. Task identity and fields

Suggested core:

```text
task_id
source
status
requester_session_id?
requester_turn_id?
requester_run_id?
child_session_id?
active_run_id?
external_handle?
created_at
queued_at
started_at
updated_at
ended_at
timeout_at?
owner_instance_id?
owner_generation
progress
summary
result_ref?
error
notification_policy
delivery_status
durability_class
idempotency_key?
```

Source examples:
- subagent;
- process;
- mcp_task;
- scheduled_run;
- remote_worker;
- future_media_job.

## 17. Execution status != delivery status

A Task may succeed while notification/delivery fails.

Keep a separate delivery machine:

```text
not_requested
pending
delivered
failed
incomplete
suppressed
```

Do not turn a successful computation into `failed` just because the UI/channel notification could not be delivered.

OpenClaw explicitly separates background task status and result delivery; SuprAI should do the same from the beginning.

## 18. Notification policy

Per Task:

```text
done_only
state_changes
silent
```

UI inspection always sees canonical state regardless of notification policy.

## 19. MCP Tasks integration

MCP Tasks extension (2026-07-28) provides long-running server-side tool execution:
- working;
- input_required;
- completed;
- failed;
- cancelled;
- tasks/get;
- tasks/update;
- tasks/cancel;
- optional task notifications/subscriptions.

Map it into TaskManager:

```text
MCP server Task
  -> SuprAI Task(source=mcp_task)
```

Do not expose MCP task IDs as SuprAI task IDs.

Store:
- SuprAI task_id;
- MCP endpoint/server identity;
- opaque external task ID;
- TTL;
- poll interval;
- current external status.

TaskManager owns polling according to `pollIntervalMs` when push notifications are unavailable.

The model never polls MCP tasks manually.

`input_required` routes through the same SuprAI user-action/approval machinery as other runtime requests.

## 20. Schedules/automations are separate from Tasks

```text
Schedule
  trigger definition
      |
      v
Occurrence
      |
      v
Task + fresh/target Session execution
```

Schedule stores:
- schedule_id;
- trigger;
- timezone;
- prompt/task brief;
- profile/model;
- requested capability envelope;
- delivery target;
- misfire policy;
- overlap policy;
- enabled state.

Every firing creates a unique occurrence ID and Task.

Use idempotency key:
`schedule_id + scheduled_occurrence_time`.

## 21. Schedule policies

### Misfire after downtime
Explicit choice:
- skip;
- run_once;
- catch_up_limited(N).

Never silently execute every missed recurring event after a long outage.

### Overlap
Explicit:
- forbid_overlap;
- allow_overlap;
- replace_previous.

Default recommendation: `forbid_overlap`.

### Authority
Creation-time capability intent is not permanent authorization.

At execution:
```text
captured requested capability envelope
INTERSECT
current global/project policy
INTERSECT
currently available tools
```

Do not persist a permission grant that bypasses later security policy.

By default a scheduled agent Task should run in a fresh isolated task Session and deliver its result, not mutate an interactive chat history invisibly.

## 22. Recovery ownership

Persisted `running` does not prove liveness.

Each active Run/Task execution owner gets:
- runtime_instance_id;
- owner_generation;
- PID/process-start identity where useful;
- lease/last-seen metadata when appropriate.

At startup:
1. discover rows claiming active ownership;
2. verify whether the exact owner is still live;
3. reconcile executor-specific state;
4. only then recover, mark lost, or require action.

A new recovery generation fences old late events.

Any event includes enough identity to reject:
- old process callbacks;
- superseded run generations;
- previous recovery attempts.

## 23. Graceful shutdown

Shutdown order:
1. fence admission of new Runs/Tasks;
2. persist recoverable/interrupted markers for owned work;
3. allow bounded drain for active safe work;
4. persist terminal results already obtained;
5. request cancellation for remaining in-process work;
6. flush persistence;
7. exit.

The supervisor's actual stop deadline bounds the drain budget.

OpenClaw's current restart design reinforces this: draining execution is insufficient if terminal persistence has not settled.

## 24. Hard-crash recovery

On startup, distinguish:

### Pure inference interrupted
Safe to start a new Run generation from canonical history after marking the partial attempt incomplete.

### Completed tool result already durable
Reuse it. Never rerun.

### Read-only/idempotent tool interrupted before durable result
May be eligible for explicit automatic retry according to tool metadata.

### Mutating/non-idempotent tool interrupted while `executing`
Mark `outcome_unknown`.
Do not replay automatically.

### Detached external Task with discoverable external handle
Query authoritative executor and reconcile.

### Process-local Task with dead owner and no external authority
Mark `lost`.

## 25. Recovery attempts and tombstones

Automatic recovery must be bounded.

Persist:
- recovery_attempt_count;
- last_recovery_at;
- last_recovery_error;
- recovery_generation;
- recovery_blocked/tombstoned state.

After repeated failure:
- stop auto-looping;
- mark requires operator/user action;
- preserve evidence.

Do not reboot into an infinite model/tool recovery loop.

## 26. Recovery freshness

Not all interrupted work should resume days later.

Each Task/Run type can define:
- freshness window;
- resume policy.

Example:
- foreground interactive Turn: short configurable window;
- scheduled occurrence: schedule misfire policy decides;
- subagent: bounded freshness;
- explicitly durable external task: reconcile regardless of age until TTL.

## 27. Background process ownership

A process started by a foreground ToolInvocation has ephemeral Run ownership by default.

If it must outlive that Run, ownership must be explicitly transferred to a Task before the Run ends.

```text
ToolInvocation
  owns process
    |
    | explicit handoff
    v
Task
  owns process
```

Without handoff:
- clean up on Run end;
- do not leave accidental orphan processes.

Hermes contains a comparable explicit process-ownership handoff mechanism.

## 28. Linux persistent process backend candidate

For work that should survive a SuprAI GUI crash/restart, Linux offers user-level systemd transient services.

Possible implementation:
- create a transient `suprai-task-<id>.service` through systemd user D-Bus;
- assign working directory/environment/resource policy;
- store unit name as external_handle;
- TaskManager observes/reconciles unit state after restart;
- cancellation maps to StopUnit;
- result/output capture strategy remains to be proven.

Advantages:
- execution lifetime independent from GUI process;
- cgroup/process-tree ownership;
- restart reconciliation;
- resource accounting;
- native Linux.

Do not make this mandatory yet:
- systemd user manager is not universal;
- AppImage environments vary;
- output/log transport and containment interaction need a prototype.

Track as proposed future `ProcessBackend::SystemdTransient`.

## 29. Concurrency scheduler

Do not let every subsystem invent concurrency.

Central `ExecutionScheduler` should enforce:
- global active Run cap;
- provider/model concurrency cap;
- subagent cap;
- per-session child cap;
- process/tool cap;
- scheduled-work cap;
- priority lanes.

Provider/model constraints matter especially for local inference.

Suggested priority:
1. foreground user Turn;
2. approval-blocked continuation;
3. explicit user-steered work;
4. parent waiting on child completion;
5. detached/background Tasks;
6. scheduled low-priority work.

Use aging/fairness so background work is not permanently starved.

## 30. No polling loops in the agent

Invariant:

The model must not spend reasoning turns repeatedly asking:
- is the process done?
- is the subagent done?
- is the MCP task done?
- has the timer fired?

TaskManager watches/polls external systems as infrastructure and emits completion/progress events.

The agent resumes only on meaningful state transition.

Goose's current task-registry proposal independently identifies this exact missing substrate: process/timer/subagent mechanisms should converge on a shared managed task layer rather than requiring model polling.

## 31. UI consequences

The shell should eventually distinguish:

```text
Session foreground:
  Idle / Working / Needs input / Cancelling

Background:
  2 running tasks
  1 completed result waiting
  1 lost/unknown task needs review
```

A Tasks inspector can show:
- source;
- owner/requester;
- start time;
- status;
- progress;
- cancellation state;
- child session;
- result/delivery state;
- recovery history.

Do not overload the chat spinner with all background activity.

## 32. Recommended implementation order

Do not implement every advanced feature in M3.

### Core now
Design types/schema for:
- Input;
- Turn;
- Run;
- Task;
- owner generation;
- input queue/disposition;
- cancellation states.

### First runtime implementation
Support:
- one foreground Turn;
- followup queue;
- explicit interrupt;
- no subagents yet.

### Next
Add:
- steering boundaries;
- TaskManager;
- background process Task;
- child subagent Task;
- push completion/yield.

### Later
Add:
- schedules;
- MCP Tasks;
- systemd transient process backend;
- nested orchestration;
- remote workers.

The important requirement is preserving the identity/lifecycle model now so later additions do not force a database rewrite.

## Reference observations

### OpenClaw
Current documentation demonstrates:
- steer/followup/collect/interrupt modes;
- steering only at safe boundaries;
- synthetic terminal results for skipped tool calls;
- push-based subagent completion and explicit yield;
- exact cancellation scopes;
- durable task registry with queued/running/succeeded/failed/timed_out/cancelled/lost states;
- separate delivery tracking;
- restart reconciliation with ownership/generation semantics;
- persisted schedules and run history.

### Hermes
Current documentation demonstrates:
- isolated child contexts;
- restricted child capabilities;
- bounded concurrency/depth;
- child steering where queued != delivered;
- explicit interrupt propagation;
- process ownership handoff for child background processes;
- scheduled work in fresh sessions.

### Goose
Current code/docs demonstrate:
- temporary subagents;
- cancellation tokens;
- state-machine steering;
- scheduler;
- active effort to unify background process/timer/subagent work into one task registry.

### ACP v2
Stable v2 deliberately separates:
- prompt insertion acknowledgement;
- foreground running/requires_action/idle state;
- background updates;
- cancellation confirmation.

That separation is directly applicable to SuprAI's internal architecture.

### MCP Tasks
The 2026-07-28 Tasks extension provides a durable server-side async tool state machine but is only one external task source, not SuprAI's global task architecture.
