#include "infection.h"

#include <cstdio>

#include <engine/shared/config.h>
#include <engine/shared/protocol.h>
#include <game/server/gamecontext.h>

#include <game/server/bot.h>

CGameControllerInfection::CGameControllerInfection(class CGameContext *pGameServer)
: IGameController(pGameServer) {
    m_pGameType = "bInfection+";
	m_BroadcastTime = 0;
	m_NextZombie = 0;
}

void CGameControllerInfection::DoWincheck() {
    int Humans = 0, Zombies = 0;
    for (int i = 0; i < MAX_CLIENTS; i ++) {
        CPlayer *pPlayer = GameServer()->m_apPlayers[i];
        if (!pPlayer)
            continue;

        if (pPlayer->GetTeam() == TEAM_SPECTATORS)
            continue;

        if (pPlayer->Infected())
            Zombies ++;
        else
            Humans ++;
    }

    if (Humans + Zombies < 2) {
        m_SuddenDeath = true;
        m_Warmup = 0;
        GameServer()->SendBroadcast("At least 2 players required to start playing", -1);
        return;
    } else if (m_SuddenDeath) {
        StartRound();
        return;
    }

    if (m_GameOverTick == -1 && !m_Warmup && !GameServer()->m_World.m_ResetRequested) {
        if (!Humans || !Zombies) {
            EndRound();
            return;
        }
    }
    IGameController::DoWincheck();
}

void CGameControllerInfection::OnCharacterSpawn(CCharacter *pChr) {
    pChr->IncreaseHealth(10);
    pChr->GiveWeapon(WEAPON_HAMMER, -1);
    if (pChr->GetPlayer()->Infected())
        pChr->SetWeapon(WEAPON_HAMMER);
    else
        pChr->GiveWeapon(WEAPON_GUN, 10);
}

int CGameControllerInfection::OnCharacterDeath(CCharacter *pVictim, CPlayer *pKiller, int Weapon) {

	if (!pKiller || Weapon == WEAPON_GAME)
		return 0;

	if (pKiller == pVictim->GetPlayer())
		pVictim->GetPlayer()->m_Score--; // suicide
	else {
		if((IsTeamplay() && pVictim->GetPlayer()->GetTeam() == pKiller->GetTeam()) ||
            IsFriendlyFire(pVictim->GetPlayer()->GetCID(), pKiller->GetCID()))
        {
            // teamkill
            pKiller->m_Score -= g_Config.m_SvTkPenalty;
        }
		else
        {
            // normal kill
            pKiller->m_Score++;
        }
	}

	if (Weapon == WEAPON_SELF)
    {
		pVictim->GetPlayer()->m_RespawnTick = Server()->Tick()+Server()->TickSpeed()*3.0f;
    }
    else
    {
        if(!pKiller->Infected() || !pKiller->m_HasSuperJump)
            pKiller->m_Kills++;

        if (pKiller->Infected()) {
            if (pKiller->m_Kills >= g_Config.m_InfSuperJumpKills && !pKiller->m_HasSuperJump) {
                pKiller->m_Kills = 0;
                pKiller->m_HasSuperJump = true;
                GameServer()->SendBroadcast("You earned super jump, hold spacebar to use it", pKiller->GetCID());
                char str[512] = {0};
                if(pKiller->IsBot() && pKiller->m_pBot)
                    sprintf(str, g_Config.m_InfSuperJumpText, pKiller->m_pBot->GetName());
                else
                    sprintf(str, g_Config.m_InfSuperJumpText, Server()->ClientName(pKiller->GetCID()));
                GameServer()->SendChatTarget(-1, str);
            }
        } else {
            if (pKiller->m_Kills >= g_Config.m_InfAirstrikeKills && !pKiller->m_HasAirstrike) {
                pKiller->m_Kills -= g_Config.m_InfAirstrikeKills;
                pKiller->m_HasAirstrike = true;
                GameServer()->SendBroadcast("You got an airstrike, use hammer to launch it :D", pKiller->GetCID());
                char str[512] = {0};
                if(pKiller->IsBot() && pKiller->m_pBot)
                    sprintf(str, g_Config.m_InfAirstrikeText, pKiller->m_pBot->GetName());
                else
                    sprintf(str, g_Config.m_InfAirstrikeText, Server()->ClientName(pKiller->GetCID()));
                GameServer()->SendChatTarget(-1, str);
            }
        }
    }
	return 0;
}

void CGameControllerInfection::PostReset()
{
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(GameServer()->m_apPlayers[i])
		{
			GameServer()->m_apPlayers[i]->Respawn();
			GameServer()->m_apPlayers[i]->m_Kills = 0;
			GameServer()->m_apPlayers[i]->m_HasSuperJump = false;
			GameServer()->m_apPlayers[i]->m_HasAirstrike = false;
			GameServer()->m_apPlayers[i]->m_RespawnTick = Server()->Tick()+Server()->TickSpeed()/2;
		}
	}
}
