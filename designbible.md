# BENEATH UNCOUNTED HEAVENS

**Master Design Bible — Version 1.2 (working design)**  
**Updated:** 29 October 2026  
**Genre:** Single-player, third-person 3D xianxia / wuxia cultivation sandbox with roguelite reincarnation and tactical combat  
**Current development experiment:** Unreal Engine 5.8.4 on Linux, driven by local Qwen through OpenCode and Unreal MCP

> **The promise:** One soul. Countless lives. Infinite paths.

> **Purpose of this document:** Preserve the full creative direction established in discussion, distinguish firm decisions from promising suggestions, and give a local coding agent a durable reference. It is a *design bible*, not a claim that every described feature has been implemented.

## How to read this bible

- **[DECIDED]** — Expressly selected or clearly endorsed by the project creator. Treat as a core requirement unless deliberately revised.
- **[PROPOSED]** — Developed during design discussion and useful as a starting point, but not yet approved as immutable canon.
- **[OPEN]** — Requires a future choice or prototype result.
- **[EXAMPLE]** — Illustrates system behavior; names, numbers, and content are placeholders rather than confirmed game content.
- **[CURRENT]** — Describes development workflow or prototype status, not necessarily a permanent product requirement.

**Source integrity:** The setting, mechanics, and ideas below summarize this project's design conversations. Inspiration from *The Matchless Kungfu*, *Your Only Move Is HUSTLE*, and cultivation fiction is acknowledged, but this game should use original narrative expression, assets, naming, characters, techniques, interfaces, and source code. Particular examples or names introduced by the assistant remain proposed rather than silently becoming canon.

---

# PART I — CREATIVE NORTH STAR

## 1. Game identity

**[DECIDED]** The title is **Beneath Uncounted Heavens**. The game is a large-scale, living **wuxia/xianxia cultivation sandbox**. It is not a literal remake of *The Matchless Kungfu*: it pursues the promise of a complete, interconnected cultivation world while avoiding disconnected features, unmaintained systems, and fragile long saves.

**[DECIDED]** The player explores in **third-person 3D**. The game gives **equal weight** to the autonomous world simulation and to **deep tactical combat**. Combat is closer in philosophy to *Your Only Move Is HUSTLE* than to *Sifu*. The user values carefully puzzling out advantageous sequences more than spectacle, though cinematic replays are still important.

**[DECIDED]** Each incarnation takes place in a **fresh, newly generated world**. This is a roguelite of *new discoveries*, not a continuing world that persists across deaths. Persistent progression belongs to the **soul**, not the physical world's people, items, or politics.

**[DECIDED]** One themed **System** is assigned per life. It maintains **exactly one visible active quest** at a time. Its quests grant random, usually in-life rewards, and following them is never mandatory. Achievements and their reincarnation rewards are a *separate* system.

**[DECIDED]** The cultivation cosmology is vast: billions of worlds and universes; cultivators at cosmic heights can destroy universes; three hidden Dao Ancestors stand on an altogether different plane. Attaining Dao Ancestor status is intended to be **nearly impossible**, even across many incarnations.

**Proposed tagline:** “One soul. Countless lives. Infinite paths.” The title is selected; the tagline is not yet locked.

## 2. Design pillars

1. **Freedom with consequence.** Players may cultivate, wander, challenge masters, trade, farm, recruit disciples, build institutions, pursue dangerous methods, or disregard factional ambition. Systems respond meaningfully.
2. **A world that acts without the player.** NPCs have histories, limited knowledge, ambitions, relationships, resources, and goals. Sects pursue their own interests. Events occur off-screen.
3. **Combat as a solvable-but-uncertain strategic contest.** Time to think is unlimited; precise timing in **instants**, positioning, techniques, uncertainty about enemy choices, and forecasting matter. Exchanges create tactical decisions; tempo describes who has the advantage. **Only one action is chosen at a time, but observable threats can create reaction opportunities during interruptible actions.** No reflex tax in the planning phase.
4. **Knowledge is a scarce world resource.** Cultivation manuals and martial arts are guarded, taught, stolen, traded, corrupted, inherited, or lost. Possession, comprehension, and mastery are distinct.
5. **Many lives, new discoveries.** Soul progression raises future potential; a new life renews exploration, sect dynamics, opportunities, and the run's System theme.
6. **Scale without obligation.** The setting extends to primordial beings and collapsing universes, but ordinary lives, modest accomplishments, and failures are complete and worthwhile stories.
7. **Coherent systems before breadth.** Resource stocks, NPC succession, procedural histories, combat results, quests, and saves must agree; avoid sprawling unfinished feature piles.

## 3. The player's fantasy and repeating loops

**Within one life:** Arrive as an inhabitant of a world → explore and evaluate opportunities → train, earn a living, enter sects, cultivate and fight → affect people and organizations → grow, stagnate, ascend, retire, or die. The game does not require a chosen-hero storyline.

**Across lives:** Die → conclude and record achievements → earn *Soul Imprints* (working name) → improve permanent soul potential through a *Soul Legacy* (working name) → be reborn in a fresh world with a newly assigned themed System.

**During combat:** Inspect state and possible enemy options → select **one** attack, defense, movement, or special action → evaluate timing in **instants** and hypothetical responses → commit → simulate until the next meaningful decision or **observable reaction opportunity** → reassess **tempo** → react, continue, or choose the next action → view a continuous cinematic replay if desired.

**Across the cosmos during a life:** Rise through appropriate realms and, if capable, ascend to a higher world **without** resetting the current character. Ascension and reincarnation are categorically different transitions.

## 4. Explicit anti-goals

- No mandatory linear main quest that overwhelms the sandbox.
- No expectation that all or most players will become Dao Ancestors.
- No arbitrary one-dimensional “good sect / evil technique” alignment system.
- No class lock that makes cross-style learning impossible.
- No generic move list with dozens of indistinguishable damage variations.
- No combat presentation whose physics contradict its tactical preview.
- No System quest list: **never two active quests, never zero**.
- No rerolling the same fresh-life world as a way to preserve prior NPC relations by default.
- No pretending to simulate billions of universes individually; detailed simulation follows relevant space.
- No reward structure in which irrelevant achievement spam or suicide farming eclipses substantive lives.

---

# PART II — COSMOLOGY, LORE, AND NARRATIVE

## 5. The vast cultivation cosmos

**[DECIDED]** Existence comprises **billions of universes and worlds**. Worlds differ in spiritual energy, laws, resources, civilizations, sects, history, threats, and the paths of cultivation possible within them. The upper bound of power is astonishing: battles between sufficiently powerful cultivators may destroy universes with a gesture. This is an authentic part of the setting, not necessarily a promise that all such clashes appear in the first release.

**[PROPOSED]** Use a nested vocabulary of *mortal worlds*, *spiritual worlds*, *immortal realms*, and *higher universes* for production planning. These are illustrative **environmental categories**, not locked cultivation ranks. A character may reach the practical ceiling of a lower world and need to ascend or travel to richer laws and resources. They are not automatically near the universal peak.

Worlds may have different dominant cultivation traditions. A metallic-energy-rich world might favor sword schools; beast-ridden worlds might produce body cultivators; worlds damaged by catastrophe might have unusual resources and dangerous methods. Differences should manifest in combat, economics, and society, not just terrain colors.

## 6. The three primordial Dao Ancestors

**[DECIDED]** There exist **three hidden Dao Ancestors**, primordial sources of the Dao and unrivaled existences far beyond conventionally supreme cultivators. Virtually no one knows they exist. They are locked in an ongoing struggle to kill or absorb the other two and become the **sole source of the Dao**.

**[DECIDED]** One ancestor creates or releases the player as an independent, reincarnating extension or fragment of itself. The intent: let that fragment experience innumerable cultivation paths across different worlds; on the extraordinarily unlikely day it reaches Dao Ancestor level, **merge it with the original**, gain an unprecedented advantage, and overwhelm the other two.

**[DECIDED]** This backstory explains unlimited reincarnation and may also explain the in-life reward System. The player is not told these truths early. The ancestor's plan is a long-shot gamble, not an automatic victory manufactured by the mechanics.

**[PROPOSED]** Why start powerless? An insignificant incarnation is harder for the other ancestors to discover, and an independently developing soul may produce insights impossible for an ancient being trapped in an established Dao. These rationales fit the story but have not been locked as the only explanations.

**[PROPOSED]** There is a thematic parallel between demonic elders refining their own disciples and a primordial ancestor cultivating an incarnation for eventual absorption. The intended merger could pose a threat to the player's individuality.

## 7. Concealed origin and emergent narrative

**[DECIDED]** The game is a sandbox first. The hidden cosmic plot should not require the player to follow the System's quests or to seek Dao Ancestor attainment. Most characters and most worlds do not know the three ancestors exist.

**[PROPOSED]** Lore revelations can be layered: unexplained reincarnation → hints of souls outside reincarnation → ancient contradictions in fate → traces of primordial interference → discovery of the player as a deliberately cultivated fragment → confrontation with the original ancestor. Some lives may reveal nothing at all; clues can appear in rare, contextually appropriate worlds.

