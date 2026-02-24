import unreal


def dump(name):
    if not hasattr(unreal, name):
        unreal.log(f"missing {name}")
        return
    cls = getattr(unreal, name)
    unreal.log(f"=== {name} methods ===")
    for method in sorted([m for m in dir(cls) if not m.startswith("_")]):
        unreal.log(method)


def main():
    for name in ["EdGraph", "EdGraphNode", "EdGraphPin", "K2Node", "K2Node_CallFunction", "K2Node_MakeMap"]:
        dump(name)


if __name__ == "__main__":
    main()
