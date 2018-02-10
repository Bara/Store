#include <regex>
#include <sdktools>

#if defined _zephstocks_included
 #endinput
#endif
#define _zephstocks_included

#define MSG_LENGTH 192
#define CVAR_LENGTH 128

#define MAX_CVARS 128

#define STRING(%1) %1, sizeof(%1)

enum CVAR_TYPE
{
	TYPE_INT = 0,
	TYPE_FLOAT,
	TYPE_STRING,
	TYPE_FLAG
}

enum CVAR_CACHE
{
	Handle:hCvar,
	CVAR_TYPE:eType,
	any:aCache,
	String:sCache[CVAR_LENGTH],
	Function:fnCallback
}

new g_eCvars[MAX_CVARS][CVAR_CACHE];

new g_iCvars = 0;

#define LoopConnectedClients(%1) for(new %1=1;%1<=MaxClients;++%1)\
								if(IsClientConnected(%1))

#define LoopIngameClients(%1) for(new %1=1;%1<=MaxClients;++%1)\
								if(IsClientInGame(%1))

#define LoopIngamePlayers(%1) for(new %1=1;%1<=MaxClients;++%1)\
								if(IsClientInGame(%1) && !IsFakeClient(%1))
								
#define LoopAuthorizedPlayers(%1) for(new %1=1;%1<=MaxClients;++%1)\
								if(IsClientConnected(%1) && IsClientAuthorized(%1))
								
#define LoopAlivePlayers(%1) for(new %1=1;%1<=MaxClients;++%1)\
								if(IsClientInGame(%1) && IsPlayerAlive(%1))


stock bool:PrintKeyHintText(client, const String:format[], any:...)
{
	new Handle:userMessage = StartMessageOne("KeyHintText", client);

	if (userMessage == INVALID_HANDLE) {
			return false;
	}

	decl String:buffer[254];

	SetGlobalTransTarget(client);
	VFormat(buffer, sizeof(buffer), format, 3);

	if(GetUserMessageType() == UM_Protobuf)
	{
		PbAddString(userMessage, "hints", buffer);
	}
	else
	{
		BfWriteByte(userMessage, 1);
		BfWriteString(userMessage, buffer);
	}
	

	EndMessage();
   
	return true;
}

stock HookConVar(String:name[], CVAR_TYPE:type, Function:callback=INVALID_FUNCTION)
{
	new Handle:cvar = FindConVar(name);
	if(cvar == INVALID_HANDLE)
		return -1;
	HookConVarChange(cvar, GlobalConVarChanged);
	g_eCvars[g_iCvars][hCvar] = cvar;
	g_eCvars[g_iCvars][eType] = type;
	g_eCvars[g_iCvars][fnCallback] = callback;
	CacheCvarValue(g_iCvars);
	return g_iCvars++;
}

stock RegisterConVar(String:name[], String:value[], String:description[], CVAR_TYPE:type, Function:callback=INVALID_FUNCTION, flags=0, bool:hasMin=false, Float:min=0.0, bool:hasMax=false, Float:max=0.0)
{
	new Handle:cvar = CreateConVar(name, value, description, flags, hasMin, min, hasMax, max);
	HookConVarChange(cvar, GlobalConVarChanged);
	g_eCvars[g_iCvars][hCvar] = cvar;
	g_eCvars[g_iCvars][eType] = type;
	g_eCvars[g_iCvars][fnCallback] = callback;
	CacheCvarValue(g_iCvars);
	return g_iCvars++;
}

public GlobalConVarChanged(Handle:convar, const String:oldValue[], const String:newValue[])
{
	for(new i=0;i<g_iCvars;++i)
		if(g_eCvars[i][hCvar]==convar)
		{
			CacheCvarValue(i);
		
			if(g_eCvars[i][fnCallback]!=INVALID_FUNCTION)
			{
				Call_StartFunction(INVALID_HANDLE, g_eCvars[i][fnCallback]);
				Call_PushCell(i);
				Call_Finish();
			}
		
			return;
		}
}

public CacheCvarValue(index)
{
	GetConVarString(g_eCvars[index][hCvar], g_eCvars[index][sCache], CVAR_LENGTH);
	if(g_eCvars[index][eType]==TYPE_INT)
		g_eCvars[index][aCache] = GetConVarInt(g_eCvars[index][hCvar]);
	else if(g_eCvars[index][eType]==TYPE_FLOAT)
		g_eCvars[index][aCache] = GetConVarFloat(g_eCvars[index][hCvar]);
	else if(g_eCvars[index][eType]==TYPE_FLAG)
		g_eCvars[index][aCache] = ReadFlagString(g_eCvars[index][sCache]);
}

