#include "../main.h"
#include "../game/game.h"
#include "../net/netgame.h"
#include "../chatwindow.h"
#include "../dialog.h"
#include "../net/admin-bot.h"
#include "../gui/gui.h"
#include "../gui/interface.h"
#include "../gui/buttons.h"
#include "../gui/gamescreen.h"
#include "../jniutil.h"

extern CGame *pGame;
extern CNetGame *pNetGame;
extern CChatWindow *pChatWindow;
extern CDialogWindow *pDialogWindow;
extern CAdminBot *pAdminBot;
extern CGUI *pGUI;
extern CGameScreen* pGameScreen;

extern CJavaWrapper *pJavaWrapper;

int iNetModeNormalOnfootSendRate	= NETMODE_ONFOOT_SENDRATE;
int iNetModeNormalInCarSendRate		= NETMODE_INCAR_SENDRATE;
int iNetModeFiringSendRate			= NETMODE_FIRING_SENDRATE;
int iNetModeSendMultiplier 			= NETMODE_SEND_MULTIPLIER;

#define EVENT_TYPE_PAINTJOB			1
#define EVENT_TYPE_CARCOMPONENT		2
#define EVENT_TYPE_CARCOLOR			3
#define EVENT_ENTEREXIT_MODSHOP		4

bool verify = false;

uint16_t moto_models[] = {448,461,462,463,468,471,521,522,581,586};

void ProcessIncomingEvent(PLAYERID playerID, int event, uint32_t param1, uint32_t param2, uint32_t param3)
{
	uint32_t v;
	int iVehicleID;
	int iPaintJob;
	int iComponent;
	int iWait;
	CVehicle *pVehicle;
	CRemotePlayer *pRemote;

	if(!pNetGame) return;
	
	CVehiclePool *pVehiclePool = pNetGame->GetVehiclePool();
	
	uint16_t veh_model = pVehiclePool->GetAt(param1)->GetModelIndex();
	for(uint16_t i = 0; i < 10; i++)
	{
		if(veh_model == moto_models[i])
			return;
	}

	switch(event)
	{
		case EVENT_TYPE_PAINTJOB:
		iVehicleID = pVehiclePool->FindGtaIDFromID(param1);
		iPaintJob = (int)param2;

		if(iPaintJob < 0 || iPaintJob > 2)
				return;

		if(iVehicleID) ScriptCommand(&change_car_skin, iVehicleID, param2);
		break;
		case EVENT_TYPE_CARCOMPONENT:
		iVehicleID = pVehiclePool->FindGtaIDFromID(param1);
		iComponent = (int)param2;

		if(iComponent < 1000 || iComponent > 1193)
			break;

		//pGame->RequestModel(iComponent);
		//pGame->LoadRequestedModels();
		(( int (*)(int,int))(g_libGTASA+0x28EB10+1))(iComponent,1); //RequestModel
		(( int (*)(int,int))(g_libGTASA+0x28F2F8+1))(iComponent,0); //RequestVehicleUpgrade
		(( int (*)(bool))(g_libGTASA+0x294CB4+1))(0); //LoadAllRequestedModels
		//ScriptCommand(&request_car_component, iComponent);

		if(iComponent > 1000 && iComponent < 1193)
			ScriptCommand(&add_car_component, iVehicleID, iComponent, &v);
		
		break;
		case EVENT_TYPE_CARCOLOR:
		pVehicle = pVehiclePool->GetAt((VEHICLEID)param1);
		if(pVehicle) pVehicle->SetColor((int)param2, (int)param3);
		break;
	}
}