**[PROPOSED, NOT CANON]** Possible late-game resolutions include: voluntarily merging with the origin; refusing and attempting to become an independent fourth primordial existence; or transcending the zero-sum conflict by discovering a different path. These are narrative possibilities only. No definitive ending or merger metaphysics has been chosen.

**[DECIDED]** Dao Ancestor attainment is a **mythic near-impossibility**, not a guaranteed reward for completing enough runs. Immense cultivation, universes destroyed, or mastery of a particular Dao do not automatically equal being one of the fundamental sources of the Dao.

## 8. Tone, morality, and authored content

The world permits kindness, ambition, cruelty, devotion, exploitation, freedom, mentorship, betrayal, family, survival, and indifference. Organizations have norms and self-interest rather than simplistic alignment tags. Demonic cultivation techniques are defined by dangerous **costs to the practitioner**. Demonic sects are defined by **predatory institutional practice**; these concepts overlap but are not equivalent.

**IP direction:** Take inspiration from broad cultivation-genre conventions and from the *premise* of the referenced novels, not their protected unique expression. The three-ancestor cosmology, the player-fragment scheme, named System types, narrative lore, artwork, locations, and characters should be reworked into an original mythology before commercial publication. “Dao Ancestors” and “The System of Ten Thousand Paths” remain working terms pending original worldbuilding review.

---

# PART III — REINCARNATION AND PERMANENT PROGRESSION

## 9. The Samsara loop

**[DECIDED]** **Model A — fresh world per incarnation.** On death, the character's current world is concluded, not kept active for the next life. New world generation supplies unfamiliar geography, sects, history, NPCs, conflicts, mysteries, and distribution of techniques. The new incarnation is connected to previous lives through soul-level progression and the player's remembered experience, **not** through a continuing shared simulation.

**[DECIDED]** In the **first life**, the player begins effectively at zero: **barely qualified to cultivate**, with very poor aptitude and little/no cultivation knowledge. Subsequent lives begin with improved *potential* based on achievements purchased in between lives. The character still needs teachers, manuals, resources, breakthroughs, and actual training.

**[DECIDED]** The system must allow deaths by combat, age, cultivation backlash, betrayal, sacrifice, and other believable causes. Death ends that incarnation. It may be a bittersweet conclusion after a meaningful life, not necessarily failure.

**[PROPOSED]** Names: the cross-life currency **Soul Imprints** and the persistent passive progression **Soul Legacy Tree**. These names appeared in discussion but have not been individually confirmed as final.

## 10. What resets and what persists

| Aspect | Next incarnation? | Rule |
|---|---|---|
| Soul achievement records | **Persists** | Permanent achievements and unlocks survive death. |
| Unspent Soul Imprints and bought Soul Legacy upgrades | **Persists** | Meta-progression is attached to the soul. |
| Initial aptitude / constitution / comprehension potential | **Modified by legacy** | A new body with improved starting potential, not a high cultivation realm. |
| Current cultivation level, Qi stores, techniques mastered | **Resets** | Must be earned through the new life. |
| Current body, possessions, money, sect contribution points | **Resets** | World-bound resources stay in the concluded incarnation. |
| NPC relations, sect membership, influence and local politics | **Resets** | New world with new characters and institutions. |
| Themed System and its active quest | **Resets** | One newly assigned theme each life. |
| Player's out-of-game understanding | **Persists in the human player** | Character memory transfer is not automatically assumed. |
| Cosmic mystery unlocks | **[OPEN]** | May persist as meta-lore discoveries without granting character omniscience. |

## 11. Soul Legacy and aptitude

**[DECIDED]** Achievements from a previous life grant currency to buy passive, permanent improvements. The most important are aptitude and talent so that later lives can cultivate more efficiently than the nearly talentless first life.

**[PROPOSED]** Organize the tree into branches: **Spiritual Roots** (Qi absorption, meridians, foundation quality), **Comprehension** (technique study, observation and insight), **Constitution** (vitality, health, resistance to backlash), and **Innate Fortune / Starting Circumstances** (possible life backgrounds and unusual opportunities). Other branches may emerge.

Do not confuse **comprehension** with automated puzzle solving. A gifted character can learn techniques or read enemy signs more quickly; the human player must still select tactically intelligent moves.

**[PROPOSED]** Special life-earned traits may unlock as future build choices (sword insight, survival of meridian disaster, etc.) rather than accumulate as free, infinitely stacked bonuses. The respec rules, level caps, trait budget, and costs are [OPEN].

**[DECIDED]** No amount of ordinary Soul Legacy upgrading is a guaranteed route to Dao Ancestor status. The tree improves opportunity, not fate.

## 12. Achievements are not System quests

**[DECIDED]** The **achievement system is a separate meta-progression tracker**. It recognizes substantial milestones and awards Soul Imprints. It does not generate the incarnation's always-visible System quest; completion of one does not automatically complete the other. They may coincide because the player performed the same meaningful action.

**[EXAMPLE]** Potential achievements: reach Foundation Building, defeat a superior opponent, become an inner disciple, survive a lethal sect conspiracy, establish a sect and train a successor, master a demonic method despite adverse effects, or die of old age after an accomplished life. Example point values are uncommitted.

**[PROPOSED]** Give disproportionate value to first-time and increasingly difficult achievements. Avoid repeatable trivial farming and death-loop exploitation. Reward long, unusual, difficult, or creatively accomplished lives while recognizing many kinds of success.

---

# PART IV — THEMED SYSTEMS AND QUEST REWARDS

## 13. One System per incarnation: inviolable rules

**[DECIDED]** Every new incarnation is assigned **exactly one** thematic System. It stays with the character until death. The System is an extra, optional route to in-life rewards and a major way each procedural run acquires a different *flavor*.

**[DECIDED]** At **all times during an active life**, the System offers **exactly one active quest**: not a quest board, not a rotating list, and not zero quests. Its current objective is **always visible** in the game UI. Finishing the objective automatically awards a random reward and replaces it with exactly one new quest. If it becomes impossible or fails, resolve it and immediately create one replacement. The player may ignore it forever without punishment for inactivity.

**[DECIDED]** Initial System assignment is **without replacement across incarnations**: select randomly from System types the player has not previously received. Once every available type has been experienced at least once, later assignments are genuinely independent random draws across the complete pool; immediate repeats are allowed. Discovery is tracked at the cross-life profile level. Handle expansion of the roster by defining whether new types re-enter the unseen pool [OPEN].

**[DECIDED]** A System's quests reflect its theme. It is not a generic random achievement generator. Completing quests gives **random rewards for the current life**, and the reward pool should reinforce the theme. Achievements still produce the separate cross-life currency.

## 14. Proposed System roster

These eight are **[PROPOSED] archetypes** based on the examples discussed. They are an initial design pool, *not* a finalized set of eight canonical Systems.

| Theme | Playstyle encouraged | Illustrative quest | Reward character |
|---|---|---|---|
| **Fate Usurper** | Interfere with exceptionally fortunate people and seize opportunities | Claim a rare inheritance sought by a favored cultivator | Fate insight, rare opportunities, special resources |
| **Spirit Farmer** | Plant, harvest, raise and care for spiritual animals | Raise a crop of spirit rice to maturity | Seeds, cultivation plants, agricultural tools |
| **Heavenly Merchant** | Start businesses, trade, develop income and supply chains | Establish a business that earns money without direct labor | Business opportunities, trade assets, capital or knowledge |
| **Grandmaster** | Seek talented disciples, teach, guide their cultivation | Help a disciple complete a major breakthrough | Teaching aids, insight, talent-sensing methods |
| **Martial Challenger** | Seek worthy fights, develop counters, master styles | Win a meaningful duel against a reputable opponent | Combat techniques, martial insights, resources |
| **Heaven's Explorer** | Find ruins, secret realms and forgotten inheritances | Discover a previously unexplored ancient site | Maps, exploration tools, rare discoveries |
| **Divine Alchemist** | Research herbs, refine pills, study formulas | Create a pill using an unfamiliar material | Ingredients, furnaces, recipes, alchemical insights |
| **Shadow Sovereign** | Build covert networks and manipulate institutions | Place an informant in a rival faction | Contacts, covert tools, influence-related knowledge |
| **Dual Cultivator** | Romance and seduce other people and cultivators, engage in dual cultivation, and build a family | Give a gift to a cultivator | Gift items, dual cultivation manuals, charm improving items |
| **Dao of Creation** | Cultivate a universe inside of yourself | Observe the birth of a creature | Seeds, 
| **Transmigrator** | Send your soul to other worlds / universes to inhabit the body of dying people and fulfil their last wishes to recieve a portion of their soul and fortuity | Avenge the young master | Soul enhancement pills, soul techniches

Novel inspirations supplied by the creator: a fate-stealing System like *I Am the Fated Villain*; farming and animals like *Beware of Chicken*; businesses and money like *Dao of Money*; and raising talented disciples like *Arrogant Young Master Template A Variation 4*. These are references for *playstyle theme* rather than content to reproduce verbatim.

Every thematic activity must remain possible **without** the matching System. A Spirit Farmer can join a demonic sect; a merchant can master a sword; a Grandmaster can remain independent. The System gives an incentive, not a class restriction.

