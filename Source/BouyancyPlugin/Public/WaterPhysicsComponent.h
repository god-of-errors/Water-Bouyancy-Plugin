#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "WaterBodyActor.h"
#include "WaterBodyComponent.h"
#include "Components/CapsuleComponent.h"
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
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyancy", meta = (ClampMin = "0.0"))
    float WaterDensity = 1000.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyancy", meta = (ClampMin = "0.0"))
    float BuoyancyForceMultiplier = 1.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Box Collision", meta = (ClampMin = "2"))
    int32 PointsPerAxis = 3;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damping", meta = (ClampMin = "0.0"))
    float LinearDamping = 2.f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damping", meta = (ClampMin = "0.0"))
    float AngularDamping = 20.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
    bool bShowDebug = true;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Debug")
    bool bIsStaticMesh = false;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Debug")
    bool bIsSphere = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Debug")
    bool bIsBox = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Debug")
    bool bIsCapsule = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
    bool bShowDetailedLogs = true;

private:
    UPROPERTY()
    TArray<FVector> BuoyancyPoints;

    UPROPERTY()
    UPrimitiveComponent* PhysicsComp = nullptr;
    
    void GenerateBuoyancyPoints();
    void GenerateBoxBuoyancyPoints();
    void GenerateSphereBuoyancyPoints();
    void GenerateStaticMeshBuoyancyPoints();
    void GenerateCapsuleBuoyancyPoints();
    void ApplyBuoyancy(float DeltaTime);
    void ApplyDampingForces(float DeltaTime) const;
    float GetWaterHeightAtLocation(const FVector& WorldLocation) const;
    void DrawDebugInfo();
};