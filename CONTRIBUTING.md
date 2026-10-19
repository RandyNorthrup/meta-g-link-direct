# Contributing

Thanks for helping. Bug reports, test reports from other hardware and focused pull requests are all welcome.

## Reports from other hardware

So far the project has been tested on one Windows 11 PC with one pair of Ray-Ban Meta Gen 2 glasses. An issue saying what worked or failed on your PC (Windows version, OBS version, Bluetooth and Wi-Fi adapter models, glasses firmware if you know it) helps as much as code. Follow [SUPPORT.md](SUPPORT.md) for what to include and what never to paste.

## Before you write code

- For anything larger than a small fix, open an issue first so the approach can be agreed.
- The product contract is in [PROJECT_BRIEF.md](PROJECT_BRIEF.md) and the project rules are in [AGENTS.md](AGENTS.md). In short: no phone bridge, relay, virtual camera or hosted service; no telemetry or update checks; nothing that changes the user's network or restarts OBS.
- Contributions are licensed under the [GNU AGPL-3.0](LICENSE), like the rest of the project. Keep upstream attribution intact and record imported code in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
- Do not add Meta code or binaries, raw protocol captures, camera footage, or any key, token or password. The only exception is the two public Meta AI app client tokens already in `include/mgl/provision.hpp`.

## Building and checking

[docs/DEVELOPMENT.md](docs/DEVELOPMENT.md) lists the toolchain and the tasks. Run them from the repository root in PowerShell 7:

```powershell
pwsh -NoProfile -File scripts/Invoke-Project.ps1 bootstrap   # once
pwsh -NoProfile -File scripts/Invoke-Project.ps1 quality
```

`quality` runs every gate: build, formatting, static analysis, dead-code and copy checks, secret and dependency scans, tests and the red drills. A pull request should pass it. `test:live` needs a Bluetooth radio; if you cannot run a gate, say which one and why in the pull request instead of skipping it silently.

## Pull requests

- Keep each pull request to one change, with tests for new behavior and its failure paths.
- Use C++20 for the library and C17 only for the OBS entry point, as described in AGENTS.md.
- Update the documentation the change affects, and add a line to [CHANGELOG.md](CHANGELOG.md) under *Unreleased*.
- Describe how you tested it, and on what hardware if it touches the glasses link.
