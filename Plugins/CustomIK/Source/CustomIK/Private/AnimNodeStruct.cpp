#include "AnimNodeStruct.h"

#include "TwoBoneIK.h"
#include "Animation/AnimInstanceProxy.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetSystemLibrary.h"

void FAnimNode_CustomIK::Initialize_AnyThread(const FAnimationInitializeContext& Context)
{
	Super::Initialize_AnyThread(Context);

	// Foot
	LeftFoot.EffectorTarget.Initialize(Context.AnimInstanceProxy);
	LeftFoot.JointTarget.Initialize(Context.AnimInstanceProxy);

	RightFoot.EffectorTarget.Initialize(Context.AnimInstanceProxy);
	RightFoot.JointTarget.Initialize(Context.AnimInstanceProxy);

	// References
	Mesh = Context.AnimInstanceProxy->GetSkelMeshComponent();
	Actor = Mesh->GetOwner();

	CapsuleComponent = Actor->FindComponentByClass<UCapsuleComponent>();
	CharacterMovement = Actor->FindComponentByClass<UCharacterMovementComponent>();
}

void FAnimNode_CustomIK::EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output,
	TArray<FBoneTransform>& OutBoneTransforms)
{
	Super::EvaluateSkeletalControl_AnyThread(Output, OutBoneTransforms);

	const FBoneContainer& BoneContainer { Output.Pose.GetPose().GetBoneContainer() };

	const float CapsuleHalfHeight { CapsuleComponent ? CapsuleComponent->GetUnscaledCapsuleHalfHeight() : 0.f };
	//const float CapsuleRadius { CapsuleComponent->GetUnscaledCapsuleRadius() };

	TEnumAsByte<enum EMovementMode> MovementMode { CharacterMovement ? CharacterMovement->MovementMode : EMovementMode::MOVE_None };

	FVector ActorLocation { Actor->GetActorLocation() };
	
	LeftFoot.Location = Mesh->GetSocketLocation(LeftFoot.SocketName);

	RightFoot.Location = Mesh->GetSocketLocation(RightFoot.SocketName);

	if(MovementMode != MOVE_Walking)
	{
		return;
	}

	LineTraceCalculations(LeftFoot.HitNormal, PelvisOffsetLeft, ActorLocation, LeftFoot.Location, CapsuleHalfHeight);
	LineTraceCalculations(RightFoot.HitNormal, PelvisOffsetRight, ActorLocation, RightFoot.Location, CapsuleHalfHeight);

	float PelvisOffsetNew = PelvisOffsetLeft < PelvisOffsetRight ? PelvisOffsetLeft : PelvisOffsetRight;

	Pelvis.Offset = Pelvis.Offset + OffsetAlpha * (PelvisOffsetNew - Pelvis.Offset);

	Pelvis.Translation = FVector(0.0f, 0.0f, Pelvis.Offset);

	if (PelvisOffsetLeft > PelvisOffsetRight)
	{
		LeftFoot.Offset += OffsetAlpha * (-PelvisOffsetRight + PelvisOffsetLeft - LeftFoot.Offset);
		RightFoot.Offset += OffsetAlpha * (0.f - RightFoot.Offset);
	}
	else
	{
		LeftFoot.Offset += OffsetAlpha * (0.f - LeftFoot.Offset);
		RightFoot.Offset += OffsetAlpha * (PelvisOffsetLeft - PelvisOffsetRight - RightFoot.Offset);
	}

	LeftFoot.EffectorLocation = FVector(LeftFoot.Offset, 0.0f, 0.0f);
	RightFoot.EffectorLocation = FVector(RightFoot.Offset, 0.0f, 0.0f);

	//Foot rotation
	const float LeftFootPitch{-180.f / PI * FGenericPlatformMath::Atan2(LeftFoot.HitNormal.X, LeftFoot.HitNormal.Z) };
	constexpr float LeftFootYaw{ 0.f };
	const float LeftFootRoll{ 180.f / PI * FGenericPlatformMath::Atan2(LeftFoot.HitNormal.Y, LeftFoot.HitNormal.Z) };

	LeftFoot.Rotation += RotationAlpha * (FRotator{ LeftFootPitch, LeftFootYaw, LeftFootRoll } - LeftFoot.Rotation);

	const float RightFootPitch{-180.f / PI * FGenericPlatformMath::Atan2(RightFoot.HitNormal.X, RightFoot.HitNormal.Z) };
	constexpr float RightFootYaw{ 0.f };
	const float RightFootRoll{ 180.f / PI * FGenericPlatformMath::Atan2(RightFoot.HitNormal.Y, RightFoot.HitNormal.Z) };

	RightFoot.Rotation += RotationAlpha * (FRotator{ RightFootPitch, RightFootYaw, RightFootRoll } - RightFoot.Rotation);

	// skeletal controls
	// Pelvis translation
	PelvisSkeletalControl(Output, OutBoneTransforms, Pelvis.BoneReference, BoneContainer, Pelvis.Translation);

	// Foot ik and rotation
	FootSkeletalControl(Output, OutBoneTransforms, LeftFoot.EffectorTarget.BoneReference, BoneContainer,
		CachedUpperLimbIndex_L, CachedLowerLimbIndex_L, LeftFoot.EffectorTarget, LeftFoot.EffectorLocation,
		LeftFoot.JointTarget, LeftFoot.JointTargetLocation, LeftFoot.Rotation);

	FootSkeletalControl(Output, OutBoneTransforms, RightFoot.EffectorTarget.BoneReference, BoneContainer,
		CachedUpperLimbIndex_R, CachedLowerLimbIndex_R, RightFoot.EffectorTarget, RightFoot.EffectorLocation,
		RightFoot.JointTarget, RightFoot.JointTargetLocation, RightFoot.Rotation);
}

