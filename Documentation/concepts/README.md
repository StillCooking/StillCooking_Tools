# Concepts

*Why the plugin is a set of independent entries, the shape every Flow node shares, the tickable object's ownership contract, what disappears in a packaged build, and the terms the other pages use.*

These pages explain the decisions behind the entries, so that you can predict what an entry will
do in a situation the reference does not spell out. None of them is required reading for using a
node; all of them are required reading before you extend one. If your question starts with *why* —
why a default is what it is, why something fails silently, why a type is marked EXPERIMENTAL —
this is the section that answers it.

## Pages in this section

| Page | What it covers |
| --- | --- |
| [Overview](overview.md) | A grab-bag, not a framework: what "every entry stands on its own" commits the plugin to, and where the stable surface ends and the EXPERIMENTAL layer begins. |
| [The Flow family](flow-family.md) | One shape under four nodes: a core that keeps itself alive, a node that hands itself out on a pin, four ways to end, two timing rules, two clock options, and a warning instead of silence. |
| [Lifecycle](lifecycle.md) | The tickable object's contract — why ticking starts in `Initialize()` and not in the constructor, the two obligations the owner takes on, how tick intent is set, and what happens when `Shutdown()` is forgotten. |
| [Builds and shipping](builds-and-shipping.md) | What each module type means for a packaged game, which entries are compiled out of Shipping, why a console command can be visible before it can run, and what has been verified where. |
| [Glossary](glossary.md) | The terms the other pages use without defining them each time — entry, Flow node, Flow core, proxy pin, latent body, Auto Continue, Development Only and `LogStillCooking` — each with a link to the page that owns it. |

← [Documentation index](../../README.md#documentation)