void InitGame(RPCParameters *rpcParams)
{
	Log("RPC: InitGame");

	unsigned char * Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;

	PLAYERID MyPlayerID;
	bool bLanMode, bStuntBonus;

	RakNet::BitStream bsInitGame(Data,(iBitLength/8)+1,false);

	bsInitGame.ReadCompressed(pNetGame->m_bZoneNames);							// unknown
	bsInitGame.ReadCompressed(pNetGame->m_bUseCJWalk);							// native UsePlayerPedAnims(); +
	bsInitGame.ReadCompressed(pNetGame->m_bAllowWeapons);						// native AllowInteriorWeapons(allow); +
	bsInitGame.ReadCompressed(pNetGame->m_bLimitGlobalChatRadius);				// native LimitGlobalChatRadius(Float:chat_radius); +
	bsInitGame.Read(pNetGame->m_fGlobalChatRadius);								// +
	
	bsInitGame.ReadCompressed(bStuntBonus);										// native EnableStuntBonusForAll(enable); +
	
	bsInitGame.Read(pNetGame->m_fNameTagDrawDistance);							// native SetNameTagDrawDistance(Float:distance); +
	bsInitGame.ReadCompressed(pNetGame->m_bDisableEnterExits);					// native DisableInteriorEnterExits(); +
	bsInitGame.ReadCompressed(pNetGame->m_bNameTagLOS);							// native DisableNameTagLOS(); +
	bsInitGame.ReadCompressed(pNetGame->m_bManualVehicleEngineAndLight);		// native ManualVehicleEngineAndLights(); +
	bsInitGame.Read(pNetGame->m_iSpawnsAvailable);								// +
	
	bsInitGame.Read(MyPlayerID);												// 
	bsInitGame.ReadCompressed(pNetGame->m_bShowPlayerTags);						// native ShowNameTags(show); +
	bsInitGame.Read(pNetGame->m_iShowPlayerMarkers);							// native ShowPlayerMarkers(mode); +
	bsInitGame.Read(pNetGame->m_byteWorldTime);									// native SetWorldTime(hour); +
	bsInitGame.Read(pNetGame->m_byteWeather);									// native SetWeather(weatherid); +
	bsInitGame.Read(pNetGame->m_fGravity);										// native SetGravity(Float:gravity); +
	bsInitGame.ReadCompressed(bLanMode);										// 
	bsInitGame.Read(pNetGame->m_iDeathDropMoney);								// native SetDeathDropAmount(amount); +
	bsInitGame.ReadCompressed(pNetGame->m_bInstagib);							// always 0

	bsInitGame.Read(iNetModeNormalOnfootSendRate);
	bsInitGame.Read(iNetModeNormalInCarSendRate);
	bsInitGame.Read(iNetModeFiringSendRate);
	bsInitGame.Read(iNetModeSendMultiplier);

	bsInitGame.Read(pNetGame->m_iLagCompensation);								// lagcomp +

	uint8_t byteStrLen;
	bsInitGame.Read(byteStrLen);
	if(byteStrLen)																// SetGameModeText(); +
	{
		memset(pNetGame->m_szHostName, 0, sizeof(pNetGame->m_szHostName));
		bsInitGame.Read(pNetGame->m_szHostName, byteStrLen);
	}
	pNetGame->m_szHostName[byteStrLen] = '\0';

	uint8_t byteVehicleModels[212];
	bsInitGame.Read((char*)&byteVehicleModels[0], 212);							// don't use?
	bsInitGame.Read(pNetGame->m_iVehicleFriendlyFire);							// native EnableVehicleFriendlyFire(); +

	CPlayerPool *pPlayerPool = pNetGame->GetPlayerPool();
	CLocalPlayer *pLocalPlayer = nullptr;
	if(pPlayerPool) pLocalPlayer = pPlayerPool->GetLocalPlayer();

	if (pGame) {
		pGame->DisableAutoAim();
	}

	pGame->SetGravity(pNetGame->m_fGravity);

	if(pNetGame->m_bDisableEnterExits)
		pGame->DisableInteriorEnterExits();

	pNetGame->SetGameState(GAMESTATE_CONNECTED);
	if(pLocalPlayer) pLocalPlayer->HandleClassSelection();

	pGame->SetWorldWeather(pNetGame->m_byteWeather);
	pGame->ToggleCJWalk(pNetGame->m_bUseCJWalk);

	//if(pChatWindow) pChatWindow->AddDebugMessage("Connected to {B9C9BF}%.64s", pNetGame->m_szHostName);
	if(pChatWindow) pChatWindow->AddChatMessage(nullptr, 0xffffffff, "���������� � Real Russia !");
	
}

void ServerJoin(RPCParameters *rpcParams)
{
	Log("RPC: ServerJoin");
	unsigned char * Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;

	char szPlayerName[MAX_PLAYER_NAME+1];
	PLAYERID playerId;
	unsigned char byteNameLen = 0;
	uint8_t bIsNPC = 0;
	int pading;

	CPlayerPool *pPlayerPool = pNetGame->GetPlayerPool();

	RakNet::BitStream bsData(Data,(iBitLength/8)+1,false);

	bsData.Read(playerId);
	bsData.Read(pading);
	bsData.Read(bIsNPC);

	bsData.Read(byteNameLen);
	bsData.Read(szPlayerName, byteNameLen);
	szPlayerName[byteNameLen] = '\0';

	if(pPlayerPool)
		pPlayerPool->New(playerId, szPlayerName, bIsNPC);
}

void ServerQuit(RPCParameters *rpcParams)
{
	Log("RPC: ServerQuit");
	unsigned char * Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;

	PLAYERID playerId;
	uint8_t byteReason;

	RakNet::BitStream bsData(Data,(iBitLength/8)+1,false);

	bsData.Read(playerId);
	bsData.Read(byteReason);

	CPlayerPool *pPlayerPool = pNetGame->GetPlayerPool();
	if(pPlayerPool)
		pPlayerPool->Delete(playerId, byteReason);
}

void ClientMessage(RPCParameters *rpcParams)
{
	Log("RPC: ClientMessage");
	unsigned char* Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;

	RakNet::BitStream bsData((unsigned char*)Data,(iBitLength/8)+1,false);
	uint32_t dwStrLen;
	uint32_t dwColor;

	bsData.Read(dwColor);
	bsData.Read(dwStrLen);
	char* szMsg = (char*)malloc(dwStrLen+1);
	bsData.Read(szMsg, dwStrLen);
	szMsg[dwStrLen] = 0;

	char utf8text[256];

	cp1251_to_utf8(utf8text, szMsg);

	pChatWindow->AddClientMessage(dwColor, szMsg);

	free(szMsg);
}

