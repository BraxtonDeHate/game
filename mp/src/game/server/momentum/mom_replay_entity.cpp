#include "cbase.h"
#include "mom_replay.h"
#include "mom_replay_entity.h"
#include "mom_shareddefs.h"
#include "util/mom_util.h"
#include "Timer.h"

#include "tier0/memdbgon.h"

MAKE_TOGGLE_CONVAR(mom_replay_firstperson, "1", FCVAR_CLIENTCMD_CAN_EXECUTE, "Watch replay in first-person");
MAKE_TOGGLE_CONVAR(mom_replay_reverse, "0", FCVAR_CLIENTCMD_CAN_EXECUTE, "Reverse playback of replay");
MAKE_TOGGLE_CONVAR(mom_replay_loop, "1", FCVAR_CLIENTCMD_CAN_EXECUTE, "Loop playback of replay ghost");
static ConVar mom_replay_ghost_bodygroup("mom_replay_ghost_bodygroup", "11",
                                         FCVAR_CLIENTCMD_CAN_EXECUTE | FCVAR_ARCHIVE,
                                         "Replay ghost's body group (model)", true, 0, true, 14);
static ConCommand mom_replay_ghost_color("mom_replay_ghost_color", CMomentumReplayGhostEntity::SetGhostColor,
                                         "Set the ghost's color. Accepts HEX color value in format RRGGBB",
                                         FCVAR_CLIENTCMD_CAN_EXECUTE | FCVAR_ARCHIVE);
static ConVar mom_replay_ghost_alpha("mom_replay_ghost_alpha", "75", FCVAR_CLIENTCMD_CAN_EXECUTE | FCVAR_ARCHIVE,
                                     "Sets the ghost's transparency, integer between 0 and 255,", true, 0, true, 255);

LINK_ENTITY_TO_CLASS(mom_replay_ghost, CMomentumReplayGhostEntity);

IMPLEMENT_SERVERCLASS_ST(CMomentumReplayGhostEntity, DT_MOM_ReplayEnt)
// MOM_TODO: Network other variables that the UI will need to reference
SendPropInt(SENDINFO(m_nReplayButtons)), 
SendPropInt(SENDINFO(m_iTotalStrafes)), 
SendPropInt(SENDINFO(m_iTotalJumps)),
SendPropFloat(SENDINFO(m_flRunTime)),
SendPropFloat(SENDINFO(m_flTickRate)),
SendPropString(SENDINFO(m_pszPlayerName)),
SendPropDataTable(SENDINFO_DT(m_RunData), &REFERENCE_SEND_TABLE(DT_MOM_RunEntData)), 
END_SEND_TABLE();

BEGIN_DATADESC(CMomentumReplayGhostEntity)
END_DATADESC()

Color CMomentumReplayGhostEntity::m_newGhostColor = COLOR_GREEN;

CMomentumReplayGhostEntity::CMomentumReplayGhostEntity()
    : m_bIsActive(false), m_nStartTick(0), step(0), m_flLastSyncVelocity(0)
{
    m_nReplayButtons = 0;
    m_iTotalStrafes = 0;
    m_bHasJumped = false;
    m_RunStats = CMomRunStats(g_Timer->GetZoneCount());
}

const char *CMomentumReplayGhostEntity::GetGhostModel() const { return m_pszModel; }
CMomentumReplayGhostEntity::~CMomentumReplayGhostEntity() { g_ReplaySystem->m_bIsWatchingReplay = false; }
void CMomentumReplayGhostEntity::Precache(void)
{
    BaseClass::Precache();
    PrecacheModel(GHOST_MODEL);
    m_ghostColor = COLOR_GREEN; // default color
}

//-----------------------------------------------------------------------------
// Purpose: Sets up the entity's initial state
//-----------------------------------------------------------------------------
void CMomentumReplayGhostEntity::Spawn(void)
{
    Precache();
    BaseClass::Spawn();
    RemoveEffects(EF_NODRAW);
    SetRenderMode(kRenderTransColor);
    SetRenderColor(m_ghostColor.r(), m_ghostColor.g(), m_ghostColor.b(), 75);
    //~~~The magic combo~~~ (collides with triggers, not with players)
    ClearSolidFlags();
    SetCollisionGroup(COLLISION_GROUP_DEBRIS_TRIGGER);
    SetMoveType(MOVETYPE_STEP);
    SetSolid(SOLID_BBOX);
    RemoveSolidFlags(FSOLID_NOT_SOLID);

    SetModel(GHOST_MODEL);
    SetBodygroup(1, mom_replay_ghost_bodygroup.GetInt());

    Q_strcpy(m_pszPlayerName.GetForModify(), g_ReplaySystem->m_loadedHeader.playerName);
}

