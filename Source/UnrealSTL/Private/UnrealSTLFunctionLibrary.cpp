// Copyright 2022, Roberto De Ioris.

#include "UnrealSTLFunctionLibrary.h"
#include "Misc/FileHelper.h"
#include "StaticMeshResources.h"

namespace UnrealSTL
{
	constexpr int32 BinaryHeaderSize = 80;
	constexpr int32 BinaryHeaderSizeAndSize = BinaryHeaderSize + 4;
	constexpr int32 BinaryTriangleSize = 50;

	template<typename ReturnType, typename RHIResource>
	static void GPUToCPU(TArray<ReturnType>& Data, RHIResource Resource, const int32 NumElements)
	{
		FEvent* GPUEvent = FGenericPlatformProcess::GetSynchEventFromPool(false);

		ENQUEUE_RENDER_COMMAND(CopyBackToCPU)(
			[&Data, Resource, NumElements, GPUEvent](FRHICommandListImmediate& RHICmdList)
			{
				const int32 NumBytes = NumElements * sizeof(ReturnType);
				FRHIGPUBufferReadback BufferReadback(TEXT("STL Buffer Readback"));
				BufferReadback.EnqueueCopy(RHICmdList, Resource.GetReference(), NumBytes);
				RHICmdList.BlockUntilGPUIdle();
				if (BufferReadback.IsReady())
				{
					ReturnType* RawData = reinterpret_cast<ReturnType*>(BufferReadback.Lock(NumBytes));
					Data.Append(RawData, NumElements);
					BufferReadback.Unlock();
				}

				GPUEvent->Trigger();
			});

		GPUEvent->Wait();

		FGenericPlatformProcess::ReturnSynchEventToPool(GPUEvent);
	}
}

FUnrealSTLMesh::FUnrealSTLMesh()
{
	TrianglesNum = 0;
}

FUnrealSTLMesh::FUnrealSTLMesh(const TArray<FUnrealSTLMesh>& Meshes) : FUnrealSTLMesh()
{
	FBox BoundingBox;
	BoundingBox.Init();

	for (const FUnrealSTLMesh& Mesh : Meshes)
	{
		Vertices += Mesh.Vertices;
		TrianglesNum += Mesh.TrianglesNum;
		BoundingBox += Mesh.Bounds.GetBox();
	}

	BoundingBox.GetCenterAndExtents(Bounds.Origin, Bounds.BoxExtent);
	Bounds.SphereRadius = 0;
	uint32 VertexBase = 0;
	for (const FUnrealSTLMesh& Mesh : Meshes)
	{
		FStaticMeshSection Section;
		Section.FirstIndex = LODIndices.Num();
		Section.NumTriangles = Mesh.TrianglesNum;
		Sections.Add(Section);
		for (const FStaticMeshBuildVertex& BuildVertex : Mesh.Vertices)
		{
			Bounds.SphereRadius = FMath::Max((BuildVertex.Position - FVector3f(Bounds.Origin)).Size(), Bounds.SphereRadius);
		}

		for (const uint32 VertexIndex : Mesh.LODIndices)
		{
			LODIndices.Add(VertexBase + VertexIndex);
		}
		VertexBase += Mesh.Vertices.Num();
	}
}