void Chat(RPCParameters *rpcParams)
{
	Log("RPC: Chat");
	unsigned char* Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;

	RakNet::BitStream bsData((unsigned char*)Data,(iBitLength/8)+1,false);
	PLAYERID playerId;
	uint8_t byteTextLen;

	if(pNetGame->GetGameState() != GAMESTATE_CONNECTED)	return;

	unsigned char szText[256];
	memset(szText, 0, 256);

	bsData.Read(playerId);
	bsData.Read(byteTextLen);
	bsData.Read((char*)szText,byteTextLen);

	szText[byteTextLen] = '\0';

	char utf8text[256];

	cp1251_to_utf8(utf8text, (char*)szText);
	//pAdminBot->CheckForCommand((char*)szText);

	CPlayerPool* pPlayerPool = pNetGame->GetPlayerPool();
	if (playerId == pPlayerPool->GetLocalPlayerID())
	{
		CLocalPlayer *pLocalPlayer = pPlayerPool->GetLocalPlayer();
		if (pLocalPlayer) 
		{
			pChatWindow->AddChatMessage(pPlayerPool->GetLocalPlayerName(),
			pLocalPlayer->GetPlayerColor(), (char*)szText);
		}
	} 
	else 
	{
		CRemotePlayer *pRemotePlayer = pPlayerPool->GetAt(playerId);
		if(pRemotePlayer)
			pRemotePlayer->Say(szText);
	}
}

void RequestClass(RPCParameters *rpcParams)
{
	Log("RPC: RequestClass");

	unsigned char * Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;

	RakNet::BitStream bsData((unsigned char*)Data,(iBitLength/8)+1,false);
	uint8_t byteRequestOutcome = 0;
	PLAYER_SPAWN_INFO SpawnInfo;

	CPlayerPool *pPlayerPool = pNetGame->GetPlayerPool();
	CLocalPlayer *pLocalPlayer = 0;

	if(pPlayerPool) pLocalPlayer = pPlayerPool->GetLocalPlayer();

	bsData.Read(byteRequestOutcome);
	bsData.Read((char*)&SpawnInfo, sizeof(PLAYER_SPAWN_INFO));

	if(pLocalPlayer)
	{
		if(byteRequestOutcome)
		{
			pLocalPlayer->SetSpawnInfo(&SpawnInfo);
			pLocalPlayer->HandleClassSelectionOutcome();
		}
	}
}

void Weather(RPCParameters *rpcParams)
{
	Log("RPC: Weather");

	unsigned char * Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;

	RakNet::BitStream bsData((unsigned char*)Data,(iBitLength/8)+1,false);
	uint8_t byteWeather;
	bsData.Read(byteWeather);
	pNetGame->m_byteWeather = byteWeather;
	pGame->SetWorldWeather(byteWeather);
}

void RequestSpawn(RPCParameters *rpcParams)
{
	Log("RPC: RequestSpawn");

	unsigned char * Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;

	uint8_t byteRequestOutcome = 0;
	RakNet::BitStream bsData((unsigned char*)Data,(iBitLength/8)+1,false);
	bsData.Read(byteRequestOutcome);

	CPlayerPool *pPlayerPool = pNetGame->GetPlayerPool();
	CLocalPlayer *pLocalPlayer = 0;
	if(pPlayerPool) pLocalPlayer = pPlayerPool->GetLocalPlayer();

	if(pLocalPlayer)
	{
		if(byteRequestOutcome == 2 || (byteRequestOutcome && pLocalPlayer->m_bWaitingForSpawnRequestReply))
			pLocalPlayer->Spawn();
		else
			pLocalPlayer->m_bWaitingForSpawnRequestReply = false;
	}
}

void WorldTime(RPCParameters *rpcParams)
{
	Log("RPC: WorldTime");
	unsigned char * Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;

	RakNet::BitStream bsData((unsigned char*)Data,(iBitLength/8)+1,false);
	uint8_t byteWorldTime;
	bsData.Read(byteWorldTime);
	pNetGame->m_byteWorldTime = byteWorldTime;
}

void SetTimeEx(RPCParameters *rpcParams)
{
	Log("RPC: SetTimeEx");

	unsigned char * Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;

	RakNet::BitStream bsData((unsigned char*)Data,(iBitLength/8)+1,false);

	uint8_t byteHour;
	uint8_t byteMinute;
	bsData.Read(byteHour);
	bsData.Read(byteMinute);

	pGame->SetWorldTime(byteHour, byteMinute);
	pNetGame->m_byteWorldTime = byteHour;
	pNetGame->m_byteWorldMinute = byteMinute;
}

void WorldPlayerAdd(RPCParameters *rpcParams)
{
	Log("RPC: WorldPlayerAdd");

	unsigned char * Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;

	RakNet::BitStream bsData(Data,(iBitLength/8)+1,false);
	CRemotePlayer *pRemotePlayer;
	CPlayerPool *pPlayerPool = pNetGame->GetPlayerPool();

	PLAYERID playerId;
	uint8_t byteFightingStyle=4;
	uint8_t byteTeam=0;
	unsigned int iSkin=0;
	VECTOR vecPos;
	float fRotation=0;
	uint32_t dwColor=0;
	bool bVisible;

	bsData.Read(playerId);
	bsData.Read(byteTeam);
	bsData.Read(iSkin);
	bsData.Read(vecPos.X);
	bsData.Read(vecPos.Y);
	bsData.Read(vecPos.Z);
	bsData.Read(fRotation);
	bsData.Read(dwColor);
	bsData.Read(byteFightingStyle);
	bsData.Read(bVisible);

	if(pPlayerPool)
	{
		CRemotePlayer* pRemotePlayer = pPlayerPool->GetAt(playerId);
		if(pRemotePlayer) 
			pRemotePlayer->Spawn(byteTeam, iSkin, &vecPos, fRotation, dwColor, byteFightingStyle, bVisible);
	}
}