void CMomentumReplayGhostEntity::StartRun(bool firstPerson, bool shouldLoop /* = false */)
{
    m_bReplayFirstPerson = firstPerson;
    m_bReplayShouldLoop = shouldLoop;

    Spawn();
    m_iTotalStrafes = 0;
    m_nStartTick = gpGlobals->curtime;
    m_bIsActive = true;
    m_bHasJumped = false;
    step = mom_replay_reverse.GetBool() ? g_ReplaySystem->m_vecRunData.Size() - 1 : 0;
    SetAbsOrigin(g_ReplaySystem->m_vecRunData[step].PlayerOrigin());
    SetNextThink(gpGlobals->curtime);
}

void CMomentumReplayGhostEntity::UpdateStep()
{
    currentStep = g_ReplaySystem->m_vecRunData[step];
    if (mom_replay_reverse.GetBool())
    {
        nextStep = g_ReplaySystem->m_vecRunData[--step];
    }
    else if (step < g_ReplaySystem->m_vecRunData.Size())
    {
        nextStep = g_ReplaySystem->m_vecRunData[++step];
    }
}
void CMomentumReplayGhostEntity::Think(void)
{
    // update color, bodygroup, and other params if they change
    if (mom_replay_ghost_bodygroup.GetInt() != m_iBodyGroup)
    {
        m_iBodyGroup = mom_replay_ghost_bodygroup.GetInt();
        SetBodygroup(1, m_iBodyGroup);
    }
    if (m_ghostColor != m_newGhostColor)
    {
        m_ghostColor = m_newGhostColor;
        SetRenderColor(m_ghostColor.r(), m_ghostColor.g(), m_ghostColor.b());
    }
    if (mom_replay_ghost_alpha.GetInt() != m_ghostColor.a())
    {
        m_ghostColor.SetColor(m_ghostColor.r(), m_ghostColor.g(),
            m_ghostColor.b(), // we have to set the previous colors in order to change alpha...
            mom_replay_ghost_alpha.GetInt());
        SetRenderColorA(mom_replay_ghost_alpha.GetInt());
    }
    mom_replay_loop.SetValue(m_bReplayShouldLoop);
    mom_replay_firstperson.SetValue(m_bReplayFirstPerson);

    //move the ghost
    if (step >= 0)
    {
        if (step + 1 < g_ReplaySystem->m_vecRunData.Size() || mom_replay_reverse.GetBool() && step - 1 > -1)
        {
            UpdateStep();
            mom_replay_firstperson.GetBool() ? HandleGhostFirstPerson() : HandleGhost();
        }
        else if (step + 1 == g_ReplaySystem->m_vecRunData.Size() && mom_replay_loop.GetBool())
        {
            step = mom_replay_reverse.GetBool() ? g_ReplaySystem->m_vecRunData.Size() - 1 : 0; // reset us to the start
        }
        else
        {
            EndRun();
        }
    }

    BaseClass::Think();
    SetNextThink(gpGlobals->curtime + gpGlobals->interval_per_tick);
}
//-----------------------------------------------------------------------------
// Purpose: called by the think function, moves and handles the ghost if we're spectating it
//-----------------------------------------------------------------------------
void CMomentumReplayGhostEntity::HandleGhostFirstPerson()
{
    CMomentumPlayer *pPlayer = ToCMOMPlayer(UTIL_GetLocalPlayer());
    if (pPlayer)
    {
        if (!pPlayer->IsObserver())
        {
            pPlayer->SetObserverTarget(this);
            pPlayer->StartObserverMode(OBS_MODE_IN_EYE);
        }

        if (pPlayer->GetObserverMode() != (OBS_MODE_IN_EYE | OBS_MODE_CHASE))
        {
            // we don't want to allow any other obs modes, only IN EYE and CHASE
            pPlayer->ForceObserverMode(OBS_MODE_IN_EYE);
        }
        pPlayer->SetViewOffset(VEC_VIEW);
        Vector origin = currentStep.PlayerOrigin();
        origin.z -= 3.5f;
        SetAbsOrigin(origin);

        if (pPlayer->GetObserverMode() == OBS_MODE_IN_EYE)
        {
            SetAbsAngles(currentStep.EyeAngles());
            // don't render the model when we're in first person mode
            SetRenderMode(kRenderNone);
            AddEffects(EF_NOSHADOW);
        }
        else
        {
            SetAbsAngles(QAngle(currentStep.EyeAngles().x /
                                    10, // we divide x angle (pitch) by 10 so the ghost doesn't look really stupid
                                currentStep.EyeAngles().y,
                                currentStep.EyeAngles().z));
            // remove the nodraw effects
            SetRenderMode(kRenderTransColor);
            RemoveEffects(EF_NOSHADOW);
        }

        // interpolate vel from difference in origin
        float distX = fabs(currentStep.PlayerOrigin().x - nextStep.PlayerOrigin().x);
        float distY = fabs(currentStep.PlayerOrigin().y - nextStep.PlayerOrigin().y);
        float distZ = fabs(currentStep.PlayerOrigin().z - nextStep.PlayerOrigin().z);
        Vector interpolatedVel = Vector(distX, distY, distZ) / gpGlobals->interval_per_tick;
        SetAbsVelocity(interpolatedVel);
        m_nReplayButtons =
            currentStep.PlayerButtons(); // networked var that allows the replay to control keypress display on the client

        if (this->m_RunData.m_bTimerRunning)
            UpdateStats(interpolatedVel);

        if (currentStep.PlayerButtons() & IN_DUCK)
        {
            // MOM_TODO: make this smoother. possibly inherit from NPC classes/CBaseCombatCharacter
            pPlayer->SetViewOffset(VEC_DUCK_VIEW);
        }
    }
}

