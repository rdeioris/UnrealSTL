// Copyright 2022, Roberto De Ioris.

#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "UnrealSTLFactory.generated.h"

/**
 * 
 */
UCLASS()
class UNREALSTLEDITOR_API UUnrealSTLFactory : public UFactory
{
    GENERATED_UCLASS_BODY()

    virtual UObject* FactoryCreateBinary(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, const TCHAR* Type, const uint8*& Buffer, const uint8* BufferEnd, FFeedbackContext* Warn, bool& bOutOperationCanceled) override;
};
