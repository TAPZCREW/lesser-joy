# AGENTS.md

Guidelines for AI agents and contributors working in this repository.

## Golden rule: never edit source files directly

**An AI agent must never write to, rename, move, or delete any source file.** This covers
everything under `driver/`, `cli-frontend/`, `scripts/`, `xmake/`, the root `xmake.lua`,
`packages.config`, `.clang-format`, `.stylua.toml`, and every `.cpp`, `.cppm`, `.hpp`,
`.inx`, `.ps1`, `.bat`, `.yml` file in the repository. Read them freely — only never
modify them in place.

All proposed changes go to the dedicated folder **`proposals/`** instead:

1. Write your change as a self-contained artifact under `proposals/`:
   - a unified diff (`proposals/<short-description>.patch`) applying against the current
     sources, **or**
   - a complete replacement file at the mirrored path
     (`proposals/driver/src/hid/foo_commands.cppm`), **or**
   - a short design note (`proposals/<short-description>.md`) when the change is not yet
     concrete enough for a patch.
2. Prefix the file name with the date or a short slug so parallel proposals do not collide
   (`2026-10-19-vibration-sample-0x02.patch`).
3. In your final message, list the files you created under `proposals/` and what the user
   must do to apply them (e.g. `git apply proposals/xxx.patch`, or copy file X over Y).
4. **Wait.** Do not apply the patch yourself, do not run a build to "verify" a change you
   made to the real tree, and do not touch `build/`, `.xmake/`, `packages/`.

Only the human user applies a proposal to the real sources. After they have applied it,
they may ask you to review the result — reviewing and reading remain allowed at all times.

Everything below documents the conventions your proposals must follow so that they can be
applied as-is.

## Project overview

`lesserjoy` is a **Windows UMDF HID driver** (plus a small CLI frontend) that enable support of the **Nintendo Switch 2 Pro Controller** on Windows (`VID_057E` / `PID_2069`).

- Pure Windows project: WDK/UMDF, `pnputil`, code signing. It does **not** build or run on Linux.
- The HID command protocol is reverse engineered. Reference documentation lives at
  <https://github.com/ndeadly/switch2_controller_research/blob/master/commands.md> and is
  linked from the sources via `// @see` comments. Keep those comments when editing.

## Repository layout

```
xmake.lua                  root build script: rules, options, dependencies
xmake/StormKit.xmake.lua   local StormKit override (used with --stormkit=<path>)
packages.config            NuGet manifest (WDK + Windows SDK, restored into packages/)
driver/                    the UMDF driver target `lesserjoy-driver`
  xmake.lua                WDK rules, signing, driver install/run rule
  lesserjoy-driver.inx     INF template (compiled to .inf by the WDK build)
  lesserjoy-driver.mof
  src/                     C++26 module sources (see "Code layout")
cli-frontend/              target `lesserjoy-cli` (placeholder `main.cpp`)
scripts/gen_certificate.ps1  generates and trusts the self-signed `lesserjoy` cert
.github/workflows/         CI: windows-msvc.yml, windows-llvm.yml (push/PR on `develop`)
build/, .xmake/            build output and xmake caches (generated, never edit)
```

There is **no** `qt-frontend/` directory, so configuring with `--gui=y` fails. Always pass
`--gui=n` unless the directory has been added.

## Build system

Everything goes through **xmake** (a patched fork: `Arthapz/xmake#branch@future_prs`).

```sh
# configure (flags are also cached in .xmake_configure_flags / .xmake_build_flags)
xmake f -c -y \
  --toolchain=llvm --sdk=F:/llvm \
  -m release \
  --devmode=y --sanitizers=true \
  --cli=n --gui=n

xmake b            # build all enabled targets
xmake b lesserjoy-driver   # build a single target
xmake run lesserjoy-driver # devmode only: sign + install via pnputil (needs sudo)
xmake run lesserjoy-cli    # runs with sudo
xmake test -v      # CI runs this; there are currently no test targets
xmake f -c         # reconfigure from scratch (use after changing xmake.lua files)
```

Useful options (see the root `xmake.lua`):

| Option | Default | Purpose |
| ------ | ------- | ------- |
| `devmode` | off | test-signing (`wdk.sign.mode=test`), cert generation, `pnputil` install on run |
| `sanitizers` | off | force-enabled by `devmode` |
| `lto` | on | force-disabled by `devmode` |
| `stormkit` | unset | path to a local StormKit checkout instead of the packaged one |
| `compile_commands` | off | generate `build/compile_commands.json` for clangd |
| `cli` / `gui` | on | build the frontends (keep `gui=n`) |
| `wdk` | `C:/Program Files (x86)/Windows kits/10` | WDK location |
| `on_ci`, `vsxmake`, `mold` | off | CI / IDE / linker helpers |

Requirements: `nuget` (restores the WDK into `packages/` on first configure), `pwsh`
(cert generation), `pnputil`, and `sudo` for devmode signing/installation.

`xmake` scripts are formatted with **Stylua** (4 spaces, 120 columns, `.stylua.toml`);
Lua completion is set up through `.luarc.json` + the xmake LuaLS addon.

## CI

Two workflows, both on `windows-2025` for pushes/PRs to `develop`:

- `windows-msvc.yml` — MSVC
- `windows-llvm.yml` — LLVM with `libc++` and `msstl` matrices (`--toolchain=llvm`)

