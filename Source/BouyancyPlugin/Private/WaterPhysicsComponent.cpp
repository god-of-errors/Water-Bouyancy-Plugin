#include "WaterPhysicsComponent.h"
#include "Components/SphereComponent.h"
#include "DrawDebugHelpers.h"
#include "EngineUtils.h"

UWaterPhysicsComponent::UWaterPhysicsComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UWaterPhysicsComponent::BeginPlay()
{
    Super::BeginPlay();
    
    UE_LOG(LogTemp, Warning, TEXT("WATER PHYSICS COMPONENT BEGIN PLAY!!!"));
    
    // Try to find BoxComponent first
    BoxComponent = GetOwner()->FindComponentByClass<UBoxComponent>();
    
    if (BoxComponent)
    {
        bIsSphere = false;
        UE_LOG(LogTemp, Warning, TEXT("✅ Found BoxComponent: %s"), *BoxComponent->GetName());
        
        if (!BoxComponent->IsSimulatingPhysics())
        {
            UE_LOG(LogTemp, Error, TEXT("❌ Enable 'Simulate Physics' on BoxComponent"));
            return;
        }
        
        UE_LOG(LogTemp, Warning, TEXT("✅ Box Collision Buoyancy Mode"));
        UE_LOG(LogTemp, Warning, TEXT("   Mass: %.2f kg"), BoxComponent->GetMass());
        
        FVector BoxExtent = BoxComponent->GetUnscaledBoxExtent();
        UE_LOG(LogTemp, Warning, TEXT("   Box Extents: (%.1f, %.1f, %.1f)"), BoxExtent.X, BoxExtent.Y, BoxExtent.Z);
        
        GenerateBuoyancyPoints();
        return;
    }
    
    // If no box, try to find SphereComponent
    SphereComponent = GetOwner()->FindComponentByClass<USphereComponent>();
    
    if (SphereComponent)
    {
        bIsSphere = true;
        UE_LOG(LogTemp, Warning, TEXT("✅ Found SphereComponent: %s"), *SphereComponent->GetName());
        
        if (!SphereComponent->IsSimulatingPhysics())
        {
            UE_LOG(LogTemp, Error, TEXT("❌ Enable 'Simulate Physics' on SphereComponent"));
            return;
        }
        
        UE_LOG(LogTemp, Warning, TEXT("✅ Sphere Collision Buoyancy Mode"));
        UE_LOG(LogTemp, Warning, TEXT("   Mass: %.2f kg"), SphereComponent->GetMass());
        UE_LOG(LogTemp, Warning, TEXT("   Radius: %.1f"), SphereComponent->GetUnscaledSphereRadius());
        
        GenerateBuoyancyPoints();
        return;
    }

    StaticMeshComponent = GetOwner()->FindComponentByClass<UStaticMeshComponent>();
    
    if (StaticMeshComponent && StaticMeshComponent->IsSimulatingPhysics())
    {
        bIsStaticMesh = true;
        
        UE_LOG(LogTemp, Warning, TEXT("✅ Found StaticMeshComponent: %s"), *StaticMeshComponent->GetName());
        UE_LOG(LogTemp, Warning, TEXT("✅ Static Mesh Mode"));
        UE_LOG(LogTemp, Warning, TEXT("   Mass: %.2f kg"), StaticMeshComponent->GetMass());
        if (StaticMeshComponent->GetStaticMesh())
        {
            UE_LOG(LogTemp, Warning, TEXT("   Mesh: %s"), *StaticMeshComponent->GetStaticMesh()->GetName());
            FBoxSphereBounds Bounds = StaticMeshComponent->GetStaticMesh()->GetBounds();
            UE_LOG(LogTemp, Warning, TEXT("   Bounds: (%.1f, %.1f, %.1f)"), Bounds.BoxExtent.X, Bounds.BoxExtent.Y, Bounds.BoxExtent.Z);
        }
        GenerateBuoyancyPoints();
        return;
    }
    
    UE_LOG(LogTemp, Error, TEXT("❌ No BoxComponent, SphereComponent or StaticMeshComponent found"));
    TArray<UActorComponent*> AllComponents;
    GetOwner()->GetComponents(AllComponents);
    UE_LOG(LogTemp, Error, TEXT("Available components:"));
    for (UActorComponent* Comp : AllComponents)
    {
        UE_LOG(LogTemp, Error, TEXT("  - %s (%s)"), *Comp->GetName(), *Comp->GetClass()->GetName());
    }
}