void WorldPlayerRemove(RPCParameters *rpcParams)
{
	Log("RPC: WorldPlayerRemove");

	unsigned char * Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;

	RakNet::BitStream bsData(Data,(iBitLength/8)+1,false);
	PLAYERID playerId;
	CPlayerPool *pPlayerPool = pNetGame->GetPlayerPool();
	bsData.Read(playerId);

	if(pPlayerPool)
	{
		CRemotePlayer* pRemotePlayer = pPlayerPool->GetAt(playerId);
		if(pRemotePlayer) 
			pRemotePlayer->Remove();
	}
}

void SetCheckpoint(RPCParameters *rpcParams)
{
	Log("RPC: SetCheckpoint");

	unsigned char * Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;

	RakNet::BitStream bsData(Data,(iBitLength/8)+1,false);
	float fX, fY, fZ, fSize;

	bsData.Read(fX);
	bsData.Read(fY);
	bsData.Read(fZ);
	bsData.Read(fSize);

	VECTOR pos, Extent;

	pos.X = fX;
	pos.Y = fY;
	pos.Z = fZ;
	Extent.X = fSize;
	Extent.Y = fSize;
	Extent.Z = fSize;

	pGame->SetCheckpointInformation(&pos, &Extent);
	pGame->ToggleCheckpoints(true);
}

void DisableCheckpoint(RPCParameters *rpcParams)
{
	Log("RPC: DisableCheckpoint");

	pGame->ToggleCheckpoints(false);
}

void SetRaceCheckpoint(RPCParameters *rpcParams)
{
	Log("RPC: SetRaceCheckpoint");

	unsigned char * Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;

	RakNet::BitStream bsData(Data,(iBitLength/8)+1,false);
	float fX,fY,fZ;
	uint8_t byteType;
	VECTOR pos, next;

	bsData.Read(byteType);
	bsData.Read(fX);
	bsData.Read(fY);
	bsData.Read(fZ);
	pos.X = fX;
	pos.Y = fY;
	pos.Z = fZ;

	bsData.Read(fX);
	bsData.Read(fY);
	bsData.Read(fZ);
	next.X = fX;
	next.Y = fY;
	next.Z = fZ;

	bsData.Read(fX);

	pGame->SetRaceCheckpointInformation(byteType, &pos, &next, fX);
	pGame->ToggleRaceCheckpoints(true);
}

void DisableRaceCheckpoint(RPCParameters *rpcParams)
{
	Log("RPC: DisableRaceCheckpoint");

	pGame->ToggleRaceCheckpoints(false);
}

void WorldVehicleAdd(RPCParameters *rpcParams)
{
	Log("RPC: WorldVehicleAdd");

	unsigned char * Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;

	RakNet::BitStream bsData((unsigned char*)Data,(iBitLength/8)+1,false);
	CVehiclePool *pVehiclePool = pNetGame->GetVehiclePool();
	if(!pVehiclePool) return;

	NEW_VEHICLE NewVehicle;
	bsData.Read((char *)&NewVehicle,sizeof(NEW_VEHICLE));

	Log("iVehicleType %d", NewVehicle.iVehicleType);
	if(NewVehicle.iVehicleType < 400 || NewVehicle.iVehicleType > 611) return;

	if(pVehiclePool)
		pVehiclePool->New(&NewVehicle);
	
	int iVehicle = pVehiclePool->FindGtaIDFromID(NewVehicle.VehicleID);
	uint16_t veh_model = pVehiclePool->GetAt(NewVehicle.VehicleID)->GetModelIndex();
	for(uint16_t i = 0; i < 10; i++)
	{
		if(veh_model == moto_models[i])
			return;
	}

	if(iVehicle)
	{
		for(int i = 0; i < 14; i++)
		{
			uint32_t v = 0;

			uint32_t modslot = NewVehicle.byteModSlots[i];
			if(modslot == 0)
				continue;

			modslot += 999;

			if(modslot < 1000 || modslot > 1193)
				continue;

			//pGame->RequestModel(iComponent);
			//pGame->LoadRequestedModels();
			(( int (*)(int,int))(g_libGTASA+0x28EB10+1))(modslot,1); //RequestModel
			(( int (*)(int,int))(g_libGTASA+0x28F2F8+1))(modslot,0); //RequestVehicleUpgrade
			(( int (*)(bool))(g_libGTASA+0x294CB4+1))(0); //LoadAllRequestedModels
			//ScriptCommand(&request_car_component, iComponent);
			

			ScriptCommand(&add_car_component, iVehicle, modslot, &v);
		}

		if(NewVehicle.bytePaintjob){
			NewVehicle.bytePaintjob--;
			
			if(NewVehicle.bytePaintjob < 0 || NewVehicle.bytePaintjob > 2)
				return;

			ScriptCommand(&change_car_skin, iVehicle, NewVehicle.bytePaintjob);
		}
	}
}

