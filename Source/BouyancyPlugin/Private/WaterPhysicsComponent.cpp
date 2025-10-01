#include "WaterPhysicsComponent.h"
#include "DrawDebugHelpers.h"
#include "EngineUtils.h"

UWaterPhysicsComponent::UWaterPhysicsComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UWaterPhysicsComponent::BeginPlay()
{
    Super::BeginPlay();
    
    UE_LOG(LogTemp, Warning, TEXT("Begin Play!!!"));
    
    BoxComponent = GetOwner()->FindComponentByClass<UBoxComponent>();
    
    if (!BoxComponent)
    {
        UE_LOG(LogTemp, Error, TEXT("No BoxComponent found on %s - Requires BoxComponent"), *GetOwner()->GetName());
        
        TArray<UActorComponent*> AllComponents;
        GetOwner()->GetComponents(AllComponents);
        UE_LOG(LogTemp, Error, TEXT("Available components on actor:"));
        for (UActorComponent* Comp : AllComponents)
        {
            UE_LOG(LogTemp, Error, TEXT("  - %s (%s)"), *Comp->GetName(), *Comp->GetClass()->GetName());
        }
        return;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("✅ Found BoxComponent: %s"), *BoxComponent->GetName());
    
    if (!BoxComponent->IsSimulatingPhysics())
    {
        UE_LOG(LogTemp, Error, TEXT("Physics NOT enabled on BoxComponent - Enable 'Simulate Physics'"));
        return;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("✅ Physics enabled on BoxComponent"));
    UE_LOG(LogTemp, Warning, TEXT("   Mass: %.2f kg"), BoxComponent->GetMass());
    
    FVector BoxExtent = BoxComponent->GetUnscaledBoxExtent();
    UE_LOG(LogTemp, Warning, TEXT("   Box Extents: (%.1f, %.1f, %.1f)"), BoxExtent.X, BoxExtent.Y, BoxExtent.Z);
    
    UE_LOG(LogTemp, Warning, TEXT("✅ Box Collision Buoyancy ready"));
    GenerateBuoyancyPoints();
}

void UWaterPhysicsComponent::TickComponent(float DeltaTime, ELevelTick TickType, 
                                          FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!BoxComponent)
    {
        UE_LOG(LogTemp, Error, TEXT("BoxComponent is NULL in Tick"));
        return;
    }
    
    if (!BoxComponent->IsSimulatingPhysics())
    {
        UE_LOG(LogTemp, Error, TEXT("Physics not simulating on BoxComponent"));
        return;
    }

    // CRITICAL DEBUG: Check mass vs force ratio
    float BoxMass = BoxComponent->GetMass();
    float WeightForce = BoxMass * 980.0f; // Weight in UE units
    
    static int32 FrameCount = 0;
    if (FrameCount % 60 == 0) // Log every 60 frames
    {
        UE_LOG(LogTemp, Error, TEXT("MASS VS FORCE"));
        UE_LOG(LogTemp, Error, TEXT("Box Mass: %.1f kg"), BoxMass);
        UE_LOG(LogTemp, Error, TEXT("Weight Force (down): %.1f N"), WeightForce);
        UE_LOG(LogTemp, Error, TEXT("To float, need buoyancy > %.1f N"), WeightForce);
    }
    FrameCount++;

    UE_LOG(LogTemp, Error, TEXT("BoxComponent Mobility: %s"), 
       BoxComponent->Mobility == EComponentMobility::Movable ? TEXT("Movable") : TEXT("NOT MOVABLE"));
    UE_LOG(LogTemp, Error, TEXT("BoxComponent Simulating: %s"), 
           BoxComponent->IsSimulatingPhysics() ? TEXT("YES") : TEXT("NO"));
    UE_LOG(LogTemp, Error, TEXT("BoxComponent World Location: %s"), 
           *BoxComponent->GetComponentLocation().ToString());

    ApplyBuoyancy(DeltaTime);
    
    if (bShowDebug)
    {
        DrawDebugInfo();
    }
}

void UWaterPhysicsComponent::GenerateBuoyancyPoints()
{
    UE_LOG(LogTemp, Warning, TEXT("Create Bouyancy Points!!!!!!"));
    BuoyancyPoints.Empty();
    
    FVector BoxExtent = BoxComponent->GetUnscaledBoxExtent();
    UE_LOG(LogTemp, Warning, TEXT("Box Extents: (%.1f, %.1f, %.1f)"), BoxExtent.X, BoxExtent.Y, BoxExtent.Z);
    UE_LOG(LogTemp, Warning, TEXT("Points Per Axis: %d"), PointsPerAxis);
    
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
    
    UE_LOG(LogTemp, Warning, TEXT("✅Created %d buoyancy points"), BuoyancyPoints.Num());
    
    // Log first few points for verification
    for (int32 i = 0; i < FMath::Min(5, BuoyancyPoints.Num()); i++)
    {
        UE_LOG(LogTemp, Warning, TEXT("   Point %d: %s"), i, *BuoyancyPoints[i].ToString());
    }
}

