import unreal


def dump_methods(cls, label):
    names = sorted([n for n in dir(cls) if not n.startswith("_")])
    unreal.log("=== {} ({}) ===".format(label, len(names)))
    for name in names:
        unreal.log(name)


def main():
    dump_methods(unreal.BlueprintEditorLibrary, "BlueprintEditorLibrary")
    if hasattr(unreal, "KismetEditorUtilities"):
        dump_methods(unreal.KismetEditorUtilities, "KismetEditorUtilities")
    if hasattr(unreal, "EditorAssetLibrary"):
        dump_methods(unreal.EditorAssetLibrary, "EditorAssetLibrary")
    if hasattr(unreal, "AssetToolsHelpers"):
        dump_methods(unreal.AssetToolsHelpers, "AssetToolsHelpers")
    if hasattr(unreal, "GraphEditorSubsystem"):
        dump_methods(unreal.GraphEditorSubsystem, "GraphEditorSubsystem")


if __name__ == "__main__":
    main()