void WorldVehicleRemove(RPCParameters *rpcParams)
{
	Log("RPC: WorldVehicleRemove");

	unsigned char * Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;

	CPlayerPed *pPlayerPed = pGame->FindPlayerPed();
	CVehiclePool *pVehiclePool = pNetGame->GetVehiclePool();
	RakNet::BitStream bsData((unsigned char*)Data,(iBitLength/8)+1,false);

	if(!pVehiclePool) return;

	VEHICLEID VehicleID;
	VEHICLEID MyVehicleID;

	bsData.Read(VehicleID);

	if(pVehiclePool)
		pVehiclePool->Delete(VehicleID);
}

void EnterVehicle(RPCParameters *rpcParams)
{
	Log("RPC: EnterVehicle");

	unsigned char * Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;

	RakNet::BitStream bsData((unsigned char*)Data,(iBitLength/8)+1,false);
	PLAYERID playerId;
	VEHICLEID VehicleID=0;
	uint8_t bytePassenger=0;
	bool bPassenger = false;

	bsData.Read(playerId);
	bsData.Read(VehicleID);
	bsData.Read(bytePassenger);

	if(bytePassenger) bPassenger = true;

	CPlayerPool *pPlayerPool = pNetGame->GetPlayerPool();
	if(pPlayerPool)
	{
		CRemotePlayer *pPlayer = pPlayerPool->GetAt(playerId);
		if(pPlayer)
			pPlayer->EnterVehicle(VehicleID, bPassenger);
	}	
}

void ExitVehicle(RPCParameters *rpcParams)
{
	Log("RPC: ExitVehicle");

	unsigned char * Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;

	RakNet::BitStream bsData((unsigned char*)Data,(iBitLength/8)+1,false);
	PLAYERID playerId;
	VEHICLEID VehicleID = 0;

	bsData.Read(playerId);
	bsData.Read(VehicleID);

	CPlayerPool *pPlayerPool = pNetGame->GetPlayerPool();

	if(pPlayerPool)
	{
		CRemotePlayer *pPlayer = pPlayerPool->GetAt(playerId);
		if(pPlayer)
			pPlayer->ExitVehicle();
	}	
}

void DialogBox(RPCParameters *rpcParams)
{
	unsigned char * Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;

	uint16_t wDialogID = -1;
	uint8_t byteDialogStyle = 0;
	uint8_t len;
	char szBuff[4096+1];
	char title[64 * 3 + 1], info[4096+1], button1[20+1], button2[20+1];

	RakNet::BitStream bsData((unsigned char *)Data,(iBitLength/8)+1,false);

	bsData.Read(wDialogID);
	bsData.Read(byteDialogStyle);
	pDialogWindow->m_wDialogID = wDialogID;
	pDialogWindow->m_byteDialogStyle = byteDialogStyle;
	// title
	bsData.Read(len);
	bsData.Read(szBuff, len);
	szBuff[len] = '\0';
	cp1251_to_utf8(title, szBuff);
	cp1251_to_utf8(pDialogWindow->m_utf8Title, szBuff);
	// button1
	bsData.Read(len);
	bsData.Read(szBuff, len);
	szBuff[len] = '\0';
	cp1251_to_utf8(button1, szBuff);
	cp1251_to_utf8(pDialogWindow->m_utf8Button1, szBuff);
	// button2
	bsData.Read(len);
	bsData.Read(szBuff, len);
	szBuff[len] = '\0';
	cp1251_to_utf8(button2, szBuff);
	cp1251_to_utf8(pDialogWindow->m_utf8Button2, szBuff);

	// info
	stringCompressor->DecodeString(szBuff, 4096, &bsData);
	cp1251_to_utf8(info, szBuff);
	pDialogWindow->SetInfo(szBuff, strlen(szBuff));

	if(wDialogID < 0) return;

    //pDialogWindow->Show(true);
	pGame->FindPlayerPed()->TogglePlayerControllable(false);
	pJavaWrapper->MakeDialog(wDialogID, byteDialogStyle, title, info, button1, button2);
}

void SendVerifyClient(RPCParameters* rpcParams)
{
    Log("RPC: SendVerifyClient");

    unsigned char* Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;

	bool value;

	RakNet::BitStream bsData((unsigned char*)Data,(iBitLength/8)+1,false);
    bsData.Read(value);

    verify = value;
}

void SetSpeedParams(RPCParameters* rpcParams)
{
    Log("RPC: SetSpeedParams");

    unsigned char* Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;

	bool engine;
	bool lock;
	bool light;

	RakNet::BitStream bsData((unsigned char*)Data,(iBitLength/8)+1,false);
	
    bsData.Read(engine);
    bsData.Read(lock);
    bsData.Read(light);

	pGameScreen->GetInterface()->EngineState(engine);
	pGameScreen->GetInterface()->LightState(light);
	pGameScreen->GetInterface()->LockState(lock);
}

