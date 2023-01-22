//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "basehlcombatweapon.h"
#include "player.h"
#include "gamerules.h"
#include "grenade_frag.h"
#include "npcevent.h"
#include "engine/IEngineSound.h"
#include "items.h"
#include "in_buttons.h"
#include "soundent.h"
#include "gamestats.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


#define GRENADE_PAUSED_NO			0
#define GRENADE_PAUSED_PRIMARY		1
#define GRENADE_PAUSED_SECONDARY	2

#define GRENADE_RADIUS	4.0f // inches

ConVar sk_plr_grenade_lob_speed("sk_plr_grenade_lob_speed", "350");
ConVar sk_plr_grenade_roll_speed("sk_plr_grenade_roll_speed", "700");
ConVar sk_plr_grenade_throw_speed("sk_plr_grenade_throw_speed", "1200");

ConVar sk_plr_grenade_timer("sk_plr_grenade_timer", "3");

ConVar sk_plr_grenade_is_cookable("sk_plr_grenade_is_cookable", "1");
ConVar sk_plr_grenade_cook_grace_period("sk_plr_grenade_cook_grace_period", "1.0");

//-----------------------------------------------------------------------------
// Fragmentation grenades
//-----------------------------------------------------------------------------
class CWeaponFrag: public CBaseHLCombatWeapon
{
	DECLARE_CLASS( CWeaponFrag, CBaseHLCombatWeapon );
public:
	DECLARE_SERVERCLASS();

public:
	CWeaponFrag();

	void	Precache( void );
	void	Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );
	void	PrimaryAttack( void );
	void	SecondaryAttack( void );
	void	DecrementAmmo( CBaseCombatCharacter *pOwner );
	void	ItemPostFrame( void );

	bool	Deploy( void );
	bool	Holster( CBaseCombatWeapon *pSwitchingTo = NULL );

	int		CapabilitiesGet( void ) { return bits_CAP_WEAPON_RANGE_ATTACK1; }
	
	bool	Reload( void );

	bool	ShouldDisplayHUDHint() { return true; }

private:
	void	ThrowGrenade( CBasePlayer *pPlayer );
	void	RollGrenade( CBasePlayer *pPlayer );
	void	LobGrenade( CBasePlayer *pPlayer );
	void	BlipSound() { EmitSound( "Grenade.Blip" ); }
	void	BlipThink();
	void	SetTimer();
	// check a throw from vecSrc.  If not valid, move the position back along the line to vecEye
	void	CheckThrowPosition( CBasePlayer *pPlayer, const Vector &vecEye, Vector &vecSrc );

	bool	m_bRedraw;	//Draw the weapon again after throwing a grenade
	
	float	m_flNextBlipTime;
	float	m_flWarnAITime;
	float	m_flCookTime;
	float	m_flDetonateTime;
	int		m_AttackPaused;
	bool	m_fDrawbackFinished;
	bool	m_bHasWarnedAI;

	DECLARE_ACTTABLE();

	DECLARE_DATADESC();
};


BEGIN_DATADESC( CWeaponFrag )
	DEFINE_FIELD( m_bRedraw, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_AttackPaused, FIELD_INTEGER ),
	DEFINE_FIELD( m_fDrawbackFinished, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bHasWarnedAI, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flNextBlipTime, FIELD_TIME ),
	DEFINE_FIELD( m_flWarnAITime, FIELD_TIME ),
	DEFINE_FIELD( m_flCookTime, FIELD_TIME ),
	DEFINE_FIELD( m_flDetonateTime, FIELD_TIME ),
END_DATADESC()

acttable_t	CWeaponFrag::m_acttable[] = 
{
	{ ACT_RANGE_ATTACK1, ACT_RANGE_ATTACK_SLAM, true },
};

IMPLEMENT_ACTTABLE(CWeaponFrag);

IMPLEMENT_SERVERCLASS_ST(CWeaponFrag, DT_WeaponFrag)
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( weapon_frag, CWeaponFrag );
PRECACHE_WEAPON_REGISTER(weapon_frag);