bool UUnrealSTLFunctionLibrary::LoadMeshFromSTLData(FArrayReader& Reader, const FUnrealSTLConfig& Config, FUnrealSTLMesh& STLMesh)
{
	STLMesh.Vertices.Empty();
	STLMesh.LODIndices.Empty();
	STLMesh.TrianglesNum = 0;

	FBox BoundingBox;
	BoundingBox.Init();

	uint32 ProcessedTriangles = 0;

	Reader.Seek(0);

	if ((Config.FileMode == EUnrealSTLFileMode::Auto && !FMemory::Memcmp(Reader.GetData(), "solid", 5)) || Config.FileMode == EUnrealSTLFileMode::ASCII)
	{
		TArray<FString> Tokens;
		FString Token;
		for (int32 Offset = 0; Offset < Reader.Num(); Offset++)
		{
			char Char = static_cast<char>(Reader[Offset]);
			if (Char == 0 || Char == '\r' || Char == '\n' || Char == ' ' || Char == '\t')
			{
				if (!Token.IsEmpty())
				{
					Tokens.Add(Token);
					Token.Empty();
				}
			}
			else
			{
				Token += Char;
			}
		}

		int32 TokenIndex = 0;
		int32 VertexState = 0; // 0 to 2

		FStaticMeshBuildVertex Vertex[3];

		while (TokenIndex < Tokens.Num())
		{
			Token = Tokens[TokenIndex++];
			if (Token == "normal")
			{
				if (TokenIndex + 3 > Tokens.Num())
				{
					return false;
				}
				else
				{
					float NormalX = FCString::Atof(*Tokens[TokenIndex++]);
					float NormalY = FCString::Atof(*Tokens[TokenIndex++]);
					float NormalZ = FCString::Atof(*Tokens[TokenIndex++]);
					Vertex[0].TangentZ = FVector3f(Config.Transform.TransformVector(FVector(-NormalX, NormalY, NormalZ)));
					Vertex[1].TangentZ = Vertex[0].TangentZ;
					Vertex[2].TangentZ = Vertex[0].TangentZ;
				}
			}
			else if (Token == "vertex")
			{
				if (TokenIndex + 3 > Tokens.Num())
				{
					return false;
				}
				else
				{
					float VertexX = FCString::Atof(*Tokens[TokenIndex++]);
					float VertexY = FCString::Atof(*Tokens[TokenIndex++]);
					float VertexZ = FCString::Atof(*Tokens[TokenIndex++]);
					Vertex[VertexState++].Position = FVector3f(Config.Transform.TransformPosition(FVector(-VertexX, VertexY, VertexZ)));
					if (VertexState > 2)
					{
						VertexState = 0;
						BoundingBox += FVector(Vertex[0].Position);
						BoundingBox += FVector(Vertex[1].Position);
						BoundingBox += FVector(Vertex[2].Position);

						STLMesh.Vertices.Append(Vertex, 3);

						ProcessedTriangles++;
					}
				}
			}
		}
	}
	else
	{
		if (Reader.Num() < UnrealSTL::BinaryHeaderSizeAndSize)
		{
			return false;
		}

		Reader.Seek(UnrealSTL::BinaryHeaderSize);

		Reader << STLMesh.TrianglesNum;

		STLMesh.Vertices.Reserve(STLMesh.TrianglesNum * 3);

		while (ProcessedTriangles < STLMesh.TrianglesNum)
		{
			if (Reader.Tell() + UnrealSTL::BinaryTriangleSize <= Reader.Num())
			{
				FStaticMeshBuildVertex Vertex[3];
				Reader << Vertex[0].TangentZ;
				Vertex[1].TangentZ = Vertex[0].TangentZ;
				Vertex[2].TangentZ = Vertex[0].TangentZ;
				Reader << Vertex[0].Position;
				Reader << Vertex[1].Position;
				Reader << Vertex[2].Position;

				for (int32 VertexIndex = 0; VertexIndex < 3; VertexIndex++)
				{
					Vertex[VertexIndex].Position = FVector3f(Config.Transform.TransformPosition(FVector(-Vertex[VertexIndex].Position.X, Vertex[VertexIndex].Position.Y, Vertex[VertexIndex].Position.Z)));
					Vertex[VertexIndex].TangentZ = FVector3f(Config.Transform.TransformVector(FVector(Vertex[VertexIndex].TangentZ.X, Vertex[VertexIndex].TangentZ.Y, Vertex[VertexIndex].TangentZ.Z)));
				}

				BoundingBox += FVector(Vertex[0].Position);
				BoundingBox += FVector(Vertex[1].Position);
				BoundingBox += FVector(Vertex[2].Position);

				STLMesh.Vertices.Append(Vertex, 3);

				uint16 AttributesBytesCount;
				Reader << AttributesBytesCount;

				if (AttributesBytesCount > 0)
				{
					int64 CurrentSeek = Reader.Tell();
					if (CurrentSeek + AttributesBytesCount <= Reader.Num())
					{
						Reader.Seek(CurrentSeek + AttributesBytesCount);
					}
					else
					{
						return false;
					}
				}
			}
			else
			{
				return false;
			}

			ProcessedTriangles++;
		}
	}

	BoundingBox.GetCenterAndExtents(STLMesh.Bounds.Origin, STLMesh.Bounds.BoxExtent);
	STLMesh.Bounds.SphereRadius = 0;
	int32 Index = 0;
	for (const FStaticMeshBuildVertex& BuildVertex : STLMesh.Vertices)
	{
		STLMesh.Bounds.SphereRadius = FMath::Max((BuildVertex.Position - FVector3f(STLMesh.Bounds.Origin)).Size(), STLMesh.Bounds.SphereRadius);
		if (Index % 3 == 0) // generate indices every 3 vertices
		{
			STLMesh.LODIndices.Add(Index);
			if (!Config.bReverseWinding)
			{
				STLMesh.LODIndices.Add(Index + 1);
				STLMesh.LODIndices.Add(Index + 2);
			}
			else
			{
				STLMesh.LODIndices.Add(Index + 2);
				STLMesh.LODIndices.Add(Index + 1);
			}
		}
		Index++;
	}

	STLMesh.TrianglesNum = ProcessedTriangles;

	return true;
}

