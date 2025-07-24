#pragma once
#include "framework.h"

namespace FortPlayerControllerAthena {
	void (*ServerAcknowledgePossessionOG)(AFortPlayerControllerAthena* This, AFortPlayerPawnAthena* Pawn);
	void ServerAcknowledgePossession(AFortPlayerControllerAthena* This, AFortPlayerPawnAthena* Pawn) {
		Log("ServerAcknowledgePossession Called!");
		This->AcknowledgedPawn = Pawn;

		return ServerAcknowledgePossessionOG(This, Pawn);
	}

	void HookAll() {
		//MH_CreateHook((LPVOID)(ImageBase + 0xC264C0), ServerAcknowledgePossession, (LPVOID*)&ServerAcknowledgePossessionOG);
		HookVTable(AFortPlayerControllerAthena::GetDefaultObj(), 0x114, ServerAcknowledgePossession, (LPVOID*)&ServerAcknowledgePossessionOG);

		Log("FortPlayerControllerAthena Hooked!");
	}
}