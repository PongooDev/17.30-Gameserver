#include "framework.h"
#include "FortGameModeAthena.h"
#include "FortPlayerControllerAthena.h"
#include "FortPlayerPawn.h"
#include "AbilitySystemComponent.h"
#include "Looting.h"
#include "BuildingActor.h"
#include "NetDriver.h"
#include "FortAthenaAIBotController.h"
#include "Misc.h"

void InitConsole() {
    AllocConsole();
    FILE* fptr;
    freopen_s(&fptr, "CONOUT$", "w+", stdout);
    SetConsoleTitleA("17.30 Gameserver | Starting...");
    Log("Welcome to 17.30 Gameserver! Made with love by ObsessedTech!");
}

void LoadWorld() {
    Log("Loading World!");
    if (!Globals::bCreativeEnabled && !Globals::bSTWEnabled) {
        UKismetSystemLibrary::ExecuteConsoleCommand(UWorld::GetWorld(), L"open Apollo_Terrain", nullptr);
    }
    else if (Globals::bCreativeEnabled) {
        
    }
    UWorld::GetWorld()->OwningGameInstance->LocalPlayers.Remove(0);
}

void Hook() {
    Misc::HookAll();

    FortGameModeAthena::HookAll();
    FortPlayerControllerAthena::HookAll();
    FortPlayerPawn::HookAll();
    AbilitySystemComponent::HookAll();
    Looting::HookAll();
    BuildingActor::HookAll();

    NetDriver::HookAll();
    FortAthenaAIBotController::HookAll();

    MH_EnableHook(MH_ALL_HOOKS);
}

static void WaitForLogin() {
    Log("Waiting for login!");

    FName Frontend = UKismetStringLibrary::Conv_StringToName(L"Frontend");
    FName MatchState = UKismetStringLibrary::Conv_StringToName(L"InProgress");

    while (true) {
        UWorld* CurrentWorld = ((UWorld*)UWorld::GetWorld());
        if (CurrentWorld) {
            if (CurrentWorld->Name == Frontend) {
                auto GameMode = (AGameMode*)CurrentWorld->AuthorityGameMode;
                if (GameMode->GetMatchState() == MatchState) {
                    break;
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1000 * 1));
    Log("Logged in!");
}

DWORD Main(LPVOID) {
    InitConsole();
    MH_Initialize();
    Log("MinHook Initialised!");

    while (UEngine::GetEngine() == 0)
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    WaitForLogin();

    Hook();

    *(bool*)(ImageBase + 0x973E49B) = false; //GIsClient
    *(bool*)(ImageBase + 0x973E49C) = true; //GIsServer

    Sleep(2000);
    LoadWorld();

    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        CreateThread(0, 0, Main, 0, 0, 0);
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
