#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
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
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyancy")
    float WaterDensity = 1000.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyancy")
    float BuoyancyForceMultiplier = 1.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Box Collision")
    int32 PointsPerAxis = 3;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damping")
    float LinearDamping = 1.f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damping")
    float AngularDamping = 20.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
    bool bShowDebug = true;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
    bool bShowSphereDebug = true;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Debug")
    bool bIsSphere = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Debug")
    bool bIsStaticMesh = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
    bool bShowDetailedLogs = true;

private:
    UPROPERTY()
    UBoxComponent* BoxComponent;
    
    UPROPERTY()
    USphereComponent* SphereComponent;

    UPROPERTY()
    UStaticMeshComponent* StaticMeshComponent;
    
    UPROPERTY()
    TArray<FVector> BuoyancyPoints;
    
    void GenerateBuoyancyPoints();
    void GenerateBoxBuoyancyPoints();
    void GenerateSphereBuoyancyPoints();
    void GenerateStaticMeshBuoyancyPoints();
    void ApplyBuoyancy(float DeltaTime);
    void ApplyDampingForces(float DeltaTime);
    float GetWaterHeightAtLocation(const FVector& WorldLocation);
    void DrawDebugInfo();
};