public SQLCallback_Void(Handle:owner, Handle:hndl, const String:error[], any:suspend_errors)
{
	if(hndl==INVALID_HANDLE && !suspend_errors)
		LogError("SQL error happened. Error: %s", error);
}

public SQLCallback_Void_PrintQuery(Handle:owner, Handle:hndl, const String:error[], any:data)
{
	if(hndl==INVALID_HANDLE)
	{
		new String:query[2048];
		ReadPackString(data, STRING(query));
		LogError("SQL error happened.\nQuery: %s\nError: %s", query, error);
	}
	CloseHandle(data);
}

public SQL_TVoid(Handle:db, String:query[])
{
	new Handle:data = CreateDataPack();
	WritePackString(data, query);
	ResetPack(data);
	SQL_TQuery(db, SQLCallback_Void_PrintQuery, query, data);
}

public SQLCallback_NoError(Handle:owner, Handle:hndl, const String:error[], any:suspend_errors)
{
}

stock GetClientBySteamID(String:steamid[])
{
	new String:authid[32];
	for(new i=1;i<=MaxClients;++i)
	{
		if(!IsClientInGame(i))
			continue;
		if(!IsClientAuthorized(i))
			continue;
		GetClientAuthId(i, AuthId_Steam2, STRING(authid));
		if(strcmp(authid[8], steamid[8])==0 || strcmp(authid, steamid)==0)
			return i;
	}
	return 0;
}

stock GetClientSightEnd(client, Float:out[3])
{
	decl Float:m_fEyes[3];
	decl Float:m_fAngles[3];
	GetClientEyePosition(client, m_fEyes);
	GetClientEyeAngles(client, m_fAngles);
	TR_TraceRayFilter(m_fEyes, m_fAngles, MASK_PLAYERSOLID, RayType_Infinite, TraceRayDontHitPlayers);
	if(TR_DidHit())
		TR_GetEndPosition(out);
}

stock GetClientByIP(String:ip[])
{
	decl String:client_ip[16];
	for(new i=1;i<=MaxClients;++i)
	{
		if(!IsClientInGame(i))
			continue;
		GetClientIP(i, client_ip, sizeof(client_ip));
		if(strcmp(client_ip, ip)==0)
			return i;
	}
	return 0;
}

stock bool:GetClientPrivilege(client, flag, flags=-1)
{
	if(flags==-1)
		flags = GetUserFlagBits(client);
	
	if(flag == 0 || flags & flag || flags & ADMFLAG_ROOT)
		return true;
	return false;
}

stock bool:GetCommunityID(String:AuthID[], String:FriendID[], size)
{
	if(strlen(AuthID) < 11 || AuthID[0]!='S' || AuthID[6]=='I')
	{
		FriendID[0] = 0;
		return false;
	}
	new iUpper = 765611979;
	new iFriendID = StringToInt(AuthID[10])*2 + 60265728 + AuthID[8]-48;
	new iDiv = iFriendID/100000000;
	new iIdx = 9-(iDiv?iDiv/10+1:0);
	iUpper += iDiv;
	IntToString(iFriendID, FriendID[iIdx], size-iIdx);
	iIdx = FriendID[9];
	IntToString(iUpper, FriendID, size);
	FriendID[9] = iIdx;
	return true;
}

stock CloseClientMenu(client)
{
	new Handle:m_hMenu = CreateMenu(MenuHandler_CloseClientMenu);
	SetMenuTitle(m_hMenu, "Empty menu");
	DisplayMenu(m_hMenu, client, 1);
}

public MenuHandler_CloseClientMenu(Handle:menu, MenuAction:action, client, param2)
{
	if (action == MenuAction_End)
		CloseHandle(menu);
}

public bool:TraceRayDontHitSelf(entity, mask, any:data)
{
	if(entity == data)
		return false;
	return true;
}

public bool:TraceRayDontHitPlayers(entity, mask, any:data)
{
	if(0 < entity <= MaxClients)
		return false;
	return true;
}

public CreateCountdown(client, seconds, String:format[])
{
	new Handle:pack = CreateDataPack();
	WritePackCell(pack, GetClientUserId(client));
	WritePackCell(pack, seconds);
	WritePackString(pack, format);
	ResetPack(pack);

	CreateTimer(0.0, Timer_Countdown, pack);
}