## 15. System quest lifecycle and UI

**Lifecycle:** Active theme → select one feasible, theme-relevant quest → display continuously → monitor world events / progress → on success give one random reward and atomically replace quest → on invalidation or failure replace quest → on death discard quest and System.

**[PROPOSED]** Prefer contextual objectives derived from the simulated world (a harvest in season, a real business, a real disciple, a genuine rival) over arbitrary targets. If a quest references an NPC or location that ceases to exist, use event-driven invalidation and generate a feasible replacement. Define whether a player can manually abandon a quest [OPEN]; do not implement a freely rerollable quest menu without an explicit design decision.

**UI [DECIDED]:** one permanent, compact, always-readable objective panel; expandable details and reward tier; no demand to accept each quest or press a button to receive the next. Do not conflate this panel with sect mission boards or the achievement journal.

## 16. Rewards and their limits

**[DECIDED]** Rewards are random on quest completion. **[PROPOSED]** Theme-specific, weighted loot pools with common through extraordinary tiers. Rewards may include material resources, manuals, treasures, recipes, abilities, rare opportunities, and occasionally something valuable that attracts unwanted attention or cannot yet be used.

**Guardrails [PROPOSED]:** scale reward pools with plausible challenge and current cultivation opportunity; allow rare outlier rewards with restrictions; avoid giving easy low-level quests artifacts that routinely erase the entire progression curve. Systems should *supplement* normal cultivation and economy, not make them redundant. There is no compulsory moral alignment for quest eligibility.

---

# PART V — CULTIVATION, KNOWLEDGE, AND MARTIAL ARTS

## 17. Three kinds of techniques

**[DECIDED]** Characters acquire cultivation knowledge from individuals and institutions in the world, rather than by opening a generic level-up menu. Three useful design categories have been established:

1. **Cultivation methods** — build foundations, gather and circulate Qi, shape spiritual roots / meridians, and determine compatibility and long-term direction.
2. **Combat techniques** — discrete attacks, guards, counters, stances, and martial combinations used in the instant-based tactical duel system. Ordinary attacks do **not** automatically advance the fighter; movement is chosen as its own action.
3. **Movement and auxiliary methods** — walking, dashing, jumping, dodging, Qinggong / lightness skills, stealth, healing, sensing, body strengthening, support arts, divination, and exploration techniques. Movement actions have their own timing and reaction availability.

**[DECIDED]** A character can know more techniques than they can optimally use. Owning a manual, understanding it, practicing it, and mastering it are separate states. Knowledge may be incomplete, guarded, or deliberately withheld.

## 18. Recruitment and Foundation Building

**[DECIDED]** A player may pass a sect's **recruitment trial**, enter as an **outer or inner disciple** depending on institution and results, receive a **basic foundation-building cultivation technique**, and receive an opportunity to **choose a combat technique from the sect library**.

The rank and access rules vary by sect. A poor first-life candidate may struggle to qualify, be restricted to menial missions, or need an unusual patron. Gaining entry is meaningful because cultivation knowledge and safe instruction are scarce.

**[PROPOSED]** Use ranked cultivation breakthroughs and challenges rather than pure XP. Lower advancement consumes resources and technique practice; upper advancement increasingly involves understanding and breakthrough conditions. Realm naming, count, dependencies, age mechanics, and precise cosmic thresholds are [OPEN].

## 19. Specialization, compatibility, and cross-training

**[DECIDED]** Cultivation grows increasingly **specialized**. The more heavily a practitioner aligns their foundation with a particular path, the less compatible unrelated disciplines become.

A sword cultivator **can** learn a fist technique, but executes it less effectively than a body cultivator whose training and Qi circulation suit it. This is **soft specialization**, not a class lock. Compatibility should consider individual manuals and method interactions, not merely broad labels.

**[PROPOSED]** Show incompatibility through tactically meaningful differences: Qi inefficiency, reduced reach or force, slower startup / recovery, unstable follow-ups, extra backlash, or restricted advanced variations, rather than a flat damage penalty alone. Some complementary combinations may work unusually well. Fundamental re-specialization remains possible but grows harder and costlier with accumulated investment; exact costs are [OPEN].

## 20. Demonic techniques versus demonic sects

**[DECIDED]** **Demonic techniques are not inherently evil.** They earn the label because they impose **serious negative consequences on their cultivator or others**, such as personality change, lifespan loss, unstable Qi or meridians, and other dangerous backlash. Or they require external costs that damage others like requiring sacrifices or draining vitality from the world. Practitioners may accept these costs for understandable reasons.

**[EXAMPLE]** A life-burning method provides bursts of power but reduces lifespan; an emotion-blunting scripture deepens concentration while undermining personal relationships; an unstable meridian method produces explosive Qi at risk of injury. These names and implementations are unapproved illustrative content.

**[PROPOSED]** Costs must appear in gameplay systems: changing social judgments and relationship options, reducing time available for a life, increasing combat instability or healing needs, altering NPC behavior, and creating pressure to seek cures or stabilizers. A player's agency should remain meaningful; personality changes should not casually seize character control.

**[DECIDED]** **Demonic sects** are institutions that pursue power regardless of others' suffering. They may deliberately sacrifice innocents and refine humans as resources. A demonic sect need not teach only demonic techniques, and an orthodox sect may hold a dangerous forbidden manual. Institutional morality and technique risk are distinct axes.

## 21. Cultivation scale and the near-impossible peak

**[DECIDED]** The game permits advancement from a weak mortal through extraordinarily high realms, potentially including inter-world ascension and cosmic powers, but **reaching Dao Ancestor level is meant to be nearly impossible**. A supreme cultivator who destroys a universe could remain incomparably below the primordial three.

**[PROPOSED]** At advanced stages, “more Qi” alone should stop being sufficient. Cultivators must develop insights and navigate unusual, legible but extremely difficult breakthroughs. Rare worlds, different laws, compatible techniques, mentors, sacrifice, and insight may affect opportunity, but there is no routine, guaranteed path or progress bar labeled “90% to Dao Ancestor.”

**Balance principle:** Talent should affect the *rate and quality* of progress, not allow the Soul Legacy to skip all meaningful in-life effort. Stronger realms matter—strategy should not let an ordinary mortal parry a universe-ending attack simply through superior button choices.

---

# PART VI — THE LIVING JIANGHU

## 22. NPCs as persistent agents within one life

**[DECIDED]** NPCs are more than disposable quest markers. They have ambitions, habits, training, resources, relationships, memories, and autonomy. They can act without being rendered or watched. The player generally operates under the same rules for cultivation, membership, possession, injury, fighting, and social consequences.

**[PROPOSED]** Each meaningful NPC has:

- **Identity:** unique stable ID, age / lifespan, location, history, role, affiliations.
- **Disposition:** ambition, loyalty, courage, compassion, greed, caution; dynamic social attitudes (trust, fear, admiration, resentment).
- **Capabilities:** cultivation, techniques, comprehension, physical traits, professions, knowledge.
- **Resources:** money, items, manuals, obligations, property, access rights.
- **Relationships:** family, friends, rivalries, teacher-disciple bonds, patrons, enemies.
- **Goals and plans:** self-preservation, advancement, revenge, status, livelihood, love, duty, sect loyalty.
- **Knowledge:** personally witnessed facts, rumors, teachings, misinformation; no magical omniscience.
- **Memories:** a sparse log of consequential events that can be summarized over time.

**[PROPOSED]** Use goal-driven/utility AI and structured event reactions for everyday autonomy. Do not depend on a generative language model to make each NPC's in-game decisions. NPCs' combat choices should honor their knowledge and temperament, not read the player's secretly selected move.

## 23. Relationships, rumors, and consequences

Relationships are multi-dimensional. An NPC may admire the player's strength and simultaneously fear their cruelty. Information propagates by witnesses, rumors, travel, and organizations. A duel can cause admiration, humiliation, grudges, new training plans, sect invitations, or revenge, depending on personalities and context.

**[EXAMPLE]** Defeat an arrogant disciple using a timing weakness. He may train a counter, tell his master, challenge you again, or persuade allies to intimidate you. The aftermath is computed from simulation state rather than an always-scripted quest chain.

**[PROPOSED]** Significant outcomes should be structured world events consumed by interested systems: `DuelEnded`, `TechniqueObserved`, `NPCInjured`, `DisciplePromoted`, `ResourceShortage`, `SectLeaderDied`, `ManualStolen`, etc. Event IDs and exact schemas are implementation decisions.

## 24. Sects as functioning organizations

**[DECIDED]** Sects have recruitment, hierarchies, libraries, protection rules, political factions, resources, missions, and contribution accounts. Their autonomy matters: NPCs recruit, train, compete, defect, ally, fight, ascend, or die. The sect survives leadership transitions when possible.

**[PROPOSED]** Common ranks: applicant → outer disciple → inner disciple → personal disciple → elder / master / sect leader. This is an example: institutions can have different hierarchies and recruitment standards.

**[DECIDED]** Sects **protect their techniques**. Cross-sect technique acquisition should be possible but difficult. Library rank, trust, contributions, personal masters, theft, bribes, captured manuals, defectors, or exceptional favors may grant access. A stolen manual may still be impossible to cultivate safely without a compatible foundation.

