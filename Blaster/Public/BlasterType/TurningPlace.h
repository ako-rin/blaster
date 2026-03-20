#pragma once

UENUM(BlueprintType)
enum class ETurningInPlace : uint8
{
	TIP_Left UMETA(DisplayName = "Turning Left"),
	TIP_Right UMETA(DisplayName = "Turning Right"),
	TIP_NotTurning UMETA(DisplayName = "Not Turning"),
	TIP_MAX UMETA(DisplayName = "DefaultMAX")
};