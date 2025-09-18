#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/PrimitiveComponent.h"
#include "WaterBodyActor.h"
#include "WaterBodyComponent.h"
#include "Engine/World.h"
#include "WaterPhysicsComponent.generated.h"

UCLASS(ClassGroup=(Physics), meta=(BlueprintSpawnableComponent))
class UWaterPhysicsComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWaterPhysicsComponent();

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, 
							  FActorComponentTickFunction* ThisTickFunction) override;

public:
	// ENHANCED DEBUG - Comprehensive water detection info
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool bShowDebug = true;
    
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool bShowDetailedLogs = true;
    
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool bShowWaterBodyInfo = true;

private:
	UPROPERTY()
	UPrimitiveComponent* PhysicsComponent;
    
	// Enhanced water detection
	float GetWaterHeightAtLocation(const FVector& WorldLocation);
	void DrawDebugInfo();
	void LogWaterSystemStatus();
	void DrawWaterBodyBounds();
	void DisplayWaterBodyInfoAtLocation(const FVector& BasePosition, int32& CurrentLine, const FVector& QueryLocation);
	void DisplayWorldWaterScan(const FVector& BasePosition, int32& CurrentLine);
};