## 25. Orthodox sect politics and protection

**[DECIDED]** A conventional sect offers comparatively safe, structured training but contains bullying, favoritism, rivalries, social debt, and internal politics. An orthodox elder sacrificing disciples for personal gain generally violates its institutional rules, even if enforcement is imperfect.

**[PROPOSED]** Patronage affects mission assignments, resource access, library privileges, promotion, and justice. A protective elder could help the player; a corrupt elder might deny deserved credit or manipulate accusations. The sect is neither a benevolent vending machine nor merely a murder arena.

## 26. Demonic sects as predatory institutions

**[DECIDED]** The player can join **demonic sects**. Joining may grant ruthless opportunities and powerful knowledge at **significantly increased danger**. A senior elder may wish to **refine the player into a puppet**; a less-talented disciple may be deemed useful as a **medicine ingredient**. Innocent sacrifice can be routine in some sects.

**[DECIDED]** An orthodox disciple worries about bullying and politics; a demonic disciple can fear being consumed by their own organization. No promise of safety merely because the player is a member.

**[PROPOSED]** Predation should follow discernible motives and resource constraints. High-talent disciples may be investments or prized refining targets; patrons and politics can offer conditional safety; excessive consumption damages recruitment and labor supply; leadership may regulate harvesting to protect the sect's own viability. Player deaths must not be arbitrary unavoidable dice rolls disconnected from warnings and meaningful choices.

**[PROPOSED]** Candidate choices: hide talent, earn a patron, find defensive resources, spy on an elder, flee, sabotage a rival, rise in rank, or exploit the sect for personal power. Normal sect infrastructure should still function around these schemes.

## 27. Contribution points and sect missions

**[DECIDED]** Sects maintain **contribution points (CP)** used to acquire resources. Members earn CP by completing **sect missions**. Low-level missions include **gathering ingredients** and **killing weak beasts**. Sect missions are *not* the incarnation's one active themed System quest; players may hold sect assignments while the System quest remains visible separately.

**[PROPOSED]** Other assignments include patrol, escort, scouting, guarding a supply route, hunting a dangerous beast, collecting debt, diplomacy, and sect-level conflict. Mission rewards reflect difficulty, rank, resource need, and political favoritism.

**[PROPOSED]** CP is a **claim on the sect's inventory and privileges**, not a magical generator of items. The sect tracks actual stock of pills, spirit stones, manuals, artifacts, training time, and materials. A character with sufficient CP may still face shortages or access restrictions. Missions should emerge where possible from inventory needs and organizational goals.

**[EXAMPLE]** Alchemists exhaust Spirit Grass → logistics elder posts gathering assignments → NPCs and player harvest → wolves or rivals interfere → herb supply returns → pills are refined → stocked pills become exchangeable for CP. If no one fulfills the mission, shortages persist and sect behavior changes.

## 28. Economic and non-combat lifestyles

**[DECIDED IN PRINCIPLE]** The world supports meaningful non-combat paths: farming, spiritual plants and animals, merchants and businesses, alchemy, teaching disciples, exploration, faction work, and eventually founding or managing a sect. These activities must exist independently from themed Systems that reward them.

**[PROPOSED]** Treat production, inventory, trade routes, shortages, and employer/worker activities as parts of the living simulation. Businesses generate income only where supply and demand support them. NPCs use and compete for resources. Economy must be abstractable at distance; individual items need not all be physically simulated at all times.

---

# PART VII — THREE-DIMENSIONAL TACTICAL COMBAT

## 29. Combat north star

**[DECIDED]** *Your Only Move Is HUSTLE* is the major **mechanical reference**, not *Sifu*. The principal pleasure is **chess-like, instant-by-instant analysis**: taking one's time to evaluate possible sequences, probabilities, counters, positioning, and **tempo advantage** to gain the largest possible edge. Cinematic real-time replay is a valued secondary pleasure.

**[DECIDED]** Combat occurs in 3D while general exploration remains third-person. The player has **unlimited thinking time** during the planning phase and at every legally available reaction decision. Time advances in discrete combat simulation steps when actions are committed. Both combatants must make choices without omniscient knowledge of the other's secret choice. **Exactly one action is selected or actively performed per fighter at a time**: movement and attack cannot be queued as simultaneous independent commands.

**[DECIDED — PLAYER-FACING TERMINOLOGY]** Tactical combat uses three distinct concepts:

- **Instant:** the named, discrete **in-world combat time unit** shown to players for move timings and timeline inspection. For example, a technique may require **6 instants of preparation**, **2 instants of strike**, and **8 instants of recovery**. Do not label these quantities as animation frames or milliseconds in the gameplay UI.
- **Exchange:** a resolved segment of combat between meaningful tactical decision opportunities. One exchange may contain overlapping actions, movement, hits, blocks, and recovery; it is **not** necessarily a fixed duration or a round where all combatants become ready simultaneously.
- **Tempo:** the relative tactical initiative or advantage arising from readiness, positioning, threats, and the opponent's commitments. Having the tempo is a description of the current situation, **not** an automatically granted damage bonus or separate resource meter.

**[PROPOSED — IMPLEMENTATION]** Use simultaneous action commitment where both combatants are ready, deterministic resolution on **fixed internal simulation ticks**, and new decisions at meaningful availability points. Rendering frames are **independent** of simulation ticks and displayed instants. A provisional scale is **1 instant = 50 milliseconds**, but its final duration and relation to the internal tick rate remain **[OPEN]** pending playtesting. Preserve exact known timings and hypothetical branches, but never reveal hidden enemy intent as fact. Do not automatically convert the historical prototype's frame counts into instants without specifying a mapping.

## 30. What makes a move

**[DECIDED — SEPARATE ACTIONS]** Attacking, defending, moving, and using a special ability are alternative **single actions**, not independently selectable action channels. The player does **not** combine an Attack choice with a separate Move choice in the same decision. Ordinary attacks never grant automatic approach or lunge movement; closing distance and repositioning require a chosen movement ability. The prototype's existing automatic closure / attack lunge behavior is **legacy implementation behavior, not a design requirement**, and should be removed when the action architecture is migrated. Explicitly movement-bearing martial techniques are not part of the current rule set; whether rare exceptions should ever exist is **[OPEN]**, not an authorization to add them implicitly.

A proposed move definition contains:

- **Preparation** (startup), **Active** (strike, defense, or movement), and **Recovery** durations, displayed in **instants**; optional cancel / branching windows. “Strike” names an attack's active phase, not the active phase of walking or guarding.
- **Action category** (Attack / Defend / Move / Special as a proposed UI taxonomy), allowed transitions, and whether the action is continuous or finite.
- **Commitment and reaction windows:** at which instants the character is locked, eligible to respond to a newly observed event, or fully ready. The ability to react is distinct from duration.
- Actor position, facing, momentum, and movement across fixed internal simulation ticks when the chosen action actually entails movement; attack animations do not supply automatic combat displacement.
- Attack arc / hit volume and valid body / weapon targets.
- Conditions for use (range, equipment, stance, resource availability).
- Health, posture, Qi, and injury effects where applicable.
- Defense interactions: guard, evasion, parry, clash, super-armor, grapple, redirection.
- Counters and vulnerabilities that are discoverable through play and study.
- Cultivation compatibility adjustments and technique mastery variations.
- Replay animation / presentation mapping, separate from authoritative results.

**[PROPOSED]** Basic interaction vocabulary: light and heavy strikes, blocks, precisely timed counters, dodges, stance transitions, Qi techniques, feints, walking, dashing, jumping, and weapon-specific reach. Guard, health, posture, and Qi were proposed as a useful foundation; final mechanics and numerical values remain [OPEN].

### 30.1. Movement actions and reaction windows

**[DECIDED — REACTION-BASED MOVEMENT]** Movement has its **own list of abilities**, not an automatic component of attacks. Each movement ability offers a different tradeoff between speed, position, commitment, and the ability to react. A movement action can continue over multiple instants; the character need not finish the entire nominal duration before being allowed to respond. The simulation should advance without requiring an input at every instant, and pause at a **meaningful, legally available** decision point.

- **Walking [DECIDED IN PRINCIPLE]:** sustained, comparatively flexible movement. If an opponent begins an **observable attack** during the walk, and the fighter is able to react, simulation pauses so the player can evaluate and choose a response. This is a tactical decision pause, **not** a reflex-time challenge.
- **Dashing [DECIDED IN PRINCIPLE]:** a faster movement with an **initial interval of committed instants** in which no reaction is allowed. Later instants can permit a reaction while the dash is still underway. The lock duration, speed, residual momentum, and allowed cancellations are **[OPEN]**.
- **Jumping [DECIDED IN PRINCIPLE]:** takeoff begins with instants that prohibit reactions; later portions of the jump may allow a new action. **[PROPOSED]** Existing trajectory and momentum need not vanish when a legal midair reaction occurs. Exact airborne restrictions and timings are **[OPEN]**.
- **Dodge, Qinggong, and other movement arts [PROPOSED]:** each receives its own displacement, commitment, recovery, and reaction properties; there is no universal “movement always cancellable” rule.

