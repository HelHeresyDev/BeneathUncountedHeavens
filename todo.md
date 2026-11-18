I agree. That's a better design for **Beneath Uncounted Heavens**, especially because we're trying to make combat feel like chess rather than a conventional action game.

The problem with our current approach is that choosing a hypothetical attack **replaces the original prediction**. But the purpose of hypothetical actions is to explore possibilities, not discard information.

## **Proposed design: Multiple prediction ghosts**

We should separate the uninterrupted movement prediction from hypothetical reaction points.

**Main movement ghost — cyan**

Follows the original Walk trajectory as you scrub. Never stops just because you select a hypothetical attack.

**Reaction ghost — gold**

Remains fixed at the predicted location where your fighter would receive a reaction opportunity.

For example, suppose you plan a 30-Instant Walk and choose a hypothetical Heavy at Instant 4.

Full movement timeline

Illustrative timeline: the marker indicates the hypothetical reaction, while the baseline remains available through Instant 30.

When you scrub to Instant 15, the cyan ghost moves to its Instant 15 location, but the gold ghost remains where the reaction would have occurred at Instant 4.

This lets you compare the uninterrupted movement and the potential interruption simultaneously.

## **How I would implement it**

Importantly, **we should still calculate the hypothetical branch, but we shouldn't replace the displayed main timeline with it**.

The existing `GenerateBranchFromBase()` calculates the reaction correctly, but installs its combined result as the current timeline. I'd separate those responsibilities.

The revised architecture would work as follows:

1. Preserve the existing baseline timeline as the primary scrubber data.
2. Calculate the hypothetical attack in a separate simulation without overwriting that baseline.
3. Find the actual reaction snapshot and position. For Dash or Jump, this may occur later than the attack's selected start Instant.
4. Position a reusable gold reaction ghost at those coordinates and mark that Instant on the timeline.
5. When the hypothetical action or its start Instant changes, update the reaction ghost. When the hypothetical is cleared, hide it.

If the hypothetical action produces no legal reaction opportunity, the UI should say so rather than inventing a reaction location.

## **One additional improvement**

Eventually, I'd like to allow **multiple hypothetical reactions on the same movement timeline**, so you could see what happens if the opponent attacks at Instant 4, Instant 8, or Instant 12.

We don't need to implement that yet. One reaction ghost is sufficient to establish the design.

**My recommendation:** Keep the full baseline Walk prediction visible at all times, treat hypothetical attacks as independent annotations, and use the gold ghost to show the resulting reaction position. Then we can build more elaborate tactical comparisons on that foundation.