//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		SLAM 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//

#ifndef	WEAPONSLAM_H
#define	WEAPONSLAM_H

#include "basegrenade_shared.h"
#include "basehlcombatweapon.h"

enum
{
	SLAM_TRIPMINE_READY,
	SLAM_SATCHEL_THROW,
	SLAM_SATCHEL_ATTACH,
};

class CWeapon_SLAM : public CBaseHLCombatWeapon
{
public:
	DECLARE_CLASS( CWeapon_SLAM, CBaseHLCombatWeapon );

	DECLARE_SERVERCLASS();

	int				m_tSlamState;
	bool				m_bDetonatorArmed;
	bool				m_bNeedDetonatorDraw;
	bool				m_bNeedDetonatorHolster;
	bool				m_bNeedReload;
	bool				m_bClearReload;
	bool				m_bThrowSatchel;
	bool				m_bAttachTripmine;
	float				m_flWallSwitchTime;
	float				m_flAttachGraceTime;
	bool				m_bNeedToggle;

	void				Spawn( void );
	void				Precache( void );

	void				PrimaryAttack( void );
	void				SecondaryAttack( void );
	void				WeaponIdle( void );
	void				Weapon_Switch( void );
	void				SLAMThink( void );
	
	void				SetPickupTouch( void );
	void				SlamTouch( CBaseEntity *pOther );	// default weapon touch
	void				ItemPostFrame( void );	
	bool				Reload( void );
	void				SetSlamState( int newState );
	bool				CanAttachSLAM(void);		// In position where can attach SLAM?
	bool				AnyUndetonatedCharges(void);
	void				StartTripmineAttach( void );
	bool				TripmineAttach( void );

	void				StartSatchelDetonate( void );
	void				SatchelDetonate( void );
	void				StartSatchelThrow( void );
	void				SatchelThrow( void );
	bool				Deploy( void );
	bool				Holster( CBaseCombatWeapon *pSwitchingTo = NULL );
	float				GetActivityAnimSpeed( Activity ideal );
	bool				HasAnyAmmo( void );
	void 				ItemHolsterFrame( void );

	CWeapon_SLAM();

	DECLARE_ACTTABLE();
	DECLARE_DATADESC();
};


#endif	//WEAPONSLAM_H
