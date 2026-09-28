# Issues

*Three symptoms that are expected behaviour, what to include in a report so the problem can be reproduced, and where to send it.*

## Before you report

These three symptoms are expected behaviour; each has its own page:

1. **A loop runs once and stops.** `Auto Continue` is cleared and nothing calls `Continue Loop`;
   [the warning after 30 seconds says
   so](../concepts/flow-family.md#warnings-instead-of-silence). [What to check in the
   graph](../troubleshooting/common-problems.md#a-loop-runs-one-iteration-and-then-nothing-happens).
2. **A console command is listed but does nothing outside Play.** [A static command is visible from
   editor start and executable only with a game
   world](../concepts/builds-and-shipping.md#visible-is-not-executable).
3. **`Completed` did not fire when the map changed or PIE ended.** [Nothing is broadcast on
   world teardown, on purpose](../concepts/flow-family.md#how-a-task-ends).

## What to include

- **The plugin version** — `VersionName` in `StillCooking_Tools.uplugin`, or the first
  versioned heading in `CHANGELOG.md`. If you are on a commit rather than a release, the commit.
- **The engine version and the build configuration.** Development Editor, Development, Test or
  Shipping. Two entries are missing from some of them, so
  [which entry each configuration drops](../concepts/builds-and-shipping.md#what-is-gone-in-shipping)
  decides whether what you saw is a bug at all.
- **The log, filtered on `LogStillCooking`,** at `Verbose` if the console registry is involved —
  [how to filter it and raise its verbosity](../troubleshooting/diagnostics.md#the-log-category):

  ```
  Log LogStillCooking Verbose
  ```

- **The entry and the pins.** Which node or class, and the values on every pin that is not at
  its default — `Auto Continue`, `Items Per Tick`, `Timeout`, `Tick When Paused` in particular.
- **A minimal graph or snippet.** For a node, a screenshot of the node with its pins visible;
  for a C++ core, the `Start()` call with its parameters and the body.

## Where to send it

**GitHub Issues**: <https://github.com/StillCooking/StillCooking_Tools/issues>. Bugs, feature
requests, and reports from unverified configurations all go there. An issue is public, takes
attachments, and gets a number that the changelog can refer to.

There is no support obligation attached to the MIT license; issues are read and answered as time
allows.

---

← [Versions, support, and license](README.md) · [Documentation index](../../README.md#documentation)
