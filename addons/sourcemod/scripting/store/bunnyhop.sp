public Bunnyhop_OnPluginStart()
{
	Store_RegisterHandler("bunnyhop", "", Bunnyhop_OnMapStart, Bunnyhop_Reset, Bunnyhop_Config, Bunnyhop_Equip, Bunnyhop_Remove, true);
}

public Bunnyhop_OnMapStart()
{
}

public Bunnyhop_Reset()
{
}

public Bunnyhop_Config(&Handle:kv, itemid)
{
	Store_SetDataIndex(itemid, 0);
	return true;
}

public Bunnyhop_Equip(client, id)
{
	return -1;
}

public Bunnyhop_Remove(client, id)
{
}

public Action:Bunnyhop_OnPlayerRunCmd(client, &buttons)
{
	new m_iEquipped = Store_GetEquippedItem(client, "bunnyhop");
	if(m_iEquipped < 0)
		return Plugin_Continue;

	new m_iWater = GetEntProp(client, Prop_Data, "m_nWaterLevel");
	if (IsPlayerAlive(client))
		if (buttons & IN_JUMP)
			if (m_iWater <= 1)
				if (!(GetEntityMoveType(client) & MOVETYPE_LADDER))
				{
					if(!GAME_TF2)
						SetEntPropFloat(client, Prop_Send, "m_flStamina", 0.0);
					if (!(GetEntityFlags(client) & FL_ONGROUND))
						buttons &= ~IN_JUMP;
				}

	return Plugin_Continue;
}