void ShowSpeedometr(RPCParameters* rpcParams)
{
    Log("RPC: ShowSpeedometr");

    unsigned char* Data = reinterpret_cast<unsigned char*>(rpcParams->input);
    int iBitLength = rpcParams->numberOfBitsOfData;

    uint8_t pID = 100;
    uint8_t sID = 100;

    RakNet::BitStream bsData((unsigned char*)Data, (iBitLength / 8) + 1, false);

    bsData.Read(pID);
    bsData.Read(sID);

    
}

void SetExp(RPCParameters* rpcParams)
{
    Log("RPC: SetExp");

    unsigned char* Data = reinterpret_cast<unsigned char*>(rpcParams->input);
    int iBitLength = rpcParams->numberOfBitsOfData;

    uint8_t currentExp;
    uint8_t expToUp;

    RakNet::BitStream bsData((unsigned char*)Data, (iBitLength / 8) + 1, false);

    bsData.Read(currentExp);
    bsData.Read(expToUp);

    pGUI->m_CurrentExp = currentExp;
    pGUI->m_ToUpExp = expToUp;
}

void SetEat(RPCParameters* rpcParams)
{
    Log("RPC: SetSpeedParams");

    unsigned char* Data = reinterpret_cast<unsigned char *>(rpcParams->input);
    int iBitLength = rpcParams->numberOfBitsOfData;

    uint8_t value;

    RakNet::BitStream bsData((unsigned char*)Data,(iBitLength/8)+1,false);

    bsData.Read(value);
    pGUI->m_Eat = value;
}

void ToggleChat(RPCParameters* rpcParams)
{
    Log("RPC: ToggleChat");

    unsigned char* Data = reinterpret_cast<unsigned char *>(rpcParams->input);
    int iBitLength = rpcParams->numberOfBitsOfData;

    bool value;

    RakNet::BitStream bsData((unsigned char*)Data,(iBitLength/8)+1,false);
    bsData.Read(value);

    pChatWindow->ToggleState();
}

void ToggleButtons(RPCParameters* rpcParams)
{
    Log("RPC: ToggleButtons");

    unsigned char* Data = reinterpret_cast<unsigned char *>(rpcParams->input);
    int iBitLength = rpcParams->numberOfBitsOfData;

    bool value;

    RakNet::BitStream bsData((unsigned char*)Data,(iBitLength/8)+1,false);
    bsData.Read(value);

   // pGameScreen->GetButtons()->ToggleState(false);
}

void ToggleHud(RPCParameters* rpcParams)
{
    Log("RPC: ToggleHud");

    unsigned char* Data = reinterpret_cast<unsigned char *>(rpcParams->input);
    int iBitLength = rpcParams->numberOfBitsOfData;

    bool value;

    RakNet::BitStream bsData((unsigned char*)Data,(iBitLength/8)+1,false);
    bsData.Read(value);

    pGameScreen->GetInterface()->ToggleHudState();
}

void GameModeRestart(RPCParameters *rpcParams)
{
	Log("RPC: GameModeRestart");
	if(pChatWindow) pChatWindow->AddChatMessage(nullptr, 0xffffffff, "Сервер перезапускается... Подождите..");
	pNetGame->ShutDownForGameRestart();
}

#define REJECT_REASON_BAD_VERSION   1
#define REJECT_REASON_BAD_NICKNAME  2
#define REJECT_REASON_BAD_MOD		3
#define REJECT_REASON_BAD_PLAYERID	4
void ConnectionRejected(RPCParameters *rpcParams)
{
	Log("RPC: ConnectionRejected");

	unsigned char * Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;

	RakNet::BitStream bsData((unsigned char*)Data,(iBitLength/8)+1,false);
	uint8_t byteRejectReason;

	bsData.Read(byteRejectReason);

	if(byteRejectReason == REJECT_REASON_BAD_VERSION)
		pChatWindow->AddInfoMessage("CONNECTION REJECTED: Incorrect Version.");
	else if(byteRejectReason == REJECT_REASON_BAD_NICKNAME)
	{
		pChatWindow->AddInfoMessage("CONNECTION REJECTED: Unacceptable NickName");
		pChatWindow->AddInfoMessage("Please choose another nick between and 3-20 characters");
		pChatWindow->AddInfoMessage("Please use only a-z, A-Z, 0-9");
		pChatWindow->AddInfoMessage("Use /quit to exit or press ESC and select Quit Game");
	}
	else if(byteRejectReason == REJECT_REASON_BAD_MOD)
	{
		pChatWindow->AddInfoMessage("CONNECTION REJECTED: Bad mod version.");
	}
	else if(byteRejectReason == REJECT_REASON_BAD_PLAYERID)
	{
		pChatWindow->AddInfoMessage("CONNECTION REJECTED: Unable to allocate a player slot.");
	}

	pNetGame->GetRakClient()->Disconnect(500);
}

void Pickup(RPCParameters *rpcParams)
{
	Log("RPC: Pickup");

	unsigned char * Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;

	RakNet::BitStream bsData(Data,(iBitLength/8)+1,false);

	PICKUP Pickup;
	int iIndex;

	bsData.Read(iIndex);
	bsData.Read((char*)&Pickup, sizeof (PICKUP));

	CPickupPool *pPickupPool = pNetGame->GetPickupPool();
	if (pPickupPool) 
		pPickupPool->New(&Pickup, iIndex);
}