public CreateCountdownAll(seconds, String:format[])
{
	new Handle:pack = CreateDataPack();
	WritePackCell(pack, 0);
	WritePackCell(pack, seconds);
	WritePackString(pack, format);
	ResetPack(pack);

	CreateTimer(0.0, Timer_Countdown, pack);
}

public Action:Timer_Countdown(Handle:timer, any:pack)
{
	new userid = ReadPackCell(pack);
	decl client;
	if(userid!=0)
	{
		client = GetClientOfUserId(userid);
		if(!client)
		{
			CloseHandle(pack);
			return Plugin_Stop;
		}
	}

	new seconds = ReadPackCell(pack);
	decl String:format[192];
	ReadPackString(pack, format, sizeof(format));

	if(userid != 0)
		PrintCenterText(client, "%t", format, seconds);
	else
		PrintCenterTextAll("%t", format, seconds);

	if(seconds != 1)
	{
		ResetPack(pack);
		ReadPackCell(pack);
		WritePackCell(pack, seconds-1);
		ResetPack(pack);

		CreateTimer(1.0, Timer_Countdown, pack);
	}
	else
		CloseHandle(pack);

	return Plugin_Stop;
}

stock bool:AddMenuItemEx(Handle:menu, style, String:info[], String:display[], any:...)
{
	decl String:m_display[256];
	VFormat(m_display, sizeof(m_display), display, 5);
	return (AddMenuItem(menu, info, m_display, style)?true:false);
}

stock SetPanelTitleEx(Handle:menu, String:display[], any:...)
{
	decl String:m_display[256];
	VFormat(m_display, sizeof(m_display), display, 3);
	SetPanelTitle(menu, m_display);
}

stock DrawPanelItemEx(Handle:menu, style, String:display[], any:...)
{
	decl String:m_display[256];
	VFormat(m_display, sizeof(m_display), display, 4);
	return DrawPanelItem(menu, m_display, style);
}

stock DrawPanelTextEx(Handle:menu, String:display[], any:...)
{
	decl String:m_display[256];
	VFormat(m_display, sizeof(m_display), display, 3);
	return DrawPanelText(menu, m_display);
}

stock bool:InsertMenuItemEx(Handle:menu, position, style, String:info[], String:display[], any:...)
{
	decl String:m_display[256];
	VFormat(m_display, sizeof(m_display), display, 6);
	if(GetMenuItemCount(menu)==position)
		return (AddMenuItem(menu, info, m_display, style)?true:false);
	else
		return (InsertMenuItem(menu, position, info, m_display, style)?true:false);
}

stock ClearTimer(&Handle:timer)
{
	if(timer != INVALID_HANDLE)
	{
		KillTimer(timer);
		timer = INVALID_HANDLE;
	}
}

stock bool:ShouldHappen(chance)
{
	return (GetRandomInt(1, 100)<=chance?true:false);
}

stock Downloader_ParseMDL(const String:model[], String:internal[], maxlen1, String:files[][], maxsize, maxlen2)
{
	if(!FileExists2(model))
		return 0;

	new m_iID;
	new m_iVersion;
	new m_iNum = 0;
	new m_iDirNum = 0;
	new m_iOffset = 0;
	new m_iDirOffset = 0;
	new m_iNameOffset = 0;
	new m_iIdx = 0;

	new Handle:m_hFile = OpenFile2(model, "rb");
	if(m_hFile==INVALID_HANDLE)
		return 0;

	ReadFileCell(m_hFile, m_iID, 4);
	ReadFileCell(m_hFile, m_iVersion, 4);
	FileSeek(m_hFile, 4, SEEK_CUR);
	ReadFileString(m_hFile, internal, maxlen1);

	FileSeek(m_hFile, 204, SEEK_SET);
	ReadFileCell(m_hFile, m_iNum, 4);
	ReadFileCell(m_hFile, m_iOffset, 4);
	ReadFileCell(m_hFile, m_iDirNum, 4);
	ReadFileCell(m_hFile, m_iDirOffset, 4);

	new String:m_szPath[PLATFORM_MAX_PATH];
	if(m_iDirNum!=0)
	{
		FileSeek(m_hFile, m_iDirOffset, SEEK_SET);
		ReadFileCell(m_hFile, m_iDirOffset, 4);
		FileSeek(m_hFile, m_iDirOffset, SEEK_SET);
		ReadFileString(m_hFile, STRING(m_szPath));
	}

	new String:m_szMaterial[PLATFORM_MAX_PATH];
	for(m_iIdx=0;m_iIdx<m_iNum;++m_iIdx)
	{
		FileSeek(m_hFile, m_iOffset+m_iIdx*64, SEEK_SET);
		ReadFileCell(m_hFile, m_iNameOffset, 4);
		FileSeek(m_hFile, m_iNameOffset-4, SEEK_CUR);
		ReadFileString(m_hFile, STRING(m_szMaterial));

		Format(files[m_iIdx], maxlen2, "materials\\%s%s.vmt", m_szPath, m_szMaterial);
	}

	return m_iNum;
}

