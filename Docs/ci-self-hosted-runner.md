# Build/Test CI on a Self-Hosted Runner

The `build-test` workflow compiles the Diorama gem through a host O3DE project
and runs its unit tests. Unlike the `lint` workflow (which runs on free
GitHub-hosted runners), this one needs the full O3DE 26.05 SDK, a host project,
and the engine's 3rdParty packages, none of which fit on a hosted runner. It
therefore runs only on a **self-hosted runner** that already has the engine.

This is opt-in by design: the always-on gate is lint. Build/test runs when a
maintainer triggers it manually or labels a PR, so contributors without the
hardware are never blocked.

## What the runner needs

- A machine with O3DE **26.05** installed (the SDK, with `scripts/o3de.sh` and
  `bin/Linux/<config>/Default/AzTestRunner`).
- The engine 3rdParty packages (default `~/.o3de/3rdParty`).
- A host O3DE project to build the gem through (any project with the standard
  layout; this repo is developed against a `DioramaSandbox` project).
- Build tooling matching the engine: CMake, Ninja (`ninja-build`), a C++
  toolchain, and clang.

## One-time setup

1. **Register a self-hosted runner** on the repository (GitHub: Settings →
   Actions → Runners → New self-hosted runner) and give it the labels
   `self-hosted` and `o3de`. The workflow targets `runs-on: [self-hosted, o3de]`.

2. **Tell the workflow where the engine and project are.** Set these as
   repository variables (Settings → Secrets and variables → Actions →
   Variables), or export them in the runner's service environment:

   | Variable           | Example                              |
   | ------------------ | ------------------------------------ |
   | `O3DE_ENGINE_PATH` | `/opt/O3DE/26.05.0`                  |
   | `DIORAMA_PROJECT`  | `/home/runner/projects/DioramaSandbox` |

   The build script reads both. `GEM_PATH` defaults to the checked-out repo.

## Triggering it

- **Manually**: Actions → `build-test` → Run workflow.
- **On a PR**: add the `ci:build` label. The job runs on label, and again on
  each push to the PR while the label is present.

## What it does

`scripts/ci_build_test.sh` (which the workflow calls, and which you can run
locally with the same environment variables):

1. Registers this gem as an external subdirectory with the engine.
2. Configures the host project with `Ninja Multi-Config`.
3. Builds `Diorama`, `Diorama.Editor`, and `Diorama.Tests`.
4. Runs the unit tests through `AzTestRunner` and fails on any test failure.

## Running the script locally

The same script works on a developer machine:

```bash
O3DE_ENGINE_PATH=/opt/O3DE/26.05.0 \
DIORAMA_PROJECT=/path/to/DioramaSandbox \
./scripts/ci_build_test.sh
```

Optional overrides: `BUILD_CONFIG` (default `profile`), `BUILD_DIR`,
`TEST_FILTER`, `GEM_PATH`.

## Not covered

This runs the unit tests, which cover the UV, animation, and batch-planning
logic. It does **not** verify on-screen rendering: that needs an interactive
editor/GPU session and is confirmed by a human, not in CI.
