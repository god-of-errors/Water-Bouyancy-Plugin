#include "WaterPhysicsComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "EngineUtils.h"
#include "Engine/Engine.h"

UWaterPhysicsComponent::UWaterPhysicsComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UWaterPhysicsComponent::BeginPlay()
{
    Super::BeginPlay();
    
    PhysicsComponent = GetOwner()->FindComponentByClass<UPrimitiveComponent>();
    
    if (!PhysicsComponent)
    {
        UE_LOG(LogTemp, Error, TEXT("No PrimitiveComponent found on %s"), *GetOwner()->GetName());
    }
    else if (!PhysicsComponent->IsSimulatingPhysics())
    {
        UE_LOG(LogTemp, Warning, TEXT("Physics not enabled on %s"), *PhysicsComponent->GetName());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Water Physics ready on %s"), *GetOwner()->GetName());
    }
}

void UWaterPhysicsComponent::TickComponent(float DeltaTime, ELevelTick TickType, 
                                          FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (PhysicsComponent && PhysicsComponent->IsSimulatingPhysics())
    {
        ApplyBasicBuoyancy(DeltaTime);
    }
    
    if (bShowDebug)
    {
        DrawDebugInfo();
    }
}

void UWaterPhysicsComponent::ApplyBasicBuoyancy(float DeltaTime)
{
    FVector ObjectLocation = GetOwner()->GetActorLocation();
    float WaterHeight = GetWaterHeightAtLocation(ObjectLocation);
    
    if (ObjectLocation.Z < WaterHeight)
    {
        float SubmersionDepth = WaterHeight - ObjectLocation.Z;
        float EffectiveRadius = ObjectRadius * 0.5f;
        
        float SubmergedVolume;
        if (SubmersionDepth >= EffectiveRadius * 2.0f)
        {
            SubmergedVolume = (4.0f/3.0f) * PI * FMath::Pow(EffectiveRadius, 3);
        }
        else
        {
            float h = FMath::Min(SubmersionDepth, EffectiveRadius * 2.0f);
            SubmergedVolume = PI * h * h * (3.0f * EffectiveRadius - h) / 3.0f;
        }
        
        float ObjectMass = PhysicsComponent->GetMass();
        float WeightForce = ObjectMass * 980.0f;
        
        float BuoyancyMultiplier = (SubmersionDepth / EffectiveRadius);
        BuoyancyMultiplier = FMath::Clamp(BuoyancyMultiplier, 0.0f, 2.0f);
        
        float BuoyancyForceUE = WeightForce * BuoyancyMultiplier * BuoyancyForceMultiplier;
        FVector BuoyancyForce = FVector(0, 0, BuoyancyForceUE);
        
        PhysicsComponent->AddForce(BuoyancyForce);
        ApplyDampingForces(DeltaTime);
    }
}

void UWaterPhysicsComponent::ApplyDampingForces(float DeltaTime)
{
    FVector Velocity = PhysicsComponent->GetPhysicsLinearVelocity();
    FVector LinearDampingForce = -Velocity * LinearDamping * PhysicsComponent->GetMass();
    PhysicsComponent->AddForce(LinearDampingForce);
    
    FVector AngularVelocity = PhysicsComponent->GetPhysicsAngularVelocityInRadians();
    FVector AngularDampingTorque = -AngularVelocity * AngularDamping;
    PhysicsComponent->AddTorqueInRadians(AngularDampingTorque);
}

float UWaterPhysicsComponent::GetWaterHeightAtLocation(const FVector& WorldLocation)
{
    if (!GetWorld()) return -99999.0f;
    
    for (TActorIterator<AWaterBody> WaterBodyIterator(GetWorld()); WaterBodyIterator; ++WaterBodyIterator)
    {
        AWaterBody* WaterBody = *WaterBodyIterator;
        if (WaterBody && WaterBody->GetWaterBodyComponent())
        {
            FVector WaterSurfaceLocation;
            FVector WaterSurfaceNormal;
            FVector WaterVelocity;
            float WaterDepth;
            
            WaterBody->GetWaterBodyComponent()->GetWaterSurfaceInfoAtLocation(
                WorldLocation,
                WaterSurfaceLocation,
                WaterSurfaceNormal,
                WaterVelocity,
                WaterDepth,
                true
            );
            
            return WaterSurfaceLocation.Z;
        }
    }
    
    return -99999.0f;
}

void UWaterPhysicsComponent::DrawDebugInfo()
{
    if (!GetWorld()) return;
    
    FVector ObjectLocation = GetOwner()->GetActorLocation();
    float WaterHeight = GetWaterHeightAtLocation(ObjectLocation);
    
    DrawDebugSphere(GetWorld(), ObjectLocation, 15.0f, 12, FColor::Red, false, -1.0f, 0, 3.0f);
    
    if (WaterHeight > -99999.0f)
    {
        FVector WaterPoint = FVector(ObjectLocation.X, ObjectLocation.Y, WaterHeight);
        DrawDebugSphere(GetWorld(), WaterPoint, 25.0f, 8, FColor::Blue, false, -1.0f, 0, 2.0f);
        DrawDebugLine(GetWorld(), ObjectLocation, WaterPoint, FColor::Cyan, false, -1.0f, 0, 3.0f);
        
        float SubmersionDepth = WaterHeight - ObjectLocation.Z;
        FVector TextPosition = ObjectLocation + FVector(0, 0, 80);
        
        if (SubmersionDepth > 0)
        {
            FVector ForceArrow = ObjectLocation + FVector(0, 0, 50);
            DrawDebugDirectionalArrow(GetWorld(), ObjectLocation, ForceArrow, 
                                    25.0f, FColor::Green, false, -1.0f, 0, 5.0f);
            
            FVector Velocity = PhysicsComponent->GetPhysicsLinearVelocity();
            if (Velocity.Size() > 10.0f)
            {
                FVector DampingArrow = ObjectLocation + (Velocity.GetSafeNormal() * -30.0f);
                DrawDebugDirectionalArrow(GetWorld(), ObjectLocation, DampingArrow, 
                                        15.0f, FColor::Orange, false, -1.0f, 0, 3.0f);
            }
            
            DrawDebugString(GetWorld(), TextPosition, 
                           FString::Printf(TEXT("BUOYANCY + DAMPING: %.1fcm deep"), SubmersionDepth), 
                           nullptr, FColor::Green, 1.0f, true);
        }
        else
        {
            DrawDebugString(GetWorld(), TextPosition, 
                           FString::Printf(TEXT("ABOVE WATER: %.1fcm"), FMath::Abs(SubmersionDepth)), 
                           nullptr, FColor::Orange, 1.0f, true);
        }
    }
    else
    {
        DrawDebugString(GetWorld(), ObjectLocation + FVector(0, 0, 80), 
                       TEXT("NO WATER DETECTED"), nullptr, FColor::Red, 1.0f, true);
    }
}