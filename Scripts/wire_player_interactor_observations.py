import sys
import unreal


ASSET_PATH = "/Game/LearningAgents/BP_PlayerLearningAgentsInteractor.BP_PlayerLearningAgentsInteractor"


def fail(message: str, code: int = 1) -> None:
    unreal.log_error(message)
    raise SystemExit(code)


def main() -> None:
    bp_obj = unreal.EditorAssetLibrary.load_asset(ASSET_PATH)
    if not bp_obj:
        fail(f"Failed to load blueprint asset: {ASSET_PATH}")

    blueprint = bp_obj
    if not isinstance(blueprint, unreal.Blueprint):
        fail(f"Loaded asset is not a Blueprint: {ASSET_PATH}")

    specify_graph = unreal.BlueprintEditorLibrary.find_graph(blueprint, "SpecifyAgentObservation")
    gather_graph = unreal.BlueprintEditorLibrary.find_graph(blueprint, "GatherAgentObservation")
    if not gather_graph:
        gather_graph = unreal.BlueprintEditorLibrary.find_graph(blueprint, "GatherObservations")

    unreal.log("Loaded BP_PlayerLearningAgentsInteractor successfully.")
    unreal.log(f"Specify graph found: {bool(specify_graph)}")
    unreal.log(f"Gather graph found: {bool(gather_graph)}")

    # UE5.6 Python API in this environment exposes graph lookup/compile/save, but not
    # programmatic K2 node spawn + pin wiring for function graphs.
    # We intentionally fail fast instead of pretending a partial edit succeeded.
    fail(
        "Cannot auto-wire K2 observation nodes via current Python API surface. "
        "BlueprintEditorLibrary exposes graph management but not call-node creation/pin links. "
        "Use editor graph paste/manual node wiring for Gather/Specify, then rerun build.")


if __name__ == "__main__":
    try:
        main()
    except SystemExit:
        raise
    except Exception as exc:
        fail(f"Unhandled exception: {exc}")
