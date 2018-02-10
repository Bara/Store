native bool:ZR_IsClientHuman(client);
native ZR_GetClassByName(const String:className[], cacheType = 0);
native ZR_SelectClientClass(client, classIndex, bool:applyIfPossible = true, bool:saveIfEnabled = true);

forward ZR_OnClientInfected(client, attacker, bool:motherInfect, bool:respawnOverride, bool:respawn);
forward ZR_OnClientHumanPost(client, bool:respawn, bool:protect);

new g_iClientClasses[MAXPLAYERS+1][2];

new g_cvarDefaultHumanClass = -1;
new g_cvarDefaultZombieClass = -1;

enum ZRClass
{
	String:szClass[64],
	bool:bZombie,
	unIndex
}

new g_eZRClasses[STORE_MAX_ITEMS][ZRClass];

new g_iZRClasses = 0;

public ZRClass_OnPluginStart()
{
	
	Store_RegisterHandler("zrclass", "class", ZRClass_OnMapStart, ZRClass_Reset, ZRClass_Config, ZRClass_Equip, ZRClass_Remove, true);

	g_cvarDefaultHumanClass = RegisterConVar("sm_store_zrclass_default_human", "Normal Human", "Name of the default human class.", TYPE_STRING);
	g_cvarDefaultZombieClass = RegisterConVar("sm_store_zrclass_default_zombie", "Classic", "Name of the default zombie class.", TYPE_STRING);

	g_bZombieMode = (FindPluginByFile("zombiereloaded")==INVALID_HANDLE?false:true);
}

public ZRClass_OnMapStart()
{
}

public ZRClass_OnLibraryAdded(const String:name[])
{
	if(strcmp(name, "zombiereloaded")==0)
		g_bZombieMode = true;
}

public ZRClass_OnClientConnected(client)
{
	g_iClientClasses[client][0] = -1;
	g_iClientClasses[client][1] = -1;
}

public ZRClass_Reset()
{
	g_iZRClasses = 0;
}

public ZRClass_Config(&Handle:kv, itemid)
{
	if(!IsPluginLoaded("zombiereloaded"))
		return false;

	Store_SetDataIndex(itemid, g_iZRClasses);
	
	KvGetString(kv, "class", g_eZRClasses[g_iZRClasses][szClass], 64);
	g_eZRClasses[g_iZRClasses][bZombie] = (KvGetNum(kv, "zombie")?true:false);
	
	if((g_eZRClasses[g_iZRClasses][unIndex] = ZR_GetClassByName(g_eZRClasses[g_iZRClasses][szClass]))!=-1)
	{
		++g_iZRClasses;
		return true;
	}
	
	return false;
}

public ZRClass_Equip(client, id)
{
	new m_iData = Store_GetDataIndex(id);
	g_iClientClasses[client][g_eZRClasses[m_iData][bZombie]] = g_eZRClasses[m_iData][unIndex];

	ZR_SelectClientClass(client, g_eZRClasses[m_iData][unIndex], false, false);

	return g_eZRClasses[m_iData][bZombie];
}

public ZRClass_Remove(client, id)
{
	new m_iData = Store_GetDataIndex(id);
	g_iClientClasses[client][g_eZRClasses[m_iData][bZombie]] = -1;

	if(g_eZRClasses[m_iData][bZombie])
		ZR_SelectClientClass(client, ZR_GetClassByName(g_eCvars[g_cvarDefaultZombieClass][sCache]), false, false);
	else
		ZR_SelectClientClass(client, ZR_GetClassByName(g_eCvars[g_cvarDefaultHumanClass][sCache]), false, false);

	return g_eZRClasses[Store_GetDataIndex(id)][bZombie];
}

public ZR_OnClientInfected(client, attacker, bool:motherInfect, bool:respawnOverride, bool:respawn)
{	
	if(motherInfect)
		return;

	if(g_iClientClasses[client][1] == -1)
	{
		if(GetClientHealth(client) < 300)
		{
			ZR_SelectClientClass(client, g_cvarDefaultZombieClass, false, false);
		}
		return;
	}

	ZR_SelectClientClass(client, g_iClientClasses[client][1], false, false);
}

public ZR_OnClientHumanPost(client, bool:respawn, bool:protect)
{
	if(g_iClientClasses[client][0] == -1)
		return;

	ZR_SelectClientClass(client, g_iClientClasses[client][0], false, false);
}