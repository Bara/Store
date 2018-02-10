new g_iRespawnRoundLimit[MAXPLAYERS+1] = {0,...};

new g_cvarRespawnRoundLimit = -1;

public Respawn_OnPluginStart()
{
	Store_RegisterHandler("respawn", "", Respawn_OnMapStart, Respawn_Reset, Respawn_Config, Respawn_Equip, Respawn_Remove, false);

	g_cvarRespawnRoundLimit = RegisterConVar("sm_store_respawn_round_limit", "1", "Number of times you can buy respawn in a round", TYPE_INT);
}

public Respawn_OnPlayerSpawn(client)
{
	g_iRespawnRoundLimit[client] = 0;
}

public Respawn_OnMapStart()
{
}

public Respawn_Reset()
{
}

public Respawn_Config(&Handle:kv, itemid)
{
	Store_SetDataIndex(itemid, 0);

	return true;
}

public Respawn_Equip(client, id)
{
	if(g_iRespawnRoundLimit[client] == g_eCvars[g_cvarRespawnRoundLimit][aCache])
	{
		CPrintToChat(client, "%t", "Respawn Round Limit");
		return 1;
	}

	if(GAME_CSGO || GAME_CSS)
	{
		CS_RespawnPlayer(client);
	}
	else
	{
		TF2_RespawnPlayer(client);
	}

	++g_iRoundLimit[client];
	return 0;
}

public Respawn_Remove(client)
{
	return 0;
}