void UWaterPhysicsComponent::TickComponent(float DeltaTime, ELevelTick TickType, 
                                          FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    
    UPrimitiveComponent* PhysicsComp = nullptr;
    
    if (StaticMeshComponent && StaticMeshComponent->IsSimulatingPhysics())
    {
        PhysicsComp = StaticMeshComponent;
    }
    else if (SphereComponent && SphereComponent->IsSimulatingPhysics())
    {
        PhysicsComp = SphereComponent;
    }
    else if (BoxComponent && BoxComponent->IsSimulatingPhysics())
    {
        PhysicsComp = BoxComponent;
    }
    
    if (!PhysicsComp)
    {
        UE_LOG(LogTemp, Error, TEXT("❌ No physics component in Tick"));
        UE_LOG(LogTemp, Error, TEXT("   BoxComponent: %s"), BoxComponent ? TEXT("Found") : TEXT("NULL"));
        UE_LOG(LogTemp, Error, TEXT("   SphereComponent: %s"), SphereComponent ? TEXT("Found") : TEXT("NULL"));
        UE_LOG(LogTemp, Error, TEXT("   StaticMeshComponent: %s"), StaticMeshComponent ? TEXT("Found") : TEXT("NULL"));
        return;
    }
    
    if (!PhysicsComp->IsSimulatingPhysics())
    {
        UE_LOG(LogTemp, Error, TEXT("❌ Physics not simulating on %s"), *PhysicsComp->GetName());
        return;
    }

    ApplyBuoyancy(DeltaTime);
    
    if (bShowDebug)
    {
        DrawDebugInfo();
    }
}

void UWaterPhysicsComponent::GenerateBuoyancyPoints()
{
    if (bIsSphere)
    {
        GenerateSphereBuoyancyPoints();
    }
    else if (bIsStaticMesh)
    {
        GenerateStaticMeshBuoyancyPoints();
    }
    else
    {
        GenerateBoxBuoyancyPoints();
    }
}

void UWaterPhysicsComponent::GenerateBoxBuoyancyPoints()
{
    UE_LOG(LogTemp, Warning, TEXT("GENERATING BOX BUOYANCY POINTS!!!"));
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
    
    UE_LOG(LogTemp, Warning, TEXT("✅ Generated %d buoyancy points"), BuoyancyPoints.Num());
    
    for (int32 i = 0; i < FMath::Min(5, BuoyancyPoints.Num()); i++)
    {
        UE_LOG(LogTemp, Warning, TEXT("   Point %d: %s"), i, *BuoyancyPoints[i].ToString());
    }
}