void DestroyPickup(RPCParameters *rpcParams)
{
	Log("RPC: DestroyPickup");

	unsigned char * Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;

	RakNet::BitStream bsData(Data,(iBitLength/8)+1,false);

	int iIndex;
	bsData.Read(iIndex);

	CPickupPool *pPickupPool = pNetGame->GetPickupPool();
	if (pPickupPool) 
		pPickupPool->Destroy(iIndex);
}

void Create3DTextLabel(RPCParameters *rpcParams)
{
	Log("RPC: Create3DTextLabel");

	unsigned char * Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;

	uint16_t LabelID;
	uint32_t color;
	VECTOR pos;
	float dist;
	uint8_t testLOS;
	PLAYERID PlayerID;
	VEHICLEID VehicleID;
	char szBuff[4096+1];

	RakNet::BitStream bsData(Data,(iBitLength/8)+1,false);

	bsData.Read(LabelID);
	bsData.Read(color);
	bsData.Read(pos.X);
	bsData.Read(pos.Y);
	bsData.Read(pos.Z);
	bsData.Read(dist);
	bsData.Read(testLOS);
	bsData.Read(PlayerID);
	bsData.Read(VehicleID);

	stringCompressor->DecodeString(szBuff, 4096, &bsData);

	CText3DLabelsPool *pLabelsPool = pNetGame->GetLabelPool();

	if(pLabelsPool)
	{
		pLabelsPool->CreateTextLabel(LabelID, szBuff, color, pos.X, pos.Y, pos.Z, dist, testLOS, PlayerID, VehicleID);
	}
}

void Delete3DTextLabel(RPCParameters *rpcParams)
{
	Log("RPC: Delete3DTextLabel");

	unsigned char * Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;

	RakNet::BitStream bsData(Data, (iBitLength / 8) + 1, false);

	uint16_t LabelID;
	bsData.Read(LabelID);
	Log("ID: %d", LabelID);

	CText3DLabelsPool *pLabelsPool = pNetGame->GetLabelPool();
	if(pLabelsPool)
	{
		pLabelsPool->Delete(LabelID);
	}
}

void Update3DTextLabel(RPCParameters *rpcParams)
{
	Log("RPC: Update3DTextLabel");

	unsigned char * Data = reinterpret_cast<unsigned char *>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;

	RakNet::BitStream bsData(Data, (iBitLength / 8) + 1, false);

	uint16_t LabelID;
	bsData.Read(LabelID);
	Log("ID: %d", LabelID);

	CText3DLabelsPool *pLabelsPool = pNetGame->GetLabelPool();
	if(pLabelsPool)
	{
		pLabelsPool->Delete(LabelID);
	}
}

void UpdateScoresPingsIPs(RPCParameters *rpcParams)
{
	Log("RPC: UpdateScoresPingsIPs");

	unsigned char *Data = reinterpret_cast<unsigned char*>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;

	RakNet::BitStream bsData(Data, (iBitLength/8)+1,false);

	PLAYERID bytePlayerId;
	int iPlayerScore;
	uint32_t dwPlayerPing;

	CPlayerPool *pPlayerPool = pNetGame->GetPlayerPool();

	for (PLAYERID i=0; i<(iBitLength/8)/9; i++)
	{
		bsData.Read(bytePlayerId);
		bsData.Read(iPlayerScore);
		bsData.Read(dwPlayerPing);

		pPlayerPool->UpdateScore(bytePlayerId, iPlayerScore);
		pPlayerPool->UpdatePing(bytePlayerId, dwPlayerPing);
	}
}

void ScmEvent(RPCParameters *rpcParams)
{
	Log("RPC: ScmEvent");

	unsigned char *Data = reinterpret_cast<unsigned char*>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;

	RakNet::BitStream bsData(Data, (iBitLength/8)+1,false);

	PLAYERID bytePlayerID;
	int iEvent;
	uint32_t dwParam1, dwParam2, dwParam3;

	bsData.Read(bytePlayerID);
	bsData.Read(iEvent);
	bsData.Read(dwParam1);
	bsData.Read(dwParam2);
	bsData.Read(dwParam3);

	ProcessIncomingEvent(bytePlayerID, iEvent, dwParam1, dwParam2, dwParam3);
}

void WorldPlayerDeath(RPCParameters *rpcParams)
{
	Log("RPC: WorldPlayerDeath");

	unsigned char *Data = reinterpret_cast<unsigned char*>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;

	RakNet::BitStream bsData(Data, (iBitLength/8)+1,false);

	PLAYERID playerId;
	bsData.Read(playerId);

	CPlayerPool *pPlayerPool = pNetGame->GetPlayerPool();
	if(pPlayerPool)
	{
		if(pPlayerPool->GetSlotState(playerId))
		{
			CRemotePlayer *pRemotePlayer = pPlayerPool->GetAt(playerId);
			if(pRemotePlayer)
			{
				pRemotePlayer->HandleDeath();
			}
		}
	}
}

