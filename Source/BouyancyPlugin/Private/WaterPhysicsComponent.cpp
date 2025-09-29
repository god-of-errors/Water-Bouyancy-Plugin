#include "WaterPhysicsComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "EngineUtils.h"
#include "Components/BoxComponent.h"
#include "Engine/Engine.h"

UWaterPhysicsComponent::UWaterPhysicsComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UWaterPhysicsComponent::BeginPlay()
{
    Super::BeginPlay();
    
    // Simple and reliable component detection
    UBoxComponent* BoxComp = GetOwner()->FindComponentByClass<UBoxComponent>();
    
    if (BoxComp)
    {
        PhysicsComponent = BoxComp;
        UE_LOG(LogTemp, Warning, TEXT("Found BoxComponent: %s"), *BoxComp->GetName());
        
        if (!BoxComp->IsSimulatingPhysics())
        {
            UE_LOG(LogTemp, Error, TEXT("Physics not enabled on BoxComponent - enable 'Simulate Physics' on the Box component"));
        }
    }
    else
    {
        // Fallback to any primitive component
        PhysicsComponent = GetOwner()->FindComponentByClass<UPrimitiveComponent>();
        if (PhysicsComponent)
        {
            UE_LOG(LogTemp, Warning, TEXT("No BoxComponent found, using: %s"), *PhysicsComponent->GetClass()->GetName());
        }
    }
    
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
        UE_LOG(LogTemp, Warning, TEXT("Using component type: %s"), *PhysicsComponent->GetClass()->GetName());

        if (bAutoGeneratePoints)
        {
            GenerateBuoyancyPoints();
        }
    }
}

void UWaterPhysicsComponent::TickComponent(float DeltaTime, ELevelTick TickType, 
                                          FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (PhysicsComponent && PhysicsComponent->IsSimulatingPhysics())
    {
        ApplyBuoyancy(DeltaTime);
    }
    
    if (bShowDebug)
    {
        DrawDebugInfo();
    }
}

void UWaterPhysicsComponent::ApplyBuoyancy(float DeltaTime)
{
    if (bAutoGeneratePoints && BuoyancyPoints.Num() > 0)
    {
        MultiPointBouyancy(DeltaTime);
    }
    else
    {
        SinglePointBouyancy(DeltaTime);
    }
}


