# Glossary

*The terms the other pages use without defining them each time — entry, Flow node, Flow core, proxy pin, latent body, Auto Continue, Development Only and `LogStillCooking` — each with a link to the page that owns it.*

**Entry** — one class or one Blueprint node from the plugin, documented on its own reference page.
[Every entry stands on its own](overview.md#independent-entries).

**Flow node** — one of the four Blueprint nodes in the `StillCooking|Flow` category: **For Each
Index With Delay**, **For Each Index Per Tick**, **Do For Duration**, **Wait Until**. They share
one shape, described in [The Flow family](flow-family.md).

**Flow core** — the C++ class a Flow node is built on: `FSCFlowLoop`, `FSCFlowDuration`,
`FSCFlowConditionPoll`, all on the shared base `FSCFlowTickingTask`. Usable from C++ directly —
[the C++ Flow cores](../reference/cpp-flow-cores.md) — and
[marked EXPERIMENTAL](overview.md#the-stable-surface-and-the-experimental-layer).

**Proxy pin** — the output pin on a Flow node that hands out the node object itself (`Loop` on
the loops, `Action` on the others). It is what you call `Continue Loop`, `Break Loop` or `Cancel`
on. [Every Flow node has an output pin that hands out the node object
itself](flow-family.md#the-proxy-pin).

**Latent body** — a `Loop Body` that contains a latent node (a `Delay`, a `Move Component To`,
an asset load) and therefore returns before its work is done. A latent body has to tell the loop
when to move on — [the latent-body protocol](../reference/for-each-index.md#auto-continue-and-the-latent-body-protocol).

**Auto Continue** — the advanced pin on both loop nodes. Checked (the default), the loop advances
as soon as `Loop Body` returns. Cleared, the loop waits for `Continue Loop` —
[why the default is checked](../reference/for-each-index.md#auto-continue-and-the-latent-body-protocol).

**Development Only** — the engine's own marker for a graph node that is compiled out of Shipping
and Test builds. `Print String Formatted` carries it, like the engine's `Print String` —
[what is gone in Shipping](builds-and-shipping.md#what-is-gone-in-shipping).

**`LogStillCooking`** — the single log category for everything in the plugin —
[how to filter the log and raise its verbosity](../troubleshooting/diagnostics.md#the-log-category).

---

← [Concepts](README.md) · [Documentation index](../../README.md#documentation)
