# UnrealSTL
Unreal Engine plugin for importing and exporting STL files in both Editor and Runtime

![StanfordBunny](Screenshot.png?raw=true "StanfordBunny")

Consider sponsoring the project becoming a patron: https://www.patreon.com/rdeioris

## Editor

Once the plugin is installed you can just import .STL files as StaticMeshes. By right clicking any StaticMesh asset, you can export it as STL.

## Runtime

The following C++/Blueprint functions are available:

```cpp
UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "Config"), Category = "UnrealSTL")
static bool SaveStaticMeshToSTLFile(UStaticMesh* StaticMesh, const int32 LOD, const FString& Filename, const FUnrealSTLConfig& Config);

UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "Config, StaticMeshConfig"), Category="UnrealSTL")
static UStaticMesh* LoadStaticMeshFromSTLFile(const FString& Filename, const FUnrealSTLConfig& Config, const FUnrealSTLStaticMeshConfig& StaticMeshConfig);

UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "StaticMeshConfig"), Category = "UnrealSTL")
static UStaticMesh* LoadStaticMeshFromSTLFileLODs(const TArray<FUnrealSTLFileLOD>& FileLODs, const FUnrealSTLStaticMeshConfig& StaticMeshConfig);
````

By using the LoadStaticMeshFromSTLFileLODs, you can combine multiple STL files in a single StaticMesh asset with multiple Sections and LODs.
