#include "WaterPhysicsComponent.h"
#include "Components/SphereComponent.h"
#include "DrawDebugHelpers.h"
#include "EngineUtils.h"
#include "PhysicsEngine/BodySetup.h"

UWaterPhysicsComponent::UWaterPhysicsComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UWaterPhysicsComponent::BeginPlay()
{
    Super::BeginPlay();
    
    UE_LOG(LogTemp, Warning, TEXT("WATER PHYSICS COMPONENT BEGIN PLAY!!!"));
    
    UBoxComponent* BoxComponent = GetOwner()->FindComponentByClass<UBoxComponent>();
    
    if (BoxComponent)
    {
        bIsBox = true;
        
        UE_LOG(LogTemp, Warning, TEXT(" Found BoxComponent: %s"), *BoxComponent->GetName());
        
        if (!BoxComponent->IsSimulatingPhysics())
        {
            UE_LOG(LogTemp, Error, TEXT(" Enable 'Simulate Physics' on BoxComponent"));
            BoxComponent->SetSimulatePhysics(true);
        }
        
        UE_LOG(LogTemp, Warning, TEXT(" Box Collision Buoyancy Mode"));
        UE_LOG(LogTemp, Warning, TEXT("   Mass: %.2f kg"), BoxComponent->GetMass());
        
        FVector BoxExtent = BoxComponent->GetUnscaledBoxExtent();
        UE_LOG(LogTemp, Warning, TEXT("   Box Extents: (%.1f, %.1f, %.1f)"), BoxExtent.X, BoxExtent.Y, BoxExtent.Z);

        PhysicsComp = BoxComponent;
        
        GenerateBuoyancyPoints();
        return;
    }
    
    USphereComponent* SphereComponent = GetOwner()->FindComponentByClass<USphereComponent>();
    
    if (SphereComponent)
    {
        bIsSphere = true;
        UE_LOG(LogTemp, Warning, TEXT(" Found SphereComponent: %s"), *SphereComponent->GetName());
        
        if (!SphereComponent->IsSimulatingPhysics())
        {
            UE_LOG(LogTemp, Error, TEXT(" Enable 'Simulate Physics' on SphereComponent"));
            SphereComponent->SetSimulatePhysics(true);
        }
        
        UE_LOG(LogTemp, Warning, TEXT(" Sphere Collision Buoyancy Mode"));
        UE_LOG(LogTemp, Warning, TEXT("   Mass: %.2f kg"), SphereComponent->GetMass());
        UE_LOG(LogTemp, Warning, TEXT("   Radius: %.1f"), SphereComponent->GetUnscaledSphereRadius());

        PhysicsComp = SphereComponent;
        
        GenerateBuoyancyPoints();
        return;
    }

    UCapsuleComponent* CapsuleComponent = GetOwner()->FindComponentByClass<UCapsuleComponent>();
    
    if (CapsuleComponent)
    {
        bIsCapsule = true;
        UE_LOG(LogTemp, Warning, TEXT(" Found CapsuleComponent: %s"), *CapsuleComponent->GetName());
        
        if (!CapsuleComponent->IsSimulatingPhysics())
        {
            UE_LOG(LogTemp, Error, TEXT(" Enable 'Simulate Physics' on CapsuleComponent"));
            CapsuleComponent->SetSimulatePhysics(true);
        }
        
        UE_LOG(LogTemp, Warning, TEXT(" Capsule Collision Buoyancy Mode"));
        UE_LOG(LogTemp, Warning, TEXT("   Mass: %.2f kg"), CapsuleComponent->GetMass());
        UE_LOG(LogTemp, Warning, TEXT("   Radius: %.1f"), CapsuleComponent->GetUnscaledCapsuleRadius());
        UE_LOG(LogTemp, Warning, TEXT("   Half Height: %.1f"), CapsuleComponent->GetUnscaledCapsuleHalfHeight());

        PhysicsComp = CapsuleComponent;
        
        GenerateBuoyancyPoints();
        return;
    }

    UStaticMeshComponent* StaticMeshComponent = GetOwner()->FindComponentByClass<UStaticMeshComponent>();
    
    if (StaticMeshComponent)
    {
        bIsStaticMesh = true;
        
        UE_LOG(LogTemp, Warning, TEXT(" Found StaticMeshComponent: %s"), *StaticMeshComponent->GetName());

        if (!StaticMeshComponent->IsSimulatingPhysics())
        {
            UE_LOG(LogTemp, Error, TEXT(" Enable 'Simulate Physics' on StaticMeshComponent"));
            StaticMeshComponent->SetSimulatePhysics(true);
        }
        
        UE_LOG(LogTemp, Warning, TEXT(" Static Mesh Mode"));
        UE_LOG(LogTemp, Warning, TEXT("   Mass: %.2f kg"), StaticMeshComponent->GetMass());

        if (StaticMeshComponent->GetStaticMesh())
        {
            UE_LOG(LogTemp, Warning, TEXT("   Mesh: %s"), *StaticMeshComponent->GetStaticMesh()->GetName());
            FBoxSphereBounds Bounds = StaticMeshComponent->GetStaticMesh()->GetBounds();
            UE_LOG(LogTemp, Warning, TEXT("   Bounds: (%.1f, %.1f, %.1f)"), Bounds.BoxExtent.X, Bounds.BoxExtent.Y, Bounds.BoxExtent.Z);
        }

        PhysicsComp = StaticMeshComponent;
        
        GenerateBuoyancyPoints();
        return;
    }
    
    PhysicsComp->SetAngularDamping(AngularDamping);
    PhysicsComp->SetLinearDamping(LinearDamping);
    
    UE_LOG(LogTemp, Error, TEXT(" No BoxComponent, SphereComponent, StaticMeshComponent or CapsuleComponent found"));
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
    
    if (!PhysicsComp)
    {
        UE_LOG(LogTemp, Error, TEXT(" No physics component has been found"));
        return;
    }
    
    if (!PhysicsComp->IsSimulatingPhysics())
    {
        UE_LOG(LogTemp, Error, TEXT(" Physics not simulating on %s"), *PhysicsComp->GetName());
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
    else if (bIsBox)
    {
        GenerateBoxBuoyancyPoints();
    }
    else if (bIsCapsule)
    {
        GenerateCapsuleBuoyancyPoints();
    }
    else
    {
        GenerateStaticMeshBuoyancyPoints();
    }
}

void UWaterPhysicsComponent::GenerateBoxBuoyancyPoints()
{
    UE_LOG(LogTemp, Warning, TEXT("GENERATING BOX BUOYANCY POINTS!!!"));
    BuoyancyPoints.Empty();
    UBoxComponent* BoxComponent = GetOwner()->FindComponentByClass<UBoxComponent>();
    
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
    
    UE_LOG(LogTemp, Warning, TEXT(" Generated %d buoyancy points"), BuoyancyPoints.Num());
    
    for (int32 i = 0; i < FMath::Min(5, BuoyancyPoints.Num()); i++)
    {
        UE_LOG(LogTemp, Warning, TEXT("   Point %d: %s"), i, *BuoyancyPoints[i].ToString());
    }
}

void UWaterPhysicsComponent::GenerateSphereBuoyancyPoints()
{
    UE_LOG(LogTemp, Warning, TEXT("GENERATING SPHERE BUOYANCY POINTS!!!"));
    USphereComponent* SphereComponent = GetOwner()->FindComponentByClass<USphereComponent>();
    
    float SphereRadius = SphereComponent->GetUnscaledSphereRadius();
    int32 ShellNum = PointsPerAxis;
    
    for (int32 Shell = 0; Shell < ShellNum; Shell++)
    {
        float ShellRadius = SphereRadius * (Shell + 1.0f) / ShellNum;
        int32 PointsInShell = FMath::Max(8, PointsPerAxis * PointsPerAxis * (Shell + 1) / ShellNum);
        
        float GoldenRatio = (1.0f + FMath::Sqrt(5.0f)) / 2.0f;
        
        for (int32 i = 0; i < PointsInShell; i++)
        {
            float Theta = 2.0f * PI * i / GoldenRatio;
            float Phi = FMath::Acos(1.0f - 2.0f * (i + 0.5f) / PointsInShell);
            
            FVector LocalPosition = FVector(
                ShellRadius * FMath::Sin(Phi) * FMath::Cos(Theta),
                ShellRadius * FMath::Sin(Phi) * FMath::Sin(Theta),
                ShellRadius * FMath::Cos(Phi)
            );
            
            BuoyancyPoints.Add(LocalPosition);
        }
    }
    
    BuoyancyPoints.Add(FVector::ZeroVector);
    
    UE_LOG(LogTemp, Warning, TEXT(" Generated %d sphere points"), BuoyancyPoints.Num());
}

void UWaterPhysicsComponent::GenerateStaticMeshBuoyancyPoints()
{
    UE_LOG(LogTemp, Warning, TEXT("GENERATING STATIC MESH BUOYANCY POINTS"));
    UStaticMeshComponent* StaticMeshComponent = GetOwner()->FindComponentByClass<UStaticMeshComponent>();
    
    if (!StaticMeshComponent->GetStaticMesh())
    {
        UE_LOG(LogTemp, Error, TEXT("No mesh assigned"));
        return;
    }
    
    UBodySetup* BodySetup = StaticMeshComponent->GetStaticMesh()->GetBodySetup();
    if (!BodySetup)
    {
        UE_LOG(LogTemp, Error, TEXT("Mesh has no collision setup"));
        return;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("Mesh: %s"), *StaticMeshComponent->GetStaticMesh()->GetName());
    UE_LOG(LogTemp, Warning, TEXT("Extracting points from collision geometry"));
    
    int32 PointsAdded = 0;
    
    for (const FKConvexElem& ConvexElem : BodySetup->AggGeom.ConvexElems)
    {
        UE_LOG(LogTemp, Warning, TEXT("  Convex hull with %d vertices"), ConvexElem.VertexData.Num());
        
        for (const FVector& Vertex : ConvexElem.VertexData)
        {
            BuoyancyPoints.Add(Vertex);
            PointsAdded++;
        }
        
        FVector HullCenter = FVector::ZeroVector;
        for (const FVector& Vertex : ConvexElem.VertexData)
        {
            HullCenter += Vertex;
        }
        if (ConvexElem.VertexData.Num() > 0)
        {
            HullCenter /= ConvexElem.VertexData.Num();
            BuoyancyPoints.Add(HullCenter);
            PointsAdded++;
        }
    }
    
    for (const FKBoxElem& BoxElem : BodySetup->AggGeom.BoxElems)
    {
        UE_LOG(LogTemp, Warning, TEXT("  Box collision found"));
        
        FVector BoxExtent = FVector(BoxElem.X, BoxElem.Y, BoxElem.Z) * 0.5f;
        FVector BoxCenter = BoxElem.Center;
        
        for (int32 X = 0; X < 3; X++)
        {
            for (int32 Y = 0; Y < 3; Y++)
            {
                for (int32 Z = 0; Z < 3; Z++)
                {
                    FVector LocalPos = BoxCenter + FVector(
                        BoxExtent.X * (X - 1),
                        BoxExtent.Y * (Y - 1),
                        BoxExtent.Z * (Z - 1)
                    );
                    BuoyancyPoints.Add(LocalPos);
                    PointsAdded++;
                }
            }
        }
    }
    
    for (const FKSphereElem& SphereElem : BodySetup->AggGeom.SphereElems)
    {
        UE_LOG(LogTemp, Warning, TEXT("  Sphere collision found"));
        
        int32 NumLayers = 3;
        for (int32 Layer = 0; Layer < NumLayers; Layer++)
        {
            float NormalizedHeight = 2.0f * Layer / FMath::Max(1, NumLayers - 1) - 1.0f;
            float LayerHeight = SphereElem.Radius * NormalizedHeight;
            float LayerRadius = FMath::Sqrt(FMath::Max(0.0f, SphereElem.Radius * SphereElem.Radius - LayerHeight * LayerHeight));
            int32 PointsInLayer = FMath::Max(6, FMath::CeilToInt(12 * (LayerRadius / SphereElem.Radius)));
            
            for (int32 i = 0; i < PointsInLayer; i++)
            {
                float Angle = 2.0f * PI * i / PointsInLayer;
                FVector LocalPos = SphereElem.Center + FVector(
                    LayerRadius * FMath::Cos(Angle),
                    LayerRadius * FMath::Sin(Angle),
                    LayerHeight
                );
                BuoyancyPoints.Add(LocalPos);
                PointsAdded++;
            }
        }
    }
    
    for (const FKSphylElem& CapsuleElem : BodySetup->AggGeom.SphylElems)
    {
        UE_LOG(LogTemp, Warning, TEXT("  Capsule collision found"));
        
        float HalfHeight = CapsuleElem.Length * 0.5f;
        float Radius = CapsuleElem.Radius;
        
        int32 HeightSegments = 5;
        int32 RadialSegments = 8;
        
        for (int32 H = 0; H < HeightSegments; H++)
        {
            float Height = -HalfHeight + (2.0f * HalfHeight * H / FMath::Max(1, HeightSegments - 1));
            
            for (int32 R = 0; R < RadialSegments; R++)
            {
                float Angle = 2.0f * PI * R / RadialSegments;
                FVector LocalPos = CapsuleElem.Center + FVector(
                    Radius * FMath::Cos(Angle),
                    Radius * FMath::Sin(Angle),
                    Height
                );
                BuoyancyPoints.Add(LocalPos);
                PointsAdded++;
            }
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT(" Generated %d buoyancy points from collision geometry"), PointsAdded);
    
    if (bShowDetailedLogs && BuoyancyPoints.Num() > 0)
    {
        for (int32 i = 0; i < FMath::Min(5, BuoyancyPoints.Num()); i++)
        {
            UE_LOG(LogTemp, Warning, TEXT("   Point %d: %s"), i, *BuoyancyPoints[i].ToString());
        }
    }
}

void UWaterPhysicsComponent::GenerateCapsuleBuoyancyPoints()
{
    UE_LOG(LogTemp, Warning, TEXT("GENERATING CAPSULE BUOYANCY POINTS!!!"));
    UCapsuleComponent* CapsuleComponent = GetOwner()->FindComponentByClass<UCapsuleComponent>();
    
    float CapsuleRadius = CapsuleComponent->GetUnscaledCapsuleRadius();
    float CapsuleHalfHeight = CapsuleComponent->GetUnscaledCapsuleHalfHeight();
    float CylinderHeight = CapsuleHalfHeight - CapsuleRadius;
    
    UE_LOG(LogTemp, Warning, TEXT("Capsule: Radius=%.1f, HalfHeight=%.1f"), CapsuleRadius, CapsuleHalfHeight);
    
    int32 HeightSegments = PointsPerAxis * 2;
    int32 RadialSegments = PointsPerAxis * 3;
    int32 RadialLayers = PointsPerAxis;
    
    for (int32 H = 0; H < HeightSegments; H++)
    {
        float Height = -CylinderHeight + (2.0f * CylinderHeight * H / FMath::Max(1, HeightSegments - 1));
        
        for (int32 Layer = 0; Layer < RadialLayers; Layer++)
        {
            float LayerRadius = CapsuleRadius * (Layer + 1.0f) / RadialLayers;
            int32 PointsInRing = FMath::Max(6, RadialSegments * (Layer + 1) / RadialLayers);
            
            for (int32 R = 0; R < PointsInRing; R++)
            {
                float Angle = 2.0f * PI * R / PointsInRing;
                FVector LocalPos = FVector(
                    LayerRadius * FMath::Cos(Angle),
                    LayerRadius * FMath::Sin(Angle),
                    Height
                );
                BuoyancyPoints.Add(LocalPos);
            }
        }
    }
    
    int32 HemispherePoints = PointsPerAxis * PointsPerAxis;
    for (int32 i = 0; i < HemispherePoints; i++)
    {
        float Theta = 2.0f * PI * i / HemispherePoints;
        float Phi = (PI / 2.0f) * i / HemispherePoints;
        
        FVector LocalPos = FVector(
            CapsuleRadius * FMath::Sin(Phi) * FMath::Cos(Theta),
            CapsuleRadius * FMath::Sin(Phi) * FMath::Sin(Theta),
            CylinderHeight + CapsuleRadius * FMath::Cos(Phi)
        );
        BuoyancyPoints.Add(LocalPos);
    }
    
    for (int32 i = 0; i < HemispherePoints; i++)
    {
        float Theta = 2.0f * PI * i / HemispherePoints;
        float Phi = (PI / 2.0f) * i / HemispherePoints;
        
        FVector LocalPos = FVector(
            CapsuleRadius * FMath::Sin(Phi) * FMath::Cos(Theta),
            CapsuleRadius * FMath::Sin(Phi) * FMath::Sin(Theta),
            -CylinderHeight - CapsuleRadius * FMath::Cos(Phi)
        );
        BuoyancyPoints.Add(LocalPos);
    }
    
    BuoyancyPoints.Add(FVector::ZeroVector);
    
    UE_LOG(LogTemp, Warning, TEXT(" Generated %d capsule points"), BuoyancyPoints.Num());
}

void UWaterPhysicsComponent::ApplyBuoyancy(float DeltaTime)
{
    int32 UnderwaterPoints = 0;
    float TotalForceApplied = 0.0f;
    USphereComponent* SphereComponent = GetOwner()->FindComponentByClass<USphereComponent>();
    
    if (bIsSphere)
    {
        float SphereRadius = SphereComponent->GetUnscaledSphereRadius();
        float TotalSphereVolume = (4.0f/3.0f) * PI * FMath::Pow(SphereRadius, 3);
        float VolumePerPoint = TotalSphereVolume / FMath::Max(1, BuoyancyPoints.Num());
        
        for (const FVector& LocalPoint : BuoyancyPoints)
        {
            FVector WorldPoint = GetOwner()->GetTransform().TransformPosition(LocalPoint);
            float WaterHeight = GetWaterHeightAtLocation(WorldPoint);
            
            if (WorldPoint.Z < WaterHeight)
            {
                UnderwaterPoints++;
                float SubmersionDepth = WaterHeight - WorldPoint.Z;
                float SubmersionRatio = FMath::Clamp(SubmersionDepth / (SphereRadius / PointsPerAxis), 0.0f, 1.0f);
                float SubmergedVolume = VolumePerPoint * SubmersionRatio;
                float BuoyancyForce = (SubmergedVolume / 1000.0f) * 9.8f * BuoyancyForceMultiplier * WaterDensity;
                TotalForceApplied += BuoyancyForce;
                
                PhysicsComp->AddForceAtLocation(FVector(0, 0, BuoyancyForce), WorldPoint);
            }
        }
    }
    else if (bIsBox)
    {
        UBoxComponent* BoxComponent = GetOwner()->FindComponentByClass<UBoxComponent>();
        
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
                UnderwaterPoints++;
                float SubmersionDepth = WaterHeight - WorldPoint.Z;
                float SubmergedHeight = FMath::Min(SubmersionDepth, PointSizeZ);
                float SubmergedVolume = PointSizeX * PointSizeY * SubmergedHeight;
                float BuoyancyForce = (SubmergedVolume / 1000.0f) * 9.8f * BuoyancyForceMultiplier * WaterDensity;
                TotalForceApplied += BuoyancyForce;
                
                PhysicsComp->AddForceAtLocation(FVector(0, 0, BuoyancyForce), WorldPoint);
            }
        }
    }
    else if (bIsStaticMesh)
    {
        UStaticMeshComponent* StaticMeshComponent = GetOwner()->FindComponentByClass<UStaticMeshComponent>();
        FBoxSphereBounds MeshBounds = StaticMeshComponent->GetStaticMesh()->GetBounds();
        FVector MeshExtent = MeshBounds.BoxExtent;
        
        float PointSizeX = (MeshExtent.X * 2.0f) / PointsPerAxis;
        float PointSizeY = (MeshExtent.Y * 2.0f) / PointsPerAxis;
        float PointSizeZ = (MeshExtent.Z * 2.0f) / PointsPerAxis;
        
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
                float BuoyancyForce = (SubmergedVolume / 1000.0f) * 9.8f * BuoyancyForceMultiplier * WaterDensity;
                TotalForceApplied += BuoyancyForce;
                
                if (bShowDetailedLogs && i < 3)
                {
                    UE_LOG(LogTemp, Warning, TEXT("   Depth: %.1f, Volume: %.1f, Force: %.1f"), 
                           SubmersionDepth, SubmergedVolume, BuoyancyForce);
                }
                
                PhysicsComp->AddForceAtLocation(FVector(0, 0, BuoyancyForce), WorldPoint);
            }
        }
        
    }
    else if (bIsCapsule)
    {
        UCapsuleComponent* CapsuleComponent = GetOwner()->FindComponentByClass<UCapsuleComponent>();
        float CapsuleRadius = CapsuleComponent->GetUnscaledCapsuleRadius();
        float CapsuleHalfHeight = CapsuleComponent->GetUnscaledCapsuleHalfHeight();
        
        // Capsule volume = cylinder + 2 hemispheres = π(r²)h + (4/3)π(r³)
        float CylinderHeight = (CapsuleHalfHeight - CapsuleRadius) * 2.0f;
        float TotalVolume = PI * CapsuleRadius * CapsuleRadius * CylinderHeight + (4.0f/3.0f) * PI * FMath::Pow(CapsuleRadius, 3);
        float VolumePerPoint = TotalVolume / FMath::Max(1, BuoyancyPoints.Num());
        
        for (const FVector& LocalPoint : BuoyancyPoints)
        {
            FVector WorldPoint = GetOwner()->GetTransform().TransformPosition(LocalPoint);
            float WaterHeight = GetWaterHeightAtLocation(WorldPoint);
            
            if (WorldPoint.Z < WaterHeight)
            {
                UnderwaterPoints++;
                float SubmersionDepth = WaterHeight - WorldPoint.Z;
                float SubmersionRatio = FMath::Clamp(SubmersionDepth / (CapsuleRadius * 2.0f), 0.0f, 1.0f);
                float SubmergedVolume = VolumePerPoint * SubmersionRatio;
                float BuoyancyForce = (SubmergedVolume / 1000.0f) * 9.8f * BuoyancyForceMultiplier * WaterDensity;
                TotalForceApplied += BuoyancyForce;
                
                PhysicsComp->AddForceAtLocation(FVector(0, 0, BuoyancyForce), WorldPoint);
            }
        }
    }

    if (UnderwaterPoints > 0 && bShowDetailedLogs)
    {
        float MeshMass = PhysicsComp->GetMass();
        float WeightForce = MeshMass * 98.0f;
        UE_LOG(LogTemp, Error, TEXT("Bouyancy report from the %s: Force: %.1f N | Weight: %.1f N | Ratio: %.2f | Underwater: %d/%d"), 
               *PhysicsComp->GetName(), TotalForceApplied, WeightForce, TotalForceApplied / WeightForce, UnderwaterPoints, BuoyancyPoints.Num());
    }
    
    if (PhysicsComp)
    {
        ApplyDampingForces(DeltaTime);
    }
}

