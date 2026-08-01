# Device console generators

Builds `docs/claude/device-console.html` — the loadout / character / skill-tree console used for
damage and protection theory-crafting. The published page is a single self-contained HTML file;
these scripts are what produce it.

## Inputs

- **`E:\GA_LOCAL\gaa.db`** — authoritative named ASM data (the path is hard-coded in each
  generator; change it there if the db moves). Read-only.
- **`E:\GA_BROADCAST_ICONS\ICONS\SKILLS\*.png`** — skill-tree node icons, keyed by `icon_id`.
- **`docs/claude/assets/device-icons/<device_id>.png`** — device artwork, collected by hand via
  `gen_icons.py` (see below).

## Build order

The generators pass data to each other through JSON files written next to the scripts, so order
matters:

```
python gen_ix.py      # skill -> device interaction resolver   -> ix.json, ixraw.json, skilldev.json, devmeta.json
python gen_tree.py    # skill trees, icons, base stats, armour -> tree.json
python gen2.py        # inventory device model (modes, chips)  -> inv_model.json
python gen_char.py    # real character builds for user 2381    -> chars.json
python gen3.py        # renders the final HTML                 -> docs/claude/device-console.html
```

`gen3.py` must run last — it consumes every other output plus `bench.js`, `builder.js`,
`app.js`, `style2.css` and `deviceimg.json`.

## The pieces

| file | role |
|---|---|
| `gen_ix.py` | Resolves which skills affect which devices, with the numeric effects and the gate that links them. Emits both a merged view (display) and an unmerged one (`ixraw.json`, used by the resolver so a skill's passive and conditional halves stay distinct). |
| `gen_tree.py` | Skill-tree layout, node icons, prerequisites, base player stats, armour configs. |
| `gen2.py` | Turns the level-50 inventory into a device model: fire modes, effect chips with numeric backing, rolled-mod signatures. |
| `gen_char.py` | Real saved builds (skills + equipped devices + armour) per character and item profile. |
| `gen3.py` | Renders everything into one HTML file and inlines the JS/CSS. |
| `bench.js` | `GA.resolve` — the stat resolver. Standalone and side-effect free. Also `GA.playerEffects`, `GA.applyStacking`, `GA.deviceChipsHTML`. |
| `builder.js` | Skill-tree builder, character loader, equipped-slot toggles, My Player sheet. |
| `app.js` | Loadout tab switching. |

The one-off icon-collection tooling has been moved to `retired/` now the artwork is collected —
see the README there if it ever needs redoing.

## Regenerating device artwork

`docs/claude/assets/device-icons/*.png` were collected by hand. To rebuild the inlined copy the
page uses, decode those PNGs into `deviceimg.json` as `{device_id: dataURL}` — `gen3.py` reads
that file and injects it as `window.__DEVIMG__`. The images are downscaled to 56px before
inlining to keep the page near 3MB rather than 10MB.

## Notes

- The model these scripts encode is documented in `docs/claude/damage-pipeline.md` — read
  §15–§20 before changing any of the resolver logic.
- Nothing here writes to the game database.
