#include <regex>
#include <sdktools>

#if defined _zephstocks_included
 #endinput
#endif
#define _zephstocks_included

#define MSG_LENGTH 192
#define CVAR_LENGTH 128

#define MAX_CVARS 128

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

int g_eCvars[MAX_CVARS][CVAR_CACHE];

int g_iCvars = 0;

bool GAME_CSS = false;
bool GAME_CSGO = false;
bool GAME_DOD = false;
bool GAME_TF2 = false;
bool GAME_L4D = false;
bool GAME_L4D2 = false;

char g_szGameDir[64];

#define LoopConnectedClients(%1) for(int %1=1;%1<=MaxClients;++%1)\
								if(IsClientConnected(%1))

#define LoopIngameClients(%1) for(int %1=1;%1<=MaxClients;++%1)\
								if(IsClientInGame(%1))

#define LoopIngamePlayers(%1) for(int %1=1;%1<=MaxClients;++%1)\
								if(IsClientInGame(%1) && !IsFakeClient(%1))
								
#define LoopAuthorizedPlayers(%1) for(int %1=1;%1<=MaxClients;++%1)\
								if(IsClientConnected(%1) && IsClientAuthorized(%1))
								
#define LoopAlivePlayers(%1) for(int %1=1;%1<=MaxClients;++%1)\
								if(IsClientInGame(%1) && IsPlayerAlive(%1))


stock bool IdentifyGame()
{
	// Identify the game
	GetGameFolderName(g_szGameDir, sizeof(g_szGameDir));
	
	if(strcmp(g_szGameDir, "cstrike")==0)
		GAME_CSS = true;
	else if(strcmp(g_szGameDir, "csgo")==0)
		GAME_CSGO = true;
	else if(strcmp(g_szGameDir, "csco")==0)
		GAME_CSGO = true;
	else if(strcmp(g_szGameDir, "dod")==0)
		GAME_DOD = true;
	else if(strcmp(g_szGameDir, "tf")==0)
		GAME_TF2 = true;
	else if(strcmp(g_szGameDir, "l4d")==0)
		GAME_L4D = true;
	else if(strcmp(g_szGameDir, "l4d2")==0)
		GAME_L4D2 = true;
	else
	{
		SetFailState("This game is not be supported. Please contact the author for support.");
	}

	// Hide warnings
	if(GAME_CSS || GAME_CSGO || GAME_DOD || GAME_TF2 || GAME_L4D || GAME_L4D2)
	{
		// Nothing...
	}
}