void UWaterPhysicsComponent::ApplyBuoyancy(float DeltaTime)
{
    UE_LOG(LogTemp, Error, TEXT("BoxComponent valid: %s"), BoxComponent ? TEXT("YES") : TEXT("NO"));
    UE_LOG(LogTemp, Error, TEXT("Is simulating: %s"), BoxComponent->IsSimulatingPhysics() ? TEXT("YES") : TEXT("NO"));
    UE_LOG(LogTemp, Error, TEXT("Box world location: %s"), *BoxComponent->GetComponentLocation().ToString());
    
    USceneComponent* RootComp = GetOwner()->GetRootComponent();
    UE_LOG(LogTemp, Error, TEXT("Actor root component: %s"), RootComp ? *RootComp->GetName() : TEXT("NULL"));
    UE_LOG(LogTemp, Error, TEXT("Is Box the root? %s"), (RootComp == BoxComponent) ? TEXT("YES") : TEXT("NO"));
    FVector BoxExtent = BoxComponent->GetUnscaledBoxExtent();
    
    float PointSizeX = (BoxExtent.X * 2.0f) / PointsPerAxis;
    float PointSizeY = (BoxExtent.Y * 2.0f) / PointsPerAxis;
    float PointSizeZ = (BoxExtent.Z * 2.0f) / PointsPerAxis;
    
    int32 UnderwaterPoints = 0;
    float TotalForceApplied = 0.0f;
    FVector CenterForceAccumulator = FVector::ZeroVector;
    
    for (int32 i = 0; i < BuoyancyPoints.Num(); i++)
    {
        const FVector& LocalPoint = BuoyancyPoints[i];
        FVector WorldPoint = GetOwner()->GetTransform().TransformPosition(LocalPoint);
        float WaterHeight = GetWaterHeightAtLocation(WorldPoint);
        
        if (WorldPoint.Z < WaterHeight)
        {
            UnderwaterPoints++;
            
            float SubmersionDepth = WaterHeight - WorldPoint.Z;
            float SubmergedHeight = FMath::Min(SubmersionDepth, PointSizeZ);
            float SubmergedVolume = PointSizeX * PointSizeY * SubmergedHeight;
            
            float BuoyancyForce = WaterDensity * SubmergedVolume * 98.0f * BuoyancyForceMultiplier;
            TotalForceApplied += BuoyancyForce;
            
            BoxComponent->AddForceAtLocation(FVector(0, 0, BuoyancyForce), WorldPoint);
        }
    }
    
    if (UnderwaterPoints > 0)
    {
        BoxComponent->AddForce(CenterForceAccumulator);
        
        UE_LOG(LogTemp, Error, TEXT("Underwater: %d points | Force per location: %.1f | Total center force: %.1f"), 
               UnderwaterPoints, TotalForceApplied, CenterForceAccumulator.Z);
    }
    
    ApplyDampingForces(DeltaTime);
}

void UWaterPhysicsComponent::ApplyDampingForces(float DeltaTime)
{
    FVector Velocity = BoxComponent->GetPhysicsLinearVelocity();
    FVector LinearDampingForce = -Velocity * LinearDamping * BoxComponent->GetMass();
    BoxComponent->AddForce(LinearDampingForce);
    
    FVector AngularVelocity = BoxComponent->GetPhysicsAngularVelocityInRadians();
    FVector AngularDampingTorque = -AngularVelocity * AngularDamping;
    BoxComponent->AddTorqueInRadians(AngularDampingTorque);
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
    if (!GetWorld() || !BoxComponent) return;
    
    FVector BoxCenter = BoxComponent->GetComponentLocation();
    FVector BoxExtent = BoxComponent->GetUnscaledBoxExtent();
    FQuat BoxRotation = BoxComponent->GetComponentQuat();
    
    DrawDebugBox(GetWorld(), BoxCenter, BoxExtent, BoxRotation, FColor::Yellow, false, -1.0f, 0, 3.0f);
    
    for (const FVector& LocalPoint : BuoyancyPoints)
    {
        FVector WorldPoint = GetOwner()->GetTransform().TransformPosition(LocalPoint);
        float WaterHeight = GetWaterHeightAtLocation(WorldPoint);
        
        FColor PointColor = (WorldPoint.Z < WaterHeight) ? FColor::Green : FColor::Orange;
        DrawDebugSphere(GetWorld(), WorldPoint, 8.0f, 8, PointColor, false, -1.0f, 0, 2.0f);
        
        if (WorldPoint.Z < WaterHeight)
        {
            FVector WaterSurface = FVector(WorldPoint.X, WorldPoint.Y, WaterHeight);
            DrawDebugLine(GetWorld(), WorldPoint, WaterSurface, FColor::Cyan, false, -1.0f, 0, 1.0f);
        }
    }
    
    DrawDebugString(GetWorld(), BoxCenter + FVector(0, 0, BoxExtent.Z + 50), 
                   FString::Printf(TEXT("BOX BUOYANCY: %d points"), BuoyancyPoints.Num()), 
                   nullptr, FColor::Green, -1.0f, true);
}