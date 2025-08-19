#pragma once
#include "framework.h"

namespace Misc {
	static void KickPlayer(AGameSession* GameSession, AController* Controller)
	{
		Log(std::format("KickPlayer Called! PlayerName: {}", Controller->PlayerState->GetPlayerName().ToString()).c_str());
		return;
	}

	bool CanCreateInCurrentContext() {
		//Log("CanCreateInCurrentContext Called!");
		return true;
	}

	void (*DispatchRequestOG)(__int64 a1, unsigned __int64* a2, int a3);
	void DispatchRequest(__int64 a1, unsigned __int64* a2, int a3)
	{
		return DispatchRequestOG(a1, a2, 3);
	}

	void HookAll() {
		//MH_CreateHook((LPVOID)(ImageBase + 0x6172BF8), KickPlayer, nullptr);
		MH_CreateHook((LPVOID)(ImageBase + 0x4D68614), KickPlayer, nullptr);
		//MH_CreateHook((LPVOID)(ImageBase + 0x1371ABC), IsFalse, nullptr); //CollectGarbage
		MH_CreateHook((LPVOID)(ImageBase + 0x28932FC), nullFunc, nullptr); //ChangeGameSessionId
		MH_CreateHook((LPVOID)(ImageBase + 0x15A07D8), DispatchRequest, (LPVOID*)&DispatchRequestOG);

		HookVTable(AActor::GetDefaultObj(), 0x1B, CanCreateInCurrentContext, nullptr);
		HookVTable(AAthenaAIDirector::GetDefaultObj(), 0x1B, CanCreateInCurrentContext, nullptr);
		HookVTable(AAthenaNavMesh::GetDefaultObj(), 0x1B, CanCreateInCurrentContext, nullptr);

		MH_CreateHook((LPVOID)(ImageBase + 0x1371e94), nullFunc, nullptr);
		MH_CreateHook((LPVOID)(ImageBase + 0x2f327b0), nullFunc, nullptr);
		MH_CreateHook((LPVOID)(ImageBase + 0x28932fc), nullFunc, nullptr);
		MH_CreateHook((LPVOID)(ImageBase + 0x4efb7dc), nullFunc, nullptr);

		MH_CreateHook((LPVOID)(ImageBase + 0x4d79308), IsTrue, nullptr);

		MH_CreateHook((LPVOID)(ImageBase + 0x46CF05C), IsTrue, nullptr);

		Log("Misc Hooked!");
	}
}