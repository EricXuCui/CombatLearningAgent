import unreal


def dump_if_exists(name):
    if hasattr(unreal, name):
        cls = getattr(unreal, name)
        unreal.log(f"=== {name} ===")
        for method in sorted([m for m in dir(cls) if not m.startswith("_")]):
            if "graph" in method.lower() or "node" in method.lower() or "import" in method.lower() or "paste" in method.lower():
                unreal.log(method)


def main():
    for name in [
        "EdGraph",
        "EdGraphNode",
        "EdGraphSchema",
        "EdGraphSchemaK2",
        "EdGraphUtilities",
        "GraphEditorSubsystem",
        "BlueprintEditorLibrary",
        "KismetEditorUtilities",
    ]:
        dump_if_exists(name)

    unreal.log("=== module names containing graph/import/paste/node ===")
    for name in sorted(dir(unreal)):
        low = name.lower()
        if "graph" in low or "import" in low or "paste" in low or "node" in low:
            unreal.log(name)


if __name__ == "__main__":
    main()
