# ADR-0027: systemd transient user service as optional durable process backend

Status: proposed
Date: 2026-10-06

## Problem

QProcess-owned background work can outlive an agent Turn but normally cannot survive a SuprAI GUI crash/restart reliably.

Linux provides a native supervision option through user-level systemd transient services.

## Candidate

Implement optional:

`ProcessBackend::SystemdTransient`

Concept:
- create `suprai-task-<id>.service` through user systemd D-Bus `StartTransientUnit()`;
- assign working directory/environment/resource properties;
- store unit name as Task external_handle;
- observe unit lifecycle over D-Bus;
- reconcile existing unit after SuprAI restart;
- cancellation maps to systemd stop;
- cgroup ownership captures descendants.

## Advantages

- execution lifetime independent of GUI process;
- native process-tree/cgroup ownership;
- restart reconciliation;
- resource accounting/limits;
- no root privileges for user units.

## Open questions

- stdout/stderr/result capture contract;
- interaction with Landlock/bubblewrap containment;
- behavior without a user systemd manager;
- distro/systemd version floor;
- transient unit garbage collection;
- AppImage path/executable lifetime;
- environment and secret passing;
- interactive process support;
- shutdown/logout semantics.

## Fallback

QProcess remains baseline.

The effective task durability class must state whether work is:
- run_local;
- app_process;
- externally_supervised;
- remote.

Never promise restart durability when the active ProcessBackend cannot provide it.

## Required proof

Before acceptance:
- KDE/GNOME user session;
- Ubuntu 22.04/24.04/26.04;
- Debian target baseline;
- GUI crash and restart;
- task cancel;
- child process tree;
- stdout/result recovery;
- sandbox integration;
- AppImage execution path.