Steps are `xmake f ...` then `xmake b` then `xmake test -v`. A commit message containing
`ci skip` bypasses CI. Match the CI configure flags when reproducing a failure locally.

## Code layout (driver)

C++26 named modules throughout; headers are essentially gone.

- Primary module `lesserjoy` (`driver/src/lesserjoy.cpp`, uses `module: private;`) with
  module partitions:
  - `:common`, `:constants`, `:driver`, `:device`, `:log`, `:hid`, `:wdf`
  - `:hid.<name>_commands` (`init`, `leds`, `vibration`, `feature_select`,
    `bluetooth_pairing`, `unknown_0xNN`, `command_ids`)
  - `:transport.usb`
- Non-exported library modules: `wdk.*` (`wdk.wdf`, `wdk.hid`, `wdk.usb`, `wdk.ntstatus`)
  in `driver/src/wdk/`, wrapping WDK/WDF APIs.
- Global module fragment (`module;` + `#include`) is only for macros and headers that
  cannot be imported (e.g. `wdk/wdfobject_macros.hpp`).
- Dependencies imported as modules: `std`, `frozen`, `stormkit.*`.
- Interface units end in `.cppm`, implementation units in `.cpp`. Each partition gets its
  own file; add new HID command groups as a new `driver/src/hid/*_commands.cppm` plus an
  `export import` line in `driver/src/hid.cppm`.

### Conventions

- Namespace is `lj` (sub-namespaces `lj::hid`, `lj::usb`, `lj::ble`, `lj::wdk`).
- `snake_case` for everything: functions, variables, types, files. Enum values too
  (`play_sample`, `unknown_0x01`).
- Trailing return type: `auto foo(int a) noexcept -> bool;`.
- `noexcept` on anything that cannot throw; kernel code must not allocate implicitly.
- Alternate operator tokens are used: `not`, `and`, `or`.
- StormKit aliases are in scope after `using namespace stormkit;`
  (`u8`, `usize`, `array`, `array_view`, `byte`, `locked`, `heap_ptr`, `as<>`, `into<>`, ...).
  Prefer them over `std::uint8_t` / `std::span` in driver code.
- Contracts: `expects(...)` / `ensures(...)` functions from `stormkit.core.contract`, `EXPECTS(...)` / `ENSURES(...)` macros from `<stormkit/core/contract_macros.hpp>`
- Formatting macros: `STORMKIT_FORCE_INLINE`, `STORMKIT_PUSH_WARNINGS` / `STORMKIT_POP_WARNINGS`.
- Logging: `lj::ilog/wlog/elog/dlog/flog` with `std::format` strings (see `:log`).
  Never `printf`/`std::cout` in driver code — output goes through `OutputDebugStringA`.
- The two blank comment separators (`//////////`) and the
  `////// IMPLEMENTATION //////` banner are the house style for splitting declarations
  from definitions inside a single file — keep them.
- `unknown_0xNN` naming for commands/subcommands that are not yet reverse engineered;
  add a `// @see <commands.md anchor>` comment when the meaning is known.

### Safety rules for driver code

- This code runs in a UMDF host process but drives real hardware: never assume a USB/HID
  call succeeded, check every `NTSTATUS`, and route failures through `CustomLoggedTryOr`
  / `lj::elog`.
- Report and payload sizes are compile-time constants (`input_report_size`,
  `output_report_size`, `command_payload_length`). Changing them requires re-checking the
  HID report descriptor in `driver/src/constants.cppm`.
- Do not edit generated outputs under `build/` (`*.inf`, `*.cat`, `*.dll`, `*.lib`).
  The `.inf` is generated from `driver/lesserjoy-driver.inx`.
- Driver installation/removal requires elevation and mutates the machine driver store —
  never run `xmake run lesserjoy-driver`, `pnputil`, or `scripts/gen_certificate.ps1`
  without the user asking for it.

## Formatting

- C++: `clang-format` with the repository `.clang-format` (LLVM base, 4-space indent,
  120 columns, aligned consecutive declarations/assignments/trailing comments,
  `InsertBraces: false`, aligned array-of-struct literals). Run it before committing.
- Byte arrays / descriptors may use `// clang-format off/on` blocks — preserve them.
- `.gitattributes` maps `.cppm` / `.mpp` / `.ixx` to C++ for Linguist.

## Before you finish

1. Your work lives **only** under `proposals/`. Nothing under `driver/`, `cli-frontend/`,
   `scripts/`, `xmake/`, or the root files has been modified, renamed, or deleted.
2. Any `.cpp`/`.cppm`/`.hpp` content you produce must already be `clang-format`-clean
   (`.clang-format`: LLVM base, 4-space indent, 120 columns) — you cannot run the formatter
   on the real tree, so match the style by hand.
3. Do not commit anything under `build/`, `.xmake/`, `packages/`, `.clice/`.
4. Do not run `xmake b`, `xmake run lesserjoy-driver`, `pnputil`, or
   `scripts/gen_certificate.ps1` as a side effect of a change you proposed — building and
   installing is the user's step, after they applied the proposal.
5. Report clearly: which proposal files you created, which source files they affect once
   applied, and what you could not verify (no test suite exists yet, and you cannot build
   unapplied changes).
