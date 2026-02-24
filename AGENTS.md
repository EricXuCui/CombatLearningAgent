# AGENTS.md

Guidance for autonomous coding agents operating in this repository.

Scope: `CombatLearningAgent-master/` (Unreal Engine 5 C++ project).

## Project Snapshot

- Engine: Unreal Engine `5.6` (see `CombatLearningAgent.uproject`).
- Primary module: `CombatLearningAgent` (runtime game module).
- Typical source layout:
  - `Source/CombatLearningAgent/Public/*.h`
  - `Source/CombatLearningAgent/Private/*.cpp`
  - `Source/CombatLearningAgent/CombatLearningAgent.Build.cs`
  - `Source/CombatLearningAgent*.Target.cs`
- Common workflow scripts in repo root:
  - `build.ps1`
  - `run_training_fast.ps1`
  - `run_tensorboard.ps1`

## Cursor / Copilot Policy Files

Checked at repository scope:

- `.cursorrules`: not found
- `.cursor/rules/`: not found
- `.github/copilot-instructions.md`: not found

No additional Cursor/Copilot repository-local constraints are currently defined.

## Build, Lint, and Test Commands

Use Windows PowerShell commands unless your environment differs.
Replace `<REPO>` with the absolute repo path.

### 1) Generate project files

```powershell
& "C:\Program Files\Epic Games\UE_5.6\Engine\Build\BatchFiles\Build.bat" -projectfiles -project="<REPO>\CombatLearningAgent.uproject" -game -engine
```

Alternative:

```powershell
& "C:\Program Files\Epic Games\UE_5.6\Engine\Binaries\Win64\UnrealVersionSelector.exe" /projectfiles "<REPO>\CombatLearningAgent.uproject"
```

### 2) Build editor target (primary validation)

Preferred local script:

```powershell
& "<REPO>\build.ps1"
```

Direct Unreal build command:

```powershell
& "C:\Program Files\Epic Games\UE_5.6\Engine\Build\BatchFiles\Build.bat" CombatLearningAgentEditor Win64 Development "<REPO>\CombatLearningAgent.uproject" -WaitMutex -FromMsBuild
```

### 3) Build game target

```powershell
& "C:\Program Files\Epic Games\UE_5.6\Engine\Build\BatchFiles\Build.bat" CombatLearningAgent Win64 Development "<REPO>\CombatLearningAgent.uproject" -WaitMutex -FromMsBuild
```

### 4) Run editor

```powershell
& "C:\Program Files\Epic Games\UE_5.6\Engine\Binaries\Win64\UnrealEditor.exe" "<REPO>\CombatLearningAgent.uproject"
```

### 5) Lint / static analysis

There is no dedicated lint script.
Use build + warnings as lint gate:

- Compile `CombatLearningAgentEditor` in Development.
- Treat new warnings in touched files as regressions.
- Keep UHT clean (invalid macros/includes break reflection generation).

Optional broader validation:

```powershell
& "C:\Program Files\Epic Games\UE_5.6\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun -project="<REPO>\CombatLearningAgent.uproject" -noP4 -build -clientconfig=Development -target=CombatLearningAgentEditor
```

### 6) Run all automation tests (when present)

```powershell
& "C:\Program Files\Epic Games\UE_5.6\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "<REPO>\CombatLearningAgent.uproject" -unattended -nop4 -nosplash -NullRHI -ExecCmds="Automation RunTests Project; Quit"
```

### 7) Run a single test (important)

Use exact automation test name:

```powershell
& "C:\Program Files\Epic Games\UE_5.6\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "<REPO>\CombatLearningAgent.uproject" -unattended -nop4 -nosplash -NullRHI -ExecCmds="Automation RunTests <Exact.Test.Name>; Quit"
```

List names first if needed:

```powershell
& "C:\Program Files\Epic Games\UE_5.6\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "<REPO>\CombatLearningAgent.uproject" -unattended -nop4 -nosplash -NullRHI -ExecCmds="Automation List; Quit"
```

Current status: no C++ tests detected (`IMPLEMENT_*AUTOMATION_TEST` / `DEFINE_SPEC`).

## Coding Conventions for Agents

Follow Unreal Engine conventions first, then repository-specific patterns below.

### Includes and file structure

- Header files: `#pragma once` at top.
- `*.generated.h` must be the last include in headers.
- `.cpp` files include their matching header first.
- Keep include order stable: own header, project headers, engine headers.
- Prefer forward declarations in headers to reduce compile dependencies.

### Formatting

- Use Allman braces.
- Preserve existing indentation style (tabs are common in this repo).
- Keep functions focused and avoid large formatting-only diffs.
- Do not reorder unrelated declarations unless required by change.

### Types and UE idioms

- Prefer UE types/containers (`FString`, `FName`, `TArray`, `TMap`, etc.).
- Use `Cast<>()` for UObject casts.
- Use `UPROPERTY` and `UFUNCTION` where reflection/Blueprint/GC needs it.
- Keep booleans prefixed with `b` (`bDead`, `bDefend`, etc.).
- Clamp and sanitize gameplay-facing numeric inputs.

### Naming

- Class prefixes: `A`, `U`, `F`, `E`, `I` per UE standard.
- Methods/types: PascalCase.
- Local variables: descriptive, concise, consistent with surrounding code.
- Behavior tree nodes/services should be explicit about intent.

### Error handling and control flow

- Do not use C++ exceptions.
- Prefer guard clauses and early returns for invalid state.
- Null-check pointers unless guaranteed valid by API contract.
- Use `check`/`ensure` only for invariants, not normal runtime branching.
- Keep branch logic readable and deterministic for AI/task code.

### Gameplay/AI specifics

- Validate controller/pawn types before use.
- Keep per-tick work lightweight; cache references when safe.
- Keep combat state transitions synchronized and reset paths explicit.
- Avoid hidden side effects in behavior tree task execution.

### Blueprint integration

- Expose only required API surface to Blueprint.
- Use consistent `Category` metadata names.
- Use `BlueprintImplementableEvent` for BP-owned behavior hooks.
- Keep BP-callable methods safe under repeated calls.

### Build/target files

- Update `CombatLearningAgent.Build.cs` only when dependency changes are necessary.
- Keep target settings aligned with active engine include order.
- Validate full editor build after Build.cs or target changes.

## Agent Workflow Expectations

- Make minimal, targeted edits.
- Preserve existing behavior unless task requests behavior change.
- If behavior changes, document rationale in PR/commit notes.
- Run at least editor build after C++ changes.
- If tests exist for touched code, run the smallest relevant single test first.

## Quick Pre-PR Checklist

1. Build `CombatLearningAgentEditor` Development Win64.
2. Run relevant single automation test (if available).
3. If test names are unknown, run `Automation List` first.
4. Confirm no unintended file churn.
