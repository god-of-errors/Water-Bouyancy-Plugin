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
    
    // Find physics component
    PhysicsComponent = GetOwner()->FindComponentByClass<UPrimitiveComponent>();
    
    if (!PhysicsComponent)
    {
        UE_LOG(LogTemp, Error, TEXT("❌ STEP 1: No PrimitiveComponent found on %s"), *GetOwner()->GetName());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("✅ STEP 1: Found PrimitiveComponent: %s"), *PhysicsComponent->GetName());
        UE_LOG(LogTemp, Warning, TEXT("   Physics Enabled: %s"), PhysicsComponent->IsSimulatingPhysics() ? TEXT("YES") : TEXT("NO"));
    }
    
    // ENHANCED DEBUG: Check water system status
    LogWaterSystemStatus();
}

void UWaterPhysicsComponent::TickComponent(float DeltaTime, ELevelTick TickType, 
                                          FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    
    // STEP 1: ENHANCED WATER DETECTION with comprehensive debug
    if (bShowDebug)
    {
        DrawDebugInfo();
        
        if (bShowWaterBodyInfo)
        {
            DrawWaterBodyBounds();
        }
    }
}

float UWaterPhysicsComponent::GetWaterHeightAtLocation(const FVector& WorldLocation)
{
    if (!GetWorld()) return -99999.0f;
    
    // Find the water body (should only be one)
    for (TActorIterator<AWaterBody> WaterBodyIterator(GetWorld()); WaterBodyIterator; ++WaterBodyIterator)
    {
        AWaterBody* WaterBody = *WaterBodyIterator;
        if (WaterBody && WaterBody->GetWaterBodyComponent())
        {
            FVector WaterSurfaceLocation;
            FVector WaterSurfaceNormal;
            FVector WaterVelocity;
            float WaterDepth;
            
            // METHOD 1: Standard water surface query
            WaterBody->GetWaterBodyComponent()->GetWaterSurfaceInfoAtLocation(
                WorldLocation,
                WaterSurfaceLocation,
                WaterSurfaceNormal,
                WaterVelocity,
                WaterDepth,
                true  // bIncludeDepth
            );
            
            float BaseWaterHeight = WaterSurfaceLocation.Z;
            float FinalWaterHeight = BaseWaterHeight;
            
            // METHOD 2: Try to get wave displacement directly
            if (UWaterWavesBase* WaterWaves = WaterBody->GetWaterWaves())
            {
                // Try different wave sampling methods
                float CurrentTime = GetWorld()->GetTimeSeconds();
                FVector2D Location2D(WorldLocation.X, WorldLocation.Y);
                
                // Method 2a: Try GetWaveAttenuationAtPosition if it exists
                float WaveHeight = 0.0f;
                
                // Note: This might need to be adjusted based on your UE version
                // Try to sample waves at this location
                
                if (bShowDetailedLogs)
                {
                    UE_LOG(LogTemp, Warning, TEXT("🌊 WAVE ANALYSIS:"));
                    UE_LOG(LogTemp, Warning, TEXT("   Wave Asset: %s"), *WaterWaves->GetName());
                    UE_LOG(LogTemp, Warning, TEXT("   Wave Class: %s"), *WaterWaves->GetClass()->GetName());
                    UE_LOG(LogTemp, Warning, TEXT("   Base Height: %.3f"), BaseWaterHeight);
                    UE_LOG(LogTemp, Warning, TEXT("   Current Time: %.3f"), CurrentTime);
                    UE_LOG(LogTemp, Warning, TEXT("   Query Location: %s"), *WorldLocation.ToString());
                }
            }
            else
            {
                if (bShowDetailedLogs)
                {
                    UE_LOG(LogTemp, Warning, TEXT("⚠️ No wave asset on water body"));
                }
            }
            
            // METHOD 3: Sample multiple nearby points to detect waves
            if (bShowDetailedLogs)
            {
                // Sample a few points around the location to see if there's variation
                TArray<float> SampleHeights;
                TArray<FVector> SamplePoints = {
                    WorldLocation,
                    WorldLocation + FVector(50, 0, 0),
                    WorldLocation + FVector(-50, 0, 0), 
                    WorldLocation + FVector(0, 50, 0),
                    WorldLocation + FVector(0, -50, 0)
                };
                
                for (const FVector& SamplePoint : SamplePoints)
                {
                    FVector SampleSurface;
                    FVector SampleNormal; 
                    FVector SampleVelocity;
                    float SampleDepth;
                    
                    WaterBody->GetWaterBodyComponent()->GetWaterSurfaceInfoAtLocation(
                        SamplePoint,
                        SampleSurface,
                        SampleNormal,
                        SampleVelocity, 
                        SampleDepth,
                        true
                    );
                    
                    SampleHeights.Add(SampleSurface.Z);
                }
                
                // Use built-in UE math functions for min/max
                float MinHeight = FMath::Min(SampleHeights);
                float MaxHeight = FMath::Max(SampleHeights);
                float HeightVariation = MaxHeight - MinHeight;
                
                UE_LOG(LogTemp, Warning, TEXT("🔍 WAVE VARIATION TEST:"));
                UE_LOG(LogTemp, Warning, TEXT("   Sample Heights: %.3f, %.3f, %.3f, %.3f, %.3f"), 
                       SampleHeights[0], SampleHeights[1], SampleHeights[2], SampleHeights[3], SampleHeights[4]);
                UE_LOG(LogTemp, Warning, TEXT("   Height Range: %.3f - %.3f"), MinHeight, MaxHeight);
                UE_LOG(LogTemp, Warning, TEXT("   Variation: %.3f"), HeightVariation);
                
                if (HeightVariation < 0.1f)
                {
                    UE_LOG(LogTemp, Error, TEXT("❌ Water appears STATIC - no wave variation detected"));
                    UE_LOG(LogTemp, Error, TEXT("   Check: Does water body have Wave Asset assigned?"));
                    UE_LOG(LogTemp, Error, TEXT("   Check: Are wave amplitudes high enough to detect?"));
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("✅ Wave variation detected - waves appear active"));
                }
            }
            
            if (bShowDetailedLogs)
            {
                UE_LOG(LogTemp, Warning, TEXT("📊 FINAL WATER SAMPLING:"));
                UE_LOG(LogTemp, Warning, TEXT("   Query Location: %s"), *WorldLocation.ToString());
                UE_LOG(LogTemp, Warning, TEXT("   Water Surface: %s"), *WaterSurfaceLocation.ToString());
                UE_LOG(LogTemp, Warning, TEXT("   Water Normal: %s"), *WaterSurfaceNormal.ToString());
                UE_LOG(LogTemp, Warning, TEXT("   Water Velocity: %s"), *WaterVelocity.ToString());
                UE_LOG(LogTemp, Warning, TEXT("   Water Depth: %.3f"), WaterDepth);
                UE_LOG(LogTemp, Warning, TEXT("   Returned Height: %.3f"), BaseWaterHeight);
            }
            
            return BaseWaterHeight;
        }
    }
    
    if (bShowDetailedLogs)
    {
        UE_LOG(LogTemp, Error, TEXT("❌ No water body found in world"));
    }
    
    return -99999.0f;
}

