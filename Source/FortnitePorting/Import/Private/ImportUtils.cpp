#include "FortnitePorting/Import/Public/ImportUtils.h"
#include "FortnitePorting/Blueprint/Public/BlueprintUtils.h"
#include "FortnitePorting/Import/Public/RemoteImportUtils.h"

#include "AutomatedAssetImportData.h"
#include "EditorAssetLibrary.h"
#include "FortnitePorting.h"
#include "JsonObjectConverter.h"
#include "PluginUtils.h"
#include "PskFactory.h"
#include "PskxFactory.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Async/Async.h"
#include "Interfaces/IPluginManager.h"
#include "Materials/MaterialInstanceConstant.h"
#include "ComponentReregisterContext.h"
#include "Misc/ScopedSlowTask.h"
#include "Factories/MaterialInstanceConstantFactoryNew.h"
#include "Factories/UEModelFactory.h"
#include "KismetCompilerModule.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Engine/SimpleConstructionScript.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/SavePackage.h"
#include "PackageTools.h"
#include "AssetToolsModule.h"


void FImportUtils::CheckForDependencies()
{
	if (DefaultMaterial == nullptr)
		DefaultMaterial = Cast<UMaterial>(UEditorAssetLibrary::LoadAsset("/FortnitePorting/Base_Materials/M_FortnitePorting_Default"));

	if (UEFNMaterial == nullptr)
		UEFNMaterial = Cast<UMaterial>(UEditorAssetLibrary::LoadAsset("/FortnitePorting/Base_Materials/M_FortnitePorting_UEFN"));
}

UMaterial* FImportUtils::GetMaterial()
{
	return CurrentExport.Settings.ForUEFN ? UEFNMaterial : DefaultMaterial;
}

FString FImportUtils::WrapPathWithImportRootFolder(const FString& Folder)
{
	auto RootName = Folder.RightChop(1);
	RootName = RootName.Left(RootName.Find("/"));

	if (RootName == "Game")
	{
		FString RelativePath;
		Folder.Split(TEXT("/Game/"), nullptr, &RelativePath);
		return "/Game/" + IMPORT_ROOT_FOLDER + "/" + RelativePath;
	}

	return Folder;
	
}

auto FImportUtils::SplitExportPath(const FString& InStr)
{
	auto RootName = InStr.RightChop(1);
	RootName = RootName.Left(RootName.Find("/"));

	if (!RootName.Equals("Game") && !RootName.Equals("Engine") && IPluginManager::Get().FindPlugin(RootName) == nullptr)
	{
		FPluginUtils::FNewPluginParamsWithDescriptor CreationParams;
		CreationParams.Descriptor.FriendlyName = RootName;
		CreationParams.Descriptor.VersionName = "1.0";
		CreationParams.Descriptor.Version = 1;
		CreationParams.Descriptor.Category = "Fortnite Porting";
		CreationParams.Descriptor.CreatedBy = "Fortnite Porting";
		CreationParams.Descriptor.CreatedByURL = "https://github.com/halfuwu/FortnitePortingServerUnreal";
		CreationParams.Descriptor.Description = RootName + " Content Plugin";
		CreationParams.Descriptor.bCanContainContent = true;

		FPluginUtils::FLoadPluginParams LoadParams;
		LoadParams.bEnablePluginInProject = true;
		LoadParams.bUpdateProjectPluginSearchPath = true;
		LoadParams.bSelectInContentBrowser = true;

		FPluginUtils::CreateAndLoadNewPlugin(RootName, FPaths::ProjectPluginsDir(), CreationParams, LoadParams);
	}

	FString Path;
	FString ObjectName;
	InStr.Split(".", &Path, &ObjectName);

	FString Folder;
	FString PackageName;
	Path.Split(TEXT("/"), &Folder, &PackageName, ESearchCase::IgnoreCase, ESearchDir::FromEnd);

	struct FSplitResult
	{
		FString Path;
		FString ObjectName;
		FString Folder;
	};

	return FSplitResult {
		Path,
		ObjectName,
		Folder
	};
}

