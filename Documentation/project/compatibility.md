# Compatibility

*The build-and-test matrix per release, what the plugin requires of your project, the limits that are permanent by design, and where the verification gaps are recorded.*

## Engine versions and platforms

| Plugin version | Unreal Engine | Platform |
| --- | --- | --- |
| 0.8.0 | 5.7, 5.8 | Win64 |
| 0.7.1 | 5.7 | Win64 |
| 0.7.0 | 5.7 | Win64 |
| 0.6.0 | 5.7 | Win64 |
| 0.5.0 | 5.7 | Win64 |
| 0.4.0 | 5.7 | Win64 |
| 0.3.1 | 5.7 | Win64 |
| 0.3.0 | 5.7 | Win64 |
| 0.2.0 | 5.7 | Win64 |
| 0.1.0 | 5.7 | Win64 |

**Only these combinations have been built and tested.** The table records what has been done,
not what is expected to work.

Other platforms, and engine versions not named here, may work: the plugin needs only `Core`, `CoreUObject`
and `Engine`, plus the Blueprint-graph modules for its editor module. None of them is verified,
and a problem on any of them is worth reporting.

### Known not to build

**Unreal Engine 5.5 and 5.6 do not compile.** The plugin uses engine API that first appeared in
5.7 — the `FTickableGameObject` constructor that takes a tick type, and `EFindObjectFlags`. Both
versions were built on Win64 and stop in the editor target, before the game targets are reached.
No release supports them yet.

**Maturity: beta.** The descriptor sets `IsBetaVersion`, and the editor's Plugins browser shows
the plugin with a Beta label.

## What the plugin requires of your project

- **A C++ project** — see
  [why the plugin has to be compiled with your project](../start/installation.md).
- **Nothing else.** No other plugin, no engine plugin beyond what a C++ project already has, no
  project setting, no `.uproject` entry, no config section.

## Permanent limits

By design, not scheduled to change:

- **Nothing replicates.** [No entry knows about the network](../advanced/limits.md#plugin-wide).
- **Game Thread only.** [Every entry is driven by the world's tick pass or by the
  console](../advanced/limits.md#plugin-wide).
- **The console command registry and `Print String Formatted` are development-only.** The
  registry's registration is
  [compiled out of Shipping](../concepts/builds-and-shipping.md#what-is-gone-in-shipping);
  the print node is
  [compiled out of Shipping and Test](../concepts/builds-and-shipping.md#what-is-gone-in-shipping).
- **The C++ Flow layer is EXPERIMENTAL** — the marker is about
  [evidence rather than quality, and what would remove it](../concepts/overview.md#the-stable-surface-and-the-experimental-layer).

---

← [Versions, support, and license](README.md) · [Documentation index](../../README.md#documentation)
