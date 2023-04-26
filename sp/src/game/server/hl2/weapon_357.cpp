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
extern ConVar 	sk_realistic_reloading;
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

	float	WeaponAutoAimScale()	{ return 0.6f; }

	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();
private:
float	m_flSoonestPrimaryAttack;
};

LINK_ENTITY_TO_CLASS( weapon_357, CWeapon357 );

PRECACHE_WEAPON_REGISTER( weapon_357 );

IMPLEMENT_SERVERCLASS_ST( CWeapon357, DT_Weapon357 )
END_SEND_TABLE()

BEGIN_DATADESC( CWeapon357 )
END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CWeapon357::CWeapon357( void )
{
	m_bReloadsSingly	= false;
	m_bFiresUnderwater	= false;
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
	}
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
	QAngle viewPunch = QAngle( -4, random->RandomFloat( -2, 2 ), 0 );
	
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
	m_bMagazineStyleReloads = sk_realistic_reloading.GetBool() ? true : false; 
	
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	
	bool m_bCanFire = m_flSoonestPrimaryAttack < gpGlobals->curtime;
	
	if ( ( ( pOwner->m_nButtons & IN_ATTACK ) == false && m_bCanFire ) && ( sk_deagle_style_357.GetBool() ) )
	{
		m_flNextPrimaryAttack = gpGlobals->curtime - 0.1f;
		m_flSoonestPrimaryAttack = gpGlobals->curtime + 0.25;
	}
	
	BaseClass::ItemPostFrame(); 
}
