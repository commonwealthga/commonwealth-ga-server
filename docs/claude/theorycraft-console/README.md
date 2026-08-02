# Theorycraft console

Everything from the `theorycraft-console` branch lives in this folder. Two related pieces:

1. **[project-status.md](project-status.md)** — **start here.** What is settled, what is still
   open, coverage, and where to look. Written as session-start orientation.
2. **[backlog.md](backlog.md)** — what to work on next. Stable item ids so they can be
   referenced directly.
3. **[damage-pipeline.md](damage-pipeline.md)** — the research. How damage, protection and
   effects actually resolve, §0–§20. The authority for every rule the console implements.
4. **[rules-and-provenance.md](rules-and-provenance.md)** — what follows a game change on its
   own, what needs a code change, and what each rule is actually based on. Read this before
   changing a combat rule server-side.

| file | what |
|---|---|
| `project-status.md` | Orientation: settled mechanics, open questions, next steps. |
| `backlog.md` | The to-do list, with stable ids. |
| `damage-pipeline.md` | The reverse-engineering write-up. |
| `rules-and-provenance.md` | Data vs code vs provenance. The drift-hazard list. |
| `device-console.html` | The generated console (~3 MB, self-contained). **Generated — do not hand-edit.** |
| `assets/device-icons/` | 115 device images, keyed by `device_id`. |
| `issues/` | Write-ups raised with the wider project. |

The console is built by the generators in [`tools/device-console/`](../../../tools/device-console) —
see that README for the build order. Open `device-console.html` directly in a browser; it needs no
server and pulls nothing from the network.