void UWaterPhysicsComponent::GenerateSphereBuoyancyPoints()
{
    UE_LOG(LogTemp, Warning, TEXT("GENERATING SPHERE BUOYANCY POINTS!!!"));
    BuoyancyPoints.Empty();
    
    float SphereRadius = SphereComponent->GetUnscaledSphereRadius();
    UE_LOG(LogTemp, Warning, TEXT("Sphere Radius: %.1f"), SphereRadius);
    UE_LOG(LogTemp, Warning, TEXT("Points Per Axis: %d"), PointsPerAxis);
    
    if (PointsPerAxis < 2)
    {
        UE_LOG(LogTemp, Error, TEXT("PointsPerAxis must be at least 2 for sphere generation"));
        PointsPerAxis = 2;
    }
    
    int32 NumLayers = PointsPerAxis;
    for (int32 Layer = 0; Layer < NumLayers; Layer++)
    {
        float NormalizedHeight = 2.0f * Layer / FMath::Max(1, NumLayers - 1) - 1.0f;
        float LayerHeight = SphereRadius * NormalizedHeight;
        
        float LayerRadius = FMath::Sqrt(FMath::Max(0.0f, SphereRadius * SphereRadius - LayerHeight * LayerHeight));
        
        int32 PointsInLayer = FMath::Max(1, FMath::CeilToInt(PointsPerAxis * (LayerRadius / SphereRadius)));
        
        for (int32 i = 0; i < PointsInLayer; i++)
        {
            float Angle = 2.0f * PI * i / FMath::Max(1, PointsInLayer);
            FVector LocalPosition = FVector(
                LayerRadius * FMath::Cos(Angle),
                LayerRadius * FMath::Sin(Angle),
                LayerHeight
            );
            
            BuoyancyPoints.Add(LocalPosition);
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("✅ Generated %d sphere buoyancy points"), BuoyancyPoints.Num());
    
    for (int32 i = 0; i < FMath::Min(5, BuoyancyPoints.Num()); i++)
    {
        UE_LOG(LogTemp, Warning, TEXT("   Point %d: %s"), i, *BuoyancyPoints[i].ToString());
    }
}

void UWaterPhysicsComponent::GenerateStaticMeshBuoyancyPoints()
{
    UE_LOG(LogTemp, Warning, TEXT("=== GENERATING STATIC MESH BUOYANCY POINTS ==="));
    
    if (!StaticMeshComponent->GetStaticMesh())
    {
        UE_LOG(LogTemp, Error, TEXT("StaticMeshComponent has no mesh assigned"));
        return;
    }
    
    FBoxSphereBounds MeshBounds = StaticMeshComponent->GetStaticMesh()->GetBounds();
    FVector MeshExtent = MeshBounds.BoxExtent;
    
    UE_LOG(LogTemp, Warning, TEXT("Mesh Bounds: (%.1f, %.1f, %.1f)"), MeshExtent.X, MeshExtent.Y, MeshExtent.Z);
    UE_LOG(LogTemp, Warning, TEXT("Testing %d^3 = %d potential points"), PointsPerAxis, PointsPerAxis * PointsPerAxis * PointsPerAxis);
    
    int32 PointsAdded = 0;
    int32 PointsRejected = 0;
    
    for (int32 X = 0; X < PointsPerAxis; X++)
    {
        for (int32 Y = 0; Y < PointsPerAxis; Y++)
        {
            for (int32 Z = 0; Z < PointsPerAxis; Z++)
            {
                FVector LocalPosition = FVector(
                    MeshExtent.X * (2.0f * X / FMath::Max(1, PointsPerAxis - 1) - 1.0f),
                    MeshExtent.Y * (2.0f * Y / FMath::Max(1, PointsPerAxis - 1) - 1.0f),
                    MeshExtent.Z * (2.0f * Z / FMath::Max(1, PointsPerAxis - 1) - 1.0f)
                );
                
                FVector WorldPosition = StaticMeshComponent->GetComponentTransform().TransformPosition(LocalPosition);
                
                // Check if this point is inside the mesh's collision
                FHitResult HitResult;
                FCollisionQueryParams QueryParams;
                QueryParams.AddIgnoredActor(GetOwner());
                
                // Test if point is inside collision by doing a small sphere overlap at this location
                bool bIsInsideCollision = StaticMeshComponent->OverlapComponent(
                    WorldPosition,
                    FQuat::Identity,
                    FCollisionShape::MakeSphere(1.0f)
                );
                
                // Alternative method: Check if point is within collision bounds
                if (!bIsInsideCollision)
                {
                    FVector ClosestPoint;
                    float DistanceSquared;
                    StaticMeshComponent->GetSquaredDistanceToCollision(WorldPosition, DistanceSquared, ClosestPoint);
                    
                    // If distance is very small, point is likely inside or on surface
                    if (DistanceSquared < 100.0f)
                    {
                        bIsInsideCollision = true;
                    }
                }
                
                if (bIsInsideCollision)
                {
                    BuoyancyPoints.Add(LocalPosition);
                    PointsAdded++;
                }
                else
                {
                    PointsRejected++;
                }
            }
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("✅ Generated %d static mesh buoyancy points (rejected %d outside collision)"), PointsAdded, PointsRejected);
    
    if (bShowDetailedLogs && BuoyancyPoints.Num() > 0)
    {
        for (int32 i = 0; i < FMath::Min(5, BuoyancyPoints.Num()); i++)
        {
            UE_LOG(LogTemp, Warning, TEXT("   Point %d: %s"), i, *BuoyancyPoints[i].ToString());
        }
    }
}

void UWaterPhysicsComponent::ApplyBuoyancy(float DeltaTime)
{
    UPrimitiveComponent* PhysicsComp = nullptr;
    
    if (SphereComponent && SphereComponent->IsSimulatingPhysics())
    {
        PhysicsComp = SphereComponent;
        float SphereRadius = SphereComponent->GetUnscaledSphereRadius();
        float TotalSphereVolume = (4.0f/3.0f) * PI * FMath::Pow(SphereRadius, 3);
        float VolumePerPoint = TotalSphereVolume / FMath::Max(1, BuoyancyPoints.Num());
        
        for (const FVector& LocalPoint : BuoyancyPoints)
        {
            FVector WorldPoint = GetOwner()->GetTransform().TransformPosition(LocalPoint);
            float WaterHeight = GetWaterHeightAtLocation(WorldPoint);
            
            if (WorldPoint.Z < WaterHeight)
            {
                float SubmersionDepth = WaterHeight - WorldPoint.Z;
                float SubmersionRatio = FMath::Clamp(SubmersionDepth / (SphereRadius / PointsPerAxis), 0.0f, 1.0f);
                float SubmergedVolume = VolumePerPoint * SubmersionRatio;
                float BuoyancyForce = (SubmergedVolume / 1000.0f) * 9.8f * BuoyancyForceMultiplier * 50.0f;
                
                PhysicsComp->AddForceAtLocation(FVector(0, 0, BuoyancyForce), WorldPoint);
            }
        }
    }
    else if (BoxComponent && BoxComponent->IsSimulatingPhysics())
    {
        PhysicsComp = BoxComponent;
        FVector BoxExtent = BoxComponent->GetUnscaledBoxExtent();
        float PointSizeX = (BoxExtent.X * 2.0f) / PointsPerAxis;
        float PointSizeY = (BoxExtent.Y * 2.0f) / PointsPerAxis;
        float PointSizeZ = (BoxExtent.Z * 2.0f) / PointsPerAxis;
        
        for (const FVector& LocalPoint : BuoyancyPoints)
        {
            FVector WorldPoint = GetOwner()->GetTransform().TransformPosition(LocalPoint);
            float WaterHeight = GetWaterHeightAtLocation(WorldPoint);
            
            if (WorldPoint.Z < WaterHeight)
            {
                float SubmersionDepth = WaterHeight - WorldPoint.Z;
                float SubmergedHeight = FMath::Min(SubmersionDepth, PointSizeZ);
                float SubmergedVolume = PointSizeX * PointSizeY * SubmergedHeight;
                float BuoyancyForce = (SubmergedVolume / 1000.0f) * 9.8f * BuoyancyForceMultiplier * 50.0f;
                
                PhysicsComp->AddForceAtLocation(FVector(0, 0, BuoyancyForce), WorldPoint);
            }
        }
    }
     else if (StaticMeshComponent && StaticMeshComponent->IsSimulatingPhysics())
    {
        PhysicsComp = StaticMeshComponent;
        FBoxSphereBounds MeshBounds = StaticMeshComponent->GetStaticMesh()->GetBounds();
        FVector MeshExtent = MeshBounds.BoxExtent;
        
        float PointSizeX = (MeshExtent.X * 2.0f) / PointsPerAxis;
        float PointSizeY = (MeshExtent.Y * 2.0f) / PointsPerAxis;
        float PointSizeZ = (MeshExtent.Z * 2.0f) / PointsPerAxis;
        
        int32 UnderwaterPoints = 0;
        float TotalForceApplied = 0.0f;
        
        for (int32 i = 0; i < BuoyancyPoints.Num(); i++)
        {
            const FVector& LocalPoint = BuoyancyPoints[i];
            FVector WorldPoint = GetOwner()->GetTransform().TransformPosition(LocalPoint);
            float WaterHeight = GetWaterHeightAtLocation(WorldPoint);
            
            if (bShowDetailedLogs && i < 3)
            {
                UE_LOG(LogTemp, Log, TEXT("Mesh Point %d: World(%.1f,%.1f,%.1f) Water(%.1f)"), 
                       i, WorldPoint.X, WorldPoint.Y, WorldPoint.Z, WaterHeight);
            }
            
            if (WorldPoint.Z < WaterHeight)
            {
                UnderwaterPoints++;
                float SubmersionDepth = WaterHeight - WorldPoint.Z;
                float SubmergedHeight = FMath::Min(SubmersionDepth, PointSizeZ);
                float SubmergedVolume = PointSizeX * PointSizeY * SubmergedHeight;
                float BuoyancyForce = (SubmergedVolume / 1000.0f) * 9.8f * BuoyancyForceMultiplier * 50.0f;
                TotalForceApplied += BuoyancyForce;
                
                if (bShowDetailedLogs && i < 3)
                {
                    UE_LOG(LogTemp, Warning, TEXT("   Depth: %.1f, Volume: %.1f, Force: %.1f"), 
                           SubmersionDepth, SubmergedVolume, BuoyancyForce);
                }
                
                PhysicsComp->AddForceAtLocation(FVector(0, 0, BuoyancyForce), WorldPoint);
            }
        }
        
        if (UnderwaterPoints > 0 && bShowDetailedLogs)
        {
            float MeshMass = StaticMeshComponent->GetMass();
            float WeightForce = MeshMass * 980.0f;
            UE_LOG(LogTemp, Error, TEXT("MESH SUMMARY: Force: %.1f N | Weight: %.1f N | Ratio: %.2f | Underwater: %d/%d"), 
                   TotalForceApplied, WeightForce, TotalForceApplied / WeightForce, UnderwaterPoints, BuoyancyPoints.Num());
        }
    }
    
    if (PhysicsComp)
    {
        ApplyDampingForces(DeltaTime);
    }
}

void UWaterPhysicsComponent::ApplyDampingForces(float DeltaTime)
{
    UPrimitiveComponent* PhysicsComp = nullptr;
    
    if (StaticMeshComponent && StaticMeshComponent->IsSimulatingPhysics())
    {
        PhysicsComp = StaticMeshComponent;
    }
    else if (SphereComponent && SphereComponent->IsSimulatingPhysics())
    {
        PhysicsComp = SphereComponent;
    }
    else if (BoxComponent && BoxComponent->IsSimulatingPhysics())
    {
        PhysicsComp = BoxComponent;
    }
    
    if (!PhysicsComp) return;
    
    FVector Velocity = PhysicsComp->GetPhysicsLinearVelocity();
    FVector LinearDampingForce = -Velocity * LinearDamping * PhysicsComp->GetMass();
    PhysicsComp->AddForce(LinearDampingForce);
    
    FVector AngularVelocity = PhysicsComp->GetPhysicsAngularVelocityInRadians();
    float AngularSpeed = AngularVelocity.Size();
    float DampingScale = 1.0f + (AngularSpeed * 2.0f);
    FVector AngularDampingTorque = -AngularVelocity * AngularDamping * DampingScale;
    PhysicsComp->AddTorqueInRadians(AngularDampingTorque);
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
            
            float BaseWaterHeight = WaterSurfaceLocation.Z;
            
            if (UWaterWavesBase* WaterWaves = WaterBody->GetWaterWaves())
            {
                float CurrentTime = GetWorld()->GetTimeSeconds();
                float WaveDisplacement = WaterWaves->GetSimpleWaveHeightAtPosition(
                    WorldLocation,
                    WaterDepth,
                    CurrentTime
                );
                
                return BaseWaterHeight + WaveDisplacement;
            }
            
            return BaseWaterHeight;
        }
    }
    
    return -99999.0f;
}

