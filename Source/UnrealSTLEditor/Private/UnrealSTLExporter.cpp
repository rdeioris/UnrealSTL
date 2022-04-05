// Copyright 2022, Roberto De Ioris.


#include "UnrealSTLExporter.h"
#include "UnrealSTLFunctionLibrary.h"

UUnrealSTLExporter::UUnrealSTLExporter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass = UStaticMesh::StaticClass();
	FormatExtension.Add(TEXT("stl"));
	PreferredFormatIndex = 0;
	FormatDescription.Add(TEXT("STL file"));
}

bool UUnrealSTLExporter::ExportBinary(UObject* Object, const TCHAR* Type, FArchive& Ar, FFeedbackContext* Warn, int32 FileIndex, uint32 PortFlags)
{
	UStaticMesh* StaticMesh = Cast<UStaticMesh>(Object);
	if (!StaticMesh)
	{
		return false;
	}

	FArrayWriter Writer;
	if (!UUnrealSTLFunctionLibrary::SaveStaticMeshToSTLData(StaticMesh, 0, Writer, FUnrealSTLConfig()))
	{
		return false;
	}

	Ar.Serialize(Writer.GetData(), Writer.Num());

	return true;
}