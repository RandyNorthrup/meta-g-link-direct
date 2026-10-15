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
      "status": "verified",
      "evidence": [
        "EV-SOFTWARE-AGGREGATE",
        "EV-RED-PARSER-LENGTH",
        "EV-RED-MODULE-METADATA",
        "EV-RED-FORMAT",
        "EV-RED-OWNED-HEADER",
        "EV-RED-COMPILER-TYPES",
        "EV-RED-UNUSED-CODE",
        "EV-RED-SCRIPT-ANALYSIS",
        "EV-RED-WORKFLOW-SECURITY",
        "EV-RED-SECRET-SCANNER",
        "EV-RED-UNSAFE-COPY",
        "EV-RED-DUPLICATED-CODE",
        "EV-RED-ADDRESS-SANITIZER",
        "EV-RED-DEPENDENCY-NPM",
        "EV-RED-DEPENDENCY-OSV",
        "EV-RED-WHATIF-GUARD",
        "EV-RED-NATIVE-EXIT-PROPAGATION"
      ],
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
    },
    {
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
      "reason": null,
      "id": "EV-SOFTWARE-AGGREGATE",
      "acceptance": [
        "AC-FRAMING",
        "AC-MODULE",
        "AC-GATES"
      ],
      "kind": "behavior",
      "status": "pass",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        ".\\scripts\\Invoke-Project.ps1",
        "package"
      ],
      "method": "Executed the configured quality aggregate and packaging: native build; real-runtime module metadata/ABI load; 83 frame cases; three Pester effects/failure tests; formatting/static/dead-code/security checks; all 16 intended-red/restored-green drills. Independently verified ZIP CRC, x64 PE, packaged/tested DLL equality and unchanged aggregate source fingerprints. No real glasses, source UI or hosted execution is claimed.",
      "exit_code": 0,
      "artifact": {
        "path": "reports/quality-package.txt",
        "sha256": "50b0172b6be808fff7c2b24c1a5d2ad4dc3bf925cd9a2c5928209d566881f148"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "ee43d446d24f0d2bdcbed79675723d60ce40dc884f727fe66e93a4adee84fda8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "ab623d802989bc87f67cf8942c822959d46ea34d92d8a9bcedebe1eec456ae76"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "ccc5cde490041c4086e9f076482ae31af2334d3d0132162109bb021aed490ae3"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "9f42752e917cd5cf9a77abffaf4ad4648ed79dac4a5354a9f1c287388b3a6f87"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "0244903cb9ac5f49cf5fc7e5c1ea48117c04ee84082d7d61bd1b3d526b5957b6"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "1dd8521c61703b03143386b81923cb8840d4777fdc65d2ae1ce40c63728ea1d6"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "1aa70c0fd03120ae45b53da26fc11f7cd81c6b6255031683e92ce0c7365e0d8e"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "f34458187fdd9585bba2d58ed8d2533d79b0ea45e9c27f3e70141df27a40eb85"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "1799a1e768ac3b121fe93084f901bc869827b63ea3da2111f074f9a56fe50080"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "d7c5a85a690218456d9d9b3b0ce19c4b47d108cba4665fb474343c9af68db6d4"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        }
      ],
      "scope_sha256": "4949ca6b56599220219cfe4da1a57f8620b11118cf1c81c80acedd944e6d7b3c",
      "red": null
    },
    {
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
      "reason": null,
      "id": "EV-RED-PARSER-LENGTH",
      "acceptance": [
        "AC-FRAMING"
      ],
      "kind": "red",
      "status": "pass",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "C:\\Users\\Randy\\Coding\\meta_g_link_direct\\scripts\\Invoke-Project.ps1",
        "test"
      ],
      "method": "Executed as part of the complete maintained drill set inside the successful package aggregate; reviewed the intended diagnostic and independent assertion or scanner mechanism.",
      "exit_code": 0,
      "artifact": {
        "path": "reports/red/parser-length.txt",
        "sha256": "8bc8158e1d561ec3f1f243c3a94dc64ef83a75f8575d93d98e57218710b6900d"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "ee43d446d24f0d2bdcbed79675723d60ce40dc884f727fe66e93a4adee84fda8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "ab623d802989bc87f67cf8942c822959d46ea34d92d8a9bcedebe1eec456ae76"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "ccc5cde490041c4086e9f076482ae31af2334d3d0132162109bb021aed490ae3"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "9f42752e917cd5cf9a77abffaf4ad4648ed79dac4a5354a9f1c287388b3a6f87"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "0244903cb9ac5f49cf5fc7e5c1ea48117c04ee84082d7d61bd1b3d526b5957b6"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "1dd8521c61703b03143386b81923cb8840d4777fdc65d2ae1ce40c63728ea1d6"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "1aa70c0fd03120ae45b53da26fc11f7cd81c6b6255031683e92ce0c7365e0d8e"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "f34458187fdd9585bba2d58ed8d2533d79b0ea45e9c27f3e70141df27a40eb85"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "1799a1e768ac3b121fe93084f901bc869827b63ea3da2111f074f9a56fe50080"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "d7c5a85a690218456d9d9b3b0ce19c4b47d108cba4665fb474343c9af68db6d4"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        }
      ],
      "scope_sha256": "4949ca6b56599220219cfe4da1a57f8620b11118cf1c81c80acedd944e6d7b3c",
      "red": {
        "mutation": "Change DataxHeader from 4 to 5 without modifying the independent expected frame lengths.",
        "baseline_exit": 0,
        "mutated_exit": 1,
        "restored_exit": 0,
        "expected_diagnostic": "DataX empty has 4 bytes: unexpected prefix status or frame length",
        "observed_diagnostic": "DataX empty has 4 bytes: unexpected prefix status or frame length",
        "before": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
          }
        ],
        "mutated": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "fcea87b9c7e60b81822b0cadc2362def9e3cd413011a19130199b857b6660278"
          }
        ],
        "after": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
          }
        ]
      }
    },
    {
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
      "reason": null,
      "id": "EV-RED-MODULE-METADATA",
      "acceptance": [
        "AC-MODULE"
      ],
      "kind": "red",
      "status": "pass",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "C:\\Users\\Randy\\Coding\\meta_g_link_direct\\scripts\\Invoke-Project.ps1",
        "test"
      ],
      "method": "Executed as part of the complete maintained drill set inside the successful package aggregate; reviewed the intended diagnostic and independent assertion or scanner mechanism.",
      "exit_code": 0,
      "artifact": {
        "path": "reports/red/module-metadata.txt",
        "sha256": "567a809d4b60e91d028d2b27f45fa03b77af878d20010737e7a74f778a793a64"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "ee43d446d24f0d2bdcbed79675723d60ce40dc884f727fe66e93a4adee84fda8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "ab623d802989bc87f67cf8942c822959d46ea34d92d8a9bcedebe1eec456ae76"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "ccc5cde490041c4086e9f076482ae31af2334d3d0132162109bb021aed490ae3"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "9f42752e917cd5cf9a77abffaf4ad4648ed79dac4a5354a9f1c287388b3a6f87"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "0244903cb9ac5f49cf5fc7e5c1ea48117c04ee84082d7d61bd1b3d526b5957b6"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "1dd8521c61703b03143386b81923cb8840d4777fdc65d2ae1ce40c63728ea1d6"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "1aa70c0fd03120ae45b53da26fc11f7cd81c6b6255031683e92ce0c7365e0d8e"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "f34458187fdd9585bba2d58ed8d2533d79b0ea45e9c27f3e70141df27a40eb85"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "1799a1e768ac3b121fe93084f901bc869827b63ea3da2111f074f9a56fe50080"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "d7c5a85a690218456d9d9b3b0ce19c4b47d108cba4665fb474343c9af68db6d4"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        }
      ],
      "scope_sha256": "4949ca6b56599220219cfe4da1a57f8620b11118cf1c81c80acedd944e6d7b3c",
      "red": {
        "mutation": "Change the exported module name to Broken module name.",
        "baseline_exit": 0,
        "mutated_exit": 1,
        "restored_exit": 0,
        "expected_diagnostic": "OBS module metadata does not match development-foundation contract",
        "observed_diagnostic": "OBS module metadata does not match development-foundation contract",
        "before": [
          {
            "path": "src/plugin/module.c",
            "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
          }
        ],
        "mutated": [
          {
            "path": "src/plugin/module.c",
            "sha256": "608f7c48d09ad85b57cce8de7cc22fedeeea6017210dfd2cc37efc9da082004b"
          }
        ],
        "after": [
          {
            "path": "src/plugin/module.c",
            "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
          }
        ]
      }
    },
    {
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
      "reason": null,
      "id": "EV-RED-FORMAT",
      "acceptance": [
        "AC-GATES"
      ],
      "kind": "red",
      "status": "pass",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "C:\\Users\\Randy\\Coding\\meta_g_link_direct\\scripts\\Invoke-Project.ps1",
        "format:check"
      ],
      "method": "Executed as part of the complete maintained drill set inside the successful package aggregate; reviewed the intended diagnostic and independent assertion or scanner mechanism.",
      "exit_code": 0,
      "artifact": {
        "path": "reports/red/format.txt",
        "sha256": "2709604244b5ea4ab4da49131d6d193bc361311b575793332a501dea5dee3c3b"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "ee43d446d24f0d2bdcbed79675723d60ce40dc884f727fe66e93a4adee84fda8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "ab623d802989bc87f67cf8942c822959d46ea34d92d8a9bcedebe1eec456ae76"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "ccc5cde490041c4086e9f076482ae31af2334d3d0132162109bb021aed490ae3"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "9f42752e917cd5cf9a77abffaf4ad4648ed79dac4a5354a9f1c287388b3a6f87"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "0244903cb9ac5f49cf5fc7e5c1ea48117c04ee84082d7d61bd1b3d526b5957b6"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "1dd8521c61703b03143386b81923cb8840d4777fdc65d2ae1ce40c63728ea1d6"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "1aa70c0fd03120ae45b53da26fc11f7cd81c6b6255031683e92ce0c7365e0d8e"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "f34458187fdd9585bba2d58ed8d2533d79b0ea45e9c27f3e70141df27a40eb85"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "1799a1e768ac3b121fe93084f901bc869827b63ea3da2111f074f9a56fe50080"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "d7c5a85a690218456d9d9b3b0ce19c4b47d108cba4665fb474343c9af68db6d4"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        }
      ],
      "scope_sha256": "4949ca6b56599220219cfe4da1a57f8620b11118cf1c81c80acedd944e6d7b3c",
      "red": {
        "mutation": "Append an intentionally unformatted function.",
        "baseline_exit": 0,
        "mutated_exit": 1,
        "restored_exit": 0,
        "expected_diagnostic": "C:\\Users\\Randy\\Coding\\meta_g_link_direct\\src\\protocol\\sdu.cpp:47:22: error: code should be clang-formatted [-Wclang-format-violations]",
        "observed_diagnostic": "C:\\Users\\Randy\\Coding\\meta_g_link_direct\\src\\protocol\\sdu.cpp:47:22: error: code should be clang-formatted [-Wclang-format-violations]",
        "before": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
          }
        ],
        "mutated": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "c2a1d1a6c8b7489cbb25561dddadb31309d7ac5c15f880bbeb3c90288eabb038"
          }
        ],
        "after": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
          }
        ]
      }
    },
    {
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
      "reason": null,
      "id": "EV-RED-OWNED-HEADER",
      "acceptance": [
        "AC-GATES"
      ],
      "kind": "red",
      "status": "pass",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "C:\\Users\\Randy\\Coding\\meta_g_link_direct\\scripts\\Invoke-Project.ps1",
        "lint:cpp"
      ],
      "method": "Executed as part of the complete maintained drill set inside the successful package aggregate; reviewed the intended diagnostic and independent assertion or scanner mechanism.",
      "exit_code": 0,
      "artifact": {
        "path": "reports/red/owned-header.txt",
        "sha256": "fac6e378d9eeee7b448697bceb5b0cbb8ec96484b25910eea094fa0d11ae0531"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "ee43d446d24f0d2bdcbed79675723d60ce40dc884f727fe66e93a4adee84fda8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "ab623d802989bc87f67cf8942c822959d46ea34d92d8a9bcedebe1eec456ae76"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "ccc5cde490041c4086e9f076482ae31af2334d3d0132162109bb021aed490ae3"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "9f42752e917cd5cf9a77abffaf4ad4648ed79dac4a5354a9f1c287388b3a6f87"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "0244903cb9ac5f49cf5fc7e5c1ea48117c04ee84082d7d61bd1b3d526b5957b6"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "1dd8521c61703b03143386b81923cb8840d4777fdc65d2ae1ce40c63728ea1d6"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "1aa70c0fd03120ae45b53da26fc11f7cd81c6b6255031683e92ce0c7365e0d8e"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "f34458187fdd9585bba2d58ed8d2533d79b0ea45e9c27f3e70141df27a40eb85"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "1799a1e768ac3b121fe93084f901bc869827b63ea3da2111f074f9a56fe50080"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "d7c5a85a690218456d9d9b3b0ce19c4b47d108cba4665fb474343c9af68db6d4"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        }
      ],
      "scope_sha256": "4949ca6b56599220219cfe4da1a57f8620b11118cf1c81c80acedd944e6d7b3c",
      "red": {
        "mutation": "Append a mutable inline global to an owned header.",
        "baseline_exit": 0,
        "mutated_exit": 1,
        "restored_exit": 0,
        "expected_diagnostic": "C:\\Users\\Randy\\Coding\\meta_g_link_direct\\include\\mgl/sdu.hpp:23:12: error: variable 'red_drill_global' is non-const and globally accessible, consider making it const [cppcoreguidelines-avoid-non-const-global-variables,-warnings-as-errors]",
        "observed_diagnostic": "C:\\Users\\Randy\\Coding\\meta_g_link_direct\\include\\mgl/sdu.hpp:23:12: error: variable 'red_drill_global' is non-const and globally accessible, consider making it const [cppcoreguidelines-avoid-non-const-global-variables,-warnings-as-errors]",
        "before": [
          {
            "path": "include/mgl/sdu.hpp",
            "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
          }
        ],
        "mutated": [
          {
            "path": "include/mgl/sdu.hpp",
            "sha256": "220f387016dea2b2cb31874833a09fd753ad9998873937e5cdfba7cb762fe0ec"
          }
        ],
        "after": [
          {
            "path": "include/mgl/sdu.hpp",
            "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
          }
        ]
      }
    },
    {
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
      "reason": null,
      "id": "EV-RED-COMPILER-TYPES",
      "acceptance": [
        "AC-GATES"
      ],
      "kind": "red",
      "status": "pass",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "C:\\Users\\Randy\\Coding\\meta_g_link_direct\\scripts\\Invoke-Project.ps1",
        "build"
      ],
      "method": "Executed as part of the complete maintained drill set inside the successful package aggregate; reviewed the intended diagnostic and independent assertion or scanner mechanism.",
      "exit_code": 0,
      "artifact": {
        "path": "reports/red/compiler-types.txt",
        "sha256": "a7e224c803008efad05581d907b1cf92d64ed5dc2b18ddc13edc3e61594ce36f"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "ee43d446d24f0d2bdcbed79675723d60ce40dc884f727fe66e93a4adee84fda8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "ab623d802989bc87f67cf8942c822959d46ea34d92d8a9bcedebe1eec456ae76"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "ccc5cde490041c4086e9f076482ae31af2334d3d0132162109bb021aed490ae3"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "9f42752e917cd5cf9a77abffaf4ad4648ed79dac4a5354a9f1c287388b3a6f87"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "0244903cb9ac5f49cf5fc7e5c1ea48117c04ee84082d7d61bd1b3d526b5957b6"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "1dd8521c61703b03143386b81923cb8840d4777fdc65d2ae1ce40c63728ea1d6"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "1aa70c0fd03120ae45b53da26fc11f7cd81c6b6255031683e92ce0c7365e0d8e"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "f34458187fdd9585bba2d58ed8d2533d79b0ea45e9c27f3e70141df27a40eb85"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "1799a1e768ac3b121fe93084f901bc869827b63ea3da2111f074f9a56fe50080"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "d7c5a85a690218456d9d9b3b0ce19c4b47d108cba4665fb474343c9af68db6d4"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        }
      ],
      "scope_sha256": "4949ca6b56599220219cfe4da1a57f8620b11118cf1c81c80acedd944e6d7b3c",
      "red": {
        "mutation": "Initialize an integer constant from a string literal.",
        "baseline_exit": 0,
        "mutated_exit": 1,
        "restored_exit": 0,
        "expected_diagnostic": "C:\\Users\\Randy\\Coding\\meta_g_link_direct\\src\\protocol\\sdu.cpp(47): error C2440: 'initializing': cannot convert from 'const char [16]' to 'const int'",
        "observed_diagnostic": "C:\\Users\\Randy\\Coding\\meta_g_link_direct\\src\\protocol\\sdu.cpp(47): error C2440: 'initializing': cannot convert from 'const char [16]' to 'const int'",
        "before": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
          }
        ],
        "mutated": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "4d3f32e4a31af074ff96c5ef217d361573917be607688d49acd001562970bc96"
          }
        ],
        "after": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
          }
        ]
      }
    },
    {
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
      "reason": null,
      "id": "EV-RED-UNUSED-CODE",
      "acceptance": [
        "AC-GATES"
      ],
      "kind": "red",
      "status": "pass",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "C:\\Users\\Randy\\Coding\\meta_g_link_direct\\scripts\\Invoke-Project.ps1",
        "deadcode:cpp"
      ],
      "method": "Executed as part of the complete maintained drill set inside the successful package aggregate; reviewed the intended diagnostic and independent assertion or scanner mechanism.",
      "exit_code": 0,
      "artifact": {
        "path": "reports/red/unused-code.txt",
        "sha256": "6644c6b1f7e244c71347df207b9fb8a9df6a4f0dda4a2fbe5ea19cf7198cc0d1"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "ee43d446d24f0d2bdcbed79675723d60ce40dc884f727fe66e93a4adee84fda8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "ab623d802989bc87f67cf8942c822959d46ea34d92d8a9bcedebe1eec456ae76"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "ccc5cde490041c4086e9f076482ae31af2334d3d0132162109bb021aed490ae3"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "9f42752e917cd5cf9a77abffaf4ad4648ed79dac4a5354a9f1c287388b3a6f87"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "0244903cb9ac5f49cf5fc7e5c1ea48117c04ee84082d7d61bd1b3d526b5957b6"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "1dd8521c61703b03143386b81923cb8840d4777fdc65d2ae1ce40c63728ea1d6"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "1aa70c0fd03120ae45b53da26fc11f7cd81c6b6255031683e92ce0c7365e0d8e"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "f34458187fdd9585bba2d58ed8d2533d79b0ea45e9c27f3e70141df27a40eb85"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "1799a1e768ac3b121fe93084f901bc869827b63ea3da2111f074f9a56fe50080"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "d7c5a85a690218456d9d9b3b0ce19c4b47d108cba4665fb474343c9af68db6d4"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        }
      ],
      "scope_sha256": "4949ca6b56599220219cfe4da1a57f8620b11118cf1c81c80acedd944e6d7b3c",
      "red": {
        "mutation": "Append an unreferenced function to owned protocol source.",
        "baseline_exit": 0,
        "mutated_exit": 1,
        "restored_exit": 0,
        "expected_diagnostic": "C:\\Users\\Randy\\Coding\\meta_g_link_direct\\src\\protocol\\sdu.cpp:47:5: style: The function 'red_drill_unused_function' is never used. [unusedFunction]",
        "observed_diagnostic": "C:\\Users\\Randy\\Coding\\meta_g_link_direct\\src\\protocol\\sdu.cpp:47:5: style: The function 'red_drill_unused_function' is never used. [unusedFunction]",
        "before": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
          }
        ],
        "mutated": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "56c7aca4962c137d9b9bfddaedb756fb220fc33620f0f3888c0b25de525b780a"
          }
        ],
        "after": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
          }
        ]
      }
    },
    {
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
      "reason": null,
      "id": "EV-RED-SCRIPT-ANALYSIS",
      "acceptance": [
        "AC-GATES"
      ],
      "kind": "red",
      "status": "pass",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "C:\\Users\\Randy\\Coding\\meta_g_link_direct\\scripts\\Invoke-Project.ps1",
        "lint:powershell"
      ],
      "method": "Executed as part of the complete maintained drill set inside the successful package aggregate; reviewed the intended diagnostic and independent assertion or scanner mechanism.",
      "exit_code": 0,
      "artifact": {
        "path": "reports/red/script-analysis.txt",
        "sha256": "5d46cad10861f1ca95791580adc151d2002908cf892a9ef48b9fdb4ea2f95398"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "ee43d446d24f0d2bdcbed79675723d60ce40dc884f727fe66e93a4adee84fda8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "ab623d802989bc87f67cf8942c822959d46ea34d92d8a9bcedebe1eec456ae76"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "ccc5cde490041c4086e9f076482ae31af2334d3d0132162109bb021aed490ae3"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "9f42752e917cd5cf9a77abffaf4ad4648ed79dac4a5354a9f1c287388b3a6f87"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "0244903cb9ac5f49cf5fc7e5c1ea48117c04ee84082d7d61bd1b3d526b5957b6"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "1dd8521c61703b03143386b81923cb8840d4777fdc65d2ae1ce40c63728ea1d6"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "1aa70c0fd03120ae45b53da26fc11f7cd81c6b6255031683e92ce0c7365e0d8e"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "f34458187fdd9585bba2d58ed8d2533d79b0ea45e9c27f3e70141df27a40eb85"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "1799a1e768ac3b121fe93084f901bc869827b63ea3da2111f074f9a56fe50080"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "d7c5a85a690218456d9d9b3b0ce19c4b47d108cba4665fb474343c9af68db6d4"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        }
      ],
      "scope_sha256": "4949ca6b56599220219cfe4da1a57f8620b11118cf1c81c80acedd944e6d7b3c",
      "red": {
        "mutation": "Append a function with an unapproved PowerShell verb.",
        "baseline_exit": 0,
        "mutated_exit": 1,
        "restored_exit": 0,
        "expected_diagnostic": "PSUseApprovedVerbs Warning  Invoke-Project.ps1 464  The cmdlet 'Badverb-RedDrill' uses an unapproved verb.",
        "observed_diagnostic": "PSUseApprovedVerbs Warning  Invoke-Project.ps1 464  The cmdlet 'Badverb-RedDrill' uses an unapproved verb.",
        "before": [
          {
            "path": "scripts/Invoke-Project.ps1",
            "sha256": "f34458187fdd9585bba2d58ed8d2533d79b0ea45e9c27f3e70141df27a40eb85"
          }
        ],
        "mutated": [
          {
            "path": "scripts/Invoke-Project.ps1",
            "sha256": "9c32be0adda020562fa3727ccd34e64553120926c808daa86761731f29dd9bb0"
          }
        ],
        "after": [
          {
            "path": "scripts/Invoke-Project.ps1",
            "sha256": "f34458187fdd9585bba2d58ed8d2533d79b0ea45e9c27f3e70141df27a40eb85"
          }
        ]
      }
    },
    {
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
      "reason": null,
      "id": "EV-RED-WORKFLOW-SECURITY",
      "acceptance": [
        "AC-GATES"
      ],
      "kind": "red",
      "status": "pass",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "C:\\Users\\Randy\\Coding\\meta_g_link_direct\\scripts\\Invoke-Project.ps1",
        "lint:workflow"
      ],
      "method": "Executed as part of the complete maintained drill set inside the successful package aggregate; reviewed the intended diagnostic and independent assertion or scanner mechanism.",
      "exit_code": 0,
      "artifact": {
        "path": "reports/red/workflow-security.txt",
        "sha256": "19e4cb7fa962c8103d15f12ac99efbf79c284e97c1eaf4b251064f3e7892133b"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "ee43d446d24f0d2bdcbed79675723d60ce40dc884f727fe66e93a4adee84fda8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "ab623d802989bc87f67cf8942c822959d46ea34d92d8a9bcedebe1eec456ae76"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "ccc5cde490041c4086e9f076482ae31af2334d3d0132162109bb021aed490ae3"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "9f42752e917cd5cf9a77abffaf4ad4648ed79dac4a5354a9f1c287388b3a6f87"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "0244903cb9ac5f49cf5fc7e5c1ea48117c04ee84082d7d61bd1b3d526b5957b6"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "1dd8521c61703b03143386b81923cb8840d4777fdc65d2ae1ce40c63728ea1d6"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "1aa70c0fd03120ae45b53da26fc11f7cd81c6b6255031683e92ce0c7365e0d8e"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "f34458187fdd9585bba2d58ed8d2533d79b0ea45e9c27f3e70141df27a40eb85"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "1799a1e768ac3b121fe93084f901bc869827b63ea3da2111f074f9a56fe50080"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "d7c5a85a690218456d9d9b3b0ce19c4b47d108cba4665fb474343c9af68db6d4"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        }
      ],
      "scope_sha256": "4949ca6b56599220219cfe4da1a57f8620b11118cf1c81c80acedd944e6d7b3c",
      "red": {
        "mutation": "Replace the pinned checkout commit SHA with a movable version tag.",
        "baseline_exit": 0,
        "mutated_exit": 1,
        "restored_exit": 0,
        "expected_diagnostic": "error[unpinned-uses]: unpinned action reference",
        "observed_diagnostic": "error[unpinned-uses]: unpinned action reference",
        "before": [
          {
            "path": ".github/workflows/quality.yml",
            "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
          }
        ],
        "mutated": [
          {
            "path": ".github/workflows/quality.yml",
            "sha256": "a151bb006b4d7e858647ffe9c81521c98e2e2d9a3c12cfa60555e229a57afd76"
          }
        ],
        "after": [
          {
            "path": ".github/workflows/quality.yml",
            "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
          }
        ]
      }
    },
    {
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
      "reason": null,
      "id": "EV-RED-SECRET-SCANNER",
      "acceptance": [
        "AC-GATES"
      ],
      "kind": "red",
      "status": "pass",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "C:\\Users\\Randy\\Coding\\meta_g_link_direct\\scripts\\Invoke-Project.ps1",
        "security:secrets"
      ],
      "method": "Executed as part of the complete maintained drill set inside the successful package aggregate; reviewed the intended diagnostic and independent assertion or scanner mechanism.",
      "exit_code": 0,
      "artifact": {
        "path": "reports/red/secret-scanner.txt",
        "sha256": "2e1fd97782814a8372e4c997e16aa9dd39e35d50eaadc8590dbfd818b2d7f94c"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "ee43d446d24f0d2bdcbed79675723d60ce40dc884f727fe66e93a4adee84fda8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "ab623d802989bc87f67cf8942c822959d46ea34d92d8a9bcedebe1eec456ae76"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "ccc5cde490041c4086e9f076482ae31af2334d3d0132162109bb021aed490ae3"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "9f42752e917cd5cf9a77abffaf4ad4648ed79dac4a5354a9f1c287388b3a6f87"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "0244903cb9ac5f49cf5fc7e5c1ea48117c04ee84082d7d61bd1b3d526b5957b6"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "1dd8521c61703b03143386b81923cb8840d4777fdc65d2ae1ce40c63728ea1d6"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "1aa70c0fd03120ae45b53da26fc11f7cd81c6b6255031683e92ce0c7365e0d8e"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "f34458187fdd9585bba2d58ed8d2533d79b0ea45e9c27f3e70141df27a40eb85"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "1799a1e768ac3b121fe93084f901bc869827b63ea3da2111f074f9a56fe50080"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "d7c5a85a690218456d9d9b3b0ce19c4b47d108cba4665fb474343c9af68db6d4"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        }
      ],
      "scope_sha256": "4949ca6b56599220219cfe4da1a57f8620b11118cf1c81c80acedd944e6d7b3c",
      "red": {
        "mutation": "Append a randomized base32 AWS-key-shaped synthetic canary; scanner output is redacted.",
        "baseline_exit": 0,
        "mutated_exit": 1,
        "restored_exit": 0,
        "expected_diagnostic": "1:07PM WRN leaks found: 1",
        "observed_diagnostic": "1:07PM WRN leaks found: 1",
        "before": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
          }
        ],
        "mutated": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "cf9ff27c94759ecb629d586ecc3b4aa0442274170c2cfd180ceb73276f9f4999"
          }
        ],
        "after": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
          }
        ]
      }
    },
    {
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
      "reason": null,
      "id": "EV-RED-UNSAFE-COPY",
      "acceptance": [
        "AC-GATES"
      ],
      "kind": "red",
      "status": "pass",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "C:\\Users\\Randy\\Coding\\meta_g_link_direct\\scripts\\Invoke-Project.ps1",
        "security:sast"
      ],
      "method": "Executed as part of the complete maintained drill set inside the successful package aggregate; reviewed the intended diagnostic and independent assertion or scanner mechanism.",
      "exit_code": 0,
      "artifact": {
        "path": "reports/red/unsafe-copy.txt",
        "sha256": "c8db8607f34ac8e2a1f449b36873aab4f0f3e17d4ad1a299520171cac8ed5277"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "ee43d446d24f0d2bdcbed79675723d60ce40dc884f727fe66e93a4adee84fda8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "ab623d802989bc87f67cf8942c822959d46ea34d92d8a9bcedebe1eec456ae76"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "ccc5cde490041c4086e9f076482ae31af2334d3d0132162109bb021aed490ae3"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "9f42752e917cd5cf9a77abffaf4ad4648ed79dac4a5354a9f1c287388b3a6f87"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "0244903cb9ac5f49cf5fc7e5c1ea48117c04ee84082d7d61bd1b3d526b5957b6"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "1dd8521c61703b03143386b81923cb8840d4777fdc65d2ae1ce40c63728ea1d6"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "1aa70c0fd03120ae45b53da26fc11f7cd81c6b6255031683e92ce0c7365e0d8e"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "f34458187fdd9585bba2d58ed8d2533d79b0ea45e9c27f3e70141df27a40eb85"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "1799a1e768ac3b121fe93084f901bc869827b63ea3da2111f074f9a56fe50080"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "d7c5a85a690218456d9d9b3b0ce19c4b47d108cba4665fb474343c9af68db6d4"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        }
      ],
      "scope_sha256": "4949ca6b56599220219cfe4da1a57f8620b11118cf1c81c80acedd944e6d7b3c",
      "red": {
        "mutation": "Append an unbounded strcpy call.",
        "baseline_exit": 0,
        "mutated_exit": 1,
        "restored_exit": 0,
        "expected_diagnostic": "❯❯❱ mgl-unsafe-string-copy",
        "observed_diagnostic": "❯❯❱ mgl-unsafe-string-copy",
        "before": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
          }
        ],
        "mutated": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "8c973ff6b7b7e0be339225d7c4a427203011da68c5fda6414bea6ec6c0ea0b03"
          }
        ],
        "after": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
          }
        ]
      }
    },
    {
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
      "reason": null,
      "id": "EV-RED-DUPLICATED-CODE",
      "acceptance": [
        "AC-GATES"
      ],
      "kind": "red",
      "status": "pass",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "C:\\Users\\Randy\\Coding\\meta_g_link_direct\\scripts\\Invoke-Project.ps1",
        "deadcode:copies"
      ],
      "method": "Executed as part of the complete maintained drill set inside the successful package aggregate; reviewed the intended diagnostic and independent assertion or scanner mechanism.",
      "exit_code": 0,
      "artifact": {
        "path": "reports/red/duplicated-code.txt",
        "sha256": "34aa70eeccf7c3c218914d604a72805519c10d0ab96c565a3e27d098df8e4895"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "ee43d446d24f0d2bdcbed79675723d60ce40dc884f727fe66e93a4adee84fda8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "ab623d802989bc87f67cf8942c822959d46ea34d92d8a9bcedebe1eec456ae76"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "ccc5cde490041c4086e9f076482ae31af2334d3d0132162109bb021aed490ae3"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "9f42752e917cd5cf9a77abffaf4ad4648ed79dac4a5354a9f1c287388b3a6f87"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "0244903cb9ac5f49cf5fc7e5c1ea48117c04ee84082d7d61bd1b3d526b5957b6"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "1dd8521c61703b03143386b81923cb8840d4777fdc65d2ae1ce40c63728ea1d6"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "1aa70c0fd03120ae45b53da26fc11f7cd81c6b6255031683e92ce0c7365e0d8e"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "f34458187fdd9585bba2d58ed8d2533d79b0ea45e9c27f3e70141df27a40eb85"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "1799a1e768ac3b121fe93084f901bc869827b63ea3da2111f074f9a56fe50080"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "d7c5a85a690218456d9d9b3b0ce19c4b47d108cba4665fb474343c9af68db6d4"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        }
      ],
      "scope_sha256": "4949ca6b56599220219cfe4da1a57f8620b11118cf1c81c80acedd944e6d7b3c",
      "red": {
        "mutation": "Insert a duplicate frame-prefix implementation under another function name.",
        "baseline_exit": 0,
        "mutated_exit": 1,
        "restored_exit": 0,
        "expected_diagnostic": "ERROR: jscpd found too many duplicates (7.0%) over threshold (0.0%)",
        "observed_diagnostic": "ERROR: jscpd found too many duplicates (7.0%) over threshold (0.0%)",
        "before": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
          }
        ],
        "mutated": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "21406acfb851f1a583b2a8e742638e4eb5e5f367c7195ef607eb5862fcd8d6df"
          }
        ],
        "after": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
          }
        ]
      }
    },
    {
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
      "reason": null,
      "id": "EV-RED-ADDRESS-SANITIZER",
      "acceptance": [
        "AC-GATES"
      ],
      "kind": "red",
      "status": "pass",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "C:\\Users\\Randy\\Coding\\meta_g_link_direct\\scripts\\Invoke-Project.ps1",
        "test:asan"
      ],
      "method": "Executed as part of the complete maintained drill set inside the successful package aggregate; reviewed the intended diagnostic and independent assertion or scanner mechanism.",
      "exit_code": 0,
      "artifact": {
        "path": "reports/red/address-sanitizer.txt",
        "sha256": "537d18b20f17cd3ebf3c8c1b76c7824c976514202361c4427adbfc4f5470f436"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "ee43d446d24f0d2bdcbed79675723d60ce40dc884f727fe66e93a4adee84fda8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "ab623d802989bc87f67cf8942c822959d46ea34d92d8a9bcedebe1eec456ae76"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "ccc5cde490041c4086e9f076482ae31af2334d3d0132162109bb021aed490ae3"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "9f42752e917cd5cf9a77abffaf4ad4648ed79dac4a5354a9f1c287388b3a6f87"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "0244903cb9ac5f49cf5fc7e5c1ea48117c04ee84082d7d61bd1b3d526b5957b6"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "1dd8521c61703b03143386b81923cb8840d4777fdc65d2ae1ce40c63728ea1d6"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "1aa70c0fd03120ae45b53da26fc11f7cd81c6b6255031683e92ce0c7365e0d8e"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "f34458187fdd9585bba2d58ed8d2533d79b0ea45e9c27f3e70141df27a40eb85"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "1799a1e768ac3b121fe93084f901bc869827b63ea3da2111f074f9a56fe50080"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "d7c5a85a690218456d9d9b3b0ce19c4b47d108cba4665fb474343c9af68db6d4"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        }
      ],
      "scope_sha256": "4949ca6b56599220219cfe4da1a57f8620b11118cf1c81c80acedd944e6d7b3c",
      "red": {
        "mutation": "Inject a runtime heap-buffer read past a one-element allocation in test code.",
        "baseline_exit": 0,
        "mutated_exit": 1,
        "restored_exit": 0,
        "expected_diagnostic": "==8120==ERROR: AddressSanitizer: heap-buffer-overflow on address 0x120083ca0034 at pc 0x7ff736b753de bp 0x00441939f800 sp 0x00441939f808",
        "observed_diagnostic": "==8120==ERROR: AddressSanitizer: heap-buffer-overflow on address 0x120083ca0034 at pc 0x7ff736b753de bp 0x00441939f800 sp 0x00441939f808",
        "before": [
          {
            "path": "tests/sdu_tests.cpp",
            "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
          }
        ],
        "mutated": [
          {
            "path": "tests/sdu_tests.cpp",
            "sha256": "f0b8a6f71a8f96f071d86024029c536756dd654d9beaea73b9a22d9535147d2e"
          }
        ],
        "after": [
          {
            "path": "tests/sdu_tests.cpp",
            "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
          }
        ]
      }
    },
    {
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
      "reason": null,
      "id": "EV-RED-DEPENDENCY-NPM",
      "acceptance": [
        "AC-GATES"
      ],
      "kind": "red",
      "status": "pass",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "C:\\Users\\Randy\\Coding\\meta_g_link_direct\\scripts\\Invoke-Project.ps1",
        "security:npm"
      ],
      "method": "Executed as part of the complete maintained drill set inside the successful package aggregate; reviewed the intended diagnostic and independent assertion or scanner mechanism.",
      "exit_code": 0,
      "artifact": {
        "path": "reports/red/dependency-npm.txt",
        "sha256": "02d6d1b6cfe118449d8746927bd3e88e5cd96acd688c11dee6c8ce730f364c6b"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "ee43d446d24f0d2bdcbed79675723d60ce40dc884f727fe66e93a4adee84fda8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "ab623d802989bc87f67cf8942c822959d46ea34d92d8a9bcedebe1eec456ae76"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "ccc5cde490041c4086e9f076482ae31af2334d3d0132162109bb021aed490ae3"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "9f42752e917cd5cf9a77abffaf4ad4648ed79dac4a5354a9f1c287388b3a6f87"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "0244903cb9ac5f49cf5fc7e5c1ea48117c04ee84082d7d61bd1b3d526b5957b6"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "1dd8521c61703b03143386b81923cb8840d4777fdc65d2ae1ce40c63728ea1d6"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "1aa70c0fd03120ae45b53da26fc11f7cd81c6b6255031683e92ce0c7365e0d8e"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "f34458187fdd9585bba2d58ed8d2533d79b0ea45e9c27f3e70141df27a40eb85"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "1799a1e768ac3b121fe93084f901bc869827b63ea3da2111f074f9a56fe50080"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "d7c5a85a690218456d9d9b3b0ce19c4b47d108cba4665fb474343c9af68db6d4"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        }
      ],
      "scope_sha256": "4949ca6b56599220219cfe4da1a57f8620b11118cf1c81c80acedd944e6d7b3c",
      "red": {
        "mutation": "Temporarily add vulnerable lodash 4.17.20 metadata to the lockfile without installing it.",
        "baseline_exit": 0,
        "mutated_exit": 1,
        "restored_exit": 0,
        "expected_diagnostic": "Command Injection in lodash - https://github.com/advisories/GHSA-35jh-r3h4-6jhm",
        "observed_diagnostic": "Command Injection in lodash - https://github.com/advisories/GHSA-35jh-r3h4-6jhm",
        "before": [
          {
            "path": "package-lock.json",
            "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
          }
        ],
        "mutated": [
          {
            "path": "package-lock.json",
            "sha256": "55a94e0ca609f9c43263164d98163a9057daf13c65dd980eccc570ad31f8c11f"
          }
        ],
        "after": [
          {
            "path": "package-lock.json",
            "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
          }
        ]
      }
    },
    {
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
      "reason": null,
      "id": "EV-RED-DEPENDENCY-OSV",
      "acceptance": [
        "AC-GATES"
      ],
      "kind": "red",
      "status": "pass",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "C:\\Users\\Randy\\Coding\\meta_g_link_direct\\scripts\\Invoke-Project.ps1",
        "security:osv"
      ],
      "method": "Executed as part of the complete maintained drill set inside the successful package aggregate; reviewed the intended diagnostic and independent assertion or scanner mechanism.",
      "exit_code": 0,
      "artifact": {
        "path": "reports/red/dependency-osv.txt",
        "sha256": "0f4ec7667ba7545976e6f4b89b37630ae4c4a9c839353177cbb935a04b1b161b"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "ee43d446d24f0d2bdcbed79675723d60ce40dc884f727fe66e93a4adee84fda8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "ab623d802989bc87f67cf8942c822959d46ea34d92d8a9bcedebe1eec456ae76"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "ccc5cde490041c4086e9f076482ae31af2334d3d0132162109bb021aed490ae3"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "9f42752e917cd5cf9a77abffaf4ad4648ed79dac4a5354a9f1c287388b3a6f87"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "0244903cb9ac5f49cf5fc7e5c1ea48117c04ee84082d7d61bd1b3d526b5957b6"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "1dd8521c61703b03143386b81923cb8840d4777fdc65d2ae1ce40c63728ea1d6"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "1aa70c0fd03120ae45b53da26fc11f7cd81c6b6255031683e92ce0c7365e0d8e"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "f34458187fdd9585bba2d58ed8d2533d79b0ea45e9c27f3e70141df27a40eb85"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "1799a1e768ac3b121fe93084f901bc869827b63ea3da2111f074f9a56fe50080"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "d7c5a85a690218456d9d9b3b0ce19c4b47d108cba4665fb474343c9af68db6d4"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        }
      ],
      "scope_sha256": "4949ca6b56599220219cfe4da1a57f8620b11118cf1c81c80acedd944e6d7b3c",
      "red": {
        "mutation": "Temporarily add vulnerable lodash 4.17.20 metadata to the lockfile without installing it.",
        "baseline_exit": 0,
        "mutated_exit": 1,
        "restored_exit": 0,
        "expected_diagnostic": "| https://osv.dev/GHSA-35jh-r3h4-6jhm | 8.1  | npm       | lodash (dev) | 4.17.20 | 4.17.21       | package-lock.json |",
        "observed_diagnostic": "| https://osv.dev/GHSA-35jh-r3h4-6jhm | 8.1  | npm       | lodash (dev) | 4.17.20 | 4.17.21       | package-lock.json |",
        "before": [
          {
            "path": "package-lock.json",
            "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
          }
        ],
        "mutated": [
          {
            "path": "package-lock.json",
            "sha256": "55a94e0ca609f9c43263164d98163a9057daf13c65dd980eccc570ad31f8c11f"
          }
        ],
        "after": [
          {
            "path": "package-lock.json",
            "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
          }
        ]
      }
    },
    {
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
      "reason": null,
      "id": "EV-RED-WHATIF-GUARD",
      "acceptance": [
        "AC-GATES"
      ],
      "kind": "red",
      "status": "pass",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "C:\\Users\\Randy\\Coding\\meta_g_link_direct\\scripts\\Invoke-Project.ps1",
        "test:runner"
      ],
      "method": "Executed as part of the complete maintained drill set inside the successful package aggregate; reviewed the intended diagnostic and independent assertion or scanner mechanism.",
      "exit_code": 0,
      "artifact": {
        "path": "reports/red/whatif-guard.txt",
        "sha256": "37f13af9529fe320c7d2027a04ff846aae24a13df23399ae7b1fe1dae8788b24"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "ee43d446d24f0d2bdcbed79675723d60ce40dc884f727fe66e93a4adee84fda8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "ab623d802989bc87f67cf8942c822959d46ea34d92d8a9bcedebe1eec456ae76"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "ccc5cde490041c4086e9f076482ae31af2334d3d0132162109bb021aed490ae3"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "9f42752e917cd5cf9a77abffaf4ad4648ed79dac4a5354a9f1c287388b3a6f87"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "0244903cb9ac5f49cf5fc7e5c1ea48117c04ee84082d7d61bd1b3d526b5957b6"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "1dd8521c61703b03143386b81923cb8840d4777fdc65d2ae1ce40c63728ea1d6"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "1aa70c0fd03120ae45b53da26fc11f7cd81c6b6255031683e92ce0c7365e0d8e"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "f34458187fdd9585bba2d58ed8d2533d79b0ea45e9c27f3e70141df27a40eb85"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "1799a1e768ac3b121fe93084f901bc869827b63ea3da2111f074f9a56fe50080"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "d7c5a85a690218456d9d9b3b0ce19c4b47d108cba4665fb474343c9af68db6d4"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        }
      ],
      "scope_sha256": "4949ca6b56599220219cfe4da1a57f8620b11118cf1c81c80acedd944e6d7b3c",
      "red": {
        "mutation": "Bypass the common ShouldProcess guard; the disposable WhatIf effect assertion must fail.",
        "baseline_exit": 0,
        "mutated_exit": 1,
        "restored_exit": 0,
        "expected_diagnostic": "Expected strings to be the same, but they were different.",
        "observed_diagnostic": "Expected strings to be the same, but they were different.",
        "before": [
          {
            "path": "scripts/Invoke-Project.ps1",
            "sha256": "f34458187fdd9585bba2d58ed8d2533d79b0ea45e9c27f3e70141df27a40eb85"
          }
        ],
        "mutated": [
          {
            "path": "scripts/Invoke-Project.ps1",
            "sha256": "f984215ef641a3fd7cbf07a4a75c8a0ca293cc75a2e510623b9872acea52f9ee"
          }
        ],
        "after": [
          {
            "path": "scripts/Invoke-Project.ps1",
            "sha256": "f34458187fdd9585bba2d58ed8d2533d79b0ea45e9c27f3e70141df27a40eb85"
          }
        ]
      }
    },
    {
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
      "reason": null,
      "id": "EV-RED-NATIVE-EXIT-PROPAGATION",
      "acceptance": [
        "AC-GATES"
      ],
      "kind": "red",
      "status": "pass",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "C:\\Users\\Randy\\Coding\\meta_g_link_direct\\scripts\\Invoke-Project.ps1",
        "test:runner"
      ],
      "method": "Executed as part of the complete maintained drill set inside the successful package aggregate; reviewed the intended diagnostic and independent assertion or scanner mechanism.",
      "exit_code": 0,
      "artifact": {
        "path": "reports/red/native-exit-propagation.txt",
        "sha256": "75d8d8a9c5714edc20a16f9b9472d43f46e7a57a0288593186708f32cf88c81c"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "ee43d446d24f0d2bdcbed79675723d60ce40dc884f727fe66e93a4adee84fda8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "ab623d802989bc87f67cf8942c822959d46ea34d92d8a9bcedebe1eec456ae76"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "ccc5cde490041c4086e9f076482ae31af2334d3d0132162109bb021aed490ae3"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "9f42752e917cd5cf9a77abffaf4ad4648ed79dac4a5354a9f1c287388b3a6f87"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "0244903cb9ac5f49cf5fc7e5c1ea48117c04ee84082d7d61bd1b3d526b5957b6"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "1dd8521c61703b03143386b81923cb8840d4777fdc65d2ae1ce40c63728ea1d6"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "1aa70c0fd03120ae45b53da26fc11f7cd81c6b6255031683e92ce0c7365e0d8e"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "f34458187fdd9585bba2d58ed8d2533d79b0ea45e9c27f3e70141df27a40eb85"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "1799a1e768ac3b121fe93084f901bc869827b63ea3da2111f074f9a56fe50080"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "d7c5a85a690218456d9d9b3b0ce19c4b47d108cba4665fb474343c9af68db6d4"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        }
      ],
      "scope_sha256": "4949ca6b56599220219cfe4da1a57f8620b11118cf1c81c80acedd944e6d7b3c",
      "red": {
        "mutation": "Bypass the native exit-code guard; the real formatter-failure assertion must fail.",
        "baseline_exit": 0,
        "mutated_exit": 1,
        "restored_exit": 0,
        "expected_diagnostic": "Expected 0 to be different from the actual value, but got the same value.",
        "observed_diagnostic": "Expected 0 to be different from the actual value, but got the same value.",
        "before": [
          {
            "path": "scripts/Invoke-Project.ps1",
            "sha256": "f34458187fdd9585bba2d58ed8d2533d79b0ea45e9c27f3e70141df27a40eb85"
          }
        ],
        "mutated": [
          {
            "path": "scripts/Invoke-Project.ps1",
            "sha256": "6317091c63172acbb754e1704c06ba88dc870ecaea9805583166c6dfdf6eed7f"
          }
        ],
        "after": [
          {
            "path": "scripts/Invoke-Project.ps1",
            "sha256": "f34458187fdd9585bba2d58ed8d2533d79b0ea45e9c27f3e70141df27a40eb85"
          }
        ]
      }
    },
    {
      "id": "EV-PORTABLE-COVERAGE-DEFERRED",
      "acceptance": [
        "AC-GATES"
      ],
      "kind": "behavior",
      "status": "deferred",
      "command": [],
      "method": "Boundary review; this gate has not run and is not counted as a passing check.",
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
        "path": "reports/VERIFICATION.md",
        "sha256": "4b1c390345a4e2bb445ffd96faaca26b9b5226190e2368420f9c0023f41965a2"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "ee43d446d24f0d2bdcbed79675723d60ce40dc884f727fe66e93a4adee84fda8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "ab623d802989bc87f67cf8942c822959d46ea34d92d8a9bcedebe1eec456ae76"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "ccc5cde490041c4086e9f076482ae31af2334d3d0132162109bb021aed490ae3"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "9f42752e917cd5cf9a77abffaf4ad4648ed79dac4a5354a9f1c287388b3a6f87"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "0244903cb9ac5f49cf5fc7e5c1ea48117c04ee84082d7d61bd1b3d526b5957b6"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "1dd8521c61703b03143386b81923cb8840d4777fdc65d2ae1ce40c63728ea1d6"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "1aa70c0fd03120ae45b53da26fc11f7cd81c6b6255031683e92ce0c7365e0d8e"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "f34458187fdd9585bba2d58ed8d2533d79b0ea45e9c27f3e70141df27a40eb85"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "1799a1e768ac3b121fe93084f901bc869827b63ea3da2111f074f9a56fe50080"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "d7c5a85a690218456d9d9b3b0ce19c4b47d108cba4665fb474343c9af68db6d4"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        }
      ],
      "scope_sha256": "4949ca6b56599220219cfe4da1a57f8620b11118cf1c81c80acedd944e6d7b3c",
      "red": null,
      "reason": "Owner: technical implementation. No portable coverage baseline/floor, fuzz or hosted/Linux sanitizer result exists. Next: establish supported portable-core execution before session/media parser certification; gcovr does not cover the current MSVC build. No calendar deadline supplied."
    },
    {
      "id": "EV-NATIVE-AUDIT-DEFERRED",
      "acceptance": [
        "AC-GATES"
      ],
      "kind": "behavior",
      "status": "deferred",
      "command": [],
      "method": "Boundary review; this gate has not run and is not counted as a passing check.",
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
        "path": "reports/VERIFICATION.md",
        "sha256": "4b1c390345a4e2bb445ffd96faaca26b9b5226190e2368420f9c0023f41965a2"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "ee43d446d24f0d2bdcbed79675723d60ce40dc884f727fe66e93a4adee84fda8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "ab623d802989bc87f67cf8942c822959d46ea34d92d8a9bcedebe1eec456ae76"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "ccc5cde490041c4086e9f076482ae31af2334d3d0132162109bb021aed490ae3"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "9f42752e917cd5cf9a77abffaf4ad4648ed79dac4a5354a9f1c287388b3a6f87"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "0244903cb9ac5f49cf5fc7e5c1ea48117c04ee84082d7d61bd1b3d526b5957b6"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "1dd8521c61703b03143386b81923cb8840d4777fdc65d2ae1ce40c63728ea1d6"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "1aa70c0fd03120ae45b53da26fc11f7cd81c6b6255031683e92ce0c7365e0d8e"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "f34458187fdd9585bba2d58ed8d2533d79b0ea45e9c27f3e70141df27a40eb85"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "1799a1e768ac3b121fe93084f901bc869827b63ea3da2111f074f9a56fe50080"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "d7c5a85a690218456d9d9b3b0ce19c4b47d108cba4665fb474343c9af68db6d4"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        }
      ],
      "scope_sha256": "4949ca6b56599220219cfe4da1a57f8620b11118cf1c81c80acedd944e6d7b3c",
      "red": null,
      "reason": "Owner: technical/security implementation. npm/OSV cover the locked development packages, not a complete native OBS/dependency component audit. Next: inventory native components and assess advisories before accepting authentication/media dependencies. No calendar deadline supplied."
    }
  ],
  "checkpoint": {
    "scope_sha256": "bea9f9ba2d7cbe474ed1b216198e3b6f35e9fab8d4f128c7cc6a34c55aa08154",
    "inputs": [
      {
        "path": ".clang-format",
        "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
      },
      {
        "path": ".clang-tidy",
        "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
      },
      {
        "path": ".cppcheck-obs.cfg",
        "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
      },
      {
        "path": ".gitattributes",
        "sha256": "ee43d446d24f0d2bdcbed79675723d60ce40dc884f727fe66e93a4adee84fda8"
      },
      {
        "path": ".github/dependabot.yml",
        "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
      },
      {
        "path": ".github/workflows/quality.yml",
        "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
      },
      {
        "path": ".gitignore",
        "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
      },
      {
        "path": ".gitleaks.toml",
        "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
      },
      {
        "path": ".jscpd.json",
        "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
      },
      {
        "path": ".pre-commit-config.yaml",
        "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
      },
      {
        "path": ".semgrep.yml",
        "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
      },
      {
        "path": "AGENTS.md",
        "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
      },
      {
        "path": "CHANGELOG.md",
        "sha256": "ab623d802989bc87f67cf8942c822959d46ea34d92d8a9bcedebe1eec456ae76"
      },
      {
        "path": "CMakeLists.txt",
        "sha256": "ccc5cde490041c4086e9f076482ae31af2334d3d0132162109bb021aed490ae3"
      },
      {
        "path": "CMakePresets.json",
        "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
      },
      {
        "path": "LICENSE",
        "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
      },
      {
        "path": "PROJECT_BRIEF.md",
        "sha256": "9f42752e917cd5cf9a77abffaf4ad4648ed79dac4a5354a9f1c287388b3a6f87"
      },
      {
        "path": "PSScriptAnalyzerSettings.psd1",
        "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
      },
      {
        "path": "README.md",
        "sha256": "0244903cb9ac5f49cf5fc7e5c1ea48117c04ee84082d7d61bd1b3d526b5957b6"
      },
      {
        "path": "THIRD_PARTY_NOTICES.md",
        "sha256": "1dd8521c61703b03143386b81923cb8840d4777fdc65d2ae1ce40c63728ea1d6"
      },
      {
        "path": "data/locale/en-US.ini",
        "sha256": null
      },
      {
        "path": "dependencies.lock.json",
        "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
      },
      {
        "path": "include/mgl/capabilities.hpp",
        "sha256": "1aa70c0fd03120ae45b53da26fc11f7cd81c6b6255031683e92ce0c7365e0d8e"
      },
      {
        "path": "include/mgl/sdu.hpp",
        "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
      },
      {
        "path": "include/mgl/session.hpp",
        "sha256": null
      },
      {
        "path": "include/mgl/stream.hpp",
        "sha256": null
      },
      {
        "path": "package-lock.json",
        "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
      },
      {
        "path": "package.json",
        "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
      },
      {
        "path": "scripts/Invoke-Project.ps1",
        "sha256": "f34458187fdd9585bba2d58ed8d2533d79b0ea45e9c27f3e70141df27a40eb85"
      },
      {
        "path": "src/media/decoder.cpp",
        "sha256": null
      },
      {
        "path": "src/media/stream.cpp",
        "sha256": null
      },
      {
        "path": "src/plugin/glasses_source.cpp",
        "sha256": null
      },
      {
        "path": "src/plugin/module.c",
        "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
      },
      {
        "path": "src/probe/main.cpp",
        "sha256": "1799a1e768ac3b121fe93084f901bc869827b63ea3da2111f074f9a56fe50080"
      },
      {
        "path": "src/protocol/sdu.cpp",
        "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
      },
      {
        "path": "src/windows/capabilities.cpp",
        "sha256": "d7c5a85a690218456d9d9b3b0ce19c4b47d108cba4665fb474343c9af68db6d4"
      },
      {
        "path": "src/windows/identity.cpp",
        "sha256": null
      },
      {
        "path": "src/windows/session.cpp",
        "sha256": null
      },
      {
        "path": "src/windows/wifi_group.cpp",
        "sha256": null
      },
      {
        "path": "tests/module_tests.cpp",
        "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
      },
      {
        "path": "tests/runner.Tests.ps1",
        "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
      },
      {
        "path": "tests/sdu_tests.cpp",
        "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
      },
      {
        "path": "tests/session_tests.cpp",
        "sha256": null
      },
      {
        "path": "tests/source_tests.cpp",
        "sha256": null
      },
      {
        "path": "tests/stream_tests.cpp",
        "sha256": null
      }
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
    },
    "verified_tasks": [
      "TASK-SETUP"
    ],
    "pending_operations": [],
    "next_action": "Begin TASK-AUTH from the confirmed brief: investigate direct Windows discovery/transport and a reproducible authorized Meta ownership/application-identity path, then real device information and a first camera frame with the phone off. The available adapter must be connected for actual radio/media proof; do not restart the product interview or request adapter model again. Keep coverage/native-audit and all current-firmware/OBS/120-minute gates open.",
    "source_revision": "6959a50efcbc758757a716507080bac7066482a2"
  }
}
```

