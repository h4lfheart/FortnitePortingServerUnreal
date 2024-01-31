#include "FortnitePorting/Import/Public/ImportUtils.h"

#include "AssetImportTask.h"
#include "AutomatedAssetImportData.h"
#include "EditorAssetLibrary.h"
#include "FortnitePorting.h"
#include "JsonObjectConverter.h"
#include "PluginUtils.h"
#include "PskFactory.h"
#include "PskxFactory.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Async/Async.h"
#include "Factories/TextureFactory.h"
#include "Interfaces/IPluginManager.h"
#include "Materials/MaterialInstanceConstant.h"
#include "ComponentReregisterContext.h"
#include "Misc/ScopedSlowTask.h"
#include "Factories/MaterialInstanceConstantFactoryNew.h"
#include "Factories/UEModelFactory.h"

void FImportUtils::CheckForDependencies()
{
	if (TextureFactory == nullptr)
	{
		auto AutomatedData = NewObject<UAutomatedAssetImportData>();
		AutomatedData->bReplaceExisting = false;

		TextureFactory = NewObject<UTextureFactory>();
		TextureFactory->NoCompression = false;
		TextureFactory->AutomatedImportData = AutomatedData;
	}
	
	if (DefaultMaterial == nullptr)
		DefaultMaterial = Cast<UMaterial>(UEditorAssetLibrary::LoadAsset("/FortnitePorting/Base_Materials/M_FortnitePorting_Default.M_FortnitePorting_Default"));
	
	if (UEFNMaterial == nullptr)
    		UEFNMaterial = Cast<UMaterial>(UEditorAssetLibrary::LoadAsset("/FortnitePorting/Base_Materials/M_FortnitePorting_UEFN.M_FortnitePorting_UEFN"));
}

UMaterial* FImportUtils::GetMaterial()
{
	return CurrentExport.Settings.ForUEFN ? UEFNMaterial : DefaultMaterial;
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
	
	AsyncTask(ENamedThreads::GameThread, [Export]
	{
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

				auto ImportParts = [&](TArray<FExportMesh> Meshes)
				{
					for (auto Mesh : Meshes)
					{
						if (ImportedParts.Contains(Mesh.Type) && (Mesh.Type.Equals("Outfit") || Mesh.Type.Equals("Backpack"))) continue;
						
						const auto Imported = ImportMesh(Mesh);
						ImportedParts.Add(Mesh.Type, FPartData(Imported, Mesh));

						for (auto Material : Mesh.Materials)
						{
							ImportMaterial(Material);
						}

						// Assign materials to Mesh After Import
						
						TMap<FString, FString> MaterialNameToPathMap;
						for (auto Material : Mesh.Materials)
						{
							// auto [MatPath, MatObjectName, MatFolder] = SplitExportPath(Material.MaterialPath);
							auto [MatPath, MatObjectName, MatFolder] = SplitExportPath(Material.Path);
							// MaterialNameToPathMap.Add(Material.MaterialName, MatFolder);							
							MaterialNameToPathMap.Add(Material.Name, MatFolder);
						}

						UStaticMesh * StaticMesh = Cast<UStaticMesh>(Imported);
						USkeletalMesh * SkeletalMesh = Cast<USkeletalMesh>(Imported);

						if (StaticMesh != nullptr){
							for ( int i = 0; i<StaticMesh->GetStaticMaterials().Num();i++)
							{
								FString MaterialSlotName = StaticMesh->GetStaticMaterials()[i].MaterialSlotName.ToString();
								FString FoundMaterialPath = *MaterialNameToPathMap.Find(MaterialSlotName);

								FString MaterialPackagePath = FPaths::Combine(FoundMaterialPath, MaterialSlotName);
								UMaterialInterface * Material = Cast<UMaterialInterface>(StaticLoadObject(UObject::StaticClass(), nullptr, *MaterialPackagePath));
								StaticMesh->SetMaterial(i, Material);
							}

							StaticMesh->PostEditChange();
						}
						if (SkeletalMesh != nullptr){
						
							for ( int i = 0; i<SkeletalMesh->GetMaterials().Num();i++)
							{
								
								FString MaterialSlotName = SkeletalMesh->GetMaterials()[i].MaterialSlotName.ToString();
								FString FoundMaterialPath = *MaterialNameToPathMap.Find(MaterialSlotName);
								FString MaterialPackagePath = FPaths::Combine(FoundMaterialPath, MaterialSlotName);

								UMaterialInterface * Material = Cast<UMaterialInterface>(StaticLoadObject(UObject::StaticClass(), nullptr, *MaterialPackagePath));

								if(Material == nullptr){
									UE_LOG(LogTemp, Error, TEXT("Material not found at: %s"),*MaterialPackagePath);	
								}								
								SkeletalMesh->GetMaterials()[i] = Material;
							}
							SkeletalMesh->PostEditChange();
						}
					}
				};
				
				ImportParts(Data.Meshes);
			}
		}
	});
	
}

