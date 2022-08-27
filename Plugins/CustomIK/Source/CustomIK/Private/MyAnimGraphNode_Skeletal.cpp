
#include "MyAnimGraphNode_Skeletal.h"
#include "AnimGraphNode_TwoBoneIK.h"

FText UMyAnimGraphNode_Skeletal::GetTooltipText() const
{
    return FText::FromString(TEXT("CustomIK"));
}

FText UMyAnimGraphNode_Skeletal::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
    return FText::FromString(TEXT("CustomIK node"));
}

FString UMyAnimGraphNode_Skeletal::GetNodeCategory() const
{
    return TEXT("CustomIK node !!!");
}

const FAnimNode_SkeletalControlBase* UMyAnimGraphNode_Skeletal::GetNode() const
{
    return &Node;
}