void UWaterPhysicsComponent::SinglePointBouyancy(float DeltaTime)
{
    FVector ObjectLocation = GetOwner()->GetActorLocation();
    float WaterHeight = SampleWaveHeightAtLocation(ObjectLocation);
    
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

void UWaterPhysicsComponent::MultiPointBouyancy(float DeltaTime)
{
    for (const FVector& LocalPoint : BuoyancyPoints)
    {
        FVector WorldPoint = GetOwner()->GetTransform().TransformPosition(LocalPoint);
        float WaterHeight = SampleWaveHeightAtLocation(WorldPoint);
            
        if (WorldPoint.Z < WaterHeight)
        {
            float SubmersionDepth = WaterHeight - WorldPoint.Z;
                
            // Calculate cubic volume for each point instead of spherical
            FVector BoxExtent;
            if (UBoxComponent* BoxComp = Cast<UBoxComponent>(PhysicsComponent))
            {
                BoxExtent = BoxComp->GetUnscaledBoxExtent();
            }
            else
            {
                BoxExtent = FVector(ObjectRadius, ObjectRadius, ObjectRadius);
            }
                
            // Volume per point (divide box into grid)
            float PointSizeX = (BoxExtent.X * 2.0f) / PointsPerAxis;
            float PointSizeY = (BoxExtent.Y * 2.0f) / PointsPerAxis;
            float PointSizeZ = (BoxExtent.Z * 2.0f) / PointsPerAxis;
                
            // Calculate submerged portion of this cubic point
            float SubmergedHeight = FMath::Min(SubmersionDepth, PointSizeZ);
            float SubmergedVolume = PointSizeX * PointSizeY * SubmergedHeight;
                
            float BuoyancyForce = WaterDensity * SubmergedVolume * 980.0f * BuoyancyForceMultiplier;
            PhysicsComponent->AddForceAtLocation(FVector(0, 0, BuoyancyForce), WorldPoint);
        }
    }
    ApplyDampingForces(DeltaTime);
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

void UWaterPhysicsComponent::GenerateBuoyancyPoints()
{
    BuoyancyPoints.Empty();
    
    if (UBoxComponent* BoxComp = Cast<UBoxComponent>(PhysicsComponent))
    {
        GenerateBoxBuoyancyPoints();
        UE_LOG(LogTemp, Warning, TEXT("Generated %d bouyancy points for box collider on %s"), BuoyancyPoints.Num(), *GetOwner()->GetName());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Non-box collider detected on %s usig single-point buoyancy"), *GetOwner()->GetName());
    }
}

void UWaterPhysicsComponent::GenerateBoxBuoyancyPoints()
{
    FVector BoxExtent;
    
    if (UBoxComponent* BoxComp = Cast<UBoxComponent>(PhysicsComponent))
    {
        BoxExtent = BoxComp->GetUnscaledBoxExtent();
    }
    else
    {
        return;
    }
    
    for (int32 X = 0; X < PointsPerAxis; X++)
    {
        for (int32 Y = 0; Y < PointsPerAxis; Y++)
        {
            for (int32 Z = 0; Z < PointsPerAxis; Z++)
            {
                FVector LocalPosition = FVector(
                    BoxExtent.X * (2.0f * X / (PointsPerAxis - 1) - 1.0f),
                    BoxExtent.Y * (2.0f * Y / (PointsPerAxis - 1) - 1.0f),
                    BoxExtent.Z * (2.0f * Z / (PointsPerAxis - 1) - 1.0f)
                );
                
                BuoyancyPoints.Add(LocalPosition);
            }
        }
    }
}

float UWaterPhysicsComponent::SampleWaveHeightAtLocation(const FVector& WorldLocation)
{
    if (!GetWorld()) return -99999.0f;
    
    for (TActorIterator<AWaterBody> WaterBodyIterator(GetWorld()); WaterBodyIterator; ++WaterBodyIterator)
    {
        AWaterBody* WaterBody = *WaterBodyIterator;
        if (WaterBody)
        {
            float BaseWaterHeight = GetWaterHeightAtLocation(WorldLocation);
            
            if (bUseWavePhysics && WaterBody->GetWaterWaves())
            {
                UWaterWavesBase* WaterWaves = WaterBody->GetWaterWaves();
                float WaterDepth = 100.0f;
                float CurrentTime = GetWorld()->GetTimeSeconds();
                
                float WaveDisplacement = WaterWaves->GetSimpleWaveHeightAtPosition(
                    WorldLocation,
                    WaterDepth,
                    CurrentTime
                );
                
                return BaseWaterHeight + WaveDisplacement;
            }
            else
            {
                return BaseWaterHeight;
            }
        }
    }
    
    return -99999.0f;
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
    
    // DEBUG: Show box collider bounds and properties
    if (UBoxComponent* BoxComp = Cast<UBoxComponent>(PhysicsComponent))
    {
        FVector BoxExtent = BoxComp->GetUnscaledBoxExtent();
        FVector BoxCenter = BoxComp->GetComponentLocation();
        FQuat BoxRotation = BoxComp->GetComponentQuat();
        
        // Draw the box collider wireframe
        DrawDebugBox(GetWorld(), BoxCenter, BoxExtent, BoxRotation, FColor::Yellow, false, -1.0f, 0, 3.0f);
        
        // Show box dimensions
        DrawDebugString(GetWorld(), BoxCenter + FVector(0, 0, BoxExtent.Z + 20), 
                       FString::Printf(TEXT("BOX: %.1fx%.1fx%.1f"), BoxExtent.X*2, BoxExtent.Y*2, BoxExtent.Z*2), 
                       nullptr, FColor::Yellow, -1.0f, true);
        
        // Show physics status
        FString PhysicsStatus = BoxComp->IsSimulatingPhysics() ? TEXT("PHYSICS: ON") : TEXT("PHYSICS: OFF");
        FColor PhysicsColor = BoxComp->IsSimulatingPhysics() ? FColor::Green : FColor::Red;
        DrawDebugString(GetWorld(), BoxCenter + FVector(0, 0, BoxExtent.Z + 40), 
                       PhysicsStatus, nullptr, PhysicsColor, -1.0f, true);
        
        // Show mass
        float Mass = BoxComp->GetMass();
        DrawDebugString(GetWorld(), BoxCenter + FVector(0, 0, BoxExtent.Z + 60), 
                       FString::Printf(TEXT("MASS: %.1f kg"), Mass), 
                       nullptr, FColor::White, -1.0f, true);
    }
    
    // Show buoyancy points if they exist
    if (bAutoGeneratePoints && BuoyancyPoints.Num() > 0)
    {
        for (const FVector& LocalPoint : BuoyancyPoints)
        {
            FVector WorldPoint = GetOwner()->GetTransform().TransformPosition(LocalPoint);
            float PointWaterHeight = SampleWaveHeightAtLocation(WorldPoint);
            
            // Color code: Green = underwater, Orange = above water
            FColor PointColor = (WorldPoint.Z < PointWaterHeight) ? FColor::Green : FColor::Orange;
            DrawDebugSphere(GetWorld(), WorldPoint, 8.0f, 8, PointColor, false, -1.0f, 0, 2.0f);
            
            // Show water line for underwater points
            if (WorldPoint.Z < PointWaterHeight)
            {
                FVector WaterSurface = FVector(WorldPoint.X, WorldPoint.Y, PointWaterHeight);
                DrawDebugLine(GetWorld(), WorldPoint, WaterSurface, FColor::Cyan, false, -1.0f, 0, 1.0f);
            }
        }
        
        // Show total point count
        DrawDebugString(GetWorld(), ObjectLocation + FVector(0, 0, 100), 
                       FString::Printf(TEXT("BUOYANCY POINTS: %d"), BuoyancyPoints.Num()), 
                       nullptr, FColor::Cyan, -1.0f, true);
    }
    
    // Existing water detection code...
    float WaterHeight = GetWaterHeightAtLocation(ObjectLocation);
    if (WaterHeight > -99999.0f)
    {
        FVector WaterPoint = FVector(ObjectLocation.X, ObjectLocation.Y, WaterHeight);
        DrawDebugSphere(GetWorld(), WaterPoint, 25.0f, 8, FColor::Blue, false, -1.0f, 0, 2.0f);
        
        float SubmersionDepth = WaterHeight - ObjectLocation.Z;
        DrawDebugString(GetWorld(), ObjectLocation + FVector(0, 0, 120), 
                       FString::Printf(TEXT("SUBMERSION: %.1fcm"), SubmersionDepth), 
                       nullptr, SubmersionDepth > 0 ? FColor::Green : FColor::Orange, -1.0f, true);
    }
    
    // Show object center
    DrawDebugSphere(GetWorld(), ObjectLocation, 15.0f, 12, FColor::Red, false, -1.0f, 0, 3.0f);
}