bool FAnimNode_CustomIK::IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones)
{
	

	// Pelvis
	if (!Pelvis.BoneReference.IsValidToEvaluate(RequiredBones))
	{
		return false;
	}
	
	// Foot
	if ((!LeftFoot.EffectorTarget.BoneReference.IsValidToEvaluate(RequiredBones)) ||
		(CachedUpperLimbIndex_L == INDEX_NONE) ||
		(CachedLowerLimbIndex_L == INDEX_NONE) ||
		(!LeftFoot.EffectorTarget.IsValidToEvaluate(RequiredBones)) ||
		(!LeftFoot.JointTarget.IsValidToEvaluate(RequiredBones)))
	{
		return false;
	}

	if ((!RightFoot.EffectorTarget.BoneReference.IsValidToEvaluate(RequiredBones)) ||
		(CachedUpperLimbIndex_R == INDEX_NONE) ||
		(CachedLowerLimbIndex_R == INDEX_NONE) ||
		(!RightFoot.EffectorTarget.IsValidToEvaluate(RequiredBones)) ||
		(!RightFoot.JointTarget.IsValidToEvaluate(RequiredBones)))
	{
		return false;
	}

	return true;
}

void FAnimNode_CustomIK::InitializeBoneReferences(const FBoneContainer& RequiredBones)
{
	// Pelvis
	Pelvis.BoneReference.Initialize(RequiredBones);

	// Foot
	LeftFoot.EffectorTarget.BoneReference.Initialize(RequiredBones);

	LeftFoot.EffectorTarget.InitializeBoneReferences(RequiredBones);
	LeftFoot.JointTarget.InitializeBoneReferences(RequiredBones);

	const FCompactPoseBoneIndex IKBoneCompactPoseIndex_L = LeftFoot.EffectorTarget.BoneReference.GetCompactPoseIndex(RequiredBones);
	CachedLowerLimbIndex_L = FCompactPoseBoneIndex(INDEX_NONE);
	CachedUpperLimbIndex_L = FCompactPoseBoneIndex(INDEX_NONE);
	if (IKBoneCompactPoseIndex_L != INDEX_NONE)
	{
		CachedLowerLimbIndex_L = RequiredBones.GetParentBoneIndex(IKBoneCompactPoseIndex_L);
		if (CachedLowerLimbIndex_L != INDEX_NONE)
		{
			CachedUpperLimbIndex_L = RequiredBones.GetParentBoneIndex(CachedLowerLimbIndex_L);
		}
	}

	RightFoot.EffectorTarget.BoneReference.Initialize(RequiredBones);

	RightFoot.EffectorTarget.InitializeBoneReferences(RequiredBones);
	RightFoot.JointTarget.InitializeBoneReferences(RequiredBones);

	const FCompactPoseBoneIndex IKBoneCompactPoseIndex_R = RightFoot.EffectorTarget.BoneReference.GetCompactPoseIndex(RequiredBones);
	CachedLowerLimbIndex_R = FCompactPoseBoneIndex(INDEX_NONE);
	CachedUpperLimbIndex_R = FCompactPoseBoneIndex(INDEX_NONE);
	if (IKBoneCompactPoseIndex_R != INDEX_NONE)
	{
		CachedLowerLimbIndex_R = RequiredBones.GetParentBoneIndex(IKBoneCompactPoseIndex_R);
		if (CachedLowerLimbIndex_R != INDEX_NONE)
		{
			CachedUpperLimbIndex_R = RequiredBones.GetParentBoneIndex(CachedLowerLimbIndex_R);
		}
	}
}

void FAnimNode_CustomIK::LineTraceCalculations(FVector_NetQuantizeNormal& HitNormal, float& PelvisOffset,
	const FVector& ActorLocation, const FVector& FootLocation, float CapsuleHalfHeight) const
{
	FHitResult HitResult;
	//FCollisionQueryParams TraceParams;

	const FVector Start = FVector(FootLocation.X, FootLocation.Y, ActorLocation.Z);
	const FVector End = FVector(FootLocation.X, FootLocation.Y, (ActorLocation.Z - CapsuleHalfHeight - ApplyDistance));

	//ActorsToIgnore.Add(Actor);

	UKismetSystemLibrary::LineTraceSingle(Mesh, Start, End, TraceTypeQuery1, false, ActorsToIgnore, EDrawDebugTrace::None, HitResult, true);

	HitNormal = HitResult.Normal;

	// Pelvis Offset
	if (HitResult.bBlockingHit)
	{
		PelvisOffset = CapsuleHalfHeight - HitResult.Distance;
	}
	else
	{
		PelvisOffset = 0.0f;
	}
}