new String:g_szModelExts[][16] = {".phy", ".sw.vtx", ".dx80.vtx", ".dx90.vtx", ".vtx", ".xbox.vtx", ".vvd"};
stock Downloader_GetModelFiles(const String:model[], const String:internal[], String:files[][], maxsize, maxlen)
{
	decl String:m_szRawPath1[PLATFORM_MAX_PATH];
	decl String:m_szRawPath2[PLATFORM_MAX_PATH];
	strcopy(STRING(m_szRawPath1), model);
	Format(STRING(m_szRawPath2), "models/%s", internal);

	new m_iDot = FindCharInString(m_szRawPath1, '.', true);
	if(m_iDot == -1)
		return 0;
	m_szRawPath1[m_iDot] = 0;

	m_iDot = FindCharInString(m_szRawPath2, '.', true);
	if(m_iDot == -1)
		return 0;
	m_szRawPath2[m_iDot] = 0;

	new m_iNum = 0;
	for(new i=0;i<sizeof(g_szModelExts);++i)
	{
		if(m_iNum == maxsize)
			break;
		Format(files[m_iNum], maxlen, "%s%s", m_szRawPath1, g_szModelExts[i]);
		if(FileExists2(files[m_iNum]))
			++m_iNum;
		else
		{
			Format(files[m_iNum], maxlen, "%s%s", m_szRawPath2, g_szModelExts[i]);
			if(FileExists2(files[m_iNum]))
				++m_iNum;
		}
	}
	return m_iNum;
}

new String:g_szMaterialKeys[][64] = {"$baseTexture", "$bumpmap", "$lightwarptexture"};
stock Downloader_GetMaterialsFromVMT(const String:vmt[], String:materials[][], maxsize, maxlen)
{
	if(!FileExists2(vmt))
		return 0;

	decl String:m_szLine[512];

	new Handle:m_hFile = OpenFile2(vmt, "r");
	
	new bool:m_bFound[sizeof(g_szMaterialKeys)];
	decl m_iPos;
	decl m_iLast;
	new m_iNum = 0;
	while(ReadFileLine(m_hFile, m_szLine, sizeof(m_szLine))!=false)
	{
		if(m_iNum == sizeof(g_szMaterialKeys) || maxsize == m_iNum)
			break;

		for(new i=0;i<sizeof(g_szMaterialKeys);++i)
		{
			if(m_bFound[i])
				continue;
			if((m_iPos = StrContains(m_szLine, g_szMaterialKeys[i], false)) > 0)
			{
				m_bFound[i]=true;
				while(m_szLine[m_iPos] != '"' && m_szLine[m_iPos] != ' ' && m_szLine[m_iPos] != '	')
					++m_iPos;
				while(m_szLine[m_iPos] == ' ' || m_szLine[m_iPos] == '	' || m_szLine[m_iPos] == '"')
					++m_iPos;
				m_iLast = m_iPos;
				while(m_szLine[m_iLast] != '"' && m_szLine[m_iLast] != '\r' && m_szLine[m_iLast] != '\n' && m_szLine[m_iLast] != ' ' && m_szLine[m_iLast] != '	' && m_szLine[m_iLast] != 0)
					++m_iLast;
				m_szLine[m_iLast] = 0;
				strcopy(materials[m_iNum], maxlen, m_szLine[m_iPos]);
				++m_iNum;
			}
		}
	}

	CloseHandle(m_hFile);

	return m_iNum;
}

