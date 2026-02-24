#include "WireInteractorObservationsCommandlet.h"

#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

#if WITH_EDITOR
#include "AssetRegistry/AssetRegistryModule.h"
#include "EdGraphUtilities.h"
#include "Editor.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Engine/Blueprint.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_FunctionResult.h"
#endif

UWireInteractorObservationsCommandlet::UWireInteractorObservationsCommandlet()
{
	IsClient = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 UWireInteractorObservationsCommandlet::Main(const FString& Params)
{
#if !WITH_EDITOR
	UE_LOG(LogTemp, Error, TEXT("This commandlet requires an editor build."));
	return 1;
#else
	FString BlueprintPath = TEXT("/Game/LearningAgents/BP_PlayerLearningAgentsInteractor.BP_PlayerLearningAgentsInteractor");
	FString SpecifyTextPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("Specify agent observation.txt"));
	FString GatherTextPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("Gather observation.txt"));

	FParse::Value(*Params, TEXT("Blueprint="), BlueprintPath);
	FParse::Value(*Params, TEXT("SpecifyText="), SpecifyTextPath);
	FParse::Value(*Params, TEXT("GatherText="), GatherTextPath);

	auto StripFunctionBoundaryNodes = [](const FString& SourceText) -> FString
	{
		TArray<FString> Lines;
		SourceText.ParseIntoArrayLines(Lines, false);

		TArray<FString> OutLines;
		OutLines.Reserve(Lines.Num());

		bool bInsideBlock = false;
		bool bDropBlock = false;

		for (const FString& Line : Lines)
		{
			const bool bBeginObject = Line.StartsWith(TEXT("Begin Object"));
			const bool bEndObject = Line.StartsWith(TEXT("End Object"));

			if (bBeginObject)
			{
				bInsideBlock = true;
				bDropBlock = Line.Contains(TEXT("Class=/Script/BlueprintGraph.K2Node_FunctionEntry"))
					|| Line.Contains(TEXT("Class=/Script/BlueprintGraph.K2Node_FunctionResult"));
			}

			if (!bDropBlock)
			{
				OutLines.Add(Line);
			}

			if (bInsideBlock && bEndObject)
			{
				bInsideBlock = false;
				bDropBlock = false;
			}
		}

		return FString::Join(OutLines, TEXT("\n"));
	};

	auto FindGraphByName = [](UBlueprint* Blueprint, const TCHAR* GraphName) -> UEdGraph*
	{
		if (!Blueprint || !GraphName || FCString::Strlen(GraphName) == 0)
		{
			return nullptr;
		}

		const FName WantedName(GraphName);
		for (UEdGraph* Graph : Blueprint->FunctionGraphs)
		{
			if (Graph && Graph->GetFName() == WantedName)
			{
				return Graph;
			}
		}

		for (UEdGraph* Graph : Blueprint->UbergraphPages)
		{
			if (Graph && Graph->GetFName() == WantedName)
			{
				return Graph;
			}
		}

		return nullptr;
	};

	auto ImportGraphFromText = [&](UBlueprint* Blueprint, const TCHAR* PreferredGraphName, const TCHAR* AlternateGraphName, const FString& TextPath) -> bool
	{
		if (TextPath.IsEmpty())
		{
			UE_LOG(LogTemp, Warning, TEXT("No text path provided for %s."), PreferredGraphName);
			return true;
		}

		if (!IFileManager::Get().FileExists(*TextPath))
		{
			UE_LOG(LogTemp, Warning, TEXT("File not found for %s: %s (skipping)"), PreferredGraphName, *TextPath);
			return true;
		}

		UEdGraph* TargetGraph = FindGraphByName(Blueprint, PreferredGraphName);
		if (!TargetGraph && AlternateGraphName && FCString::Strlen(AlternateGraphName) > 0)
		{
			TargetGraph = FindGraphByName(Blueprint, AlternateGraphName);
		}
		if (!TargetGraph)
		{
			UE_LOG(LogTemp, Error, TEXT("Could not find graph '%s' (or alternate '%s')."), PreferredGraphName, AlternateGraphName ? AlternateGraphName : TEXT(""));
			return false;
		}

		FString ImportText;
		if (!FFileHelper::LoadFileToString(ImportText, *TextPath))
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to read file: %s"), *TextPath);
			return false;
		}

		bool bHasFunctionEntry = false;
		for (const UEdGraphNode* Node : TargetGraph->Nodes)
		{
			if (Node && Node->IsA<UK2Node_FunctionEntry>())
			{
				bHasFunctionEntry = true;
				break;
			}
		}

		if (bHasFunctionEntry)
		{
			ImportText = StripFunctionBoundaryNodes(ImportText);
		}

		if (ImportText.TrimStartAndEnd().IsEmpty())
		{
			UE_LOG(LogTemp, Warning, TEXT("Import text is empty for graph '%s' from file %s (skipping)."), PreferredGraphName, *TextPath);
			return true;
		}

		TArray<UEdGraphNode*> ExistingNodes = TargetGraph->Nodes;
		for (UEdGraphNode* Node : ExistingNodes)
		{
			if (!Node)
			{
				continue;
			}

			if (bHasFunctionEntry && (Node->IsA<UK2Node_FunctionEntry>() || Node->IsA<UK2Node_FunctionResult>()))
			{
				continue;
			}

			FBlueprintEditorUtils::RemoveNode(Blueprint, Node, true);
		}

		for (UEdGraphNode* Node : TargetGraph->Nodes)
		{
			if (bHasFunctionEntry)
			{
				if (UK2Node_FunctionEntry* Entry = Cast<UK2Node_FunctionEntry>(Node))
				{
					Entry->ReconstructNode();
					break;
				}
			}
		}

		TSet<UEdGraphNode*> ImportedNodes;
		FEdGraphUtilities::ImportNodesFromText(TargetGraph, ImportText, ImportedNodes);
		if (ImportedNodes.Num() == 0)
		{
			UE_LOG(LogTemp, Error, TEXT("Imported 0 nodes into graph '%s' from %s"), PreferredGraphName, *TextPath);
			return false;
		}

		for (UEdGraphNode* ImportedNode : ImportedNodes)
		{
			if (ImportedNode)
			{
				ImportedNode->CreateNewGuid();
				ImportedNode->PostPlacedNewNode();
				ImportedNode->ReconstructNode();
			}
		}

		UE_LOG(LogTemp, Display, TEXT("Imported %d nodes into graph '%s' from: %s"), ImportedNodes.Num(), PreferredGraphName, *TextPath);
		return true;
	};

	UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *BlueprintPath);
	if (!Blueprint)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load blueprint: %s"), *BlueprintPath);
		return 1;
	}

	bool bOk = true;
	bOk &= ImportGraphFromText(Blueprint, TEXT("SpecifyAgentObservation"), TEXT("SpecifyObservations"), SpecifyTextPath);
	bOk &= ImportGraphFromText(Blueprint, TEXT("GatherAgentObservation"), TEXT("GatherObservations"), GatherTextPath);

	if (!bOk)
	{
		UE_LOG(LogTemp, Error, TEXT("Graph import failed; blueprint not saved."));
		return 1;
	}

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
	FKismetEditorUtilities::CompileBlueprint(Blueprint);

	UPackage* Package = Blueprint->GetOutermost();
	if (!Package)
	{
		UE_LOG(LogTemp, Error, TEXT("Could not resolve package for blueprint save."));
		return 1;
	}

	const FString PackageName = Package->GetName();
	const FString PackageFileName = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());

	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	SaveArgs.SaveFlags = SAVE_NoError;

	const bool bSaved = UPackage::SavePackage(Package, Blueprint, *PackageFileName, SaveArgs);
	if (!bSaved)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to save blueprint package: %s"), *PackageFileName);
		return 1;
	}

	UE_LOG(LogTemp, Display, TEXT("Successfully rewired and saved interactor blueprint: %s"), *BlueprintPath);
	return 0;
#endif
}
