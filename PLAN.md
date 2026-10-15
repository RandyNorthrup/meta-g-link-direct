# Delivery plan

## Contract and scope

The confirmed [project brief](PROJECT_BRIEF.md) is authoritative. This plan covers the personal Windows 11 x64 / OBS 32.2.2 prototype and later direct-hardware milestones. Only the structured ledger below owns task and verification states. Setup is not a claim of functioning camera capture.

## Scan and reuse decisions

The initial workspace was empty. PROJECT_BRIEF.md now owns product decisions and AGENTS.md owns rules. Reuse Visor's documented framing and later protocol/session behavior at commit e7077b4051b1e3e605c667aa2a010fbe41f346a0 with AGPL-3.0 attribution. Reuse OBS's published module API and actual 32.2.2 development inputs; do not create private OBS ABI declarations or a fake camera source. New protocol, platform and module paths separate different responsibilities and have actual test/diagnostic consumers. Rescan these canonical paths before every change.

## Architecture and verified compatibility inputs

Use C++20 for owned code, CMake 3.31.3, installed LLVM 22.1.1 and Visual Studio 2022/MSVC compiler 19.44.35222.0 (toolset 14.44.35207) with Windows SDK 10.0.26100.0. OBS 32.2.2 is installed and its official release/source tag was queried successfully. The official plugin template documents Windows Visual Studio 2022 and CMake >=3.28; owned CMake targets will use the published libobs package/module contract. The frame-prefix library stays portable, while device probing, transport and protected identity live under Windows-specific paths. OBS is a thin consumer of the proved connection/media library. No Qt/custom web UI is required for the planned native properties controls.

