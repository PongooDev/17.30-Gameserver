#pragma once
#include "framework.h"

namespace Vehicles {
	void SpawnVehicles() {
        auto VehicleSpawners = GetAllActorsOfClass<AFortAthenaVehicleSpawner>();

        for (auto& VehicleSpawner : VehicleSpawners) {
            if (!VehicleSpawner)
                continue;

            auto VehicleClass = VehicleSpawner->GetVehicleClass();
            if (!VehicleClass)
                continue;

            AFortAthenaVehicle* Vehicle = SpawnActor<AFortAthenaVehicle>(VehicleSpawner->K2_GetActorLocation(), VehicleSpawner->K2_GetActorRotation(), nullptr, VehicleClass);

            auto FuelComponent = Cast<UFortAthenaVehicleFuelComponent>(
                Vehicle->GetComponentByClass(UFortAthenaVehicleFuelComponent::StaticClass())
            );

            if (FuelComponent) {
                FuelComponent->ServerFuel = UKismetMathLibrary::GetDefaultObj()->RandomIntegerInRange(60, 100);
                FuelComponent->OnRep_ServerFuel(0);
            }

            if (!Vehicle)
                continue;
        }

        VehicleSpawners.Free();
        Log("Spawned Vehicles!");
	}
}