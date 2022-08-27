
#pragma once

#include "CoreMinimal.h"
#include "AnimNodeStruct.h"
#include "AnimGraphNode_SkeletalControlBase.h"
#include "MyAnimGraphNode_Skeletal.generated.h"


struct FAnimNode_CustomIK; 
struct FAnimNode_SkeletalControlBase;

UCLASS()
class CUSTOMIK_API UMyAnimGraphNode_Skeletal : public UAnimGraphNode_SkeletalControlBase
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere)
	FAnimNode_CustomIK Node;

	// UPROPERTY(EditAnywhere, Category = "Settings")
	// FAnimNode_SkeletalControlBase Node;

    virtual FText GetTooltipText() const override;

	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;

	virtual FString GetNodeCategory() const override;

	virtual const FAnimNode_SkeletalControlBase* GetNode() const override;

};