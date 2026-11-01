---
tags: [sdd, dcc-control-app, journal]
banner: "[[microchip_banner.png]]"
banner_y: 70
---

# Decisions  Journal logs 

> Short Format logs : One line per decision, in most recent order. detail are given by a link to the note reference.

- 2026-10-29
**Docs site:**  migration to Zensical a fork of MkDocs. natives features are a easier way to match with Obsidian vault sources.

- 2026-10-28
**Docs site:** Obsidian vault in `docs/` built with MkDocs Material (callouts, Mermaid) _[mkdocs.yml untested]_. The CI needs **Python ≥ 3.10**. The `ezlinks` plugin mangles names (lower-casing, dropping the dot of the extension), so a custom link-mapping hook is planned; banners are handled by the author’s own hook. Material for MkDocs is in maintenance mode, and MkDocs 2.0 breaks plugins, so pin `mkdocs<2`.

- 2026-10-18
**Debug in VS Code:** the status-bar “bug” icon belongs to CMake Tools, not MPLAB; the MPLAB adapter is `mplab-core-da`; a “ToolMediator data” error remains unresolved.

- 2026-10-13
**VsCode!** Add `tasks.json`to integrate fashing MCU with `pk2cmd`

- 2026-10-11
**Sources:** Migration to news repo organisation [02-Modular-Architecture - Repo](02-Modular-Architecture.md#repo-organisation).

- 2026-10-10
**Kick Off:** introduce project to IA (Claude/Anthropic) , Start of actual project review.
**Interface:** Fusion of Rs485 and RwnBus modules to simplify and reduce call stack. Use a specific hardware driver reserved for this project and `Starterboard`
**QueueBuffer:** remove generics functions interface for queue managing. cost too much for this project and not relevant at the end [04-Modules - RwnBus](04-Modules.md#rwnbus).