#### 30.1.1 First-playable Jump prototype (implemented)

The current tactical-duel prototype implements Jump as a real simulated 3D movement action, not merely a curved preview line. The provisional values are:

- Action ID 9, startup 1 Instant, flight 8 Instants, recovery 3 Instants.
- Maximum horizontal range 480 uu, maximum height 220 uu.
- The first 3 flight Instants are reaction-locked (red); the remaining 5 flight Instants permit qualifying reactions (green).
- The flight duration is always 8 Instants, even for short horizontal distances. A short Jump still rises, reaches its apex, descends, and lands.
- A vertical jump in place (zero horizontal distance) is valid.
- Jump performs no automatic attack or damage.

The airborne trajectory is a true parabola: horizontal travel along a straight XY segment from takeoff to the selected landing point, with vertical height `4 * MaxHeight * t * (1 - t)` over the 8 flight Instants. The same deterministic calculation drives authoritative motion, previews, and timeline ghosts.

Intentional prototype limitations:

- No aerial invulnerability or new attack hit-detection mechanics; existing damage rules apply.
- No Qinggong, double jumps, stamina, wall jumps, or aerial attacks.
- Grounded fighter separation behavior is preserved; airborne collision with terrain or other fighters is not modeled.
- The Jump path renderer reuses `ABUHPathRenderer` and colors segments by flight progress and reaction eligibility, not by rendered length.

**[DECIDED — ONE ACTION AT A TIME]** A reaction **replaces, cancels, or legally transitions out of** the currently active action; it is not a free concurrent attack or movement slot. A decision pause does not rewind movement, remove accrued recovery, grant automatic defense, or guarantee the chosen response becomes effective before the threat lands. What remains of momentum or recovery after cancellation is determined by that ability's rules **[OPEN]**.

**[DECIDED — OBSERVABILITY]** A reaction opportunity must arise from something the fighter can actually perceive, such as the opponent visibly preparing an attack, an incoming projectile, or a detectable Qi disturbance. The player must not be paused or shown secret information merely because enemy AI privately committed an attack. Perception, distance, concealment, technique recognition, and cultivation may alter when and how threats become observable; exact detection mechanics are **[PROPOSED / OPEN]**.

**[PROPOSED — EVENT-DRIVEN SCHEDULING]** A qualifying event creates a reaction opportunity only when the fighter's current action permits it. If a threat appears during the locked part of a dash or jump, the simulation continues until the first subsequent eligible instant **if the threat still justifies a response**; earlier locked instants cannot be retroactively changed. The scheduler must avoid repeatedly pausing on an unchanged already-addressed threat. An interrupted or ongoing enemy action retains its genuine phase and remaining timing across decision boundaries. NPCs use the same legal reaction windows and information restrictions as the player.

**[EXAMPLE — NOT BALANCE CANON]** The player walks toward a rival. At instant 5, the rival visibly begins a sword strike; the game pauses because walking is reactable. The player may switch to Guard, Dodge, or another **single** legal action. In a different branch, the player is in a dash with four initial locked instants and cannot respond until that period expires. Neither example implies that the chosen response succeeds automatically.

### 30.2. Consequences for timing and simulation

**[DECIDED IN PRINCIPLE]** A fighter's **action phase**, **remaining action duration**, **commitment state**, and **current reaction eligibility** are separate concepts. Decisions arise from readiness **or a valid observable reaction opportunity**, not merely from both fighters completing one synchronized exchange. Fixed-step authoritative simulation remains responsible for movement, observation timing, hits, interrupt/cancel transitions, and forecasting. Reaction decisions and their consequences must be reproducible under the same initial state, inputs, and knowledge constraints.

## 31. Forecasting and decision UI

**[DECIDED]** The player should be able to examine the current situation in detail and try to solve likely exchanges. The planning view should support:

1. Selecting an available technique and seeing its timing and resource cost.
2. Ghost trajectories and threatened attack volumes in **three dimensions**.
3. **Instant-by-instant timeline scrubbing**, with finer inspection only if the simulation requires it, to examine when preparation, strikes, movement, and recovery overlap.
4. Branching hypothetical enemy responses and alternate follow-ups.
5. Knowledge limits: witnessed or studied techniques are easier to identify; unknown techniques still obey combat physics, but their full data may be hidden.
6. A clear difference between *what this move does under a hypothetical response* and *what the enemy will actually choose*.

**[DECIDED]** The player should never be forced to rush a single-player planning **or reaction** choice. **[PROPOSED]** Offer streamlined handling for trivial encounters later to prevent long interruptions, but do not sacrifice the depth of important duels.

### 31.1. Hierarchical combat-command interface

**[PROPOSED — UI DIRECTION]** Replace the prototype's permanently visible flat list of four moves with a **hierarchical command menu** that can scale to many techniques. Candidate top-level entries are **Attack / Defend / Move / Special**, each opening a submenu of available actions. Example Move entries: Walk, Dash, Jump, Dodge, and later Qinggong abilities. A chosen category does not itself spend combat time; selecting the actual action and confirming it commits the **one** action. The timeline, hypothetical opponent branches, and 3D ghost previews remain available while browsing.

**[OPEN — PRESENTATION]** The creator is considering either a **radial menu** or a **Persona-style command menu**. Neither visual layout nor these exact four category names are finalized. Special is a proposed grouping for unusual Qi, utility, transformation, or otherwise atypical abilities, not a hard rule that all powerful attacks must be removed from Attack. The interface should support a data-driven action catalog and avoid hardcoding combat mechanics into menu layout.

**[PROPOSED]** When walking or another reactable action is interrupted by an observable threat, reuse the same command selection and hypothetical preview UI, with the current action, available legal transitions, and the remaining time until the threat may arrive made clear. Avoid filling the screen with every future technique simultaneously.

## 32. Spatial combat and Qinggong

**[DECIDED IN PRINCIPLE]** 3D positioning is strategically meaningful, including distance, angle, facing, elevation, terrain, walls, knockback, weapon reach, and movement routes. **Movement is an explicit action choice** (Walk, Dash, Jump, Dodge, or higher-level techniques); basic attacks do not automatically close distance. Qinggong/lightness skills enable martial-arts fantasy movement—leaps, rooftop traversal, dashes, eventual extraordinary motion—and support both exploration and combat. Different movement arts have different early commitment and later reaction windows.

**[PROPOSED]** At higher cultivation levels, tactical concepts evolve: spatial positioning may become crossing boundaries or spatial distortion; defense may redirect laws/attacks; counters may stabilize space, negate exotic movement, or exploit preparation. The tactical system must remain intelligible, and strength disparity must remain consequential.

## 33. Determinism, fairness, and replay

**[DECIDED IN PRINCIPLE]** The *combat simulation* must determine results; the animation system depicts them. 3D ragdolls, frame-rate-dependent motion, or animation quirks cannot contradict the preview or change who hit whom.

**[PROPOSED]** Implement an independent fixed-timestep rules engine using recorded inputs, fixed or tightly controlled numeric operations, positions/facings/hit volumes, **action commitment / reaction availability**, **observable threat events**, and testable state transitions. Predictions run the same resolver on alternate hypothetical inputs, including branches where a walk is interrupted by a reaction or a dash remains temporarily locked. Replays consume the actual committed decisions and timeline, rendering an uninterrupted fight with camera and effects. The authoritative state must retain ongoing actions, observation history, and timing across pauses; rendering and UMG must not decide when a reaction is legal.

**[PROPOSED]** A martial analysis journal could preserve seen techniques, known **instant-based timing**, counters, opponents' tendencies, and past exchange timelines. The player can review an earlier defeat like a chess game, including hypothetical branches. Novel abilities gained through training should enrich available decisions rather than just increase damage.

## 34. AI opponents and off-screen fights

NPC fighters use the same core technique definitions, relative combat rules, **movement commitment and reaction eligibility**, and knowledge constraints. Their strategy may vary with caution, experience, preparation, injuries, and learned enemy tendencies. They must not simply inspect the player's hidden move choice or react to an attack they have not perceived. NPCs should respond to threats during reactable actions under the same rules as the player.

**[PROPOSED]** Off-screen encounters use the authoritative combat rules where practical, potentially with batched resolution or calibrated abstractions for large-scale simulation. These must produce outcomes compatible with what a nearby player could have observed. Do not require high-frequency rendering or animation for distant fights.

**[PROPOSED]** Prototype **one-on-one duels first**. Multi-enemy tactical planning in 3D is a later challenge because the action and forecast branch space rises dramatically.

## 35. Combat examples (not final balance)

**[EXAMPLE]** If an opponent has **18 instants of recovery**, a **seven-instant** palm strike may connect before they can act; a **fifteen-instant** heavy blow is a tighter but larger punish; a **twenty-two-instant** technique cannot claim guaranteed priority; and repositioning might buy a better follow-up and greater **tempo**. Range, cancel windows, protection, and opponent options can overturn simplistic comparisons. These durations are illustrative, not final balance.

