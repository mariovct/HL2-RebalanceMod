//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		357 - hand gun
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "npcevent.h"
#include "basehlcombatweapon.h"
#include "basecombatcharacter.h"
#include "ai_basenpc.h"
#include "player.h"
#include "gamerules.h"
#include "in_buttons.h"
#include "soundent.h"
#include "game.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "te_effect_dispatch.h"
#include "gamestats.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar sk_deagle_style_357("sk_deagle_style_357", "0" );
extern ConVar 	hl2r_realistic_reload;
extern ConVar sk_alternate_recoil;

//-----------------------------------------------------------------------------
// CWeapon357
//-----------------------------------------------------------------------------

class CWeapon357 : public CBaseHLCombatWeapon
{
	DECLARE_CLASS( CWeapon357, CBaseHLCombatWeapon );
public:

	CWeapon357( void );
	
	void	ItemPostFrame( void );

	void	PrimaryAttack( void );
	void	Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );
	float 	GetActivityAnimSpeed( Activity ideal );

	float	WeaponAutoAimScale()	{ return 0.6f; }
	
	int		CapabilitiesGet( void ) { return bits_CAP_WEAPON_RANGE_ATTACK1; }
	
	virtual int		GetMinBurst(){ return 1;}
	virtual int		GetMaxBurst(){ return 1;}
	virtual float	GetMinRestTime(){ return 1.5; };
	virtual float	GetMaxRestTime(){ return 2.0; };
	
	virtual float 	GetFireRate(){ return 1.0; }
	
	void FireNPCPrimaryAttack( CBaseCombatCharacter *pOperator, Vector &vecShootOrigin, Vector &vecShootDir );
	void Operator_ForceNPCFire( CBaseCombatCharacter *pOperator, bool bSecondary );
	
	virtual const Vector& GetBulletSpread( void )
	{
		static Vector cone = VECTOR_CONE_1DEGREES;
		return cone;
	}

	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();
private:
float	m_flSoonestPrimaryAttack;

DECLARE_ACTTABLE();
};

LINK_ENTITY_TO_CLASS( weapon_357, CWeapon357 );

PRECACHE_WEAPON_REGISTER( weapon_357 );

IMPLEMENT_SERVERCLASS_ST( CWeapon357, DT_Weapon357 )
END_SEND_TABLE()

BEGIN_DATADESC( CWeapon357 )
END_DATADESC()

// Use Pistol animations as a base. All we really need are the reload and fire anims.
acttable_t	CWeapon357::m_acttable[] = 
{
	{ ACT_IDLE,						ACT_IDLE_PISTOL,				true },
	{ ACT_IDLE_ANGRY,				ACT_IDLE_ANGRY_PISTOL,			true },
	//{ ACT_RANGE_ATTACK1,			ACT_RANGE_ATTACK_PISTOL,		true },
	//{ ACT_RELOAD,					ACT_RELOAD_PISTOL,				true },
	{ ACT_RANGE_ATTACK1,			ACT_RANGE_ATTACK_REVOLVER,		true },
	{ ACT_RELOAD,					ACT_RELOAD_REVOLVER,			true },
	{ ACT_WALK_AIM,					ACT_WALK_AIM_PISTOL,			true },
	{ ACT_RUN_AIM,					ACT_RUN_AIM_PISTOL,				true },
	//{ ACT_GESTURE_RANGE_ATTACK1,	ACT_GESTURE_RANGE_ATTACK_PISTOL,true },
	//{ ACT_RELOAD_LOW,				ACT_RELOAD_PISTOL_LOW,			false },
	{ ACT_GESTURE_RANGE_ATTACK1,	ACT_GESTURE_RANGE_ATTACK_REVOLVER,true },
	{ ACT_RELOAD_LOW,				ACT_RELOAD_REVOLVER_LOW,		false },
	//{ ACT_RANGE_ATTACK1_LOW,		ACT_RANGE_ATTACK_PISTOL_LOW,	false },
	{ ACT_RANGE_ATTACK1_LOW,		ACT_RANGE_ATTACK_REVOLVER_LOW,	false },
	{ ACT_COVER_LOW,				ACT_COVER_PISTOL_LOW,			false },
	{ ACT_RANGE_AIM_LOW,			ACT_RANGE_AIM_PISTOL_LOW,		false },
	//{ ACT_GESTURE_RELOAD,			ACT_GESTURE_RELOAD_PISTOL,		false },
	{ ACT_GESTURE_RELOAD,			ACT_GESTURE_RELOAD_REVOLVER,	false },
	{ ACT_WALK,						ACT_WALK_PISTOL,				false },
	{ ACT_RUN,						ACT_RUN_PISTOL,					false },
};

