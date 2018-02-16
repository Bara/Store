new String:g_szNameTags[STORE_MAX_ITEMS][MAXLENGTH_NAME];
new String:g_szNameColors[STORE_MAX_ITEMS][32];
new String:g_szMessageColors[STORE_MAX_ITEMS][32];

new g_iNameTags = 0;
new g_iNameColors = 0;
new g_iMessageColors = 0;

public CPSupport_OnPluginStart()
{	
	if(FindPluginByFile("chat-processor.smx") == INVALID_HANDLE)
	{
		LogError("Chat-Processor isn't installed or failed to load. Chat-Processor support will be disabled. (https://forums.alliedmods.net/showthread.php?t=286913)");
		return;
	}

	Store_RegisterHandler("nametag", "tag", CPSupport_OnMappStart, CPSupport_Reset, NameTags_Config, CPSupport_Equip, CPSupport_Remove, true);
	Store_RegisterHandler("namecolor", "color", CPSupport_OnMappStart, CPSupport_Reset, NameColors_Config, CPSupport_Equip, CPSupport_Remove, true);
	Store_RegisterHandler("msgcolor", "color", CPSupport_OnMappStart, CPSupport_Reset, MsgColors_Config, CPSupport_Equip, CPSupport_Remove, true);
}

public CPSupport_OnMappStart()
{
}

public CPSupport_Reset()
{
	g_iNameTags = 0;
	g_iNameColors = 0;
	g_iMessageColors = 0;
}

public NameTags_Config(&Handle:kv, itemid)
{
	Store_SetDataIndex(itemid, g_iNameTags);
	KvGetString(kv, "tag", g_szNameTags[g_iNameTags], sizeof(g_szNameTags[]));
	++g_iNameTags;
	
	return true;
}

public NameColors_Config(&Handle:kv, itemid)
{
	Store_SetDataIndex(itemid, g_iNameColors);
	KvGetString(kv, "color", g_szNameColors[g_iNameColors], sizeof(g_szNameColors[]));
	++g_iNameColors;
	
	return true;
}

public MsgColors_Config(&Handle:kv, itemid)
{
	Store_SetDataIndex(itemid, g_iMessageColors);
	KvGetString(kv, "color", g_szMessageColors[g_iMessageColors], sizeof(g_szMessageColors[]));
	++g_iMessageColors;
	
	return true;
}

public CPSupport_Equip(client, id)
{
	return -1;
}

public CPSupport_Remove(client, id)
{
}

public Action CP_OnChatMessage(int& client, ArrayList recipients, char[] flagstring, char[] name, char[] message, bool& processcolors, bool& removecolors)
{
	new m_iEquippedNameTag = Store_GetEquippedItem(client, "nametag");
	new m_iEquippedNameColor = Store_GetEquippedItem(client, "namecolor");
	new m_iEquippedMsgColor = Store_GetEquippedItem(client, "msgcolor");
	
	if(m_iEquippedNameTag < 0 && m_iEquippedNameColor < 0 && m_iEquippedMsgColor < 0)
		return Plugin_Continue;
	
	new String:m_szName[MAXLENGTH_NAME*2];
	new String:m_szNameTag[MAXLENGTH_NAME];
	new String:m_szNameColor[32];
	
	if(m_iEquippedNameTag >= 0)
	{
		new m_iNameTag = Store_GetDataIndex(m_iEquippedNameTag);
		strcopy(m_szNameTag, sizeof(m_szNameTag), g_szNameTags[m_iNameTag]);
	}
	if(m_iEquippedNameColor >= 0)
	{
		new m_iNameColor = Store_GetDataIndex(m_iEquippedNameColor);
		strcopy(m_szNameColor, sizeof(m_szNameColor), g_szNameColors[m_iNameColor]);
	}
	Format(m_szName, sizeof(m_szName), "%s%s%s", m_szNameTag, m_szNameColor, name);
	CFormatColor(m_szName, sizeof(m_szName), client);
	strcopy(name, MAXLENGTH_NAME, m_szName);

	if(m_iEquippedMsgColor >= 0)
	{
		new String:m_szMessage[MAXLENGTH_BUFFER];
		strcopy(m_szMessage, sizeof(m_szMessage), message);
		Format(message, MAXLENGTH_BUFFER, "%s%s", g_szMessageColors[Store_GetDataIndex(m_iEquippedMsgColor)], m_szMessage);
		CFormatColor(message, MAXLENGTH_BUFFER, client);
	}

	return Plugin_Changed;
}