CWeaponFrag::CWeaponFrag() :
	CBaseHLCombatWeapon(),
	m_bRedraw( false )
{
	NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponFrag::Precache( void )
{
	PrecacheScriptSound( "Grenade.Blip" );

	UTIL_PrecacheOther( "npc_grenade_frag" );

	PrecacheScriptSound( "WeaponFrag.Throw" );
	PrecacheScriptSound( "WeaponFrag.Roll" );
	
	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWeaponFrag::Deploy( void )
{
	m_bRedraw = false;
	m_fDrawbackFinished = false;
	m_bHasWarnedAI = false;
	m_flDetonateTime = 0;

	return BaseClass::Deploy();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponFrag::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	m_bRedraw = false;
	m_fDrawbackFinished = false;
	m_bHasWarnedAI = false;
	m_flDetonateTime = 0;

	return BaseClass::Holster( pSwitchingTo );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEvent - 
//			*pOperator - 
//-----------------------------------------------------------------------------
void CWeaponFrag::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	bool fThrewGrenade = false;

	switch( pEvent->event )
	{
		case EVENT_WEAPON_SEQUENCE_FINISHED:
			m_fDrawbackFinished = true;
			break;

		case EVENT_WEAPON_THROW:
			ThrowGrenade( pOwner );
			DecrementAmmo( pOwner );
			fThrewGrenade = true;
			break;

		case EVENT_WEAPON_THROW2:
			RollGrenade( pOwner );
			DecrementAmmo( pOwner );
			fThrewGrenade = true;
			break;

		case EVENT_WEAPON_THROW3:
			LobGrenade( pOwner );
			DecrementAmmo( pOwner );
			fThrewGrenade = true;
			break;

		default:
			BaseClass::Operator_HandleAnimEvent( pEvent, pOperator );
			break;
	}

#define RETHROW_DELAY	0.5
	if( fThrewGrenade )
	{
		m_flNextPrimaryAttack	= gpGlobals->curtime + RETHROW_DELAY;
		m_flNextSecondaryAttack	= gpGlobals->curtime + RETHROW_DELAY;
		m_flTimeWeaponIdle = FLT_MAX; //NOTE: This is set once the animation has finished up!

		// Make a sound designed to scare snipers back into their holes!
		CBaseCombatCharacter *pOwner = GetOwner();

		if( pOwner )
		{
			Vector vecSrc = pOwner->Weapon_ShootPosition();
			Vector	vecDir;

			AngleVectors( pOwner->EyeAngles(), &vecDir );

			trace_t tr;

			UTIL_TraceLine( vecSrc, vecSrc + vecDir * 1024, MASK_SOLID_BRUSHONLY, pOwner, COLLISION_GROUP_NONE, &tr );

			CSoundEnt::InsertSound( SOUND_DANGER_SNIPERONLY, tr.endpos, 384, 0.2, pOwner );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponFrag::Reload( void )
{
	if ( !HasPrimaryAmmo() )
		return false;

	if ( ( m_bRedraw ) && ( m_flNextPrimaryAttack <= gpGlobals->curtime ) && ( m_flNextSecondaryAttack <= gpGlobals->curtime ) )
	{
		//Redraw the weapon
		SendWeaponAnim( ACT_VM_DRAW );

		//Update our times
		m_flNextPrimaryAttack	= gpGlobals->curtime + SequenceDuration();
		m_flNextSecondaryAttack	= gpGlobals->curtime + SequenceDuration();
		m_flTimeWeaponIdle = gpGlobals->curtime + SequenceDuration();

		//Mark this as done
		m_bRedraw = false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponFrag::SecondaryAttack( void )
{
	if ( m_bRedraw )
		return;

	if ( !HasPrimaryAmmo() )
		return;

	CBaseCombatCharacter *pOwner  = GetOwner();

	if ( pOwner == NULL )
		return;

	CBasePlayer *pPlayer = ToBasePlayer( pOwner );
	
	if ( pPlayer == NULL )
		return;

	// Note that this is a secondary attack and prepare the grenade attack to pause.
	m_AttackPaused = GRENADE_PAUSED_SECONDARY;
	SendWeaponAnim( ACT_VM_PULLBACK_LOW );

	// Don't let weapon idle interfere in the middle of a throw!
	m_flTimeWeaponIdle = FLT_MAX;
	m_flNextSecondaryAttack	= FLT_MAX;

	// If I'm now out of ammo, switch away
	if ( !HasPrimaryAmmo() )
	{
		pPlayer->SwitchToNextBestWeapon( this );
	}
	
	if ( sk_plr_grenade_is_cookable.GetBool() )
	{
		SetTimer();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponFrag::PrimaryAttack( void )
{
	if ( m_bRedraw )
		return;

	CBaseCombatCharacter *pOwner  = GetOwner();
	
	if ( pOwner == NULL )
	{ 
		return;
	}

	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );;

	if ( !pPlayer )
		return;

	// Note that this is a primary attack and prepare the grenade attack to pause.
	m_AttackPaused = GRENADE_PAUSED_PRIMARY;
	SendWeaponAnim( ACT_VM_PULLBACK_HIGH );
	
	// Put both of these off indefinitely. We do not know how long
	// the player will hold the grenade.
	m_flTimeWeaponIdle = FLT_MAX;
	m_flNextPrimaryAttack = FLT_MAX;

	// If I'm now out of ammo, switch away
	if ( !HasPrimaryAmmo() )
	{
		pPlayer->SwitchToNextBestWeapon( this );
	}
	
	if ( sk_plr_grenade_is_cookable.GetBool() )
	{
		SetTimer();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pOwner - 
//-----------------------------------------------------------------------------
void CWeaponFrag::DecrementAmmo( CBaseCombatCharacter *pOwner )
{
	pOwner->RemoveAmmo( 1, m_iPrimaryAmmoType );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponFrag::ItemPostFrame( void )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	
	if ( pOwner && m_flDetonateTime < gpGlobals->curtime + sk_plr_grenade_cook_grace_period.GetFloat() && m_flDetonateTime != 0 )
	{
		pOwner->m_nButtons &= ~IN_ATTACK;
		pOwner->m_nButtons &= ~IN_ATTACK2;
	}
	
	if( m_fDrawbackFinished )
	{
		if (pOwner)
		{
			switch( m_AttackPaused )
			{
			case GRENADE_PAUSED_PRIMARY:
				if( !(pOwner->m_nButtons & IN_ATTACK) )
				{
					SendWeaponAnim( ACT_VM_THROW );
					m_fDrawbackFinished = false;
				}
				break;

			case GRENADE_PAUSED_SECONDARY:
				if( !(pOwner->m_nButtons & IN_ATTACK2) )
				{
					//See if we're ducking
					if ( pOwner->m_nButtons & IN_DUCK )
					{
						//Send the weapon animation
						SendWeaponAnim( ACT_VM_SECONDARYATTACK );
					}
					else
					{
						//Send the weapon animation
						SendWeaponAnim( ACT_VM_HAULBACK );
					}

					m_fDrawbackFinished = false;
				}
				break;

			default:
				break;
			}
		}
	}

	BaseClass::ItemPostFrame();

	if ( m_bRedraw )
	{
		if ( IsViewModelSequenceFinished() )
		{
			Reload();
		}
	}
}

	// check a throw from vecSrc.  If not valid, move the position back along the line to vecEye
void CWeaponFrag::CheckThrowPosition( CBasePlayer *pPlayer, const Vector &vecEye, Vector &vecSrc )
{
	trace_t tr;

	UTIL_TraceHull( vecEye, vecSrc, -Vector(GRENADE_RADIUS+2,GRENADE_RADIUS+2,GRENADE_RADIUS+2), Vector(GRENADE_RADIUS+2,GRENADE_RADIUS+2,GRENADE_RADIUS+2), 
		pPlayer->PhysicsSolidMaskForEntity(), pPlayer, pPlayer->GetCollisionGroup(), &tr );
	
	if ( tr.DidHit() )
	{
		vecSrc = tr.endpos;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPlayer - 
//-----------------------------------------------------------------------------
void CWeaponFrag::ThrowGrenade( CBasePlayer *pPlayer )
{
	float m_flCookTimeCurrent = sk_plr_grenade_is_cookable.GetBool() ? gpGlobals->curtime - m_flCookTime : 0;
	Vector	vecEye = pPlayer->EyePosition();
	Vector	vForward, vRight;

	pPlayer->EyeVectors( &vForward, &vRight, NULL );
	Vector vecSrc = vecEye + vForward * 18.0f - vRight * 8.0f;
	CheckThrowPosition( pPlayer, vecEye, vecSrc );
//	vForward[0] += 0.1f;
	vForward[2] += 0.1f;

	Vector vecThrow;
	pPlayer->GetVelocity( &vecThrow, NULL );
	vecThrow += vForward * sk_plr_grenade_throw_speed.GetFloat();
	Fraggrenade_PlayerCreate( vecSrc, vec3_angle, vecThrow, AngularImpulse(600,random->RandomInt(-1200,1200),0), pPlayer, sk_plr_grenade_timer.GetFloat() - m_flCookTimeCurrent, m_flNextBlipTime, m_flWarnAITime );

	m_bRedraw = true;

	WeaponSound( SINGLE );

	m_iPrimaryAttacks++;
	gamestats->Event_WeaponFired( pPlayer, true, GetClassname() );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPlayer - 
//-----------------------------------------------------------------------------
void CWeaponFrag::LobGrenade( CBasePlayer *pPlayer )
{
	float m_flCookTimeCurrent = sk_plr_grenade_is_cookable.GetBool() ? gpGlobals->curtime - m_flCookTime : 0;
	Vector	vecEye = pPlayer->EyePosition();
	Vector	vForward, vRight;

	pPlayer->EyeVectors( &vForward, &vRight, NULL );
	Vector vecSrc = vecEye + vForward * 18.0f - vRight * 8.0f + Vector( 0, 0, -8 );
	CheckThrowPosition( pPlayer, vecEye, vecSrc );
	
	Vector vecThrow;
	pPlayer->GetVelocity( &vecThrow, NULL );
	vecThrow += vForward * sk_plr_grenade_lob_speed.GetFloat() + Vector( 0, 0, 50 );
	Fraggrenade_PlayerCreate( vecSrc, vec3_angle, vecThrow, AngularImpulse(200,random->RandomInt(-600,600),0), pPlayer, sk_plr_grenade_timer.GetFloat() - m_flCookTimeCurrent, m_flNextBlipTime, m_flWarnAITime );

	WeaponSound( WPN_DOUBLE );

	m_bRedraw = true;

	m_iPrimaryAttacks++;
	gamestats->Event_WeaponFired( pPlayer, true, GetClassname() );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPlayer - 
//-----------------------------------------------------------------------------
void CWeaponFrag::RollGrenade( CBasePlayer *pPlayer )
{
	float m_flCookTimeCurrent = sk_plr_grenade_is_cookable.GetBool() ? gpGlobals->curtime - m_flCookTime : 0;
	// BUGBUG: Hardcoded grenade width of 4 - better not change the model :)
	Vector vecSrc;
	pPlayer->CollisionProp()->NormalizedToWorldSpace( Vector( 0.5f, 0.5f, 0.0f ), &vecSrc );
	vecSrc.z += GRENADE_RADIUS;

	Vector vecFacing = pPlayer->BodyDirection2D( );
	// no up/down direction
	vecFacing.z = 0;
	VectorNormalize( vecFacing );
	trace_t tr;
	UTIL_TraceLine( vecSrc, vecSrc - Vector(0,0,16), MASK_PLAYERSOLID, pPlayer, COLLISION_GROUP_NONE, &tr );
	if ( tr.fraction != 1.0 )
	{
		// compute forward vec parallel to floor plane and roll grenade along that
		Vector tangent;
		CrossProduct( vecFacing, tr.plane.normal, tangent );
		CrossProduct( tr.plane.normal, tangent, vecFacing );
	}
	vecSrc += (vecFacing * 18.0);
	CheckThrowPosition( pPlayer, pPlayer->WorldSpaceCenter(), vecSrc );

	Vector vecThrow;
	pPlayer->GetVelocity( &vecThrow, NULL );
	vecThrow += vecFacing * sk_plr_grenade_roll_speed.GetFloat();
	// put it on its side
	QAngle orientation(0,pPlayer->GetLocalAngles().y,-90);
	// roll it
	AngularImpulse rotSpeed(0,0,720);
	Fraggrenade_PlayerCreate( vecSrc, orientation, vecThrow, rotSpeed, pPlayer, sk_plr_grenade_timer.GetFloat() - m_flCookTimeCurrent, m_flNextBlipTime, m_flWarnAITime );

	WeaponSound( SPECIAL1 );

	m_bRedraw = true;

	m_iPrimaryAttacks++;
	gamestats->Event_WeaponFired( pPlayer, true, GetClassname() );
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponFrag::SetTimer()
{
	// Don't re-arm the grenade if we're still cooking.
	if ( m_flDetonateTime < gpGlobals->curtime )
	{
		m_flCookTime = gpGlobals->curtime;
		
		// These should be shared between this and the grenade itself for parity.
		m_flNextBlipTime = gpGlobals->curtime + FRAG_GRENADE_BLIP_FREQUENCY;
		m_flWarnAITime = gpGlobals->curtime + ( sk_plr_grenade_timer.GetFloat() - FRAG_GRENADE_WARN_TIME );
		
		m_flDetonateTime = gpGlobals->curtime + sk_plr_grenade_timer.GetFloat();
		
		BlipSound();
	}
	
	SetThink( &CWeaponFrag::BlipThink );
	SetNextThink( gpGlobals->curtime );
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponFrag::BlipThink()
{
	if ( m_bRedraw )
	{
		m_bHasWarnedAI = false;
		m_flDetonateTime = 0;
		return;
	}
	
	if( !m_bHasWarnedAI && gpGlobals->curtime >= m_flWarnAITime )
	{
		
	#if !defined( CLIENT_DLL )
		CSoundEnt::InsertSound ( SOUND_DANGER, GetAbsOrigin(), 400, 1.5, this );
	#endif
	
		m_bHasWarnedAI = true;
	}
	if( gpGlobals->curtime > m_flNextBlipTime )
	{
		BlipSound();		
		if( m_bHasWarnedAI )
		{
			m_flNextBlipTime = gpGlobals->curtime + FRAG_GRENADE_BLIP_FAST_FREQUENCY;
		}
		else
		{
			m_flNextBlipTime = gpGlobals->curtime + FRAG_GRENADE_BLIP_FREQUENCY;
		}
	}
	SetNextThink( gpGlobals->curtime + 0.1 );
}