void UWaterPhysicsComponent::DrawDebugInfo()
{
    if (!GetWorld()) return;
    
    UPrimitiveComponent* PhysicsComp = nullptr;
    FVector ComponentCenter;
    FString ComponentType = TEXT("Unknown");
    
    if (StaticMeshComponent && StaticMeshComponent->IsSimulatingPhysics())
    {
        PhysicsComp = StaticMeshComponent;
        ComponentCenter = StaticMeshComponent->GetComponentLocation();
        ComponentType = TEXT("STATIC MESH");
        
        FBoxSphereBounds Bounds = StaticMeshComponent->GetStaticMesh()->GetBounds();
        FVector MeshExtent = Bounds.BoxExtent;
        FQuat MeshRotation = StaticMeshComponent->GetComponentQuat();
        DrawDebugBox(GetWorld(), ComponentCenter, MeshExtent, MeshRotation, FColor::Magenta, false, -1.0f, 0, 3.0f);
        
        if (bShowDetailedLogs)
        {
            DrawDebugString(GetWorld(), ComponentCenter + FVector(0, 0, MeshExtent.Z + 20), 
                           FString::Printf(TEXT("MESH: %s"), *StaticMeshComponent->GetStaticMesh()->GetName()), 
                           nullptr, FColor::Magenta, -1.0f, true);
        }
    }
    else if (SphereComponent && SphereComponent->IsSimulatingPhysics())
    {
        PhysicsComp = SphereComponent;
        ComponentCenter = SphereComponent->GetComponentLocation();
        ComponentType = TEXT("SPHERE");
        
        float SphereRadius = SphereComponent->GetUnscaledSphereRadius();
        DrawDebugSphere(GetWorld(), ComponentCenter, SphereRadius, 16, FColor::Yellow, false, -1.0f, 0, 2.0f);
    }
    else if (BoxComponent && BoxComponent->IsSimulatingPhysics())
    {
        PhysicsComp = BoxComponent;
        ComponentCenter = BoxComponent->GetComponentLocation();
        ComponentType = TEXT("BOX");
        
        FVector BoxExtent = BoxComponent->GetUnscaledBoxExtent();
        FQuat BoxRotation = BoxComponent->GetComponentQuat();
        DrawDebugBox(GetWorld(), ComponentCenter, BoxExtent, BoxRotation, FColor::Yellow, false, -1.0f, 0, 3.0f);
    }
    
    if (!PhysicsComp) return;
    
    int32 UnderwaterCount = 0;
    
    for (int32 i = 0; i < BuoyancyPoints.Num(); i++)
    {
        const FVector& LocalPoint = BuoyancyPoints[i];
        FVector WorldPoint = GetOwner()->GetTransform().TransformPosition(LocalPoint);
        float WaterHeight = GetWaterHeightAtLocation(WorldPoint);
        
        FColor PointColor = (WorldPoint.Z < WaterHeight) ? FColor::Green : FColor::Orange;
        DrawDebugSphere(GetWorld(), WorldPoint, 8.0f, 8, PointColor, false, -1.0f, 0, 2.0f);
        
        if (WorldPoint.Z < WaterHeight)
        {
            UnderwaterCount++;
            FVector WaterSurface = FVector(WorldPoint.X, WorldPoint.Y, WaterHeight);
            DrawDebugLine(GetWorld(), WorldPoint, WaterSurface, FColor::Cyan, false, -1.0f, 0, 1.0f);
        }
    }
    
    DrawDebugString(GetWorld(), ComponentCenter + FVector(0, 0, 100), 
                   FString::Printf(TEXT("%s: %d points (%d underwater)"), *ComponentType, BuoyancyPoints.Num(), UnderwaterCount), 
                   nullptr, FColor::Green, -1.0f, true);
}