void FAnimNode_CustomIK::PelvisSkeletalControl(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms,
	const FBoneReference& BoneReference, const FBoneContainer& BoneContainer, const FVector& Translation)
{
	const FCompactPoseBoneIndex CompactPoseBoneReference{ BoneReference.GetCompactPoseIndex(BoneContainer) };

	FTransform NewBoneTM{ Output.Pose.GetComponentSpaceTransform(CompactPoseBoneReference) };

	const FTransform ComponentTransform{ Output.AnimInstanceProxy->GetComponentTransform() };

	FAnimationRuntime::ConvertCSTransformToBoneSpace(ComponentTransform, Output.Pose, NewBoneTM, CompactPoseBoneReference, BCS_ComponentSpace);

	NewBoneTM.AddToTranslation(Translation);

	FAnimationRuntime::ConvertBoneSpaceTransformToCS(ComponentTransform, Output.Pose, NewBoneTM, CompactPoseBoneReference, BCS_ComponentSpace);

	OutBoneTransforms.Add(FBoneTransform{ BoneReference.GetCompactPoseIndex(BoneContainer), NewBoneTM });
}

void FAnimNode_CustomIK::FootSkeletalControl(FComponentSpacePoseContext& Output,
	TArray<FBoneTransform>& OutBoneTransforms, const FBoneReference& BoneReference, const FBoneContainer& BoneContainer,
	const FCompactPoseBoneIndex& CachedUpperLimbIndex, const FCompactPoseBoneIndex& CachedLowerLimbIndex,
	const FBoneSocketTarget& EffectorTarget, const FVector& EffectorLocation, const FBoneSocketTarget& JointTarget,
	const FVector& JointTargetLocation, const FRotator& Rotation)
{
	// TwoBone IK
	// Get indices of the lower and upper limb bones and check validity.
	FCompactPoseBoneIndex BoneReferenceCompactPoseIndex { BoneReference.GetCompactPoseIndex(BoneContainer) };

	// Get Component Space transforms for our bones.
	FTransform UpperLimbCSTransform { Output.Pose.GetComponentSpaceTransform(CachedUpperLimbIndex) };
	FTransform LowerLimbCSTransform { Output.Pose.GetComponentSpaceTransform(CachedLowerLimbIndex) };
	FTransform EndBoneCSTransform { Output.Pose.GetComponentSpaceTransform(BoneReferenceCompactPoseIndex) };

	// This is our reach goal.
	FVector DesiredPos { ((EffectorTarget.GetTargetTransform(EffectorLocation, Output.Pose,
		Output.AnimInstanceProxy->GetComponentTransform())).GetTranslation() + Pelvis.Translation) };

	FVector	JointTargetPos { (JointTarget.GetTargetTransform(JointTargetLocation, Output.Pose,
		Output.AnimInstanceProxy->GetComponentTransform())).GetTranslation() };

	UpperLimbCSTransform.SetLocation(UpperLimbCSTransform.GetTranslation() + Pelvis.Translation);
	LowerLimbCSTransform.SetLocation(LowerLimbCSTransform.GetTranslation() + Pelvis.Translation);
	EndBoneCSTransform.SetLocation(EndBoneCSTransform.GetTranslation() + Pelvis.Translation);

	AnimationCore::SolveTwoBoneIK(UpperLimbCSTransform, LowerLimbCSTransform,
		EndBoneCSTransform, JointTargetPos, DesiredPos, false, 1.0f, 1.2f);

	// Rotation
	const FTransform ComponentTransform { Output.AnimInstanceProxy->GetComponentTransform() };
	FAnimationRuntime::ConvertCSTransformToBoneSpace(ComponentTransform, Output.Pose, EndBoneCSTransform, BoneReferenceCompactPoseIndex, BCS_WorldSpace);
	const FQuat BoneQuat(Rotation);
	EndBoneCSTransform.SetRotation(BoneQuat * EndBoneCSTransform.GetRotation());
	FAnimationRuntime::ConvertBoneSpaceTransformToCS(ComponentTransform, Output.Pose, EndBoneCSTransform, BoneReferenceCompactPoseIndex, BCS_WorldSpace);

	// Update transforms. Order is important.
	OutBoneTransforms.Add(FBoneTransform{ CachedUpperLimbIndex, UpperLimbCSTransform });
	OutBoneTransforms.Add(FBoneTransform{ CachedLowerLimbIndex, LowerLimbCSTransform });
	OutBoneTransforms.Add(FBoneTransform{ BoneReferenceCompactPoseIndex, EndBoneCSTransform });
}