Official references: [OBS template](https://github.com/obsproject/obs-plugintemplate/tree/3e7d7ac3b5342cd7d9b88890b9c70b472d1520fc), [OBS 32.2.2](https://github.com/obsproject/obs-studio/tree/ba2f32bdf791005443988a4955e963663e16b1ed), [source API](https://docs.obsproject.com/reference-sources), and the pinned research links in the brief. Source tar SHA-256: ec81fb66b03e75ddb3076b576f62679c39262e0e9960cef3e17a40dc5d68e6b4. OBS upstream dependency hashes must be retained and verified; dependencies.lock.json owns exact downloadable inputs.

## Milestone contracts and certification

M0 (TASK-SETUP) creates the native build and quality foundation, a bounded no-allocation prefix parser, read-only local radio diagnostics, and a C17/MSVC OBS module with metadata only. Its independent vectors and mutation protect framing; isolated loader checks protect ABI/metadata; gate drills protect command wiring. Required security checks include staged/history secret scans, SAST, dependency inventory, and workflow lint. Package only the development foundation with explicit status; it must not appear as a functional camera source. Documentation, upstream notices and exact command evidence are part of this milestone. Parser inputs are bounded; performance profiling begins when real receive/decode workloads exist, not with invented benchmark claims.

M1 (TASK-AUTH) ports actual connection/identity behavior and validates it on glasses. A local iPhone export remains optional research; it is not a prerequisite now that Meta authentication is permitted. A reproducible application-token/ownership path, protected local key storage, explicit timeouts/cancellation and return-to-phone behavior must be proved. Review credential exposure and session teardown; tests/drills must reject bad trust signatures and swallowed timeouts. No ownership change occurs merely because a build/probe command runs. Update the brief if product constraints change, and README/CHANGELOG when behavior exists.

M2 (TASK-MEDIA) ports the actual Wi-Fi group-owner/access-point role, frame reassembly, HEVC/AAC decode, timing and supported quality selection. Test malformed/truncated/oversized input, codec configuration changes, bounded queues, timestamps and decoder errors with intended red drills. Measure real resolution/fps, independent end-to-end latency, CPU/memory and thermal behavior. Audio remains optional. Record exact current stable firmware; do not demand a downgrade or treat an Android upstream result as Windows proof. Update use/troubleshooting and privacy documentation with actual behavior.

M3 (TASK-OBS) adds the real source only after the direct path is demonstrated. Test source start/stop/destroy, reconnect, optional audio synchronization, quality changes and failures; verify native UI with keyboard, Narrator, themes and scaling. The final certification requires the complete >=120-minute run, phone off, no cloud media path, no unbounded memory/queue growth and all numeric/visual gates in AC-OBS. Hardware limits may make acceptance fail; report this without lowering the target. Native source tests/red drills and current artifact/source binding accompany manual hardware and visual records. README, CHANGELOG, supported-version evidence, installation/uninstall/rollback and diagnostics instructions must match the final artifact.

## Gate policy and evidence

Expose conventional format, format:check, lint, typecheck, deadcode, security:audit, test, test:red, build, quality and package tasks through scripts/Invoke-Project.ps1. A missing or unsupported tool is an explicit deferred gate, never a silent success. Independent gate/test commands must propagate native exit codes. Test and gate drills use exact restoration in finally and record baseline, intended diagnostic/nonzero exit and restored green. The maintained set runs at setup and affected changes, then milestone/release review.

ASan/UBSan and TSan are separate supported builds. Use portable protocol code on Linux for sanitizer coverage that Windows cannot provide; Windows/OBS/radio testing remains distinct. Coverage floors must follow measured owned-code baselines. cppcheck, gcovr, jscpd and platform-specific sanitizer availability require actual provisioning/verification or a concrete deferred entry with owner and next action. OSV support for pinned OBS native archives must be assessed honestly; a scanner finding no supported lockfile does not constitute a native dependency audit.

Detailed logs and context belong under reports/. Do not copy full command output into this narrative. Readiness review is separate from implementation/test proof. Technical unknowns and future manual gates remain open in the ledger even when M0 software checks pass.

## Open technical verification and deferred public scope

The local Wi-Fi adapter is available but not currently enumerated; test actual capability when connected, without another adapter-model interview. Current-firmware direct operation, authorized desktop setup, iPhone/Meta coexistence, actual video quality/latency and battery/thermal behavior over 120 minutes all require new evidence. Technical investigation owns these, due before their respective hardware milestone, with no calendar deadline supplied.

Public repository, hosted CI execution, public distribution support, signing/provenance services, procurement, public incident response and broader platforms are deferred by the personal-prototype contract. Randy owns reopening these decisions if distribution is requested. No remote/publishing/signing identity is created during M0.

## Analysis decisions and remaining verification boundaries

The actual native build uses MSVC for both languages because CMake rejects a mixed clang-cl/MSVC project and OBS's util_uint64.h needs `_udiv128`, which clang-cl does not implement ([LLVM issue 59168](https://github.com/llvm/llvm-project/issues/59168)). Owned libraries remain C++20; the small OBS ABI file is C17 with `/W4 /WX /analyze`. C++ clang-tidy consumes the actual MSVC compilation database. No compiler-version macro is spoofed in production and no private OBS declaration is introduced.

The copied strict clang-tidy preset contained contradictory static/anonymous-namespace rules. Disable only llvm-prefer-static-over-anonymous-namespace while retaining misc-use-anonymous-namespace; this is a style conflict, not a correctness suppression. Two exact-signature GetProcAddress conversions in tests/module_tests.cpp carry scoped cppcoreguidelines-pro-type-reinterpret-cast annotations after independent non-null/export checks; this is the documented dynamic Windows ABI boundary. Existing Windows umbrella headers are kept with IWYU comments because SDK leaf headers require their prerequisites.

Cppcheck initially lost MSVC `-external:I` include paths. Generate only an analysis copy of compile_commands.json with those paths normalized and the observed compiler builtins added. The .cppcheck-obs.cfg models six real externally loaded OBS entry points; it does not mark arbitrary functions as used. Suppress only missingIncludeSystem informational notices from the deliberately model-based std/windows analysis and findings under the external .deps header tree. The real compiler/LLVM gates still parse owned and system headers; missing quoted owned headers and owned unusedFunction findings remain failures. Native dependency headers are not certified by this scope. Revisit these analysis decisions on toolchain/OBS upgrades; the header and unused-function drills must continue to fail as intended.

The first synthetic AWS canary used hexadecimal rather than the scanner's documented A-Z2-7 base32 suffix. Its miss was preserved in reports/red-secret-canary-initial.txt. The generator was corrected without changing scanner rules. A native-exit mutation was rejected by Pester but the harness expected different assertion wording; the original receipt is retained in reports/red-native-exit-initial.txt. Final proof requires a complete restored run against the corrected harness.

Windows ASan is configured and exercised separately. Linux/hosted ASan+UBSan remains unexecuted because no remote is authorized and this session verifies the Windows host. TSan/Valgrind require a supported Linux environment; the current parser/probe has no owned concurrent streaming code. Coverage baseline/floor and fuzzing remain work before accepting the upcoming session/media parser expansion; gcovr cannot establish coverage for the current MSVC build. Owner: technical implementation; next action: establish portable-core coverage/fuzz and supported sanitizer execution before M1/M2 certification. No coverage percentage, Linux sanitizer pass or hosted CI pass is claimed.

OSV and npm audit currently cover the nine locked npm development packages. The pinned native OBS/dependency archives have integrity and version evidence, not a complete SBOM/advisory assessment. Owner: technical/security work; next action: inventory native components and assess advisories before authentication/media dependencies are accepted. Current SDK input should not be presented as a full native supply-chain audit. Hardware, actual OBS source UI and the 120-minute target remain separate later acceptance, not setup-test conclusions.

```quality-ledger
{
  "schema_version": 1,
  "work": {
    "id": "MGL-DIRECT",
    "title": "Direct Gen 2 glasses source for personal Windows OBS use",
    "scope_revision": 2,
    "brief": "PROJECT_BRIEF.md",
    "brief_reason": null,
    "rules": [
      {
        "path": "AGENTS.md",
        "revision": "2"
      }
    ],
    "inputs": [
      "dependencies.lock.json",
      "package-lock.json",
      "CMakePresets.json"
    ],
    "environment": {
      "tools": {
        "python": "3.14.0",
        "cmake": "3.31.3",
        "llvm": "22.1.1",
        "powershell": "7.6.6",
        "gitleaks": "8.30.1",
        "node": "24.20.0",
        "npm": "11.19.0",
        "semgrep": "1.177.0",
        "actionlint": "1.7.12",
        "zizmor": "1.25.2",
        "osv_scanner": "2.6.0",
        "pester": "5.7.1",
        "psscriptanalyzer": "1.25.0",
        "jscpd": "5.4.1",
        "precommit": "4.5.1",
        "msvc": "19.44.35222.0",
        "cppcheck": "2.22.0"
      },
      "platform": "windows"
    }
  },
  "requirements": [
    {
      "id": "REQ-SETUP",
      "statement": "Provide a reproducible native Windows project foundation with honest boundaries and enforceable quality gates.",
      "priority": "critical",
      "acceptance": [
        "AC-FRAMING",
        "AC-MODULE",
        "AC-GATES"
      ],
      "superseded_by": null
    },
    {
      "id": "REQ-AUTH",
      "statement": "Establish a real authenticated direct Windows connection to Gen 2; Meta setup is allowed when required and streaming sessions should use local links.",
      "priority": "critical",
      "acceptance": [
        "AC-AUTH"
      ],
      "superseded_by": null
    },
    {
      "id": "REQ-MEDIA",
      "statement": "Receive and decode real video locally with adjustable quality and optional glasses audio on latest stable firmware.",
      "priority": "critical",
      "acceptance": [
        "AC-MEDIA"
      ],
      "superseded_by": null
    },
    {
      "id": "REQ-OBS",
      "statement": "Deliver an accessible responsive native OBS source and prove at least 120 minutes continuous streaming.",
      "priority": "critical",
      "acceptance": [
        "AC-OBS"
      ],
      "superseded_by": null
    }
  ],
  "acceptance": [
    {
      "id": "AC-FRAMING",
      "requirement": "REQ-SETUP",
      "given": "Pinned Visor framing rules and independent boundary/invalid prefix examples",
      "when": "The portable frame-prefix parser is built and its tests and mutation drill run",
      "then": "DataX uses four bytes plus declared length; AirShield uses ten bytes plus 16 times block count plus one; incomplete and invalid prefixes remain distinct; the intended defect is rejected.",
      "checks": [
        "behavior",
        "red"
      ],
      "manual_reason": null
    },
    {
      "id": "AC-MODULE",
      "requirement": "REQ-SETUP",
      "given": "OBS 32.2.2 headers/import libraries and installed matching runtime",
      "when": "The native module is built and loaded in an isolated test harness",
      "then": "The DLL resolves against the real OBS runtime and exports valid module metadata; no camera source or hardware success is falsely registered.",
      "checks": [
        "behavior",
        "red"
      ],
      "manual_reason": null
    },
    {
      "id": "AC-GATES",
      "requirement": "REQ-SETUP",
      "given": "Owned source, scripts, configuration, dependency pins and no real credentials",
      "when": "The quality runner and maintained red drills execute",
      "then": "Configured applicable checks fail on intended violations and restore cleanly; missing tools or unproven checks are explicit deferred gates, never reported as green.",
      "checks": [
        "behavior",
        "red"
      ],
      "manual_reason": null
    },
    {
      "id": "AC-AUTH",
      "requirement": "REQ-AUTH",
      "given": "Real Gen 2 glasses with latest stable firmware and Windows wireless hardware",
      "when": "Setup completes, phone is powered off, a new session starts and another starts after glasses restart",
      "then": "Authenticated device information is read directly on PC; local session proof and exact firmware are recorded; Meta setup credentials are protected and phone recovery is documented.",
      "checks": [
        "manual"
      ],
      "manual_reason": "Requires physical glasses, user-controlled login/ownership steps and radio behavior; mocks cannot establish this contract."
    },
    {
      "id": "AC-MEDIA",
      "requirement": "REQ-MEDIA",
      "given": "A proved direct authenticated session and latest stable firmware",
      "when": "A real camera stream and optional glasses audio are received and measured",
      "then": "Decode 720×1280 nominal 30fps and at least one lower profile; target p95 glass-to-PC latency ≤500ms without unbounded buffering; audio can remain disabled and its optional enable path is verified.",
      "checks": [
        "behavior",
        "red",
        "manual"
      ],
      "manual_reason": "Decoder/parser automated evidence must be complemented by actual camera, timing and audio observations."
    },
    {
      "id": "AC-OBS",
      "requirement": "REQ-OBS",
      "given": "A proved direct media path, OBS 32.2.2 on this PC and external audio normally selected",
      "when": "User pairs/connects, starts/stops, adjusts quality, toggles glasses audio, reconnects and streams for at least 120 minutes",
      "then": "Native OBS source remains stable on latest stable firmware; startup ≤15s after ready connection, reconnect ≤30s after availability; no growing queue; keyboard/Narrator/theme checks pass at 480px+ and 100–200% scaling; phone stays off during streaming.",
      "checks": [
        "behavior",
        "red",
        "manual"
      ],
      "manual_reason": "Requires a real OBS scene, actual glasses, assistive technology, visual inspection and a timed 120-minute run."
    }
  ],
  "tasks": [
    {
      "id": "TASK-SETUP",
      "purpose": "M0: establish the native build, protocol foundation, diagnostics, module skeleton, rules and repeatable gates",
      "acceptance": [
        "AC-FRAMING",
        "AC-MODULE",
        "AC-GATES"
      ],
      "depends_on": [],
      "changes": [
        {
          "path": "AGENTS.md",
          "action": "create"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "action": "create"
        },
        {
          "path": ".gitignore",
          "action": "create"
        },
        {
          "path": "CMakeLists.txt",
          "action": "create"
        },
        {
          "path": "CMakePresets.json",
          "action": "create"
        },
        {
          "path": ".clang-format",
          "action": "create"
        },
        {
          "path": ".clang-tidy",
          "action": "create"
        },
        {
          "path": ".gitleaks.toml",
          "action": "create"
        },
        {
          "path": ".semgrep.yml",
          "action": "create"
        },
        {
          "path": ".pre-commit-config.yaml",
          "action": "create"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "action": "create"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "action": "create"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "action": "create"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "action": "create"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "action": "create"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "action": "create"
        },
        {
          "path": "src/probe/main.cpp",
          "action": "create"
        },
        {
          "path": "src/plugin/module.c",
          "action": "create"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "action": "create"
        },
        {
          "path": "tests/module_tests.cpp",
          "action": "create"
        },
        {
          "path": "dependencies.lock.json",
          "action": "create"
        },
        {
          "path": "LICENSE",
          "action": "create"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "action": "create"
        },
        {
          "path": "README.md",
          "action": "create"
        },
        {
          "path": "CHANGELOG.md",
          "action": "create"
        },
        {
          "path": ".github/workflows/quality.yml",
          "action": "create"
        },
        {
          "path": ".github/dependabot.yml",
          "action": "create"
        },
        {
          "path": ".gitattributes",
          "action": "create"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "action": "create"
        },
        {
          "path": ".jscpd.json",
          "action": "create"
        },
        {
          "path": "package.json",
          "action": "create"
        },
        {
          "path": "package-lock.json",
          "action": "create"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "action": "create"
        }
      ],
      "status": "active",
      "evidence": [],
      "blocker": null,
      "superseded_by": null
    },
    {
      "id": "TASK-AUTH",
      "purpose": "M1: port discovery, transport, protected identity and authorized Meta ownership setup; prove direct authentication",
      "acceptance": [
        "AC-AUTH"
      ],
      "depends_on": [
        "TASK-SETUP"
      ],
      "changes": [
        {
          "path": "src/windows/session.cpp",
          "action": "create"
        },
        {
          "path": "src/windows/identity.cpp",
          "action": "create"
        },
        {
          "path": "include/mgl/session.hpp",
          "action": "create"
        },
        {
          "path": "src/probe/main.cpp",
          "action": "modify"
        },
        {
          "path": "tests/session_tests.cpp",
          "action": "create"
        }
      ],
      "status": "planned",
      "evidence": [],
      "blocker": null,
      "superseded_by": null
    },
    {
      "id": "TASK-MEDIA",
      "purpose": "M2: port local access-point negotiation, frame reception, real media decoding and quality/audio control",
      "acceptance": [
        "AC-MEDIA"
      ],
      "depends_on": [
        "TASK-AUTH"
      ],
      "changes": [
        {
          "path": "src/windows/wifi_group.cpp",
          "action": "create"
        },
        {
          "path": "src/media/decoder.cpp",
          "action": "create"
        },
        {
          "path": "src/media/stream.cpp",
          "action": "create"
        },
        {
          "path": "include/mgl/stream.hpp",
          "action": "create"
        },
        {
          "path": "tests/stream_tests.cpp",
          "action": "create"
        }
      ],
      "status": "planned",
      "evidence": [],
      "blocker": null,
      "superseded_by": null
    },
    {
      "id": "TASK-OBS",
      "purpose": "M3: integrate the proved connection as an OBS source and certify long-session/accessibility behavior",
      "acceptance": [
        "AC-OBS"
      ],
      "depends_on": [
        "TASK-MEDIA"
      ],
      "changes": [
        {
          "path": "src/plugin/glasses_source.cpp",
          "action": "create"
        },
        {
          "path": "src/plugin/module.c",
          "action": "modify"
        },
        {
          "path": "data/locale/en-US.ini",
          "action": "create"
        },
        {
          "path": "tests/source_tests.cpp",
          "action": "create"
        }
      ],
      "status": "planned",
      "evidence": [],
      "blocker": null,
      "superseded_by": null
    }
  ],
  "evidence": [
    {
      "id": "EV-READINESS-1",
      "acceptance": [
        "AC-AUTH",
        "AC-FRAMING",
        "AC-GATES",
        "AC-MEDIA",
        "AC-MODULE",
        "AC-OBS"
      ],
      "kind": "readiness",
      "status": "stale",
      "command": [],
      "method": "Independent review of confirmed scope, source scan, acceptance coverage, task dependencies, current tool observations and declared hardware/credential limits.",
      "environment": {
        "platform": "windows",
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1"
        }
      },
      "exit_code": null,
      "artifact": {
        "path": "reports/readiness.txt",
        "sha256": "b0569cc44129507979954b2b4ea85d5f92678265af6b1ae032274295951893e5"
      },
      "inputs": [
        {
          "path": "AGENTS.md",
          "sha256": "71fbdb1732ce2b68f11b7a92e8d3d6bbdc174885cd1eea36b81b770735936af9"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "11ba1d3922963c5a72e269035c3e6fb03804ef3ac92258b7247f3a222235e380"
        }
      ],
      "scope_sha256": "00703e3105d324d9d00af55d6c853b8730d7cb040a2e695a690b915bcf37013a",
      "red": null,
      "reason": "Rule revision 2, C17/MSVC ABI glue and additional dev-tool/runner-test scope supersede the initial review. Re-review before final aggregate verification."
    },
    {
      "id": "EV-READINESS-2",
      "acceptance": [
        "AC-AUTH",
        "AC-FRAMING",
        "AC-GATES",
        "AC-MEDIA",
        "AC-MODULE",
        "AC-OBS"
      ],
      "kind": "readiness",
      "status": "pass",
      "command": [],
      "method": "Re-review after observed compiler compatibility correction, scoped analysis configuration, added runner tests, source inventory and updated verification boundaries.",
      "environment": {
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "msvc": "19.44.35222.0",
          "cppcheck": "2.22.0"
        },
        "platform": "windows"
      },
      "exit_code": null,
      "artifact": {
        "path": "reports/readiness-2.txt",
        "sha256": "77c46bcf7b127e7e47cb3945ccf4fa5c561a23728569af42e7c6a59d258b71c3"
      },
      "inputs": [
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "9f42752e917cd5cf9a77abffaf4ad4648ed79dac4a5354a9f1c287388b3a6f87"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        }
      ],
      "scope_sha256": "bea9f9ba2d7cbe474ed1b216198e3b6f35e9fab8d4f128c7cc6a34c55aa08154",
      "red": null,
      "reason": null
    }
  ],
  "checkpoint": null
}
```