new Handle:g_hCachedFiles = INVALID_HANDLE;
new Handle:g_hCachedNums = INVALID_HANDLE;
stock Downloader_AddFileToDownloadsTable(const String:filename[])
{
	if(!FileExists2(filename))
		return 0;

	if(g_hCachedNums == INVALID_HANDLE)
	{
		g_hCachedNums = CreateTrie();
		g_hCachedFiles = CreateArray(PLATFORM_MAX_PATH);
	}

	AddFileToDownloadsTable(filename);

	decl m_iValue;
	if(GetTrieValue(g_hCachedNums, filename, m_iValue))
	{
		new m_iStart = FindStringInArray(g_hCachedFiles, filename)+1;
		decl String:m_szFile[PLATFORM_MAX_PATH];
		for(new i=m_iStart-m_iValue-1;i<m_iStart-1;++i)
		{
			if(i<0)
				break;
			GetArrayString(g_hCachedFiles, i, m_szFile, sizeof(m_szFile));
			AddFileToDownloadsTable(m_szFile);
		}
		return true;
	}

	decl String:m_szExt[16];
	new m_iDot = FindCharInString(filename, '.', true);
	if(m_iDot == -1)
		return true;

	new m_iNumFiles = 0;

	strcopy(m_szExt, sizeof(m_szExt), filename[m_iDot]);
	decl String:m_szMaterials[32][PLATFORM_MAX_PATH];
	decl m_iNum;
	if(strcmp(m_szExt, ".mdl") == 0)
	{
		decl String:m_szFiles[sizeof(g_szModelExts)][PLATFORM_MAX_PATH];
		new String:m_szInternal[64];

		m_iNum = Downloader_ParseMDL(filename, STRING(m_szInternal), m_szMaterials, sizeof(m_szMaterials), sizeof(m_szMaterials[]));
		for(new i=0;i<m_iNum;++i)
		{
			if(FileExists2(m_szMaterials[i]))
				m_iNumFiles += Downloader_AddFileToDownloadsTable(m_szMaterials[i])+1;
		}

		m_iNum = Downloader_GetModelFiles(filename, m_szInternal, m_szFiles, sizeof(m_szFiles), sizeof(m_szFiles[]));
		for(new i=0;i<m_iNum;++i)
			m_iNumFiles += Downloader_AddFileToDownloadsTable(m_szFiles[i])+1;
	} else if(strcmp(m_szExt, ".vmt") == 0)
	{
		m_iNum = Downloader_GetMaterialsFromVMT(filename, m_szMaterials, sizeof(m_szMaterials), sizeof(m_szMaterials[]));
		decl String:m_szMaterial[PLATFORM_MAX_PATH];
		for(new i=0;i<m_iNum;++i)
		{
			Format(m_szMaterial, sizeof(m_szMaterial), "materials\\%s.vtf", m_szMaterials[i]);
			if(FileExists2(m_szMaterial))
				m_iNumFiles += Downloader_AddFileToDownloadsTable(m_szMaterial)+1;
		}
	}

	PushArrayString(g_hCachedFiles, filename);
	SetTrieValue(g_hCachedNums, filename, m_iNumFiles);

	return m_iNumFiles;
}

new Handle:g_hCustomFiles = INVALID_HANDLE;

public CacheCustomDirectory()
{
	g_hCustomFiles = CreateTrie();

	new Handle:m_hDir = OpenDirectory("custom");
	if(m_hDir == INVALID_HANDLE)
		return;

	new String:m_szDirectory[PLATFORM_MAX_PATH] = "custom/";
	decl FileType:m_eType;
	new m_unLen = strlen(m_szDirectory);

	while(ReadDirEntry(m_hDir, m_szDirectory[m_unLen], sizeof(m_szDirectory)-m_unLen, m_eType))
	{
		if(m_eType != FileType_Directory)
			continue;

		if(strcmp(m_szDirectory[m_unLen], ".")==0 || strcmp(m_szDirectory[m_unLen], "..")==0)
			continue;

		CacheDirectory(m_szDirectory);
	}
	CloseHandle(m_hDir);
}

public CacheDirectory(const String:directory[])
{
	new Handle:m_hDir = OpenDirectory(directory);
	decl String:m_szPath[PLATFORM_MAX_PATH];
	decl FileType:m_eType;
	Format(STRING(m_szPath), "%s/", directory);
	new m_unLen = strlen(m_szPath);
	new m_unOffset = FindCharInString(m_szPath, '/')+1;
	m_unOffset += FindCharInString(m_szPath[m_unOffset], '/')+1;

	while(ReadDirEntry(m_hDir, m_szPath[m_unLen], sizeof(m_szPath)-m_unLen, m_eType))
	{
		if(strcmp(m_szPath[m_unLen], ".")==0 || strcmp(m_szPath[m_unLen], "..")==0)
			continue;

		if(m_eType == FileType_Directory)
			CacheDirectory(m_szPath);
		else if(m_eType == FileType_File)
		{
			SetTrieString(g_hCustomFiles, m_szPath[m_unOffset], m_szPath);
		}
	}
	CloseHandle(m_hDir);
}