IMPLEMENT_ACTTABLE( CWeapon357 );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CWeapon357::CWeapon357( void )
{
	m_bReloadsSingly	= false;
	m_bFiresUnderwater	= false;

	m_fMinRange1		= 0;
	m_fMaxRange1		= 1500;
	m_fMinRange2		= 0;
	m_fMaxRange2		= 200;
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeapon357::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	switch( pEvent->event )
	{
		case EVENT_WEAPON_RELOAD:
			{
				CEffectData data;

				// Emit six spent shells
				for ( int i = 0; i < 6; i++ )
				{
					data.m_vOrigin = pOwner->WorldSpaceCenter() + RandomVector( -4, 4 );
					data.m_vAngles = QAngle( 90, random->RandomInt( 0, 360 ), 0 );
					data.m_nEntIndex = entindex();

					DispatchEffect( "ShellEject", data );
				}

				break;
			}
		case EVENT_WEAPON_PISTOL_FIRE:
			{
				Vector vecShootOrigin, vecShootDir;
				vecShootOrigin = pOperator->Weapon_ShootPosition();

				CAI_BaseNPC *npc = pOperator->MyNPCPointer();
				ASSERT( npc != NULL );

				vecShootDir = npc->GetActualShootTrajectory( vecShootOrigin );

				FireNPCPrimaryAttack( pOperator, vecShootOrigin, vecShootDir );
			}
			break;
		default:
			BaseClass::Operator_HandleAnimEvent( pEvent, pOperator );
			break;
	}
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeapon357::FireNPCPrimaryAttack( CBaseCombatCharacter *pOperator, Vector &vecShootOrigin, Vector &vecShootDir )
{
	CSoundEnt::InsertSound( SOUND_COMBAT|SOUND_CONTEXT_GUNFIRE, pOperator->GetAbsOrigin(), SOUNDENT_VOLUME_PISTOL, 0.2, pOperator, SOUNDENT_CHANNEL_WEAPON, pOperator->GetEnemy() );

	WeaponSound( SINGLE_NPC );
	pOperator->FireBullets( 1, vecShootOrigin, vecShootDir, VECTOR_CONE_PRECALCULATED, MAX_TRACE_LENGTH, m_iPrimaryAmmoType, 1 );
	pOperator->DoMuzzleFlash();
	m_iClip1 = m_iClip1 - 1;
}

//-----------------------------------------------------------------------------
// Purpose: Some things need this. (e.g. the new Force(X)Fire inputs or blindfire actbusy)
//-----------------------------------------------------------------------------
void CWeapon357::Operator_ForceNPCFire( CBaseCombatCharacter *pOperator, bool bSecondary )
{
	// Ensure we have enough rounds in the clip
	m_iClip1++;

	Vector vecShootOrigin, vecShootDir;
	QAngle	angShootDir;
	GetAttachment( LookupAttachment( "muzzle" ), vecShootOrigin, angShootDir );
	AngleVectors( angShootDir, &vecShootDir );
	FireNPCPrimaryAttack( pOperator, vecShootOrigin, vecShootDir );
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeapon357::PrimaryAttack( void )
{
	// Only the player fires this way so we can cast
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	
	m_flSoonestPrimaryAttack = gpGlobals->curtime + 0.25;

	if ( !pPlayer )
	{
		return;
	}

	if ( m_iClip1 <= 0 )
	{
		if ( !m_bFireOnEmpty )
		{
			Reload();
		}
		else
		{
			WeaponSound( EMPTY );
			m_flNextPrimaryAttack = 0.15;
		}

		return;
	}

	m_iPrimaryAttacks++;
	gamestats->Event_WeaponFired( pPlayer, true, GetClassname() );

	WeaponSound( SINGLE );
	pPlayer->DoMuzzleFlash();

	SendWeaponAnim( ACT_VM_PRIMARYATTACK );
	pPlayer->SetAnimation( PLAYER_ATTACK1 );

	m_flNextPrimaryAttack = sk_deagle_style_357.GetBool() ? FLT_MAX : gpGlobals->curtime + 0.75;
	m_flNextSecondaryAttack = gpGlobals->curtime + 0.75;

	m_iClip1--;

	Vector vecSrc		= pPlayer->Weapon_ShootPosition();
	Vector vecAiming	= pPlayer->GetAutoaimVector( AUTOAIM_SCALE_DEFAULT );	

	pPlayer->FireBullets( 1, vecSrc, vecAiming, vec3_origin, MAX_TRACE_LENGTH, m_iPrimaryAmmoType, 1 );

	pPlayer->SetMuzzleFlashTime( gpGlobals->curtime + 0.5 );

	//Disorient the player
	QAngle viewPunch = QAngle( -8 * ( sk_deagle_style_357.GetBool() ? 0.5 : 1.0 ), random->RandomFloat( -2, 2 ), 0 );
	
	if ( sk_alternate_recoil.GetBool() )
	{
		QAngle angles = pPlayer->GetLocalAngles();
		
		angles += viewPunch * 0.5;
		
		pPlayer->SnapEyeAngles( angles );
	}
	else
	{
		//Disorient the player
		QAngle angles = pPlayer->GetLocalAngles();

		angles.x += random->RandomInt( -0.5, -0.5 );
		angles.y += random->RandomInt( -0.5, -0.5 );
		angles.z = 0;

		pPlayer->SnapEyeAngles( angles );
	}

	pPlayer->ViewPunch( viewPunch );

	CSoundEnt::InsertSound( SOUND_COMBAT, GetAbsOrigin(), 600, 0.2, GetOwner() );

	if ( !m_iClip1 && pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) <= 0 )
	{
		// HEV suit - indicate out of ammo condition
		pPlayer->SetSuitUpdate( "!HEV_AMO0", FALSE, 0 ); 
	}
}
void CWeapon357::ItemPostFrame( void )
{ 
	m_bMagazineStyleReloads = hl2r_realistic_reload.GetBool() ? true : false; 
	
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	
	bool m_bCanFire = m_flSoonestPrimaryAttack < gpGlobals->curtime;
	
	if ( ( ( pOwner->m_nButtons & IN_ATTACK ) == false && m_bCanFire ) && ( sk_deagle_style_357.GetBool() ) )
	{
		m_flNextPrimaryAttack = gpGlobals->curtime - 0.1f;
		m_flSoonestPrimaryAttack = gpGlobals->curtime + 0.35;
	}
	
	BaseClass::ItemPostFrame(); 
}
float CWeapon357::GetActivityAnimSpeed( Activity ideal )
{
	if ( ideal == ACT_VM_RELOAD && sk_deagle_style_357.GetBool() )
	{
		return 1.25;
	}
	
	return BaseClass::GetActivityAnimSpeed(ideal); 
}