void UWaterPhysicsComponent::DrawDebugInfo()
{
    if (!GetWorld()) return;
    
    FVector ObjectLocation = GetOwner()->GetActorLocation();
    float WaterHeight = GetWaterHeightAtLocation(ObjectLocation);
    
    // BASE POSITION for floating text (above object)
    FVector TextBasePosition = ObjectLocation + FVector(0, 0, 100);
    
    // SINGLE MESSAGE SYSTEM - Only show current status
    if (WaterHeight > -99999.0f)
    {
        FVector WaterPoint = FVector(ObjectLocation.X, ObjectLocation.Y, WaterHeight);
        
        // Water surface point (larger, more visible)
        DrawDebugSphere(GetWorld(), WaterPoint, 35.0f, 12, FColor::Blue, false, -1.0f, 0, 3.0f);
        
        // Connection line (thicker)
        DrawDebugLine(GetWorld(), ObjectLocation, WaterPoint, FColor::Cyan, false, -1.0f, 0, 5.0f);
        
        // SINGLE STATUS MESSAGE
        float SubmersionDepth = WaterHeight - ObjectLocation.Z;
        
        FString CurrentMessage;
        FColor MessageColor;
        
        if (SubmersionDepth > 0)
        {
            CurrentMessage = FString::Printf(TEXT("🌊 IN WATER: %.1fcm deep | Height: %.3f"), 
                                           SubmersionDepth, WaterHeight);
            MessageColor = FColor::Green;
        }
        else
        {
            CurrentMessage = FString::Printf(TEXT("☁️ ABOVE WATER: %.1fcm above | Height: %.3f"), 
                                           FMath::Abs(SubmersionDepth), WaterHeight);
            MessageColor = FColor::Orange;
        }
        
        // Show ONLY the current message
        DrawDebugString(GetWorld(), TextBasePosition, CurrentMessage, 
                       nullptr, MessageColor, 1.0f, true, 1.2f);
        
        // Optional: Show additional info if enabled
        if (bShowWaterBodyInfo)
        {
            // Check wave system status
            FString WaveStatus = TEXT("Static Water");
            for (TActorIterator<AWaterBody> WaterBodyIterator(GetWorld()); WaterBodyIterator; ++WaterBodyIterator)
            {
                AWaterBody* WaterBody = *WaterBodyIterator;
                if (WaterBody && WaterBody->GetWaterWaves())
                {
                    WaveStatus = TEXT("Wave System Active");
                    break;
                }
            }
            
            // Show wave status below main message
            DrawDebugString(GetWorld(), TextBasePosition + FVector(0, 0, -25), 
                           FString::Printf(TEXT("🌊 %s"), *WaveStatus), 
                           nullptr, FColor::Cyan, 1.0f, true, 0.9f);
        }
        
        // Water level indicator (horizontal plane)
        DrawDebugSphere(GetWorld(), WaterPoint + FVector(50, 0, 0), 15.0f, 8, FColor::Blue, false, -1.0f, 0, 2.0f);
        DrawDebugSphere(GetWorld(), WaterPoint + FVector(-50, 0, 0), 15.0f, 8, FColor::Blue, false, -1.0f, 0, 2.0f);
        DrawDebugSphere(GetWorld(), WaterPoint + FVector(0, 50, 0), 15.0f, 8, FColor::Blue, false, -1.0f, 0, 2.0f);
        DrawDebugSphere(GetWorld(), WaterPoint + FVector(0, -50, 0), 15.0f, 8, FColor::Blue, false, -1.0f, 0, 2.0f);
    }
    else
    {
        // No water found - single error message
        DrawDebugString(GetWorld(), TextBasePosition, 
                       TEXT("❌ NO WATER DETECTED - Check water body setup"), 
                       nullptr, FColor::Red, 1.0f, true, 1.3f);
    }
    
    // Object center (larger, more visible)
    DrawDebugSphere(GetWorld(), ObjectLocation, 15.0f, 12, FColor::Red, false, -1.0f, 0, 3.0f);
    
    // Object name label (at base)
    DrawDebugString(GetWorld(), ObjectLocation + FVector(0, 0, 50), GetOwner()->GetName(), 
                   nullptr, FColor::White, 1.0f, true, 1.0f);
}