void UWaterPhysicsComponent::ApplyDampingForces(float DeltaTime) const
{
    if (!PhysicsComp) return;
    
    FVector Velocity = PhysicsComp->GetPhysicsLinearVelocity();
    FVector LinearDampingForce = -Velocity * LinearDamping * PhysicsComp->GetMass();
    PhysicsComp->AddForce(LinearDampingForce);
    
    FVector AngularVelocity = PhysicsComp->GetPhysicsAngularVelocityInRadians();
    FVector AngularDampingTorque = -AngularVelocity * AngularDamping;
    PhysicsComp->AddTorqueInRadians(AngularDampingTorque);
}

float UWaterPhysicsComponent::GetWaterHeightAtLocation(const FVector& WorldLocation) const
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
    
    FVector ComponentCenter;
    UStaticMeshComponent* StaticMeshComponent = GetOwner()->FindComponentByClass<UStaticMeshComponent>();
    
    if (bIsStaticMesh)
    {
        ComponentCenter = StaticMeshComponent->GetComponentLocation();
        
        if (UBodySetup* BodySetup = StaticMeshComponent->GetStaticMesh()->GetBodySetup())
        {
            FTransform ComponentTransform = StaticMeshComponent->GetComponentTransform();
            
            for (const FKConvexElem& ConvexElem : BodySetup->AggGeom.ConvexElems)
            {
                for (int32 i = 0; i < ConvexElem.VertexData.Num(); i++)
                {
                    FVector V1 = ComponentTransform.TransformPosition(ConvexElem.VertexData[i]);
                    
                    for (int32 j = i + 1; j < ConvexElem.VertexData.Num(); j++)
                    {
                        FVector V2 = ComponentTransform.TransformPosition(ConvexElem.VertexData[j]);
                        
                        float Distance = FVector::Dist(V1, V2);
                        float MaxEdgeLength = 200.0f;
                        
                        if (Distance < MaxEdgeLength)
                        {
                            DrawDebugLine(GetWorld(), V1, V2, FColor::Green, false, -1.0f, 0, 2.0f);
                        }
                    }
                }
            }
            
            for (const FKBoxElem& BoxElem : BodySetup->AggGeom.BoxElems)
            {
                FVector BoxCenter = ComponentTransform.TransformPosition(BoxElem.Center);
                FQuat BoxRot = ComponentTransform.GetRotation() * BoxElem.Rotation.Quaternion();
                FVector BoxExtent = FVector(BoxElem.X, BoxElem.Y, BoxElem.Z) * 0.5f;
                DrawDebugBox(GetWorld(), BoxCenter, BoxExtent, BoxRot, FColor::Green, false, -1.0f, 0, 2.0f);
            }
            
            for (const FKSphereElem& SphereElem : BodySetup->AggGeom.SphereElems)
            {
                FVector SphereCenter = ComponentTransform.TransformPosition(SphereElem.Center);
                DrawDebugSphere(GetWorld(), SphereCenter, SphereElem.Radius, 16, FColor::Green, false, -1.0f, 0, 2.0f);
            }
            
            for (const FKSphylElem& CapsuleElem : BodySetup->AggGeom.SphylElems)
            {
                FVector CapsuleCenter = ComponentTransform.TransformPosition(CapsuleElem.Center);
                FQuat CapsuleRot = ComponentTransform.GetRotation() * CapsuleElem.Rotation.Quaternion();
                DrawDebugCapsule(GetWorld(), CapsuleCenter, CapsuleElem.Length * 0.5f, CapsuleElem.Radius, 
                               CapsuleRot, FColor::Green, false, -1.0f, 0, 2.0f);
            }
        }
        
        for (const FVector& LocalPoint : BuoyancyPoints)
        {
            FVector WorldPoint = GetOwner()->GetTransform().TransformPosition(LocalPoint);
            float WaterHeight = GetWaterHeightAtLocation(WorldPoint);
            
            FColor PointColor = (WorldPoint.Z < WaterHeight) ? FColor::Red : FColor::Yellow;
            DrawDebugPoint(GetWorld(), WorldPoint, 8.0f, PointColor, false, -1.0f, 0);
        }
    }
    else if (bIsSphere)
    {
        USphereComponent* SphereComponent = GetOwner()->FindComponentByClass<USphereComponent>();
        ComponentCenter = SphereComponent->GetComponentLocation();
        
        float SphereRadius = SphereComponent->GetUnscaledSphereRadius();
        DrawDebugSphere(GetWorld(), ComponentCenter, SphereRadius, 16, FColor::Green, false, -1.0f, 0, 2.0f);
    }
    else if (bIsBox)
    {
        UBoxComponent* BoxComponent = GetOwner()->FindComponentByClass<UBoxComponent>();
        ComponentCenter = BoxComponent->GetComponentLocation();
        
        FVector BoxExtent = BoxComponent->GetUnscaledBoxExtent();
        FQuat BoxRotation = BoxComponent->GetComponentQuat();
        DrawDebugBox(GetWorld(), ComponentCenter, BoxExtent, BoxRotation, FColor::Green, false, -1.0f, 0, 3.0f);
    }
    else if (bIsCapsule)
    {
        UCapsuleComponent* CapsuleComponent = GetOwner()->FindComponentByClass<UCapsuleComponent>();
        ComponentCenter = CapsuleComponent->GetComponentLocation();
    
        float CapsuleRadius = CapsuleComponent->GetUnscaledCapsuleRadius();
        float CapsuleHalfHeight = CapsuleComponent->GetUnscaledCapsuleHalfHeight();
        FQuat CapsuleRotation = CapsuleComponent->GetComponentQuat();
        DrawDebugCapsule(GetWorld(), ComponentCenter, CapsuleHalfHeight, CapsuleRadius, 
                       CapsuleRotation, FColor::Green, false, -1.0f, 0, 2.0f);
    }

    for (const FVector& LocalPoint : BuoyancyPoints)
    {
        FVector WorldPoint = GetOwner()->GetTransform().TransformPosition(LocalPoint);
        float WaterHeight = GetWaterHeightAtLocation(WorldPoint);
            
        FColor PointColor = (WorldPoint.Z < WaterHeight) ? FColor::Red : FColor::Yellow;
        DrawDebugPoint(GetWorld(), WorldPoint, 8.0f, PointColor, false, -1.0f, 0);
    }
    
    if (!PhysicsComp) return;
    
    DrawDebugString(GetWorld(), ComponentCenter + FVector(0, 0, 150), 
                   FString::Printf(TEXT("%d collision vertices"), BuoyancyPoints.Num()), 
                   nullptr, FColor::White, -1.0f, true, 1.5f);
}