**[CURRENT TEST EXAMPLE — LEGACY UNITS]** The Unreal MCP capability prototype was originally implemented with *Quick Strike* (**6 prototype startup frames**, 10 damage), *Heavy Strike* (**13 prototype startup frames**, 24 damage), and *Guard* (immediate). These are **historical test fixtures in the prototype's previous units**, not approved instant values or final balance. Subsequent combat prototypes also used **automatic approach/lunge while attacking**. That motion was a prototype simplification and **must not be mistaken for the newly decided separate-movement action design**. Changing terminology or design requirements does not silently migrate existing C++ / Blueprint mechanics; the prototype needs an explicit, tested transition to single-action movement and reaction scheduling.

---

# PART VIII — PROCEDURAL GENERATION AND WORLD CONTINUITY

## 36. A different world, not a reshuffled map

**[DECIDED]** Exploration and discovery are reasons to choose fresh-world reincarnation. Each life should encounter novel world histories, resources, dominant sects, characters, power relationships, techniques, secret realms, and opportunities.

**[PROPOSED]** A generation pipeline:

1. Determine seed and cosmological/environmental conditions for the starting world.
2. Generate geographic regions, spiritual density, hazards, resource ecologies, and routes.
3. Generate history, political structures, fallen institutions, and prevailing cultivation traditions.
4. Generate sects, their leadership and guarded technique libraries, economies, and rivalries.
5. Generate NPC social graphs, relationships, obligations, ambitions, and personal starting conditions.
6. Distribute rumors, trials, techniques, events, inheritances, and locations consistent with history.
7. Validate the generated world is playable: initial cultivation opportunity, accessible supplies, routes, institutions, and no impossible critical conditions.
8. Spawn the player with appropriate low first-life or upgraded later-life starting aptitude.
9. Assign one themed System via the cross-life selection rule and start its first quest.

**[PROPOSED]** Mix handcrafted architecture, technique primitives, encounters, character archetypes, and regional motifs with procedural layout and simulation-driven change. Pure random terrain is insufficient.

## 37. Scope and simulation level of detail

**[DECIDED IN PRINCIPLE]** The fiction may contain billions of worlds, but the game must simulate only the spaces relevant to a life in meaningful detail. New worlds are generated on demand when ascension or travel makes them relevant. The next reincarnation starts a separate world state.

**[PROPOSED]** Spatial tiers: full 3D actors when visible/nearby; simplified schedules and travel nearby; event-driven NPC actions at distance; regional aggregates and faction-level updates far away. NPCs need stable IDs and state independent of rendered scene actors.

A world must withstand leader death, disaster, lost manuals, emptied settlements, broken trade, and failed quests. Succession, replacement NPCs, recruitment, business transfer, and recovery mechanisms prevent long-run collapse. This is not a rule that key NPCs are immortal—rather, institutions and objectives respond to their death.

---

# PART IX — HOW THE SYSTEMS CONNECT

## 38. The cross-system event model

**[PROPOSED]** All major gameplay subsystems share a small set of authoritative events and stable references. This enables simulation, quest updates, achievements, System quest progress, NPC memory, economy, faction politics, and saves to react to the same occurrence without duplicating logic.

**Example: defeating a sect disciple.** The combat resolver produces an outcome and witnessed techniques → the NPC updates injury, memory and rivalry → witnesses spread the news → a teacher changes training priorities → sect politics may shift → a sect mission or active themed System quest can observe the event **independently** → achievements may record a milestone if its own requirements are satisfied.

**Example: resource shortage.** An alchemy hall consumes its stocked herbs → inventory reports a shortage → sect leadership posts gathering missions → members earn CP for actual deliveries → pill production resumes → market prices and training opportunities react. A Spirit Farmer System may offer an unrelated or contextual one-quest objective, but it does not *become* the sect mission board.

**Example: demonic elder predation.** An elder evaluates disciples as investments, rivals, or cultivation ingredients → patronage and hidden intentions emerge → victims disappear or flee → rumors and evidence spread → the player can intervene, exploit the situation, or leave → relationships and faction law respond. No unavoidable assassination scene merely because an RNG roll fired.

## 39. Role of quests, objectives, and achievements

Keep three separate interfaces and accounts:

| Feature | Source | Available simultaneously | Reward | Persistence |
|---|---|---|---|---|
| **Sect missions** | An actual sect's needs and officials | Potentially multiple eligible assignments | Contribution points and sect benefits | Current life; tied to membership |
| **Themed System quest** | The run's one assigned System | **Exactly one, always visible** | Random themed rewards | Current life only |
| **Achievements** | Cross-life milestone catalog | Many possible milestones | Soul Imprints for passive upgrades | Across all lives |

**[DECIDED]** Achievements **must not** be merged into the System quest mechanism. The same action can incidentally advance several separate systems; their ledgers, presentation, and goals remain separate.

---

# PART X — EXPERIENCE, INTERFACE, AND CONTENT DIRECTION

## 40. Key game interfaces

**[PROPOSED]** The player's principal interfaces:

- **Third-person exploration HUD:** location, nearby opportunities, health/Qi as needed, interaction and navigation prompts, and a persistent compact one-quest System panel.
- **Tactical combat planning and reactions:** a **proposed hierarchical Attack / Defend / Move / Special menu** with ability submenus (radial versus Persona-style is open); one chosen action at a time; explicit preparation/active/recovery and commitment / reaction availability **in instants**; an instant-indexed timeline scrubber, exchange state, readable **tempo** cues, 3D ghost trajectories, range and facing, observable incoming threats, hypothetical branches, Qi/posture/health.
- **Combat replay:** controls for replay speed, camera framing, continuous playback, and inspection of decisive turns.
- **Cultivation and technique journal:** foundation, meridian/affinity picture, compatibility warnings, manuals owned versus mastered, breakthrough opportunities and risks.
- **Sect administration and mission board:** membership rank, contribution points, resource inventory, rank-gated library, faction relations, missions and political context.
- **World and relationship journal:** acquaintances, sects, observed techniques, rumors, known histories, personal events.
- **End-of-life and Soul Legacy:** life summary, achievements earned, Soul Imprints, permanent upgrades, next-life preparation.

**UI priority:** The one System quest is always visible but must not dominate the screen. Distinguish known facts from hypothetical simulation branches. Allow detailed study for players who enjoy slow strategy without obscuring essential state.

## 41. Art and presentation direction

**[PROPOSED]** Stylized, readable 3D rather than immediate photorealism. Visual effort goes first to characters, poses, silhouettes, movement, clarity of 3D reach, readable ghost projections, and replay animation. Locations can be assembled from modular, authored pieces to support procedural variety.

**[PROPOSED]** The first demo uses primitives or simple humanoid placeholders. High-grade character art, full-cinematic choreography, complex VFX, vast terrain, multiplayer, and universe-destroying battles are not early prerequisites.

## 42. Stories that the simulation should produce

**[EXAMPLE]** A weak outer disciple advances through low-level gathering missions, is bullied by an inner disciple, secretly learns a dangerous technique, and eventually wins a carefully calculated duel. The former bully trains a counter and returns months later.

**[EXAMPLE]** A player enters a demonic sect for rapid growth but discovers an elder's puppet-refinement scheme. They can court another elder, hide cultivation, escape, expose a rival, or attempt to turn the trap against its creator.

**[EXAMPLE]** A merchant with a farming-themed System starts an herb supply enterprise, accidentally changes a sect's pill availability, and attracts the attention of rival sect alchemists.

**[EXAMPLE]** A celebrated grandmaster dies peacefully, leaving a mature school and students who mourn them. The run counts as meaningful even if the character never ascended.

---

# PART XI — TECHNICAL IMPLEMENTATION AND AI DEVELOPMENT

## 43. Engine and workflow status

**[CURRENT]** The creator is actively experimenting with **Unreal Engine 5.8.4** on **Ubuntu Linux**, with **local Qwen through OpenCode** and the Unreal **Model Context Protocol (MCP)** plugin. Earlier design exploration recommended Godot/.NET; that was **not a permanently locked engine choice**. The current practical development direction is Unreal, pending real-world proof that the AI tooling works reliably.

**[CURRENT]** The developer's GPU is a **Radeon RX 7900 XTX with 24 GB VRAM**. Running the local Qwen model and Unreal simultaneously causes VRAM pressure. The model **already unloads after about 60 seconds of idle time**. A Qwen-triggered test runner that waits for unload, launches Unreal for a prepared test, saves results, exits, and lets the model reload is **proposed**, not yet independently verified end-to-end.

**[CURRENT]** The Unreal MCP capability report describes 40+ exposed toolsets, including scene editing, Blueprint graph/DSL operations, UMG, primitive meshes, data assets, automation, PIE, logs and UI inspection. It reports limitations around C++ compilation through its tools, binary import, packaging, latency and keeping the editor open. These are **agent-observed tool affordances**, not proof that every creation workflow succeeds.

**[CURRENT]** In a tactical duel prototype, Qwen reported that combat logic and the UMG HUD worked, while the fighter cylinder mesh remained `None` during PIE despite a discovered StaticMeshComponent. A separate camera/framing problem was also reported. The repair and verification were discussed but not confirmed as completed. Treat visual gameplay integration as **unproven**.

## 44. Recommended architecture

**[PROPOSED]** Separate game rules from the scene engine:

