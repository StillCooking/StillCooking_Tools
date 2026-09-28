# Verification

*Three checks — the palette, the console command, the graph node — and what each one tells you when it fails.*

The plugin adds no menu entry and no settings page. What it adds is nodes, a subsystem and one
console command — so those are what you check.

## 1. The palette

Open any actor Blueprint, right-click in the event graph and type `StillCooking`. The list shows
these categories:

- `StillCooking|Flow` — four nodes: **For Each Index With Delay**, **For Each Index Per Tick**,
  **Do For Duration**, **Wait Until**;
- `StillCooking|Console` — the console registry's functions;
- `StillCooking|Debug` — **Print String Formatted** (check 3).

The tickable object's functions, under `StillCooking|Tickable`, appear only when you drag from a
reference to one.

**Nodes not there?** The plugin is disabled, or `StillCookingCore` did not build. Check, in this
order:

1. **Edit → Plugins**, search for `StillCooking_Tools`. It has to be listed and enabled. If it is
   missing, the clone is not under `Plugins/`, or the `.uplugin` file is not directly inside
   `Plugins/StillCooking_Tools/`.
2. The build output. A plugin that fails to compile is reported there, not in the editor.
3. The editor's startup log for `LogPluginManager` lines mentioning the plugin.

## 2. The console command

Press Play, open the console with `~`, and type `SC.` — the plugin's commands and variables
group under that prefix in autocomplete. Run `SC.Debug.Ping`.

In a project with no Blueprint child of the console registry, the command reaches the registry
and does nothing visible — the body is an event nobody has implemented yet. That is the expected
result, and it still proves the runtime module started: a plugin whose module never loaded has no
`SC.` commands at all.

Run the same command **outside** Play and the log shows
[this Warning on `LogStillCooking`](../troubleshooting/diagnostics.md#uscconsolecommandregistrysubsystem):

```
SC.Ping: no game world - a statically registered command is visible from editor start, but its body lives in a Blueprint that exists only once the game or a PIE session runs.
```

The message shows `SC.` plus the command's identifier (`SC.Ping`), not its console name
(`SC.Debug.Ping`). This, too, is expected:
[a command can be visible before Play and still only execute during it](../concepts/builds-and-shipping.md#visible-is-not-executable).
Both paths are on [Console commands](../reference/console-commands.md).

## 3. The graph node

Place **Print String Formatted** in a graph and type `{A}` into its `Format` pin. An input pin
named `A` appears. Delete the closing brace and the pin disappears again.

**Node not in the palette?** The `StillCookingCoreEditor` module did not build or did not load.
It is a separate module from the runtime one, so the Flow nodes can be present while this one is
missing. Check the build output for errors in `Source/StillCookingCoreEditor/`.

The node carries the engine's dashed **Development Only** banner. That is not a fault; it is how
the graph shows you a node that is [compiled out of Shipping and Test](../concepts/builds-and-shipping.md#what-is-gone-in-shipping).

## When a check fails

**The clone is fine, the build is fine, and a node is still missing.** Search the palette with
context-sensitivity *off* (the checkbox at the top of the search results) — a node hides in a
context where it cannot be placed. The four Flow nodes are [placeable on an event graph
only](../concepts/flow-family.md#where-a-flow-node-can-go), never inside a function graph.

**Everything is there in the editor and gone in a packaged build.** Two of the entries are
development-only by design — check
[which configuration drops which one](../concepts/builds-and-shipping.md#what-is-gone-in-shipping) before filing a report.

**Nothing on this page matches what you see.** The log is the next stop:
[Diagnostics](../troubleshooting/diagnostics.md) lists what the plugin writes to
`LogStillCooking` and how to raise its verbosity.

---

← [Start here](README.md) · [Documentation index](../../README.md#documentation)