UStaticMesh* UUnrealSTLFunctionLibrary::LoadStaticMeshFromSTLFile(const FString& Filename, const FUnrealSTLConfig& Config, const FUnrealSTLStaticMeshConfig& StaticMeshConfig)
{
	FUnrealSTLFile STLFile;
	STLFile.Filename = Filename;
	STLFile.Config = Config;

	FUnrealSTLFileLOD STLFileLOD;
	STLFileLOD.Sections = { STLFile };
	STLFileLOD.ScreenSize = 1.0f;
	return LoadStaticMeshFromSTLFileLODs({ STLFileLOD }, StaticMeshConfig);
}

UStaticMesh* UUnrealSTLFunctionLibrary::LoadStaticMeshFromSTLFileLODs(const TArray<FUnrealSTLFileLOD>& FileLODs, const FUnrealSTLStaticMeshConfig& StaticMeshConfig)
{
	if (FileLODs.Num() < 1)
	{
		return nullptr;
	}

	UStaticMesh* StaticMesh = NewObject<UStaticMesh>(StaticMeshConfig.Outer ? StaticMeshConfig.Outer : GetTransientPackage());

	StaticMesh->NeverStream = true;

#if WITH_EDITOR
	StaticMesh->bAutoComputeLODScreenSize = false;
#endif

	if (StaticMesh->GetRenderData())
	{
		StaticMesh->GetRenderData()->ReleaseResources();
	}

	StaticMesh->SetRenderData(MakeUnique<FStaticMeshRenderData>());
	FStaticMeshRenderData* RenderData = StaticMesh->GetRenderData();

	RenderData->AllocateLODResources(FileLODs.Num());

	int32 LODIndex = 0;
	TArray<FStaticMaterial> StaticMaterials;
	for (const FUnrealSTLFileLOD& FileLOD : FileLODs)
	{
		FStaticMeshLODResources& LODResources = RenderData->LODResources[LODIndex];
		RenderData->ScreenSize[LODIndex] = FileLOD.ScreenSize > 0 ? FileLOD.ScreenSize : (1.0f - ((1.f / FileLODs.Num()) * LODIndex));

#if WITH_EDITOR
		FStaticMeshSourceModel& SourceModel = StaticMesh->AddSourceModel();
		SourceModel.ScreenSize = RenderData->ScreenSize[LODIndex];
#endif

#if ENGINE_MAJOR_VERSION < 5
		FStaticMeshLODResources::FStaticMeshSectionArray& Sections = LODResources.Sections;
#else
		FStaticMeshSectionArray& Sections = LODResources.Sections;
#endif

		TArray<FUnrealSTLMesh> STLMeshes;
		for (const FUnrealSTLFile& File : FileLOD.Sections)
		{
			FArrayReader Data;
			if (!FFileHelper::LoadFileToArray(Data, *File.Filename))
			{
				return nullptr;
			}
			FUnrealSTLMesh STLMesh;
			if (!LoadMeshFromSTLData(Data, File.Config, STLMesh))
			{
				return nullptr;
			}
			STLMeshes.Add(MoveTemp(STLMesh));
		}

		// a bit of hacky optimizations...
		FUnrealSTLMesh STLMergedSections;
		FUnrealSTLMesh* STLSectionsPtr;
		if (STLMeshes.Num() == 1)
		{
			STLSectionsPtr = &STLMeshes[0];
		}
		else
		{
			STLMergedSections = FUnrealSTLMesh(STLMeshes);
			STLSectionsPtr = &STLMergedSections;
		}

		if (STLSectionsPtr->Sections.Num() == 0)
		{
			FStaticMeshSection DefaultSection;
			DefaultSection.NumTriangles = STLSectionsPtr->TrianglesNum;
			STLSectionsPtr->Sections.Add(DefaultSection);
		}

		for (int32 SectionIndex = 0; SectionIndex < STLSectionsPtr->Sections.Num(); SectionIndex++)
		{
			FStaticMeshSection& Section = Sections.AddDefaulted_GetRef();

			Section.FirstIndex = STLSectionsPtr->Sections[SectionIndex].FirstIndex;
			Section.NumTriangles = STLSectionsPtr->Sections[SectionIndex].NumTriangles;
			Section.MaterialIndex = StaticMaterials.Num();

#if WITH_EDITOR
			FMeshSectionInfo SectionInfo;
			SectionInfo.MaterialIndex = Section.MaterialIndex;
			StaticMesh->GetSectionInfoMap().Set(LODIndex, SectionIndex, SectionInfo);
#endif

			const FUnrealSTLConfig& Config = FileLOD.Sections[SectionIndex].Config;
			FStaticMaterial Material(Config.Material ? Config.Material : UMaterial::GetDefaultMaterial(MD_Surface), *FString::Printf(TEXT("LOD_%u_Section_%u"), LODIndex, SectionIndex));
			Material.UVChannelData.bInitialized = true;
			StaticMaterials.Add(Material);
		}

		LODResources.VertexBuffers.PositionVertexBuffer.Init(STLSectionsPtr->Vertices, StaticMeshConfig.bAllowCPUAccess);
		LODResources.VertexBuffers.StaticMeshVertexBuffer.Init(STLSectionsPtr->Vertices, 1, StaticMeshConfig.bAllowCPUAccess);
		if (StaticMeshConfig.bAllowCPUAccess)
		{
			LODResources.IndexBuffer = FRawStaticIndexBuffer(true);
		}
		LODResources.IndexBuffer.SetIndices(STLSectionsPtr->LODIndices, EIndexBufferStride::Force32Bit);

		if (LODIndex == 0)
		{
			RenderData->Bounds = STLSectionsPtr->Bounds;
		}

		LODIndex++;
	}

	StaticMesh->SetStaticMaterials(StaticMaterials);

	StaticMesh->InitResources();

	StaticMesh->CalculateExtendedBounds();

	return StaticMesh;
}

