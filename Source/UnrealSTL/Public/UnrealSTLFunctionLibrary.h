// Copyright 2022, Roberto De Ioris.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Serialization/ArrayReader.h"
#include "Serialization/ArrayWriter.h"
#include "UnrealSTLFunctionLibrary.generated.h"

struct UNREALSTL_API FUnrealSTLMesh
{
	TArray<FStaticMeshBuildVertex> Vertices;
	TArray<uint32> LODIndices;
	uint32 TrianglesNum;
	FBoxSphereBounds Bounds;
	TArray<FStaticMeshSection> Sections;

	FUnrealSTLMesh();
	FUnrealSTLMesh(const TArray<FUnrealSTLMesh>& Meshes);
};

UENUM()
enum class EUnrealSTLFileMode : uint8
{
	Auto,
	ASCII,
	Binary
};

USTRUCT(BlueprintType)
struct FUnrealSTLConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnrealSTL")
	FTransform Transform;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnrealSTL")
	EUnrealSTLFileMode FileMode;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnrealSTL")
	bool bReverseWinding;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnrealSTL")
	UMaterialInterface* Material;

	FUnrealSTLConfig()
	{
		Transform = FTransform::Identity;
		FileMode = EUnrealSTLFileMode::Auto;
		bReverseWinding = false;
		Material = nullptr;
	}
};

USTRUCT(BlueprintType)
struct FUnrealSTLStaticMeshConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnrealSTL")
	bool bAllowCPUAccess;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnrealSTL")
	UObject* Outer;

	FUnrealSTLStaticMeshConfig()
	{
		bAllowCPUAccess = false;
		Outer = nullptr;
	}
};

USTRUCT(BlueprintType)
struct FUnrealSTLFile
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnrealSTL")
	FString Filename;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnrealSTL")
	FUnrealSTLConfig Config;
};

USTRUCT(BlueprintType)
struct FUnrealSTLFileLOD
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnrealSTL")
	TArray<FUnrealSTLFile> Sections;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnrealSTL")
	float ScreenSize;

	FUnrealSTLFileLOD()
	{
		ScreenSize = 0;
	}
};

/**
 * 
 */
UCLASS()
class UNREALSTL_API UUnrealSTLFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	static bool LoadMeshFromSTLData(FArrayReader& Reader, const FUnrealSTLConfig& Config, FUnrealSTLMesh& STLMesh);
	static bool SaveStaticMeshToSTLData(UStaticMesh* StaticMesh, const int32 LOD, FArrayWriter& Writer, const FUnrealSTLConfig& Config);

	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "Config"), Category = "UnrealSTL")
	static bool SaveStaticMeshToSTLFile(UStaticMesh* StaticMesh, const int32 LOD, const FString& Filename, const FUnrealSTLConfig& Config);

	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "Config, StaticMeshConfig"), Category="UnrealSTL")
    static UStaticMesh* LoadStaticMeshFromSTLFile(const FString& Filename, const FUnrealSTLConfig& Config, const FUnrealSTLStaticMeshConfig& StaticMeshConfig);

	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "StaticMeshConfig"), Category = "UnrealSTL")
	static UStaticMesh* LoadStaticMeshFromSTLFileLODs(const TArray<FUnrealSTLFileLOD>& FileLODs, const FUnrealSTLStaticMeshConfig& StaticMeshConfig);
	
};