UObject* FImportUtils::ImportMesh(const FExportMesh& Mesh)
{
	auto [Path, ObjectName, Folder] = SplitExportPath(Mesh.Path);

	TMap<FString, FString> MaterialNameToPathMap;
	for (auto Material : Mesh.Materials)
	{
		auto [MatPath, MatObjectName, MatFolder] = SplitExportPath(Material.Path);
		MaterialNameToPathMap.Add(Material.Name, MatFolder);
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
		return UEModelFactory::Import(SourceMeshPath + ".uemodel", Path, FName(*ObjectName), RF_Public | RF_Standalone, MaterialNameToPathMap);
	}

	UE_LOG(LogTemp, Error, TEXT("Unsupported mesh format: %s"),*Mesh.Path);
	
	return nullptr;
}

void FImportUtils::ImportMaterial(const FExportMaterial& Material)
{
	if (!CurrentExport.Settings.ImportMaterials) return;
	
	auto [Path, ObjectName, Folder] = SplitExportPath(Material.Path);
	
	const auto MatPackage = CreatePackage(*Path);

	auto MaterialInstance = LoadObject<UMaterialInstanceConstant>(MatPackage, *ObjectName);
	if (MaterialInstance == nullptr)
	{

		UMaterialInstanceConstantFactoryNew* MaterialFactory = NewObject<UMaterialInstanceConstantFactoryNew>();
		MaterialFactory->InitialParent = GetMaterial();

		MaterialInstance = (UMaterialInstanceConstant*)MaterialFactory->FactoryCreateNew(UMaterialInstanceConstant::StaticClass(), MatPackage, *ObjectName, RF_Standalone | RF_Public, NULL, GWarn);
		
	}
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

	MaterialInstance->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(MaterialInstance);
	MaterialInstance->PreEditChange(nullptr);
	MaterialInstance->PostEditChange();
	MatPackage->FullyLoad();

	FString PackageFileName = FPackageName::LongPackageNameToFilename(MatPackage->GetPathName(), FPackageName::GetAssetPackageExtension());
	UPackage::SavePackage(MatPackage, MaterialInstance, EObjectFlags::RF_Public | EObjectFlags::RF_Standalone, *PackageFileName);
	
	FGlobalComponentReregisterContext RecreateComponents;
}

UTexture* FImportUtils::ImportTexture(const FTextureParameter& Texture)
{
	auto [Path, ObjectName, Folder] = SplitExportPath(Texture.Value);
	if (Path.StartsWith("/Engine")) return nullptr;

	const auto TexturePath = FPaths::Combine(CurrentExport.AssetsFolder, Path + ".png");
	const auto TexturePackage = CreatePackage(*Path);
	
	const auto ExistingTexture = LoadObject<UTexture2D>(TexturePackage, *ObjectName);
	if (ExistingTexture != nullptr) return ExistingTexture;
		
	bool bCancelled;
	const auto ImportedTexture = Cast<UTexture2D>(TextureFactory->FactoryCreateFile(UTexture2D::StaticClass(), TexturePackage, FName(*ObjectName), RF_Public | RF_Standalone, TexturePath, nullptr, GWarn, bCancelled));
		
	ImportedTexture->PreEditChange(nullptr);
	ImportedTexture->SRGB = Texture.sRGB;
	ImportedTexture->CompressionSettings = Texture.CompressionSettings;
	ImportedTexture->PostEditChange();

	ImportedTexture->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(ImportedTexture);
		
	TexturePackage->FullyLoad();

	FString PackageFileName = FPackageName::LongPackageNameToFilename(TexturePackage->GetPathName(), FPackageName::GetAssetPackageExtension());
	UPackage::SavePackage(TexturePackage, ImportedTexture, EObjectFlags::RF_Public | EObjectFlags::RF_Standalone, *PackageFileName);
	
	return ImportedTexture;
}