// NEW FUNCTION: Display detailed water body info as floating text
void UWaterPhysicsComponent::DisplayWaterBodyInfoAtLocation(const FVector& BasePosition, int32& CurrentLine, const FVector& QueryLocation)
{
    if (!GetWorld()) return;
    
    int32 LineHeight = 25;
    int32 WaterBodyCount = 0;
    
    for (TActorIterator<AWaterBody> WaterBodyIterator(GetWorld()); WaterBodyIterator; ++WaterBodyIterator)
    {
        AWaterBody* WaterBody = *WaterBodyIterator;
        WaterBodyCount++;
        
        if (WaterBody && WaterBody->GetWaterBodyComponent())
        {
            // Water body name and type
            DrawDebugString(GetWorld(), BasePosition + FVector(0, 0, LineHeight * CurrentLine++), 
                           FString::Printf(TEXT("🌊 #%d: %s"), WaterBodyCount, *WaterBody->GetName()), 
                           nullptr, FColor::Cyan, 1.0f, true);
            
            DrawDebugString(GetWorld(), BasePosition + FVector(0, 0, LineHeight * CurrentLine++), 
                           FString::Printf(TEXT("   Type: %s"), *WaterBody->GetClass()->GetName()), 
                           nullptr, FColor::White, 1.0f, true, 0.8f);
            
            // Get detailed water info at this location
            FVector WaterSurfaceLocation;
            FVector WaterSurfaceNormal;
            FVector WaterVelocity;
            float WaterDepth;
            
            WaterBody->GetWaterBodyComponent()->GetWaterSurfaceInfoAtLocation(
                QueryLocation,
                WaterSurfaceLocation,
                WaterSurfaceNormal,
                WaterVelocity,
                WaterDepth,
                true
            );
            
            // Display the detailed info
            DrawDebugString(GetWorld(), BasePosition + FVector(0, 0, LineHeight * CurrentLine++), 
                           FString::Printf(TEXT("   Surface: (%.1f, %.1f, %.1f)"), 
                           WaterSurfaceLocation.X, WaterSurfaceLocation.Y, WaterSurfaceLocation.Z), 
                           nullptr, FColor::White, 1.0f, true, 0.8f);
            
            DrawDebugString(GetWorld(), BasePosition + FVector(0, 0, LineHeight * CurrentLine++), 
                           FString::Printf(TEXT("   Normal: (%.2f, %.2f, %.2f)"), 
                           WaterSurfaceNormal.X, WaterSurfaceNormal.Y, WaterSurfaceNormal.Z), 
                           nullptr, FColor::White, 1.0f, true, 0.8f);
            
            DrawDebugString(GetWorld(), BasePosition + FVector(0, 0, LineHeight * CurrentLine++), 
                           FString::Printf(TEXT("   Velocity: (%.1f, %.1f, %.1f)"), 
                           WaterVelocity.X, WaterVelocity.Y, WaterVelocity.Z), 
                           nullptr, FColor::White, 1.0f, true, 0.8f);
            
            DrawDebugString(GetWorld(), BasePosition + FVector(0, 0, LineHeight * CurrentLine++), 
                           FString::Printf(TEXT("   Depth: %.1f"), WaterDepth), 
                           nullptr, FColor::White, 1.0f, true, 0.8f);
            
            CurrentLine++; // Space between water bodies
        }
    }
    
    if (WaterBodyCount == 0)
    {
        DrawDebugString(GetWorld(), BasePosition + FVector(0, 0, LineHeight * CurrentLine++), 
                       TEXT("❌ No water bodies found!"), nullptr, FColor::Red, 1.0f, true);
    }
}

