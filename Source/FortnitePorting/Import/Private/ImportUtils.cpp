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
#include "Misc/ScopedSlowTask.h"

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

		auto AssetsRoot = Export.AssetsRoot;
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

				auto ImportParts = [&](TArray<FExportMesh> Parts)
				{
					for (auto Mesh : Parts)
					{
						if (ImportedParts.Contains(Mesh.Part) && (Mesh.Part.Equals("Outfit") || Mesh.Part.Equals("Backpack"))) continue;
						
						const auto Imported = ImportMesh(Mesh);
						ImportedParts.Add(Mesh.Part, FPartData(Imported, Mesh));

						for (auto Material : Mesh.Materials)
						{
							ImportMaterial(Material);
						}
					}
				};
				
				ImportParts(Data.StyleParts);
				ImportParts(Data.Parts);
			}
		}
	});
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

UObject* FImportUtils::ImportMesh(const FExportMesh& Mesh)
{
	auto [Path, ObjectName, Folder] = SplitExportPath(Mesh.MeshPath);

	TMap<FString, FString> MaterialNameToPathMap;
	for (auto Material : Mesh.Materials)
	{
		auto [MatPath, MatObjectName, MatFolder] = SplitExportPath(Material.MaterialPath);
		MaterialNameToPathMap.Add(Material.MaterialName, MatFolder);
	}
	
	auto MeshPath = FPaths::Combine(CurrentExport.AssetsRoot, Path + "_LOD0");
	if (FPaths::FileExists(MeshPath + ".psk"))
	{
		return UPskFactory::Import(MeshPath + ".psk", CreatePackage(*Folder), FName(*ObjectName), RF_Public | RF_Standalone, MaterialNameToPathMap);
	}
	if (FPaths::FileExists(MeshPath + ".pskx"))
	{
		return UPskxFactory::Import(MeshPath + ".pskx", CreatePackage(*Folder), FName(*ObjectName), RF_Public | RF_Standalone, MaterialNameToPathMap);
	}
	
	return nullptr;
}

void FImportUtils::ImportMaterial(const FExportMaterial& Material)
{
	if (!CurrentExport.Settings.ImportMaterials) return;
	
	auto [Path, ObjectName, Folder] = SplitExportPath(Material.MaterialPath);
	const auto MatPackage = CreatePackage(*Path);
	auto MaterialInstance = LoadObject<UMaterialInstanceConstant>(MatPackage, *ObjectName);
	if (MaterialInstance == nullptr)
	{
		MaterialInstance = NewObject<UMaterialInstanceConstant>(MatPackage, *ObjectName);
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
	
	FGlobalComponentReregisterContext RecreateComponents;
}

UTexture* FImportUtils::ImportTexture(const FTextureParameter& Texture)
{
	auto [Path, ObjectName, Folder] = SplitExportPath(Texture.Value);
	if (Path.StartsWith("/Engine")) return nullptr;
	
	const auto TexturePath = FPaths::Combine(CurrentExport.AssetsRoot, Path + ".png");
	const auto TexturePackage = CreatePackage(*Path);
	UE_LOG(LogFortnitePorting, Log, TEXT("%s"), *Path)
	
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
	return ImportedTexture;
}