void FImportUtils::ImportResponse(const FString& Response)
{
	FExport Export;
	if (!FJsonObjectConverter::JsonObjectStringToUStruct(Response, &Export))
	{
		UE_LOG(LogFortnitePorting, Error, TEXT("Unable to deserialize response from FortnitePorting."));
		return;
	}

	CheckForDependencies();

	CurrentExport = Export;

	auto AssetsRoot = Export.AssetsFolder;
	auto BaseData = Export.Data;

	FScopedSlowTask ImportTask(BaseData.Num(), FText::FromString("Importing..."));
	ImportTask.MakeDialog(true);
	auto ResponseIndex = 0;
	for (const auto Data : BaseData)
	{
		ResponseIndex++;
		if (ImportTask.ShouldCancel())
			break;
		
		auto Name = Data.Name;
		auto Type = Data.Type;
		UE_LOG(LogFortnitePorting, Log, TEXT("Received Import for %s: %s"), *Type, *Name)

		ImportTask.DefaultMessage = FText::FromString(FString::Printf(TEXT("Importing %s: %s (%d of %d)"), *Type, *Name, ResponseIndex, BaseData.Num()));
		ImportTask.EnterProgressFrame();
		
		if (Type.Equals("Dance"))
		{
			// TODO
		}
		else
		{
			TMap<FString, FPartData> ImportedParts;
			TMap<FString, FString> StaticMeshAssetPaths;
			TMap<FString, FString> SkeletalMeshAssetPaths;

			auto ImportParts = [&](FString Name, TArray<FExportMesh> Meshes)
			{
				for (auto Mesh : Meshes)
				{
					if (ImportedParts.Contains(Mesh.Type) && (Mesh.Type.Equals("Outfit") || Mesh.Type.Equals("Backpack"))) continue;
					
					const auto Imported = ImportMesh(Mesh);

					// Skipped mesh import if mesh already exists at path
					if(Imported == nullptr)
					{
						continue;
					}

					ImportedParts.Add(Mesh.Type, FPartData(Imported, Mesh));
					
					for (auto Material : Mesh.Materials)
					{
						ImportMaterial(Material);
					}

					// Assign materials to Mesh After Import and resave
					TMap<FString, FString> MaterialNameToPathMap;
					for (auto Material : Mesh.Materials)
					{
						auto [MatPath, MatObjectName, MatFolder] = SplitExportPath(Material.Path);
						MaterialNameToPathMap.Add(Material.Name, WrapPathWithImportRootFolder(MatFolder));
					}

					UStaticMesh * StaticMesh = Cast<UStaticMesh>(Imported);
					USkeletalMesh * SkeletalMesh = Cast<USkeletalMesh>(Imported);

					FSavePackageArgs SaveArgs;
					SaveArgs.TopLevelFlags = RF_Standalone | RF_Public;

					if (StaticMesh != nullptr)
					{
						StaticMesh->Modify();
						StaticMesh->PreEditChange(nullptr);

						for ( int i = 0; i<StaticMesh->GetStaticMaterials().Num();i++)
						{
							FString MaterialSlotName = StaticMesh->GetStaticMaterials()[i].MaterialSlotName.ToString();
							FString FoundMaterialPath = *MaterialNameToPathMap.Find(MaterialSlotName);
							FString MaterialPackagePath = FPaths::Combine(FoundMaterialPath, MaterialSlotName);

							UMaterialInterface * Material = Cast<UMaterialInterface>(StaticLoadObject(UObject::StaticClass(), nullptr, *MaterialPackagePath));

							if(Material == nullptr){
								UE_LOG(LogFortnitePorting, Error, TEXT("Static Mesh Material not found at: %s"),*MaterialPackagePath);
							}

							StaticMesh->SetMaterial(i, Material);
						}

						StaticMesh->PostEditChange();
						StaticMesh->GetPackage()->MarkPackageDirty();

						FString StaticMeshPackageFileName = FPackageName::LongPackageNameToFilename(StaticMesh->GetPackage()->GetPathName(), FPackageName::GetAssetPackageExtension());
						UPackage::SavePackage(StaticMesh->GetPackage(), StaticMesh, *StaticMeshPackageFileName, SaveArgs);

						StaticMeshAssetPaths.Add(Mesh.Name, Mesh.Path);
					}

					if (SkeletalMesh != nullptr)
					{	
						SkeletalMesh->Modify();
						SkeletalMesh->PreEditChange(nullptr);

						for ( int i = 0; i<SkeletalMesh->GetMaterials().Num();i++)
						{
							FString MaterialSlotName = SkeletalMesh->GetMaterials()[i].MaterialSlotName.ToString();
							FString FoundMaterialPath = *MaterialNameToPathMap.Find(MaterialSlotName);
							FString MaterialPackagePath = FPaths::Combine(FoundMaterialPath, MaterialSlotName);

							UMaterialInterface * MaterialInstance = Cast<UMaterialInterface>(StaticLoadObject(UObject::StaticClass(), nullptr, *MaterialPackagePath));
							
							if(MaterialInstance == nullptr){
								UE_LOG(LogFortnitePorting, Error, TEXT("Skeletal Mesh Material Instance not found at: %s"),*MaterialPackagePath);
							}
							
							SkeletalMesh->GetMaterials()[i] = MaterialInstance;
							SkeletalMesh->GetMaterials()[i].MaterialSlotName = FName(*MaterialSlotName);
						}

						SkeletalMesh->PostEditChange();
						SkeletalMesh->GetPackage()->MarkPackageDirty();
						
						FString SkeletalMeshPackageFileName = FPackageName::LongPackageNameToFilename(SkeletalMesh->GetPackage()->GetPathName(), FPackageName::GetAssetPackageExtension());
						UPackage::SavePackage(SkeletalMesh->GetPackage(), SkeletalMesh, *SkeletalMeshPackageFileName, SaveArgs);

						SkeletalMeshAssetPaths.Add(Mesh.Name, Mesh.Path);
					}
					
				}
				
				// Create actor and setup blueprint
				FString ImportAssetPath = "/Game/" + IMPORT_ROOT_FOLDER + "/Blueprints";
				FString ActorBlueprintName = "BP_" + UPackageTools::SanitizePackageName(*Name);
				FString ActorAssetPath = FPaths::Combine(ImportAssetPath, ActorBlueprintName);

				if(StaticLoadObject(UObject::StaticClass(), nullptr, *ActorAssetPath) != nullptr)
				{
					UE_LOG(LogFortnitePorting, Error, TEXT("Skipping Actor Blueprint Generation. An asset already exists at: %s"),*ActorAssetPath);
				}
				else{
					UBlueprint* ActorBlueprint = CreateActorBlueprint(ActorAssetPath);
					GenerateActorComponents(ActorBlueprint, StaticMeshAssetPaths, SkeletalMeshAssetPaths);
				}			
			};
			
			ImportParts(Data.Name, Data.Meshes);
		}
	}
}

