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
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic Buoyancy")
	float WaterDensity = 1000.0f;
    
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic Buoyancy")
	float BuoyancyForceMultiplier = 1.0f;
    
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic Buoyancy")
	float ObjectRadius = 100.0f;
    
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damping")
	float LinearDamping = 0.1f;
    
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damping")
	float AngularDamping = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool bShowDebug = true;

private:
	UPROPERTY()
	UPrimitiveComponent* PhysicsComponent;
    
	void ApplyBasicBuoyancy(float DeltaTime);
	void ApplyDampingForces(float DeltaTime);
	float GetWaterHeightAtLocation(const FVector& WorldLocation);
	void DrawDebugInfo();
};