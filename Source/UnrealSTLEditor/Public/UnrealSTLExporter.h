// Copyright 2022, Roberto De Ioris.

#pragma once

#include "CoreMinimal.h"
#include "Exporters/Exporter.h"
#include "UnrealSTLExporter.generated.h"

/**
 * 
 */
UCLASS()
class UNREALSTLEDITOR_API UUnrealSTLExporter : public UExporter
{
    GENERATED_UCLASS_BODY()

    virtual bool ExportBinary(UObject* Object, const TCHAR* Type, FArchive& Ar, FFeedbackContext* Warn, int32 FileIndex, uint32 PortFlags) override;
};
