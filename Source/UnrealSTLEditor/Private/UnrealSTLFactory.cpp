// Copyright 2022, Roberto De Ioris.


#include "UnrealSTLFactory.h"
#include "StaticMeshDescription.h"
#include "StaticMeshAttributes.h"
#include "UnrealSTLFunctionLibrary.h"

UUnrealSTLFactory::UUnrealSTLFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass = UStaticMesh::StaticClass();
	Formats.Add(TEXT("stl;STL file"));
	bEditorImport = true;
}

UObject* UUnrealSTLFactory::FactoryCreateBinary(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, const TCHAR* Type, const uint8*& Buffer, const uint8* BufferEnd, FFeedbackContext* Warn, bool& bOutOperationCanceled)
{
	UStaticMesh* StaticMesh = NewObject<UStaticMesh>(InParent, InName, Flags);

	FArrayReader Reader;
	Reader.Append(Buffer, BufferEnd - Buffer);

	FUnrealSTLMesh STLMesh;
	if (!UUnrealSTLFunctionLibrary::LoadMeshFromSTLData(Reader, FUnrealSTLConfig(), STLMesh))
	{
		return nullptr;
	}

	UStaticMeshDescription* MeshDescription = UStaticMesh::CreateStaticMeshDescription();

	TVertexAttributesRef<FVector3f> Positions = MeshDescription->GetVertexPositions();
	TVertexInstanceAttributesRef<FVector3f> Normals = MeshDescription->GetVertexInstanceNormals();

	FPolygonGroupID PolygonGroup = MeshDescription->CreatePolygonGroup();

	for (int32 VertexIndex = 0; VertexIndex < STLMesh.LODIndices.Num(); VertexIndex += 3)
	{
		const uint32 VertexIndex1 = STLMesh.LODIndices[VertexIndex];
		const uint32 VertexIndex2 = STLMesh.LODIndices[VertexIndex + 1];
		const uint32 VertexIndex3 = STLMesh.LODIndices[VertexIndex + 2];

		const FVector3f Vertex1 = STLMesh.Vertices[VertexIndex1].Position;
		const FVector3f Normal1 = STLMesh.Vertices[VertexIndex1].TangentZ;

		const FVector3f Vertex2 = STLMesh.Vertices[VertexIndex2].Position;
		const FVector3f Normal2 = STLMesh.Vertices[VertexIndex2].TangentZ;

		const FVector3f Vertex3 = STLMesh.Vertices[VertexIndex3].Position;
		const FVector3f Normal3 = STLMesh.Vertices[VertexIndex3].TangentZ;

		const FVertexID VertexID1 = MeshDescription->CreateVertex();
		Positions[VertexID1] = Vertex1;
		FVertexInstanceID VertexInstanceID1 = MeshDescription->CreateVertexInstance(FVertexID(VertexIndex));
		Normals[VertexInstanceID1] = Normal1;

		const FVertexID VertexID2 = MeshDescription->CreateVertex();
		Positions[VertexID2] = Vertex2;
		FVertexInstanceID VertexInstanceID2 = MeshDescription->CreateVertexInstance(FVertexID(VertexIndex + 1));
		Normals[VertexInstanceID2] = Normal2;

		const FVertexID VertexID3 = MeshDescription->CreateVertex();
		Positions[VertexID3] = Vertex3;
		FVertexInstanceID VertexInstanceID3 = MeshDescription->CreateVertexInstance(FVertexID(VertexIndex + 2));
		Normals[VertexInstanceID3] = Normal3;

		TArray<FEdgeID> Edges;
		// fix winding when using mesh description TODO: add an import panel
		TArray<FVertexInstanceID> Instances = { VertexInstanceID1, VertexInstanceID3, VertexInstanceID2 };
		MeshDescription->CreatePolygon(PolygonGroup, Instances, Edges);
	}

	StaticMesh->BuildFromStaticMeshDescriptions({ MeshDescription }, false);

	return StaticMesh;
}