void CMomentumReplayGhostEntity::HandleGhost()
{
    SetAbsOrigin(currentStep.PlayerOrigin());
    SetAbsAngles(QAngle(
        currentStep.EyeAngles().x / 10, // we divide x angle (pitch) by 10 so the ghost doesn't look really stupid
        currentStep.EyeAngles().y, currentStep.EyeAngles().z));
    // remove the nodraw effects
    SetRenderMode(kRenderTransColor);
    RemoveEffects(EF_NOSHADOW);

    CMomentumPlayer *pPlayer = ToCMOMPlayer(UTIL_GetLocalPlayer());
    if (pPlayer && pPlayer->IsObserver()) // bring the player out of obs mode if theyre currently observing
    {
        // pPlayer->m_bIsWatchingReplay = false;
        pPlayer->StopObserverMode();
        pPlayer->ForceRespawn();
    }
}

void CMomentumReplayGhostEntity::UpdateStats(Vector ghostVel)
{
    // --- STRAFE SYNC ---
    // calculate strafe sync based on replay ghost's movement, in order to update the player's HUD

    float SyncVelocity = ghostVel.Length2DSqr(); // we always want HVEL for checking velocity sync
    if (GetGroundEntity() == nullptr)            // The ghost is in the air
    {
        m_bHasJumped = false;
        if (EyeAngles().y > m_qLastEyeAngle.y) // player turned left
        {
            m_nStrafeTicks++;
            if ((currentStep.PlayerButtons() & IN_MOVELEFT) && !(currentStep.PlayerButtons() & IN_MOVERIGHT))
                m_nPerfectSyncTicks++;
            if (SyncVelocity > m_flLastSyncVelocity)
                m_nAccelTicks++;
        }
        else if (EyeAngles().y < m_qLastEyeAngle.y) // player turned right
        {
            m_nStrafeTicks++;
            if ((currentStep.PlayerButtons() & IN_MOVERIGHT) && !(currentStep.PlayerButtons() & IN_MOVELEFT))
                m_nPerfectSyncTicks++;
            if (SyncVelocity > m_flLastSyncVelocity)
                m_nAccelTicks++;
        }
    }
    if (m_nStrafeTicks && m_nAccelTicks && m_nPerfectSyncTicks)
    {
        m_RunData.m_flStrafeSync =
            (float(m_nPerfectSyncTicks) / float(m_nStrafeTicks)) * 100.0f; // ticks strafing perfectly / ticks strafing
        m_RunData.m_flStrafeSync2 =
            (float(m_nAccelTicks) / float(m_nStrafeTicks)) * 100.0f; // ticks gaining speed / ticks strafing
    }

    // --- JUMP AND STRAFE COUNTER ---
    // MOM_TODO: This needs to calculate better. It currently counts every other jump, and sometimes spams (player on
    // ground for a while)
    if (!m_bHasJumped && GetGroundEntity() != nullptr && GetFlags() & FL_ONGROUND &&
        currentStep.PlayerButtons() & IN_JUMP)
    {
        m_bHasJumped = true;
        m_RunData.m_flLastJumpVel = GetLocalVelocity().Length2D();
        m_RunData.m_flLastJumpTime = gpGlobals->curtime;
        m_iTotalJumps++;
    }

    if ((currentStep.PlayerButtons() & IN_MOVELEFT && !(m_nOldReplayButtons & IN_MOVELEFT)) ||
        (currentStep.PlayerButtons() & IN_MOVERIGHT && !(m_nOldReplayButtons & IN_MOVERIGHT)))
        m_iTotalStrafes++;

    m_flLastSyncVelocity = SyncVelocity;
    m_qLastEyeAngle = EyeAngles();
    m_nOldReplayButtons = currentStep.PlayerButtons();
}
void CMomentumReplayGhostEntity::SetGhostModel(const char *newmodel)
{
    if (newmodel)
    {
        Q_strcpy(m_pszModel, newmodel);
        PrecacheModel(m_pszModel);
        SetModel(m_pszModel);
    }
}
void CMomentumReplayGhostEntity::SetGhostBodyGroup(int bodyGroup)
{
    if (bodyGroup > sizeof(ghostModelBodyGroup) || bodyGroup < 0)
    {
        Warning("CMomentumReplayGhostEntity::SetGhostBodyGroup() Error: Could not set bodygroup!");
    }
    else
    {
        m_iBodyGroup = bodyGroup;
        SetBodygroup(1, bodyGroup);
    }
}
void CMomentumReplayGhostEntity::SetGhostColor(const CCommand &args)
{
    if (mom_UTIL->GetColorFromHex(args.ArgS()))
    {
        m_newGhostColor = *mom_UTIL->GetColorFromHex(args.ArgS());
    }
}