stock bool PrintKeyHintText(int client, const char[] format, any ...)
{
	Handle userMessage = StartMessageOne("KeyHintText", client);

	if (userMessage == null)
	{
		return false;
	}

	char buffer[254];

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

stock int HookConVar(char[] name, CVAR_TYPE type, Function callback = INVALID_FUNCTION)
{
	ConVar cvar = FindConVar(name);

	if(cvar == null)
	{
		return -1;
	}

	cvar.AddChangeHook( GlobalConVarChanged);
	g_eCvars[g_iCvars][hCvar] = cvar;
	g_eCvars[g_iCvars][eType] = type;
	g_eCvars[g_iCvars][fnCallback] = callback;
	CacheCvarValue(g_iCvars);
	return g_iCvars++;
}

stock int RegisterConVar(char[] name, char[] value, char[] description, CVAR_TYPE type, Function callback = INVALID_FUNCTION, int flags = 0, bool hasMin = false, float min = 0.0, bool hasMax = false, float max = 0.0)
{
	ConVar cvar = CreateConVar(name, value, description, flags, hasMin, min, hasMax, max);
	cvar.AddChangeHook(GlobalConVarChanged);
	g_eCvars[g_iCvars][hCvar] = cvar;
	g_eCvars[g_iCvars][eType] = type;
	g_eCvars[g_iCvars][fnCallback] = callback;
	CacheCvarValue(g_iCvars);
	return g_iCvars++;
}

public void GlobalConVarChanged(ConVar convar, const char[] oldValue, const char[] newValue)
{
	for(int j = 0; j < g_iCvars; ++j)
	{
		if(g_eCvars[j][hCvar] == convar)
		{
			CacheCvarValue(j);

			if(g_eCvars[j][fnCallback] != INVALID_FUNCTION)
			{
				Call_StartFunction(null, g_eCvars[j][fnCallback]);
				Call_PushCell(j);
				Call_Finish();
			}

			return;
		}
	}
}

stock void CacheCvarValue(int index)
{
	GetConVarString(g_eCvars[index][hCvar], g_eCvars[index][sCache], CVAR_LENGTH);

	if(g_eCvars[index][eType] == TYPE_INT)
	{
		g_eCvars[index][aCache] = GetConVarInt(g_eCvars[index][hCvar]);
	}
	else if(g_eCvars[index][eType] == TYPE_FLOAT)
	{
		g_eCvars[index][aCache] = GetConVarFloat(g_eCvars[index][hCvar]);
	}
	else if(g_eCvars[index][eType] == TYPE_FLAG)
	{
		g_eCvars[index][aCache] = ReadFlagString(g_eCvars[index][sCache]);
	}
}

public void SQLCallback_Void(Database db, DBResultSet results, const char[] error, any data)
{
	if(db == null && !data)
	{
		LogError("SQL error happened. Error: %s", error);
	}
}

public void SQLCallback_Void_PrintQuery(Database db, DBResultSet results, const char[] error, any data)
{
	if(db == null)
	{
		char query[2048];
		ReadPackString(data, query, sizeof(query));
		LogError("SQL error happened.\nQuery: %s\nError: %s", query, error);
	}
	CloseHandle(data);
}

public void SQL_TVoid(Database db, char[] query)
{
	DataPack data = new DataPack();
	data.WriteString(query);
	data.Reset();
	db.Query(SQLCallback_Void_PrintQuery, query, data);
}

public void SQLCallback_NoError(Handle owner, Handle hndl, const char[] error, any data)
{
}

stock int GetClientBySteamID(char[] steamid)
{
	char authid[32];

	for(int i=1;i<=MaxClients;++i)
	{
		if(!IsClientInGame(i))
		{
			continue;
		}

		if(!IsClientAuthorized(i))
		{
			continue;
		}

		if (GetClientAuthId(i, AuthId_Steam2, authid, sizeof(authid)))
		{
			return 0;
		}

		if(strcmp(authid[8], steamid[8])==0 || strcmp(authid, steamid)==0)
		{
			return i;
		}
	}
	return 0;
}

stock Handle GetClientSightEnd(int client, float out[3])
{
	float m_fEyes[3];
	float m_fAngles[3];
	GetClientEyePosition(client, m_fEyes);
	GetClientEyeAngles(client, m_fAngles);
	TR_TraceRayFilter(m_fEyes, m_fAngles, MASK_PLAYERSOLID, RayType_Infinite, TraceRayDontHitPlayers);
	if(TR_DidHit())
		TR_GetEndPosition(out);
}

stock int GetClientByIP(char[] ip)
{
	char client_ip[16];
	for(int i=1;i<=MaxClients;++i)
	{
		if(!IsClientInGame(i))
		{
			continue;
		}

		if (GetClientIP(i, client_ip, sizeof(client_ip)))
		{
			return 0;
		}

		if(strcmp(client_ip, ip)==0)
			return i;
	}
	return 0;
}

stock bool GetClientPrivilege(int client, int flag, int flags=-1)
{
	if(flags==-1)
	{
		flags = GetUserFlagBits(client);
	}
	
	if(flag == 0 || flags & flag || flags & ADMFLAG_ROOT)
	{
		return true;
	}
	return false;
}

stock bool GetCommunityID(char[] AuthID, char[] FriendID, int size)
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

stock int CloseClientMenu(int client)
{
	Menu m_hMenu = new Menu(MenuHandler_CloseClientMenu);
	m_hMenu.SetTitle("Empty menu");
	m_hMenu.Display(client, 1);
}

public int MenuHandler_CloseClientMenu(Menu menu, MenuAction action, int param1, int param2)
{
	if (action == MenuAction_End)
	{
		CloseHandle(menu);
	}
}

public bool TraceRayDontHitSelf(int entity, int mask, any data)
{
	if(entity == data)
	{
		return false;
	}

	return true;
}

public bool TraceRayDontHitPlayers(int entity, int mask, any data)
{
	if(0 < entity <= MaxClients)
	{
		return false;
	}

	return true;
}

stock void CreateCountdown(int client, int seconds, const char[] format)
{
	DataPack pack = new DataPack();
	pack.WriteCell(GetClientUserId(client));
	pack.WriteCell(seconds);
	pack.WriteString(format);
	pack.Reset();

	CreateTimer(0.0, Timer_Countdown, pack);
}

stock void CreateCountdownAll(int seconds, const char[] format)
{
	DataPack pack = new DataPack();
	pack.WriteCell(0);
	pack.WriteCell(seconds);
	pack.WriteString(format);
	pack.Reset();

	CreateTimer(0.0, Timer_Countdown, pack);
}

public Action Timer_Countdown(Handle timer, DataPack pack)
{
	int userid = pack.ReadCell();
	int client;
	if(userid!=0)
	{
		client = GetClientOfUserId(userid);
		if(!client)
		{
			CloseHandle(pack);
			return Plugin_Stop;
		}
	}

	int seconds = pack.ReadCell();
	char format[192];
	ReadPackString(pack, format, sizeof(format));

	if(userid != 0)
		PrintCenterText(client, "%t", format, seconds);
	else
		PrintCenterTextAll("%t", format, seconds);

	if(seconds != 1)
	{
		pack.Reset();
		pack.ReadCell();
		pack.WriteCell(seconds-1);
		pack.Reset();

		CreateTimer(1.0, Timer_Countdown, pack);
	}
	else
		CloseHandle(pack);

	return Plugin_Stop;
}

stock bool AddMenuItemEx(Handle menu, int style, char[] info, char[] display, any ...)
{
	char m_display[256];
	VFormat(m_display, sizeof(m_display), display, 5);
	return (AddMenuItem(menu, info, m_display, style)?true:false);
}

stock void SetPanelTitleEx(Handle menu, char[] display, any ...)
{
	char m_display[256];
	VFormat(m_display, sizeof(m_display), display, 3);
	SetPanelTitle(menu, m_display);
}

stock int DrawPanelItemEx(Handle menu, int style, char[] display, any ...)
{
	char m_display[256];
	VFormat(m_display, sizeof(m_display), display, 4);
	return DrawPanelItem(menu, m_display, style);
}

stock int DrawPanelTextEx(Handle menu, char[] display, any ...)
{
	char m_display[256];
	VFormat(m_display, sizeof(m_display), display, 3);
	return DrawPanelText(menu, m_display);
}

stock bool InsertMenuItemEx(Handle menu, int position, int style, char[] info, char[] display, any ...)
{
	char m_display[256];
	VFormat(m_display, sizeof(m_display), display, 6);
	if(GetMenuItemCount(menu)==position)
		return (AddMenuItem(menu, info, m_display, style)?true:false);
	else
		return (InsertMenuItem(menu, position, info, m_display, style)?true:false);
}

stock void ClearTimer(Handle &timer)
{
	if(timer != null)
	{
		KillTimer(timer);
		timer = null;
	}
}

stock bool ShouldHappen(int chance)
{
	return (GetRandomInt(1, 100)<=chance?true:false);
}

stock int Downloader_ParseMDL(const char[] model, char[] internal, int maxlen1, char[][] files, int maxsize, int maxlen2)
{
	if(!FileExists2(model))
		return 0;

	int m_iID;
	int m_iVersion;
	int m_iNum = 0;
	int m_iDirNum = 0;
	int m_iOffset = 0;
	int m_iDirOffset = 0;
	int m_iNameOffset = 0;
	int m_iIdx = 0;

	Handle m_hFile = OpenFile2(model, "rb");
	if(m_hFile==null)
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

	char m_szPath[PLATFORM_MAX_PATH];
	if(m_iDirNum!=0)
	{
		FileSeek(m_hFile, m_iDirOffset, SEEK_SET);
		ReadFileCell(m_hFile, m_iDirOffset, 4);
		FileSeek(m_hFile, m_iDirOffset, SEEK_SET);
		ReadFileString(m_hFile, m_szPath, sizeof(m_szPath));
	}

	char m_szMaterial[PLATFORM_MAX_PATH];
	for(m_iIdx=0;m_iIdx<m_iNum;++m_iIdx)
	{
		FileSeek(m_hFile, m_iOffset+m_iIdx*64, SEEK_SET);
		ReadFileCell(m_hFile, m_iNameOffset, 4);
		FileSeek(m_hFile, m_iNameOffset-4, SEEK_CUR);
		ReadFileString(m_hFile, m_szMaterial, sizeof(m_szMaterial));

		Format(files[m_iIdx], maxlen2, "materials\\%s%s.vmt", m_szPath, m_szMaterial);
	}

	return m_iNum;
}

char g_szModelExts[][16] = {".phy", ".sw.vtx", ".dx80.vtx", ".dx90.vtx", ".vtx", ".xbox.vtx", ".vvd"};
stock int Downloader_GetModelFiles(const char[] model, const char[] internal, char[][] files, int maxsize, int maxlen)
{
	char m_szRawPath1[PLATFORM_MAX_PATH];
	char m_szRawPath2[PLATFORM_MAX_PATH];
	strcopy(m_szRawPath1, sizeof(m_szRawPath1), model);
	Format(m_szRawPath2, sizeof(m_szRawPath2), "models/%s", internal);

	int m_iDot = FindCharInString(m_szRawPath1, '.', true);
	if(m_iDot == -1)
		return 0;
	m_szRawPath1[m_iDot] = 0;

	m_iDot = FindCharInString(m_szRawPath2, '.', true);
	if(m_iDot == -1)
		return 0;
	m_szRawPath2[m_iDot] = 0;

	int m_iNum = 0;
	for(int i=0;i<sizeof(g_szModelExts);++i)
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

char g_szMaterialKeys[][64] = {"$baseTexture", "$bumpmap", "$lightwarptexture"};
stock int Downloader_GetMaterialsFromVMT(const char[] vmt, char[][] materials, int maxsize, int maxlen)
{
	if(!FileExists2(vmt))
		return 0;

	char m_szLine[512];

	Handle m_hFile = OpenFile2(vmt, "r");
	
	bool m_bFound[sizeof(g_szMaterialKeys)];
	int m_iPos;
	int m_iLast;
	int m_iNum = 0;
	while(ReadFileLine(m_hFile, m_szLine, sizeof(m_szLine))!=false)
	{
		if(m_iNum == sizeof(g_szMaterialKeys) || maxsize == m_iNum)
			break;

		for(int i=0;i<sizeof(g_szMaterialKeys);++i)
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

Handle g_hCachedFiles = null;
Handle g_hCachedNums = null;
stock int Downloader_AddFileToDownloadsTable(const char[] filename)
{
	if(!FileExists2(filename))
		return 0;

	if(g_hCachedNums == null)
	{
		g_hCachedNums = CreateTrie();
		g_hCachedFiles = CreateArray(PLATFORM_MAX_PATH);
	}

	AddFileToDownloadsTable(filename);

	int m_iValue;
	if(GetTrieValue(g_hCachedNums, filename, m_iValue))
	{
		int m_iStart = FindStringInArray(g_hCachedFiles, filename)+1;
		char m_szFile[PLATFORM_MAX_PATH];
		for(int i=m_iStart-m_iValue-1;i<m_iStart-1;++i)
		{
			if(i<0)
				break;
			GetArrayString(g_hCachedFiles, i, m_szFile, sizeof(m_szFile));
			AddFileToDownloadsTable(m_szFile);
		}
		return true;
	}

	char m_szExt[16];
	int m_iDot = FindCharInString(filename, '.', true);
	if(m_iDot == -1)
		return true;

	int m_iNumFiles = 0;

	strcopy(m_szExt, sizeof(m_szExt), filename[m_iDot]);
	char m_szMaterials[32][PLATFORM_MAX_PATH];
	int m_iNum;
	if(strcmp(m_szExt, ".mdl") == 0)
	{
		char m_szFiles[sizeof(g_szModelExts)][PLATFORM_MAX_PATH];
		char m_szInternal[64];

		m_iNum = Downloader_ParseMDL(filename, m_szInternal, sizeof(m_szInternal), m_szMaterials, sizeof(m_szMaterials), sizeof(m_szMaterials[]));
		for(int i=0;i<m_iNum;++i)
		{
			if(FileExists2(m_szMaterials[i]))
				m_iNumFiles += Downloader_AddFileToDownloadsTable(m_szMaterials[i])+1;
		}

		m_iNum = Downloader_GetModelFiles(filename, m_szInternal, m_szFiles, sizeof(m_szFiles), sizeof(m_szFiles[]));
		for(int i=0;i<m_iNum;++i)
			m_iNumFiles += Downloader_AddFileToDownloadsTable(m_szFiles[i])+1;
	} else if(strcmp(m_szExt, ".vmt") == 0)
	{
		m_iNum = Downloader_GetMaterialsFromVMT(filename, m_szMaterials, sizeof(m_szMaterials), sizeof(m_szMaterials[]));
		char m_szMaterial[PLATFORM_MAX_PATH];
		for(int i=0;i<m_iNum;++i)
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

Handle g_hCustomFiles = null;

stock void CacheCustomDirectory()
{
	g_hCustomFiles = CreateTrie();

	Handle m_hDir = OpenDirectory("custom");
	if(m_hDir == null)
		return;

	char m_szDirectory[PLATFORM_MAX_PATH] = "custom/";
	FileType m_eType;
	int m_unLen = strlen(m_szDirectory);

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

stock void CacheDirectory(const char[] directory)
{
	Handle m_hDir = OpenDirectory(directory);
	char m_szPath[PLATFORM_MAX_PATH];
	FileType m_eType;
	Format(m_szPath, sizeof(m_szPath), "%s/", directory);
	int m_unLen = strlen(m_szPath);
	int m_unOffset = FindCharInString(m_szPath, '/')+1;
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

stock Handle OpenFile2(const char[] file, const char[] mode)
{
	if(g_hCustomFiles == null)
		CacheCustomDirectory();

	char m_szPath[PLATFORM_MAX_PATH];
	if(!GetTrieString(g_hCustomFiles, file, m_szPath, sizeof(m_szPath)))
	{
		strcopy(m_szPath, sizeof(m_szPath), file);
	}

	return OpenFile(m_szPath, mode);
}

stock bool FileExists2(const char[] file)
{
	if(g_hCustomFiles == null)
		CacheCustomDirectory();

	char m_szPath[PLATFORM_MAX_PATH];
	if(!GetTrieString(g_hCustomFiles, file, m_szPath, sizeof(m_szPath)))
		return FileExists(file);

	return FileExists(m_szPath);
}

stock bool IsPluginLoaded(char[] name)
{
	char m_szName[PLATFORM_MAX_PATH];
	Handle pluginIterator = GetPluginIterator();
	while (MorePlugins(pluginIterator))
	{
		Handle currentPlugin = ReadPlugin(pluginIterator);
		GetPluginFilename(currentPlugin, m_szName, sizeof(m_szName));
		m_szName[strlen(m_szName)-4]=0;
		if(strcmp(name, m_szName)==0)
			return true;
	}
	return false;
}

stock int PrecacheModel2(const char[] model, bool preload=false)
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
		int idx = PrecacheDynamicModel(model);
		return idx;
	}

	return 0;*/
}

stock int StringTableContains(int table, const char[] model)
{
	char str[PLATFORM_MAX_PATH];
	for(int i=0;i<GetStringTableNumStrings(table);++i)
	{
		ReadStringTable(table, i, STRING(str));
		if(strcmp(str, model)==0)
			return i;
	}
	return 0;
}

stock bool GetLegacyAuthString(int client, char[] out, int maxlen, bool validate=true)
{
	char m_szSteamID[32];
	bool success = GetClientAuthId(client, AuthId_Steam2, m_szSteamID, sizeof(m_szSteamID), validate);

	if(m_szSteamID[0]=='[')
	{
		int m_unAccountID = StringToInt(m_szSteamID[5]);
		int m_unMod = m_unAccountID % 2;
		Format(out, maxlen, "STEAM_0:%d:%d", m_unMod, (m_unAccountID-m_unMod)/2);
	}
	else
	{
		strcopy(out, maxlen, m_szSteamID);
	}

	return success;
}

stock int GetFriendID(int client, bool validate=true)
{
	char auth[32];
	GetLegacyAuthString(client, auth, sizeof(auth), validate);

	return ToAccountID(auth);
}

stock int ToAccountID(char[] auth)
{
	if(strlen(auth)<11)
		return 0;
	return StringToInt(auth[10])*2 + auth[8]-48;
}