1. **Authoritative world state** — seeded generated world; entities, sects, inventory, relationships, techniques, cultivation, and event history.
2. **Combat rules core** — deterministic fixed-tick simulation with player-facing **instant** timing, **one active action per fighter**, separate attack / defense / movement abilities, commitment and reaction windows, visibility/observation-limited threat events, independent decision scheduling, reach, collision/defense, previews, exchange boundaries, tempo assessment, outcomes, and replay event stream.
3. **Living world simulation** — schedules, goals, politics, economy, travel, off-screen combat, rumors, event propagation.
4. **Quest and progression services** — sect mission assignments and CP; *one* active System quest; independent achievements; per-life and cross-life saves.
5. **Unreal presentation** — nearby actors, exploration, cameras, animation, VFX, level building, tactical UMG, cinematic replay.
6. **Tooling and tests** — command-line/headless simulation, editor automation, logging, schema validation, visual capture, performance reports, seed replays.

**[PROPOSED]** Keep content definitions data driven: technique tables, cultivation-method definitions, mission templates, System archetypes, reward pools, sect policies, NPC archetypes, region parameters. Stable identifiers and versioned save schemas are essential.

**[CURRENT PRACTICAL CONSTRAINT]** Blueprint-first may be appropriate for initial MCP capability tests because the agent can compile and edit Blueprints within the editor. For the long-lived deterministic simulation, **ordinary testable C++ is a strong option** once command-line Unreal Build Tool access is demonstrated. The final Blueprint/C++ division is [OPEN]; do not make a permanent architectural choice solely because MCP currently lacks a dedicated C++ compile button.

## 45. Unreal MCP development discipline

**[PROPOSED]** Give Qwen small, verifiable assignments with explicit asset scopes, success conditions, and a clear list of allowed changes. Do not ask the agent to implement the entire cultivation world in one task. Project-specific `AGENTS.md`, design documents, source control, and repeatable commands should keep decisions consistent between agent sessions.

**Verification loop:** inspect current state → make limited changes → compile/validate → run automated tests → PIE or screenshot inspection where necessary → inspect logs and runtime properties → repair failures → summarize and commit working checkpoints.

**[CURRENT]** The OpenCode workspace can contain the Unreal project as a nested directory; `AGENTS.md` / `opencode.json` in the workspace should not themselves be treated as Unreal gameplay assets. Preserve unrelated `.uproject`, Blueprint, and Content files.

## 46. VRAM-aware test execution

**[PROPOSED]** Implement a **single foreground runner** called by OpenCode: Qwen prepares a test and invokes runner → runner waits slightly longer than the existing 60-second model idle timeout → verifies Qwen is unloaded (`ollama ps`) → launches a **runner-owned** Unreal process → runs a specified test and records logs/screenshots → closes only its own Unreal process → waits for exit → returns a test summary → Qwen naturally reloads for the next inference.

**Critical:** Don't modify Ollama's keep-alive setting—it already unloads after 60 seconds. Don't kill a manually opened Unreal editor or a live model inference. Headless logic tests and rendered visual tests are distinct. Unreal MCP operations that require active agent deliberation may still need overlapping VRAM unless pre-batched into autonomous editor work. Do not claim the workflow is implemented until verified end-to-end.

---

# PART XII — PROTOTYPES, MILESTONES, AND QUALITY GATES

## 47. Proof of feasibility: the duel

**Immediate priority [CURRENT]:** Confirm Qwen can construct and *visually verify* a working 3D Unreal prototype. Current tests target two primitive fighters, a camera, a few deterministic moves, a prediction/selection UI, PIE execution, and observed attack/block visuals. The reported static mesh and framing issue must be genuinely resolved before calling the visual portion successful.

**[PROPOSED]** A deeper combat prototype has two combatants, a compact set of separate attack / defense / movement actions, accurate **preparation/active/recovery and reaction-window timing in instants**, simultaneous commitment when both are ready, **walking interrupted by an observable attack**, **dash/jump early lockouts followed by later reaction availability**, ghost move previews, branching hypothetical responses, **instant-indexed scrubbing**, clear exchange boundaries and tempo cues, deterministic outcome tests, and seamless replay. A one-on-one duel must be enjoyable with placeholder art before many styles or multi-target combat are added.

## 48. Proof of the living simulation

**[PROPOSED]** In parallel, create a headless miniature world: 20–30 persistent NPCs, two sects with leaders and rank structures, relationships and training, one local economy, actual inventory, missions and CP, travel and event history. Advance simulated time in repeatable test runs and inspect outcomes. No 3D rendering required for that test.

**Critical test:** The player or an NPC challenges a disciple; the disciple learns from the result, trains a counter or changes plans, and acts differently during a later encounter. Replaying the same seed and inputs should produce a reproducible event log where applicable.

## 49. Forgotten Valley: first integrated vertical slice

**[PROPOSED]** One 3D valley, one village, nearby wilderness, two rival martial sects, approximately 25 independently tracked NPCs, three contrasting martial styles, basic cultivation and training, sect entrance trials and ranks, introductory manuals, CP missions, relationships, and actual persistent consequences within the life.

The player can explore, join a sect or remain independent, acquire a foundation-building method, learn a combat technique, duel, make allies/rivals, and influence the region. The **single themed System** and achievement ledger are separate. The game must support saving, reloading, and eventually dying and beginning a **fresh new** incarnation.

**Important scope constraint:** An endlessly growing galaxy, fully simulated cosmic combat, hundreds of styles, detailed dynasties, multiplayer, elaborate marriage systems, and all eight proposed Systems are *not* required to prove the first slice.

## 50. Acceptance tests

- **Determinism:** given the same combat state and committed action sequence, prediction and execution yield consistent outcomes.
- **Decision fairness:** NPC combat AI does not read unrevealed player choices; known **instant-based technique timings**, observable threats, and hidden enemy intent are differentiated.
- **One-action invariant:** a fighter cannot independently commit an attack and a separate movement action at the same time; ordinary attacks do not cause automatic approach or lunge.
- **Walk reaction:** while walking, an observable attack creates an eligible pause and a new choice without rewinding elapsed simulation time; unobservable enemy plans never trigger a privileged pause.
- **Dash / jump commitment:** a threat during the initial locked instants does not allow premature reaction; later, a fighter can react when legally eligible without resetting the threat's or movement's ongoing timing.
- **Reaction continuity:** choosing a reaction correctly transitions out of the previous action, retains applicable momentum/recovery, does not grant free defense, and reproduces prediction results under the same inputs.
- **NPC parity:** NPCs and player have the same timing, observation, and cancel restrictions; repeated reaction triggers cannot lock simulation into an infinite pause loop.
- **Visual fidelity to rules:** rendered hit and block events match the authoritative combat log.
- **Persistent NPC identity:** relationships and training developments survive leaving an area and reloading a save.
- **Sect economy:** gathering real ingredients changes stock; CP redemption cannot create unlimited resources from nothing.
- **Organizational recovery:** an elder or leader can die without orphaning the entire sect's membership or breaking all relevant objectives.
- **Long-run simulation:** advancing roughly 100 in-game days reveals no invalid references, frozen economies, or catastrophic unhandled disappearance of populations.
- **Quest invariant:** every active incarnation has exactly **one visible System quest**, including immediately after success, failure, reload, and world changes.
- **System discovery:** initial sequence exhausts unseen themes before any repeat; later draws are independent and can repeat.
- **Achievement separation:** System quest completion grants in-life rewards and does not itself award meta currency except when a distinct achievement condition is met.
- **Reincarnation:** death ends current world; next life generates a new world and System, retains soul upgrades, and resets world-bound state.
- **VRAM testing:** scripts do not kill manually started editors; tests report actual pass/failure and resource outcomes rather than merely successful tool calls.

---

# PART XIII — OPEN QUESTIONS AND CHANGE CONTROL

## 51. Decisions that should NOT silently change

1. Name: **Beneath Uncounted Heavens**.
2. Third-person 3D exploration; equal ambition for living simulation and tactical combat.
3. Turn/decision-based, unlimited-time tactical planning **and reaction decisions**; *Your Only Move Is HUSTLE* inspiration; **instants** measure timing, **exchanges** segment combat, and **tempo** describes tactical advantage; replay subordinate to the strategy.
4. **One action at a time:** Attack / Defend / Move / Special are alternative actions, not parallel attack-plus-movement commands. Ordinary attacks do not automatically reposition the fighter.
5. **Movement reaction windows:** walking can be interrupted at an observable attack; dashes and jumps begin with locked instants and allow reaction later. Reactions occur only when legal and based on information the fighter can perceive.
6. Every incarnation generates a **new world**; there is **no cross-life persistent world**.
7. First life begins with barely enough aptitude to cultivate; achievements enable Soul Imprints / future aptitude improvement.
8. Achievements and themed System quests are separate mechanics.
9. Exactly **one** active, **always visible** System quest; never a list, never a mandatory playstyle.
10. Each run gets one themed System; random without replacement until all types experienced, then independent random choice.
11. Orthodox sects have bullying and politics; demonic sects may sacrifice innocents, refine disciples and puppeteer promising victims; player may join either.
12. Sect entrance can grant a basic Foundation Building method and a library combat-technique choice; sects carefully guard knowledge.
13. Contribution points come from sect assignments, including starter herb gathering / weak-beast hunting, and purchase sect resources.
14. Demonic *technique* means severe self-inflicted risks and is not inherently morally evil.
15. Increasing specialization reduces efficiency of incompatible combat arts without forbidding their study.
16. Three secret Dao Ancestors war to become the sole source of Dao; the player is a fragment intended to be cultivated and reunited with one.
17. The cosmos spans billions of worlds/universes and supports absurdly high power; Dao Ancestor attainment is **nearly impossible**.

