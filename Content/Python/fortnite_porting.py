import unreal

@unreal.uclass()
class FortnitePortingFunctionLibrary(unreal.BlueprintFunctionLibrary):
    
    @unreal.ufunction(static=True,params=[str])
    def import_response_remote_api(response:str):
        unreal.RemoteImportUtils.import_response_remote_api(response)