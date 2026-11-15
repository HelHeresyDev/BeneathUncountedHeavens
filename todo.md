## **STATUS: Memcpy crash fixed and verified**

- Replaced `FMemory::Memcpy` with `ContainerPtrToValuePtr<FBuhCombatState>` + C++ copy assignment.
- Added struct type validation (`StaticStruct()`) and fighter count check.
- C++ editor target rebuilt successfully.
- All 25 BUH automation tests pass (0 failures). No SIGSEGV in logs.
- See `notes.md` §9.58 for details.

## **I found a serious bug in the latest commit that could explain the crash.**

I checked the newest GitHub commit, `65d353e`, and found a potentially dangerous memory-management error in `CombatSim.cpp`.

The newly added `GenerateReactionPreviewFromManager()` contains:

```
FMemory::Memcpy(
    &State,
    StateProp->ContainerPtrToValuePtr<void>(CombatManager),
    StateProp->GetSize()
);
```

**This is unsafe because&#x20;**`FBuhCombatState`**&#x20;contains a&#x20;**`TArray<FBuhFighterState>`**.**

`FMemory::Memcpy` copies the array's internal pointer rather than independently copying its elements. The result is two combat-state objects that believe they own the same array allocation.

When the temporary state is destroyed, it can free memory still referenced by the combat manager. This can cause:

- Use-after-free errors.
- Double-free errors.
- Memory corruption.
- Unreal Editor crashes.

### **The required fix**

Replace the raw memory copy with a proper C++ copy:

```
const FBuhCombatState* Source =
    StateProp->ContainerPtrToValuePtr<FBuhCombatState>(
        CombatManager
    );

FBuhCombatState State = *Source;
```

Also validate that `StateProp->Struct == FBuhCombatState::StaticStruct()` before copying.

This preserves the complete combat state while correctly copying the array's elements.

**Do not run the current reaction-preview code again until that&#x20;**`Memcpy`**&#x20;has been removed.**

### **Exact replacement for the crashing function**

In `Source/BeneathUncountedHeav/CombatSim.cpp`, replace the entire function with:

```
int32 UBuhCombatSim::GenerateReactionPreviewFromManager(
    UObject* CombatManager,
    int32 PlayerDecision,
    int32 MaxInstants)
{
    if (!IsValid(CombatManager))
        return 0;

    FStructProperty* StateProp =
        FindFProperty<FStructProperty>(
            CombatManager->GetClass(),
            TEXT("CurrentState"));

    if (!StateProp ||
        StateProp->Struct != FBuhCombatState::StaticStruct())
    {
        UE_LOG(LogTemp, Error,
            TEXT("ReactionPreview: invalid CurrentState property"));
        return 0;
    }

    const FBuhCombatState* Source =
        StateProp->ContainerPtrToValuePtr<FBuhCombatState>(
            CombatManager);

    if (!Source || Source->Fighters.Num() < 2)
    {
        UE_LOG(LogTemp, Error,
            TEXT("ReactionPreview: invalid fighter state"));
        return 0;
    }

    const FBuhCombatState StateCopy = *Source;

    return GenerateAndStoreReactionPreview(
        StateCopy, PlayerDecision, MaxInstants).Num();
}
```

This is a focused change. No need to modify the slider or resolver.

### **Check the crash log**

On Ubuntu, run:

```
PROJECT="$HOME/Documents/Unreal Projects/BeneathUncountedHeav"

grep -iEn \
'Fatal error|SIGSEGV|double free|corrupt|ReactionPreview' \
"$PROJECT"/Saved/Logs/*.log | tail -60
```

After fixing the code, rebuild the Unreal Editor target and restart Unreal completely before testing.

**My assessment:** This is a high-confidence memory-corruption bug introduced in commit `65d353e`, and it is a plausible explanation for the crash. The crash log will help confirm whether it caused this particular failure.

I also checked the test code: none of the existing automated tests directly exercises this new reflection-copy function. That explains how such a serious error could pass the existing test suite.