// NEW FUNCTION: Display world water scan results  
void UWaterPhysicsComponent::DisplayWorldWaterScan(const FVector& BasePosition, int32& CurrentLine)
{
    if (!GetWorld()) return;
    
    int32 LineHeight = 25;
    int32 TotalWaterBodies = 0;
    
    for (TActorIterator<AWaterBody> WaterBodyIterator(GetWorld()); WaterBodyIterator; ++WaterBodyIterator)
    {
        AWaterBody* WaterBody = *WaterBodyIterator;
        TotalWaterBodies++;
        
        DrawDebugString(GetWorld(), BasePosition + FVector(0, 0, LineHeight * CurrentLine++), 
                       FString::Printf(TEXT("Found: %s at %s"), 
                       *WaterBody->GetName(), *WaterBody->GetActorLocation().ToString()), 
                       nullptr, FColor::Yellow, 1.0f, true, 0.8f);
        
        if (UWaterBodyComponent* WaterComp = WaterBody->GetWaterBodyComponent())
        {
            DrawDebugString(GetWorld(), BasePosition + FVector(0, 0, LineHeight * CurrentLine++), 
                           TEXT("   ✅ Has WaterBodyComponent"), 
                           nullptr, FColor::Green, 1.0f, true, 0.8f);
        }
        else
        {
            DrawDebugString(GetWorld(), BasePosition + FVector(0, 0, LineHeight * CurrentLine++), 
                           TEXT("   ❌ Missing WaterBodyComponent!"), 
                           nullptr, FColor::Red, 1.0f, true, 0.8f);
        }
    }
    
    DrawDebugString(GetWorld(), BasePosition + FVector(0, 0, LineHeight * CurrentLine++), 
                   FString::Printf(TEXT("Total Water Bodies: %d"), TotalWaterBodies), 
                   nullptr, TotalWaterBodies > 0 ? FColor::Green : FColor::Red, 1.0f, true);
}