void WorldActorAdd(RPCParameters* rpcParams)
{
	Log("RPC: WorldActorAdd");
	
	RakNet::BitStream bsData(rpcParams->input, (rpcParams->numberOfBitsOfData / 8) + 1, false);

	uint16_t actorId;
	uint32_t iSkinId;
	VECTOR vecPos;
	float fRotation;
	float fHealth;
	bool bInvulnerable;

	bsData.Read(actorId);
	bsData.Read(iSkinId);
	Log("iSkinId: %d", iSkinId);
	bsData.Read(vecPos.X);
	bsData.Read(vecPos.Y);
	bsData.Read(vecPos.Z);
	bsData.Read(fRotation);
	bsData.Read(fHealth);
	bsData.Read(bInvulnerable);

	CActorPool *pActorPool = pNetGame->GetActorPool();
	if(pActorPool) {
		pActorPool->Spawn(actorId, iSkinId, vecPos, fRotation, fHealth, bInvulnerable);
	}
}

void WorldActorRemove(RPCParameters* rpcParams)
{
	Log("RPC: WorldActorRemove");
	
	RakNet::BitStream bsData(rpcParams->input, (rpcParams->numberOfBitsOfData / 8) + 1, false);

	uint16_t actorId;

	bsData.Read(actorId);
	
	CActorPool *pActorPool = pNetGame->GetActorPool();
	if(pActorPool) {
		pActorPool->Delete(actorId);
	}
}



void RegisterRPCs(RakClientInterface* pRakClient)
{
	Log("Registering RPC's..");
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_InitGame, InitGame);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_ServerJoin, ServerJoin);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_ServerQuit, ServerQuit);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_ClientMessage, ClientMessage);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_Chat, Chat);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_RequestClass, RequestClass);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_RequestSpawn, RequestSpawn);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_Weather, Weather);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_WorldTime, WorldTime);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_SetTimeEx, SetTimeEx);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_WorldPlayerAdd, WorldPlayerAdd);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_WorldPlayerRemove, WorldPlayerRemove);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_SetCheckpoint, SetCheckpoint);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_DisableCheckpoint, DisableCheckpoint);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_SetRaceCheckpoint, SetRaceCheckpoint);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_DisableRaceCheckpoint, DisableRaceCheckpoint);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_WorldVehicleAdd, WorldVehicleAdd);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_WorldVehicleRemove, WorldVehicleRemove);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_EnterVehicle, EnterVehicle);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_ExitVehicle, ExitVehicle);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_ScrDialogBox, DialogBox);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_GameModeRestart, GameModeRestart);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_ConnectionRejected, ConnectionRejected);

	pRakClient->RegisterAsRemoteProcedureCall(&RPC_Pickup, Pickup);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_DestroyPickup, DestroyPickup);

	pRakClient->RegisterAsRemoteProcedureCall(&RPC_ScrCreate3DTextLabel, Create3DTextLabel);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_ScrUpdate3DTextLabel, Update3DTextLabel);

	pRakClient->RegisterAsRemoteProcedureCall(&RPC_UpdateScoresPingsIPs, UpdateScoresPingsIPs);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_ScmEvent, ScmEvent);

	pRakClient->RegisterAsRemoteProcedureCall(&RPC_WorldPlayerDeath, WorldPlayerDeath);

	pRakClient->RegisterAsRemoteProcedureCall(&RPC_ShowActor, WorldActorAdd);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_HideActor, WorldActorRemove);


	pRakClient->RegisterAsRemoteProcedureCall(&RPC_SendVerifyClient, SendVerifyClient);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_SetSpeedParams, SetSpeedParams);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_ShowSpeedometr, ShowSpeedometr);

	pRakClient->RegisterAsRemoteProcedureCall(&RPC_SendExp, SetExp);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_SendEat, SetEat);

	pRakClient->RegisterAsRemoteProcedureCall(&RPC_ToggleHud, ToggleHud);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_ToggleChat, ToggleChat);
	pRakClient->RegisterAsRemoteProcedureCall(&RPC_ToggleButtons, ToggleButtons);

    Log("> netrpc | All RPC successfully Registered");
}

void UnRegisterRPCs(RakClientInterface* pRakClient)
{
	Log("UnRegistering RPC's..");
	//pRakClient->UnregisterAsRemoteProcedureCall(&RPC_PlayerGiveTakeDamage);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_InitGame);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_ServerJoin);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_ServerQuit);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_ClientMessage);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_Chat);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_RequestClass);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_RequestSpawn);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_Weather);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_WorldTime);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_SetTimeEx);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_WorldPlayerAdd);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_WorldPlayerRemove);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_SetCheckpoint);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_DisableCheckpoint);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_SetRaceCheckpoint);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_DisableRaceCheckpoint);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_WorldVehicleAdd);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_WorldVehicleRemove);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_EnterVehicle);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_ExitVehicle);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_ScrDialogBox);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_GameModeRestart);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_ConnectionRejected);

	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_Pickup);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_DestroyPickup);

	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_ScrCreate3DTextLabel);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_ScrUpdate3DTextLabel);

	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_UpdateScoresPingsIPs);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_ScmEvent);

	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_WorldPlayerDeath);

	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_ShowActor);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_HideActor);


	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_SendVerifyClient);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_SetSpeedParams);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_ShowSpeedometr);

	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_SendExp);
	pRakClient->UnregisterAsRemoteProcedureCall(&RPC_SendEat);
}
