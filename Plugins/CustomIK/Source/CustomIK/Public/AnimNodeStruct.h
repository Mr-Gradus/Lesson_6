#pragma once

#include "CoreMinimal.h"
#include "BoneControllers/AnimNode_SkeletalControlBase.h"
#include "AnimNodeStruct.generated.h"


class USkeletalMeshComponent;
class UCapsuleComponent;
class UCharacterMovementComponent;


USTRUCT()
struct FPelvis
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "SkeletalControl")
	FBoneReference BoneReference;

	FVector Location;

	float Offset{0.f};

	FVector Translation;
};

USTRUCT()
struct FLeftFoot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "SkeletalControl")
	FBoneSocketTarget EffectorTarget;

	FVector EffectorLocation;

	UPROPERTY(EditAnywhere, Category = "SkeletalControl")
	FName SocketName;

	UPROPERTY(EditAnywhere, Category = "SkeletalControl")
	FBoneSocketTarget JointTarget;

	UPROPERTY(EditAnywhere, Category = "SkeletalControl")
	FVector JointTargetLocation;

	FVector Location;
	float Offset{0.f};
	FRotator Rotation;

	// Line Trace
	bool Hit{false};
	FVector_NetQuantizeNormal HitNormal;

	// Correction
	float HitNormalMax{0.f};
};

USTRUCT()
struct FRightFoot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "SkeletalControl")
	FBoneSocketTarget EffectorTarget;

	FVector EffectorLocation;

	UPROPERTY(EditAnywhere, Category = "SkeletalControl")
	FName SocketName;

	UPROPERTY(EditAnywhere, Category = "SkeletalControl")
	FBoneSocketTarget JointTarget;

	UPROPERTY(EditAnywhere, Category = "SkeletalControl")
	FVector JointTargetLocation;

	FVector Location;
	float Offset{0.f};
	FRotator Rotation;

	// Line Trace
	bool Hit{false};
	FVector_NetQuantizeNormal HitNormal;

	// Correction
	float HitNormalMax{0.f};
};

USTRUCT(BlueprintInternalUseOnly)
struct CUSTOMIK_API FAnimNode_CustomIK : public FAnimNode_SkeletalControlBase
{
	GENERATED_BODY()

public:
	
	UPROPERTY(EditAnywhere, Category = "SkeletalControl")
	FPelvis Pelvis;

	UPROPERTY(EditAnywhere, Category = "SkeletalControl")
	FLeftFoot LeftFoot;

	UPROPERTY(EditAnywhere, Category = "SkeletalControl")
	FRightFoot RightFoot;

	UPROPERTY(EditAnywhere, Category = "Calculations", meta = (ClampMin = "0.0", DisplayName = "Foot IK Apply Distance"))
	float ApplyDistance{ 0.f };

	UPROPERTY(EditAnywhere, Category = "Calculations", meta = (ClampMin = "0.0", ClampMax = "1.0", DisplayName = "Offset Interpolation Alpha"))
	float OffsetAlpha{ 0.f };

	UPROPERTY(EditAnywhere, Category = "Calculations", meta = (ClampMin = "0.0", ClampMax = "1.0", DisplayName = "Foot Rotation Interpolation Alpha"))
	float RotationAlpha{ 0.f };

	float PelvisOffsetLeft{ 0.f};
	float PelvisOffsetRight{ 0.f};

private:
	UPROPERTY()
	USkeletalMeshComponent* Mesh = nullptr;

	UPROPERTY()
	AActor* Actor = nullptr;

	UPROPERTY()
	UCapsuleComponent* CapsuleComponent = nullptr;

	UPROPERTY()
	UCharacterMovementComponent* CharacterMovement = nullptr;

	FCompactPoseBoneIndex CachedLowerLimbIndex_L{ INDEX_NONE };
	FCompactPoseBoneIndex CachedUpperLimbIndex_L{ INDEX_NONE };

	FCompactPoseBoneIndex CachedLowerLimbIndex_R{ INDEX_NONE };
	FCompactPoseBoneIndex CachedUpperLimbIndex_R{ INDEX_NONE };

	TArray<AActor*> ActorsToIgnore{ Actor };

	
public:

	// FAnimNode_Base interface
	virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
	// End of FAnimNode_Base interface

	// FAnimNode_SkeletalControlBase interface
	virtual void EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms) override;
	virtual bool IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) override;
	virtual void InitializeBoneReferences(const FBoneContainer& RequiredBones) override;
	// End of FAnimNode_SkeletalControlBase interface

	void LineTraceCalculations(FVector_NetQuantizeNormal& HitNormal, float& PelvisOffset, const FVector& ActorLocation,
		const FVector& FootLocation, float CapsuleHalfHeight) const;

	void PelvisSkeletalControl(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms,
		const FBoneReference& BoneReference, const FBoneContainer& BoneContainer, const FVector& Translation);

	void FootSkeletalControl(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms,
		const FBoneReference& BoneReference, const FBoneContainer& BoneContainer, const FCompactPoseBoneIndex& CachedUpperLimbIndex,
		const FCompactPoseBoneIndex& CachedLowerLimbIndex, const FBoneSocketTarget& EffectorTarget, const FVector& EffectorLocation,
		const FBoneSocketTarget& JointTarget, const FVector& JointTargetLocation, const FRotator& Rotation);
};