// NEW ENHANCED DEBUG FUNCTIONS
void UWaterPhysicsComponent::LogWaterSystemStatus()
{
    if (!bShowDetailedLogs) return;
    
    UE_LOG(LogTemp, Warning, TEXT("=== 🌊 WATER SYSTEM STATUS CHECK ==="));
    
    // Check if Water plugin is enabled
    UE_LOG(LogTemp, Warning, TEXT("🔧 Water Plugin Status: Checking..."));
    
    // Count total water bodies in world
    int32 TotalWaterBodies = 0;
    if (UWorld* World = GetWorld())
    {
        for (TActorIterator<AWaterBody> WaterBodyIterator(World); WaterBodyIterator; ++WaterBodyIterator)
        {
            AWaterBody* WaterBody = *WaterBodyIterator;
            TotalWaterBodies++;
            
            UE_LOG(LogTemp, Warning, TEXT("   🌊 Found Water Body #%d: %s"), TotalWaterBodies, *WaterBody->GetName());
            UE_LOG(LogTemp, Warning, TEXT("      Type: %s"), *WaterBody->GetClass()->GetName());
            UE_LOG(LogTemp, Warning, TEXT("      Location: %s"), *WaterBody->GetActorLocation().ToString());
            
            if (UWaterBodyComponent* WaterComp = WaterBody->GetWaterBodyComponent())
            {
                UE_LOG(LogTemp, Warning, TEXT("      ✅ Has WaterBodyComponent"));
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("      ❌ Missing WaterBodyComponent!"));
            }
        }
    }
    
    if (TotalWaterBodies == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("❌ NO WATER BODIES FOUND IN WORLD!"));
        UE_LOG(LogTemp, Error, TEXT("   Make sure you have:"));
        UE_LOG(LogTemp, Error, TEXT("   1. Water plugin enabled"));
        UE_LOG(LogTemp, Error, TEXT("   2. Water body (Lake/Ocean/River) in the level"));
        UE_LOG(LogTemp, Error, TEXT("   3. Water Zone actor in the level"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("✅ Found %d water bodies in world"), TotalWaterBodies);
    }
    
    UE_LOG(LogTemp, Warning, TEXT("======================================="));
}

void UWaterPhysicsComponent::DrawWaterBodyBounds()
{
    if (!GetWorld()) return;
    
    // Draw bounds of all water bodies for debugging
    for (TActorIterator<AWaterBody> WaterBodyIterator(GetWorld()); WaterBodyIterator; ++WaterBodyIterator)
    {
        AWaterBody* WaterBody = *WaterBodyIterator;
        if (WaterBody)
        {
            FVector WaterLocation = WaterBody->GetActorLocation();
            FBox WaterBounds = WaterBody->GetComponentsBoundingBox();
            
            // Draw water body bounds as wireframe box
            DrawDebugBox(GetWorld(), WaterBounds.GetCenter(), WaterBounds.GetExtent(), 
                        FColor::Cyan, false, -1.0f, 0, 2.0f);
            
            // Label the water body
            DrawDebugString(GetWorld(), WaterLocation + FVector(0, 0, 100), 
                          FString::Printf(TEXT("WATER: %s"), *WaterBody->GetName()), 
                          nullptr, FColor::Cyan, 1.0f, true);
        }
    }
}