bool UUnrealSTLFunctionLibrary::SaveStaticMeshToSTLData(UStaticMesh* StaticMesh, const int32 LOD, FArrayWriter& Writer, const FUnrealSTLConfig& Config)
{
	if (!StaticMesh || LOD < 0)
	{
		return false;
	}

	if (LOD >= StaticMesh->GetNumLODs())
	{
		return false;
	}

	if (!StaticMesh->GetRenderData())
	{
		StaticMesh->SetRenderData(MakeUnique<FStaticMeshRenderData>());
	}

	if (!StaticMesh->GetRenderData()->IsInitialized())
	{
		StaticMesh->InitResources();
	}

	FStaticMeshRenderData* RenderData = StaticMesh->GetRenderData();

	FStaticMeshLODResources& LODResources = RenderData->LODResources[LOD];

	TArray<uint32> Indices;
	TArray<FVector3f> Vertices;

#if ENGINE_MAJOR_VERSION < 5
	if (StaticMesh->bAllowCPUAccess)
#else
	if (LODResources.IndexBuffer.GetAllowCPUAccess())
#endif
	{
		LODResources.IndexBuffer.GetCopy(Indices);
	}
	else
	{
		if (LODResources.IndexBuffer.Is32Bit())
		{
			UnrealSTL::GPUToCPU(Indices, LODResources.IndexBuffer.IndexBufferRHI, LODResources.IndexBuffer.GetNumIndices());
		}
		else
		{
			TArray<uint16> TmpIndices;
			UnrealSTL::GPUToCPU(TmpIndices, LODResources.IndexBuffer.IndexBufferRHI, LODResources.IndexBuffer.GetNumIndices());
			for (const uint16 Index : TmpIndices)
			{
				Indices.Add(Index);
			}
		}
	}

#if ENGINE_MAJOR_VERSION < 5
	if (StaticMesh->bAllowCPUAccess)
#else
	if (LODResources.VertexBuffers.PositionVertexBuffer.GetAllowCPUAccess())
#endif
	{
		for (uint32 VertexIndex = 0; VertexIndex < LODResources.VertexBuffers.PositionVertexBuffer.GetNumVertices(); VertexIndex++)
		{
			Vertices.Add(LODResources.VertexBuffers.PositionVertexBuffer.VertexPosition(VertexIndex));
		}
	}
	else
	{
		UnrealSTL::GPUToCPU(Vertices, LODResources.VertexBuffers.PositionVertexBuffer.VertexBufferRHI, LODResources.VertexBuffers.PositionVertexBuffer.GetNumVertices());
	}

	TArray<uint8> Header;
	Header.AddZeroed(UnrealSTL::BinaryHeaderSize);

	const int32 SolidNameLen = FCStringAnsi::Strlen(TCHAR_TO_ANSI(*StaticMesh->GetFullName()));
	FMemory::Memcpy(Header.GetData(), TCHAR_TO_ANSI(*StaticMesh->GetFullName()), FMath::Min(UnrealSTL::BinaryHeaderSize, SolidNameLen));

	Writer.Append(Header);
	Writer.Seek(UnrealSTL::BinaryHeaderSize);

	uint32 NumTriangles = Indices.Num() / 3;
	Writer << NumTriangles;
	for (uint32 TriangleIndex = 0; TriangleIndex < NumTriangles; TriangleIndex++)
	{
		uint32 VertexIndex1 = Indices[TriangleIndex * 3];
		uint32 VertexIndex2 = Indices[TriangleIndex * 3 + 1];
		uint32 VertexIndex3 = Indices[TriangleIndex * 3 + 2];
		if (Config.bReverseWinding)
		{
			Swap(VertexIndex2, VertexIndex3);
		}
		const FVector3f NegateX = FVector3f(-1, 1, 1);
		FVector3f Vertex1 = FVector3f(Config.Transform.TransformPosition(FVector(Vertices[VertexIndex1]))) * NegateX;
		FVector3f Vertex2 = FVector3f(Config.Transform.TransformPosition(FVector(Vertices[VertexIndex2]))) * NegateX;
		FVector3f Vertex3 = FVector3f(Config.Transform.TransformPosition(FVector(Vertices[VertexIndex3]))) * NegateX;
		FVector3f Normal = FVector3f::CrossProduct(Vertex3 - Vertex1, Vertex2 - Vertex1).GetSafeNormal();

		uint16 Attributes = 0;

		Writer << Normal;
		Writer << Vertex1;
		Writer << Vertex2;
		Writer << Vertex3;
		Writer << Attributes;
	}

	return true;
}

bool UUnrealSTLFunctionLibrary::SaveStaticMeshToSTLFile(UStaticMesh* StaticMesh, const int32 LOD, const FString& Filename, const FUnrealSTLConfig& Config)
{
	FArrayWriter Writer;
	if (!SaveStaticMeshToSTLData(StaticMesh, LOD, Writer, Config))
	{
		return false;
	}

	return FFileHelper::SaveArrayToFile(Writer, *Filename);
}
