# Retired — one-off icon collection

These files did a single job: getting artwork associated with each device. That job is **done**
— the results are committed at `docs/claude/theorycraft-console/assets/device-icons/<device_id>.png` (115 images) and
inlined into the console.

Kept rather than deleted so the process is repeatable if artwork ever needs recollecting or
extending (new devices, better source images). Nothing in the live build depends on any of this:
`gen3.py` has no reference to it, and the only thing that reads `devicon.json` is `gen_icons.py`.

| file | role |
|---|---|
| `gen_icons.py` | Builds the collection page below. Reads `inv_model.json` for the device list. |
| `device-icons.html` | The generated page: one tile per device, click / drag / paste an image onto it. Progress is kept in `localStorage`, exported as `{device_id: dataURL}` JSON. |
| `devicons.js` | An earlier variant that lived on the loadout page itself, keyed by `icon_id`. Superseded — see the note below. |
| `devicon.json` | `device_id -> icon_id` map, used only by the import path that expands an old icon-keyed export. |

## If you ever need to redo this

1. `python gen_icons.py` — writes the collection page.
2. **Open the page from disk (`file://`), not through the published artifact.** Artifacts run in
   a sandboxed iframe that silently blocks downloads, which is why the first attempt at this
   produced nothing. Opened locally, Export gets a real "where do you want to save" dialog.
3. Collect, Export, then decode the JSON into `docs/claude/theorycraft-console/assets/device-icons/` and rebuild
   `deviceimg.json` for `gen3.py`.

## Why keyed by device_id

The first version keyed on `asm_data_set_items.icon_id`, which looked efficient — 128 devices
share only 105 distinct icon ids, so one image could fill several tiles. But those shared ids do
not reflect the artwork the game actually shows (e.g. Ballista and Dweller Sniper Rifle share
icon 820; all eight jetpacks share 265), so devices that should look different came out
identical. Keying on `device_id` gives every device its own image and sidesteps the problem
entirely. The import path still accepts the old icon-keyed format and fans it out.
