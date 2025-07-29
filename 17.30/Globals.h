#pragma once

namespace Globals {
	bool bIsProdServer = false;

	bool bCreativeEnabled = false;
	bool bSTWEnabled = false;
	bool bEventEnabled = false;

	bool bBotsEnabled = true;
	bool bBotsShouldUseManualTicking = false;

	int MaxBotsToSpawn = 95;
	int MinPlayersForEarlyStart = 95;

	//REAL PLAYERS
	static int NextTeamIndex = 0;
	static int CurrentPlayersOnTeam = 0;
	static int MaxPlayersPerTeam = 1;
}