## 52. Open design decisions

The following remain **[OPEN]** and should not be invented as settled canon:

- Final original names, relationships, histories, visual language, and rules of the three Dao Ancestors.
- Whether the player retains character-level memories, fragments, or only soul aptitudes across rebirth.
- Exact cultivation realm ladder, breakthrough requirements, and how old age/immortality interact.
- The final calculation for compatibility, cultivation instability, injury, and Qi circulation.
- Whether meridians can be rebuilt and how demonic technique drawbacks may be treated.
- Which Systems make the final roster; exact distribution, scaling, quest replacement/failure policy, and a manual abandon option.
- Exact achievement catalog, Soul Imprint costs, caps, and meta-progression shape.
- Initial world size, how much of a multi-world ascension chain the first release supports, and performance budgets.
- Detailed legality and cultural behavior of particular sects; fine-grained reputation, contracts, and hierarchy.
- Final **milliseconds per instant**, internal simulation tick granularity, real-time animation interpolation, and detailed action-conflict/cancel rules; the provisional 50 ms/instant is not locked.
- Exact commitment durations, cancel consequences, remaining momentum, threat detection/perception, and reaction trigger deduplication for Walk, Dash, Jump, Dodge, and Qinggong.
- Whether any explicitly authored martial technique may someday combine damage and displacement as a rare exception; **the current default is separate movement and attacks**.
- Final hierarchical combat-menu presentation (**radial versus Persona-style**) and definitive top-level category names; Attack / Defend / Move / Special are a proposed starting taxonomy.
- Final health/posture/Qi model, damage/injury thresholds, and tactical UI ergonomics.
- Whether high-realm cosmic conflict is simulated at the same abstraction level or requires different presentation rules.
- Endgame possibility of willing merger, rebellion, or other transcendence; whether the origin fragment retains agency.
- Unreal MCP reliability, final engine commitment, long-term Blueprint/C++ split, and VRAM runner verification.

## 53. Production risks and responses

| Risk | Response |
|---|---|
| System interactions multiply and create invalid state | Stable IDs, events, automatic regression tests, seed replay, long-run simulation. |
| Combat previews contradict 3D animation | Rules-authoritative fixed-tick simulation; renderer only displays outcomes. |
| Reaction pauses reveal hidden enemy choices or permit impossible cancels | Trigger only from perceived events, enforce action-specific commitment and legal transitions, and test player/NPC parity. |
| Attack movement leaks back into combat or state is lost at reaction boundaries | Preserve complete authoritative state and regression-test one-action-per-fighter, no automatic attack lunge, and ongoing action continuity. |
| Too many tactical branches | Show meaningful branches, prune dominated/irrelevant paths; profile before optimization. |
| Sects collapse through random deaths | Succession, recruitment, economic demand, resource replenishment where plausible. |
| Demonic sect deaths feel arbitrary | Legible threats, actual NPC motives, opportunities for meaningful response. |
| Meta-progression trivializes a life | Potential-only upgrades, hard cultivation breakthroughs, costs and limits. |
| Themed System becomes mandatory | Viable core careers independent of System; no inactivity penalty. |
| Procedural worlds feel superficial | Generate cultures, histories, institutions, resources, and knowledge, not just terrain. |
| Qwen creates unverified Unreal assets | Compile, PIE, log, inspect runtime data and actual screenshots; no false “passed” status. |
| VRAM forces AI/engine contention | Batch work, use headless tests, and eventually verified model-sleep/engine-wake runner. |
| Ambition prevents completion | Stage work from duel and mini-simulation to a finished small valley before expansion. |
| Inspiration becomes unoriginal borrowing | Original terminology, mythology, expressive dialogue, assets, quest text, and designs. |

## 54. Glossary of working terms

**Dao:** Underlying cultivation principles / cosmic order; the Dao Ancestors are primordial sources of it in the hidden backstory.

**Dao Ancestor:** One of three hidden, primordial, unrivaled sources of Dao. A nearly unreachable attainment for the reincarnating player.

**Jianghu:** The social world of martial artists, sects, wanderers, feuds, reputations, teachings, and obligations.

**Qi:** Internal/spiritual energy used in cultivation and potentially specialized martial techniques.

**Foundation Building:** Early structured cultivation phase; sect recruits can be granted a basic foundational method. Exact realm name and place in final ladder are open.

**Qinggong:** Extraordinary movement/lightness techniques for traversal and combat positioning.

**Demonic technique:** Cultivation method defined by serious adverse effects on its practitioner; not automatically evil.

**Demonic sect:** An organization willing to sacrifice and exploit people for power; distinct from the classification of a technique.

**Sect contribution points (CP):** Sect-specific ledger credits earned for service, redeemable for stock-limited resources or rank-gated privileges.

**Themed System:** One incarnation-specific mechanic encouraging a lifestyle and continuously offering exactly one visible quest with random in-life rewards.

**Achievement:** Separate cross-life milestone awarding Soul Imprints / persistent unlocks, not one of the System's quests.

**Soul Imprints:** Proposed name for achievement-earned currency spent improving future reincarnations.

**Soul Legacy Tree:** Proposed name for permanent aptitude/talent/passive upgrades between lives.

**Samsara:** Reincarnation cycle: life → death → reflection and soul upgrades → new world, new life, new System.

**Instant:** The player-facing, discrete unit for tactical combat timings (preparation, strike, recovery, and timeline analysis). Not a rendering frame or an exposed millisecond count. A 50 ms mapping has been proposed but is unconfirmed.

**Exchange:** A segment of tactical combat that resolves until another meaningful decision opportunity. Its duration can vary and characters need not regain readiness together.

**Tempo:** Relative initiative and tactical advantage inferred from attack commitments, recovery, positioning, and threats; not inherently a bonus statistic.

**Reaction window:** Instants during an ongoing action when a fighter is eligible to replace or transition out of it in response to a **perceived** event. Reactability is independent of action duration and does not guarantee a successful defense.

**Commitment / locked instants:** Portion of an action, often at the beginning of Dash or Jump, when the fighter cannot choose a new reaction. Lost time cannot be retroactively undone.

**Movement action:** A separately chosen action such as Walk, Dash, Jump, or Dodge. The player selects **one action at a time**; ordinary attacks do not provide automatic approach movement.

**Combat command categories [PROPOSED]:** Attack / Defend / Move / Special, each opening available actions. The final radial or Persona-style presentation remains open.

## 55. Document governance

This bible is the **current single reference for project intent**. For each proposed implementation, the developer or AI agent should:

1. Identify which requirements are **[DECIDED]** and cannot be contradicted.
2. Keep **[PROPOSED]** content labeled until approved.
3. Surface **[OPEN]** decisions rather than silently turning examples into canon.
4. Update the bible when the creator makes a new decision, with a dated change note.
5. Maintain implementation-specific specs, test cases, and asset maps separately from the lore and design bible.

### Version notes

**Version 1.0 — 25 October 2026:** Initial consolidation of the game's selected title, living cultivation sandbox design, third-person tactical combat, sect politics and economy, demonic sects versus demonic techniques, specialization, achievement-driven reincarnation, fresh worlds, themed single-quest Systems, hidden primordial backstory, nearly unattainable Dao Ancestor goal, and current Unreal/Qwen development context.

**Version 1.1 — 26 October 2026:** Adopted the player-facing combat language **Instants** (timing), **Exchanges** (action-resolution segments), and **Tempo** (tactical advantage). Renamed move phases to **Preparation / Strike / Recovery** in the UI, while keeping deterministic fixed-tick simulation and rendering timing distinct. Provisional 50 ms per instant remains open; legacy Unreal prototype frame values have not been silently converted. Preserved the creator's intervening edits to thematic Systems and demonic techniques.

**Version 1.2 — 29 October 2026:** Locked **one action per fighter at a time** and made **movement actions separate from ordinary attacks** (no automatic approach/lunge). Adopted action-specific **reaction windows**: walking allows responses to observable attacks; dashing and jumping have initial locked instants followed by later reaction opportunities. Documented event-driven threat pauses, cancellation/continuity safeguards, NPC information parity, and associated simulation tests. Added the **proposed** Attack / Defend / Move / Special hierarchical command taxonomy; radial versus Persona-style presentation and exact timings remain open. Updated combat, interface, architecture, acceptance tests, glossary, and open questions; preserved unrelated user design changes.

**Design north star:** *One soul explores worlds without end; every life is distinct, every technique has a history and a cost, every victory is earned by understanding, and the heavens remain immeasurably greater than any one cultivator.*