stock Handle:OpenFile2(const String:file[], const String:mode[])
{
	if(g_hCustomFiles == INVALID_HANDLE)
		CacheCustomDirectory();

	decl String:m_szPath[PLATFORM_MAX_PATH];
	if(!GetTrieString(g_hCustomFiles, file, STRING(m_szPath)))
	{
		strcopy(STRING(m_szPath), file);
	}

	return OpenFile(m_szPath, mode);
}

stock bool:FileExists2(const String:file[])
{
	if(g_hCustomFiles == INVALID_HANDLE)
		CacheCustomDirectory();

	decl String:m_szPath[PLATFORM_MAX_PATH];
	if(!GetTrieString(g_hCustomFiles, file, STRING(m_szPath)))
		return FileExists(file);

	return FileExists(m_szPath);
}

stock bool:IsPluginLoaded(String:name[])
{
	new String:m_szName[PLATFORM_MAX_PATH];
	new Handle:pluginIterator = GetPluginIterator();
	while (MorePlugins(pluginIterator))
	{
		new Handle:currentPlugin = ReadPlugin(pluginIterator);
		GetPluginFilename(currentPlugin, STRING(m_szName));
		m_szName[strlen(m_szName)-4]=0;
		if(strcmp(name, m_szName)==0)
			return true;
	}
	return false;
}

stock PrecacheModel2(const String:model[], bool:preload=false)
{
	static m_unModelPrecache = INVALID_STRING_TABLE;
	//static m_unDynamicModel = INVALID_STRING_TABLE;
	//static m_unModelPrecacheMax = 0;
	//static m_unDynamicModelMax = 0;
	if(m_unModelPrecache == INVALID_STRING_TABLE)
	{
		m_unModelPrecache = FindStringTable("modelprecache");
		//m_unModelPrecacheMax = GetStringTableMaxStrings(m_unModelPrecache);
	}
	/*if(m_unDynamicModel == INVALID_STRING_TABLE)
	{
		m_unDynamicModel = FindStringTable("dynamicmodel");
		if(m_unDynamicModel == INVALID_STRING_TABLE)
			m_unDynamicModel = -2;
		else
			m_unDynamicModelMax = GetStringTableMaxStrings(m_unDynamicModel);
	}*/

	return PrecacheModel(model, preload);

	/*if(GetStringTableNumStrings(m_unModelPrecache)<m_unModelPrecacheMax)
	{
		return PrecacheModel(model, preload);
	}
	else if(m_unDynamicModel != -2)
	{
		new idx = PrecacheDynamicModel(model);
		return idx;
	}

	return 0;*/
}

stock StringTableContains(const table, const String:model[])
{
	new String:str[PLATFORM_MAX_PATH];
	for(new i=0;i<GetStringTableNumStrings(table);++i)
	{
		ReadStringTable(table, i, STRING(str));
		if(strcmp(str, model)==0)
			return i;
	}
	return 0;
}

stock GetLegacyAuthString(client, String:out[], maxlen, bool:validate=true)
{
	new String:m_szSteamID[32];
	new bool:success = GetClientAuthId(client, AuthId_Steam2, STRING(m_szSteamID), validate);

	if(m_szSteamID[0]=='[')
	{
		new m_unAccountID = StringToInt(m_szSteamID[5]);
		new m_unMod = m_unAccountID % 2;
		Format(out, maxlen, "STEAM_0:%d:%d", m_unMod, (m_unAccountID-m_unMod)/2);
	}
	else
	{
		strcopy(out, maxlen, m_szSteamID);
	}

	return success;
}

stock GetFriendID(client, bool:validate=true)
{
	new String:auth[32];
	GetLegacyAuthString(client, STRING(auth), validate);

	return ToAccountID(auth);
}

stock ToAccountID(String:auth[])
{
	if(strlen(auth)<11)
		return 0;
	return StringToInt(auth[10])*2 + auth[8]-48;
}