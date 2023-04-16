#include "FortnitePorting/Public/Utils.h"

#include <shldisp.h>

#include "AutomatedAssetImportData.h"
#include "EditorAssetLibrary.h"
#include "ExportModel.h"
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

void FUtils::ImportResponse(const FString& Response)
{
	FExport Export;
	if (!FJsonObjectConverter::JsonObjectStringToUStruct(Response, &Export))
	{
		UE_LOG(LogFortnitePorting, Error, TEXT("Unable to deserialize response from FortnitePorting."));
		return;
	}
	
	AsyncTask(ENamedThreads::GameThread, [Export]
	{
		if (FFortnitePortingModule::DefaultMaterial == nullptr)
		{
			FFortnitePortingModule::DefaultMaterial = Cast<UMaterial>(UEditorAssetLibrary::LoadAsset("/FortnitePorting/Base_Materials/M_FortnitePorting_Default.M_FortnitePorting_Default"));
		}

		CurrentExport = Export;

		auto AssetsRoot = Export.AssetsRoot;
		//auto Settings = Export.Settings;
		auto BaseData = Export.Data;

		for (const auto Data : BaseData)
		{
			auto Name = Data.Name;
			auto Type = Data.Type;
			UE_LOG(LogFortnitePorting, Log, TEXT("Received Import for %s: %s"), *Type, *Name)
			
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

FString FUtils::BytesToString(TArray<uint8>& Message, int32 BytesLength)
{
	if (BytesLength <= 0)
	{
		return FString("");
	}
	if (Message.Num() < BytesLength)
	{
		return FString("");
	}

	TArray<uint8> StringAsArray;
	StringAsArray.Reserve(BytesLength);

	for (int i = 0; i < BytesLength; i++)
	{
		StringAsArray.Add(Message[0]);
		Message.RemoveAt(0);
	}

	std::string cstr(reinterpret_cast<const char*>(StringAsArray.GetData()), StringAsArray.Num());
	return FString(UTF8_TO_TCHAR(cstr.c_str()));
}

TArray<uint8> FUtils::StringToBytes(const FString& InStr)
{
	const FTCHARToUTF8 Convert(*InStr);
	const auto BytesLength = Convert.Length();
	const auto MessageBytes = static_cast<uint8*>(FMemory::Malloc(BytesLength));
	FMemory::Memcpy(MessageBytes, TCHAR_TO_UTF8(InStr.GetCharArray().GetData()), BytesLength);

	TArray<uint8> Result;
	for (auto i = 0; i < BytesLength; i++)
	{
		Result.Add(MessageBytes[i]);
	}

	FMemory::Free(MessageBytes);
	return Result;
}

auto FUtils::SplitExportPath(const FString& InStr)
{
	auto RootName = InStr.RightChop(1);
	RootName = RootName.Left(RootName.Find("/"));
	
	if (!RootName.Equals("Game") && IPluginManager::Get().FindPlugin(RootName) == nullptr)
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

UObject* FUtils::ImportMesh(const FExportMesh& Mesh)
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

void FUtils::ImportMaterial(const FExportMaterial& Material)
{
	auto [Path, ObjectName, Folder] = SplitExportPath(Material.MaterialPath);
	const auto MatPackage = CreatePackage(*Path);
	auto MaterialInstance = LoadObject<UMaterialInstanceConstant>(MatPackage, *ObjectName);
	if (MaterialInstance == nullptr)
	{
		MaterialInstance = NewObject<UMaterialInstanceConstant>(MatPackage, *ObjectName);
	}
	MaterialInstance->Parent = FFortnitePortingModule::DefaultMaterial;

	for (auto TextureParameter : Material.Textures)
	{
		// TODO Mappings
		const auto Texture = ImportTexture(TextureParameter);
		if (TextureParameter.Name.Equals("Diffuse"))
		{
			MaterialInstance->SetTextureParameterValueEditorOnly(FMaterialParameterInfo("Diffuse", GlobalParameter), Texture);
		}
		else if (TextureParameter.Name.Equals("SpecularMasks"))
		{
			MaterialInstance->SetTextureParameterValueEditorOnly(FMaterialParameterInfo("SpecularMasks", GlobalParameter), Texture);
		}
		else if (TextureParameter.Name.Equals("Normals"))
		{
			MaterialInstance->SetTextureParameterValueEditorOnly(FMaterialParameterInfo("Normals", GlobalParameter), Texture);
		}
		else if (TextureParameter.Name.Equals("M"))
		{
			MaterialInstance->SetTextureParameterValueEditorOnly(FMaterialParameterInfo("M", GlobalParameter), Texture);
		}
	}

	MaterialInstance->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(MaterialInstance);
	MaterialInstance->PreEditChange(nullptr);
	MaterialInstance->PostEditChange();
	MatPackage->FullyLoad();
	
	FGlobalComponentReregisterContext RecreateComponents;
}

UTexture* FUtils::ImportTexture(const FTextureParameter& Texture)
{
	auto AutomatedData = NewObject<UAutomatedAssetImportData>();
	AutomatedData->bReplaceExisting = false;
		
	const auto Factory = NewObject<UTextureFactory>();
	Factory->NoCompression = true;
	Factory->AutomatedImportData = AutomatedData;
		
	auto [Path, ObjectName, Folder] = SplitExportPath(Texture.Value);
	const auto TexturePath = FPaths::Combine(CurrentExport.AssetsRoot, Path + ".png");
	const auto TexturePackage = CreatePackage(*Path);
		
	bool bCancelled;
	const auto ImportedTexture = Cast<UTexture2D>(Factory->FactoryCreateFile(UTexture2D::StaticClass(), TexturePackage, FName(*ObjectName), RF_Public | RF_Standalone, TexturePath, nullptr, GWarn, bCancelled));
	ImportedTexture->SRGB = Texture.sRGB;
	ImportedTexture->CompressionSettings = Texture.CompressionSettings;

	ImportedTexture->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(ImportedTexture);
	ImportedTexture->PreEditChange(nullptr);
	ImportedTexture->PostEditChange();
		
	TexturePackage->FullyLoad();
	return ImportedTexture;
}