void CMomentumReplayGhostEntity::StartTimer(int m_iStartTick)
{
    m_RunData.m_iStartTick = m_iStartTick;

    FOR_EACH_VEC(spectators, i)
    {
        CMomentumPlayer *pPlayer = spectators[i];
        if (pPlayer && pPlayer->GetReplayEnt() == this)
        {
            mom_UTIL->DispatchTimerStateMessage(pPlayer, m_iStartTick, true);
        }
    }
}

void CMomentumReplayGhostEntity::StopTimer()
{
    FOR_EACH_VEC(spectators, i)
    {
        CMomentumPlayer *pPlayer = spectators[i];
        if (pPlayer && pPlayer->GetReplayEnt() == this)
        {
            mom_UTIL->DispatchTimerStateMessage(pPlayer, 0, false);
        }
    }
}

void CMomentumReplayGhostEntity::SetRunStats(CMomRunStats &stats) { m_RunStats = stats; }

void CMomentumReplayGhostEntity::EndRun()
{
    IGameEvent *zoneExitEvent = gameeventmanager->CreateEvent("zone_exit");
    if (zoneExitEvent)
    {
        //This tells the event listener to remove/clear the stats for the given ent
        zoneExitEvent->SetInt("num", MAX_STAGES + 1);
        zoneExitEvent->SetInt("ent", entindex());
        gameeventmanager->FireEvent(zoneExitEvent);
    }
    
    StopTimer();
    SetNextThink(-1);
    Remove();
    m_bIsActive = false;

    FOR_EACH_VEC(spectators, i)
    {
        CMomentumPlayer *pPlayer = spectators[i];
        if (pPlayer && pPlayer->GetReplayEnt() == this)
        {
            pPlayer->StopObserverMode();
            pPlayer->ForceRespawn();
            pPlayer->SetMoveType(MOVETYPE_WALK);
            // pPlayer->m_bIsWatchingReplay = false;
        }
    }

    spectators.RemoveAll();
}