void FImportUtils::GenerateActorComponents(UBlueprint* ActorBlueprint, TMap<FString, FString> StaticMeshAssetPaths, TMap<FString, FString> SkeletalMeshAssetPaths)
{
	auto [BPPath, BPObjectName, BPFolder] = SplitExportPath(ActorBlueprint->GetPathName());
		
	FBlueprintUtils::AddSceneComponentToBlueprint(BPPath,"Root","");

	for (const auto& SkeletalMeshAssetPath : SkeletalMeshAssetPaths)
	{
		FString ComponentName = SkeletalMeshAssetPath.Key;
		FString ComponentAssetPath = SkeletalMeshAssetPath.Value;
		auto [Path, ObjectName, Folder] = SplitExportPath(ComponentAssetPath);
		Path = WrapPathWithImportRootFolder(Path);

		FBlueprintUtils::AddSkeletalMeshComponentToBlueprint(BPPath,ComponentName,"Root",Path);
	}

	for (const auto& StaticMeshAssetPath : StaticMeshAssetPaths)
	{
		FString ComponentName = StaticMeshAssetPath.Key;
		FString ComponentAssetPath = StaticMeshAssetPath.Value;
		auto [Path, ObjectName, Folder] = SplitExportPath(ComponentAssetPath);
		Path = WrapPathWithImportRootFolder(Path);

		FBlueprintUtils::AddStaticMeshComponentToBlueprint(BPPath,ComponentName,"Root",Path);
	}

	FKismetEditorUtilities::CompileBlueprint(ActorBlueprint, EBlueprintCompileOptions::SkipGarbageCollection);
	FString PackageFileName = FPackageName::LongPackageNameToFilename(ActorBlueprint->GetPackage()->GetPathName(), FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Standalone | RF_Public;
	UPackage::SavePackage(ActorBlueprint->GetPackage(), ActorBlueprint, *PackageFileName, SaveArgs);
}

UBlueprint* FImportUtils::CreateActorBlueprint(FString ActorBlueprintAssetPath)
{
	UE_LOG(LogFortnitePorting, Display, TEXT("Creating Actor Blueprint at: %s"),*ActorBlueprintAssetPath);
	if(StaticLoadObject(UObject::StaticClass(), nullptr, *ActorBlueprintAssetPath) != nullptr)
	{
		UE_LOG(LogFortnitePorting, Error, TEXT("An asset already exists at: %s"),*ActorBlueprintAssetPath);
		return nullptr;
	}

	UPackage * ActorPackage = CreatePackage(*ActorBlueprintAssetPath);
	if (ActorPackage == nullptr){
		UE_LOG(LogFortnitePorting, Error, TEXT("Unable to create package at: %s"),*ActorBlueprintAssetPath);
		return nullptr;
	}

	UClass* ActorBlueprintClass = nullptr;
	UClass* ActorBlueprintGenClass = nullptr;

	FModuleManager::LoadModuleChecked<IKismetCompilerInterface>("KismetCompiler").GetBlueprintTypesForClass(AActor::StaticClass(), ActorBlueprintClass, ActorBlueprintGenClass);
	UBlueprint * ActorBlueprint = FKismetEditorUtilities::CreateBlueprint(AActor::StaticClass(), ActorPackage, *FPaths::GetBaseFilename(ActorBlueprintAssetPath), BPTYPE_Normal, ActorBlueprintClass, ActorBlueprintGenClass);
   
	FAssetRegistryModule::AssetCreated(ActorBlueprint);
	ActorBlueprint->MarkPackageDirty();


	FString PackageFileName = FPackageName::LongPackageNameToFilename(ActorPackage->GetPathName(), FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Standalone | RF_Public;
	UPackage::SavePackage(ActorPackage, ActorBlueprint, *PackageFileName, SaveArgs);
	
	UE_LOG(LogFortnitePorting, Display, TEXT("Created Actor Blueprint at: %s"),*ActorBlueprintAssetPath);

	return ActorBlueprint;
}

UObject* FImportUtils::ImportMesh(const FExportMesh& Mesh)
{
	auto [Path, ObjectName, Folder] = SplitExportPath(Mesh.Path);
	FString ImportPath = WrapPathWithImportRootFolder(Path);

	TMap<FString, FString> MaterialNameToPathMap;
	for (auto Material : Mesh.Materials)
	{
		auto [MatPath, MatObjectName, MatFolder] = SplitExportPath(Material.Path);
		FString ImportMatFolder = WrapPathWithImportRootFolder(MatFolder);
		MaterialNameToPathMap.Add(Material.Name, ImportMatFolder);
	}
   
	auto SourceMeshPath = FPaths::Combine(CurrentExport.AssetsFolder, Path);
	auto MeshPath = FPaths::Combine(CurrentExport.AssetsFolder, Path + "_LOD0");

	if (FPaths::FileExists(MeshPath + ".psk"))
	{
		return UPskFactory::Import(MeshPath + ".psk", CreatePackage(*Folder), FName(*ObjectName), RF_Public | RF_Standalone, MaterialNameToPathMap);
	}

	if (FPaths::FileExists(MeshPath + ".pskx"))
	{
		return UPskxFactory::Import(MeshPath + ".pskx", CreatePackage(*Folder), FName(*ObjectName), RF_Public | RF_Standalone, MaterialNameToPathMap);
	}
   
	if (FPaths::FileExists(SourceMeshPath + ".uemodel"))
	{
		return UEModelFactory::Import(SourceMeshPath + ".uemodel", ImportPath, FName(*ObjectName), RF_Public | RF_Standalone, MaterialNameToPathMap);
	}


	UE_LOG(LogFortnitePorting, Error, TEXT("Unsupported mesh format: %s"),*Mesh.Path);
   
	return nullptr;
}

void FImportUtils::ImportMaterial(const FExportMaterial& Material)
{
	if (!CurrentExport.Settings.ImportMaterials) return;
   
	auto [Path, ObjectName, Folder] = SplitExportPath(Material.Path);
	
	FString ImportPath = WrapPathWithImportRootFolder(Path);
   
	const auto MatPackage = CreatePackage(*ImportPath);

	auto MaterialInstance = LoadObject<UMaterialInstanceConstant>(MatPackage, *ObjectName);
	if (MaterialInstance == nullptr)
	{
		UMaterialInstanceConstantFactoryNew* MaterialFactory = NewObject<UMaterialInstanceConstantFactoryNew>();
		MaterialFactory->InitialParent = GetMaterial();

		MaterialInstance = (UMaterialInstanceConstant*)MaterialFactory->FactoryCreateNew(UMaterialInstanceConstant::StaticClass(), MatPackage, *ObjectName, RF_Standalone | RF_Public, NULL, GWarn);
	}

	MaterialInstance->PreEditChange(nullptr);
	MaterialInstance->Parent = GetMaterial();

	for (auto TextureParameter : Material.Textures)
	{
		const auto Texture = ImportTexture(TextureParameter);
		const auto ParamName = TextureMappings.Find(TextureParameter.Name);
		if (ParamName != nullptr && Texture != nullptr)
		{
			MaterialInstance->SetTextureParameterValueEditorOnly(FMaterialParameterInfo(**ParamName, GlobalParameter), Texture);
		}
	}
	
	for (auto ScalarParameter : Material.Scalars)
	{
		if (const auto ParamName = ScalarMappings.Find(ScalarParameter.Name))
		{
			MaterialInstance->SetScalarParameterValueEditorOnly(FMaterialParameterInfo(**ParamName, GlobalParameter), ScalarParameter.Value);
		}
	}
   
	for (auto SwitchParameter : Material.Switches)
	{
		if (const auto ParamName = SwitchMappings.Find(SwitchParameter.Name))
		{
			MaterialInstance->SetStaticSwitchParameterValueEditorOnly(FMaterialParameterInfo(**ParamName, GlobalParameter), SwitchParameter.Value);
		}
	}

	MaterialInstance->SetScalarParameterValueEditorOnly(FMaterialParameterInfo("Ambient Occlusion", GlobalParameter), CurrentExport.Settings.AmbientOcclusion);
	MaterialInstance->SetScalarParameterValueEditorOnly(FMaterialParameterInfo("Cavity", GlobalParameter), CurrentExport.Settings.Cavity);
	MaterialInstance->SetScalarParameterValueEditorOnly(FMaterialParameterInfo("Subsurface", GlobalParameter), CurrentExport.Settings.Subsurface);

	MaterialInstance->PostEditChange();

	MaterialInstance->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(MaterialInstance);

	FString PackageFileName = FPackageName::LongPackageNameToFilename(MatPackage->GetPathName(), FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Standalone | RF_Public;
	UPackage::SavePackage(MatPackage, MaterialInstance, *PackageFileName, SaveArgs);

	MatPackage->FullyLoad();
   
	// FGlobalComponentReregisterContext RecreateComponents;
	
}

UTexture* FImportUtils::ImportTexture(const FTextureParameter& Texture)
{
	auto [Path, ObjectName, Folder] = SplitExportPath(Texture.Value);
	if (Path.StartsWith("/Engine")) return nullptr;
	
	FString ImportPath = WrapPathWithImportRootFolder(Path);
	
	const auto TexturePath = FPaths::Combine(CurrentExport.AssetsFolder, Path + ".png");

	const auto ExistingTexture = Cast<UTexture2D>(StaticLoadObject(UObject::StaticClass(), nullptr, *ImportPath));
	if (ExistingTexture != nullptr) return ExistingTexture;

	const auto ImportedTexture = Cast<UTexture2D>(ImportAsset(TexturePath, ImportPath));
	
	ImportedTexture->PreEditChange(nullptr);
	ImportedTexture->SRGB = Texture.sRGB;
	ImportedTexture->CompressionSettings = Texture.CompressionSettings;
	ImportedTexture->PostEditChange();

	ImportedTexture->MarkPackageDirty();
	
	FString PackageFileName = FPackageName::LongPackageNameToFilename(ImportedTexture->GetPackage()->GetPathName(), FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Standalone | RF_Public;
	UPackage::SavePackage(ImportedTexture->GetPackage(), ImportedTexture, *PackageFileName, SaveArgs);

	ImportedTexture->GetPackage()->FullyLoad();
	
	return ImportedTexture;
}

UAssetImportTask* FImportUtils::CreateImportTask(FString SourcePath, FString DestinationPath, UFactory* ExtraFactory, UObject* ExtraOptions)
{
	UAssetImportTask* RetTask = NewObject<UAssetImportTask>();
	
	if (RetTask == nullptr)
	{
		UE_LOG(LogFortnitePorting, Error, TEXT("Create import task failed."));
		return nullptr;
	}
	
	RetTask->Filename = SourcePath;
	RetTask->DestinationPath = FPaths::GetPath(DestinationPath);
	RetTask->DestinationName = FPaths::GetCleanFilename(DestinationPath);

	RetTask->bSave = true;
	RetTask->bAutomated = true;
	RetTask->bAsync = false;
	RetTask->bReplaceExisting = true;
	RetTask->bReplaceExistingSettings = false;

	RetTask->Factory = ExtraFactory;
	RetTask->Options = ExtraOptions;

	return RetTask;
}

UObject* FImportUtils::ProcessImportTask(UAssetImportTask* ImportTask)
{
	
	if (ImportTask == nullptr)
	{
		UE_LOG(LogFortnitePorting, Error, TEXT("Invalid import task."));
		return nullptr;
	}
	
	FAssetToolsModule* AssetTools = FModuleManager::LoadModulePtr<FAssetToolsModule>("AssetTools");

	if (AssetTools == nullptr)
	{
		UE_LOG(LogFortnitePorting, Error, TEXT("Unable to retrieve AssetTools module."));
		return nullptr;
	}
	
	AssetTools->Get().ImportAssetTasks({ ImportTask });
	
	if (ImportTask->GetObjects().Num() == 0)
	{
		UE_LOG(LogFortnitePorting, Error, TEXT("Import task failed. Nothing was imported for path: %s"), *ImportTask->Filename);
		return nullptr;
	}

	UObject* ImportedAsset = StaticLoadObject(UObject::StaticClass(), nullptr, *FPaths::Combine(ImportTask->DestinationPath, ImportTask->DestinationName));

	return ImportedAsset;
}

UObject* FImportUtils::ImportAsset(FString SourcePath, FString DestinationPath)
{
	UAssetImportTask* Task = CreateImportTask(SourcePath, DestinationPath,nullptr, nullptr);

	if (Task == nullptr)
	{
		return nullptr;
	}

	UObject* RetAsset = ProcessImportTask(Task);

	if(RetAsset == nullptr)
	{
		return nullptr;
	}
	
	return RetAsset;
}