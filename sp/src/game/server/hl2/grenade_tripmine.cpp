//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Implements the tripmine grenade.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "beam_shared.h"
#include "shake.h"
#include "grenade_tripmine.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "explode.h"
#include "npc_bullseye.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define BEAM_RANGE 512

extern const char* g_pModelNameLaser;

extern ConVar sk_plr_dmg_slam;
extern ConVar sk_slam_radius;

LINK_ENTITY_TO_CLASS( npc_tripmine, CTripmineGrenade );

BEGIN_DATADESC( CTripmineGrenade )

	DEFINE_FIELD( m_hOwner,		FIELD_EHANDLE ),
	DEFINE_FIELD( m_flPowerUp,	FIELD_TIME ),
	//DEFINE_FIELD( m_vecDir,		FIELD_VECTOR ),
	DEFINE_FIELD( m_vecEnd,		FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( m_flBeamLength, FIELD_FLOAT ),
	DEFINE_FIELD( m_pBeam,		FIELD_CLASSPTR ),
	DEFINE_FIELD( m_posOwner,		FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( m_angleOwner,	FIELD_VECTOR ),
	DEFINE_FIELD( m_bHasParent,	FIELD_BOOLEAN ),

	// Function Pointers
	DEFINE_THINKFUNC( WarningThink ),
	DEFINE_THINKFUNC( PowerupThink ),
	DEFINE_THINKFUNC( BeamBreakThink ),
	DEFINE_THINKFUNC( DelayDeathThink ),

END_DATADESC()

CTripmineGrenade::CTripmineGrenade()
{
	//m_vecDir.Init();
	m_vecEnd.Init();
	m_posOwner.Init();
	m_angleOwner.Init();
}

void CTripmineGrenade::Spawn( void )
{
	Precache( );
	// motor
	SetMoveType( MOVETYPE_FLY );
	SetSolid( SOLID_BBOX );
	SetModel( "models/Weapons/w_slam.mdl" );

    IPhysicsObject *pObject = VPhysicsInitNormal( SOLID_BBOX, GetSolidFlags() | FSOLID_TRIGGER, true );
	pObject->EnableMotion( false );
	SetCollisionGroup( COLLISION_GROUP_WEAPON );

	SetCycle( 0.0f );
	m_nBody			= 3;
	m_flDamage		= sk_plr_dmg_slam.GetFloat();
	m_DmgRadius		= sk_slam_radius.GetFloat();

	ResetSequenceInfo( );
	m_flPlaybackRate	= 0;
	
	UTIL_SetSize(this, Vector( -4, -4, -2), Vector(4, 4, 2));

	m_flPowerUp = gpGlobals->curtime + 2.0;
	
	SetThink( &CTripmineGrenade::PowerupThink );
	SetNextThink( gpGlobals->curtime + 0.2 );

	m_takedamage		= DAMAGE_YES;

	m_iHealth = 1;

	EmitSound( "TripmineGrenade.Place" );
	SetDamage ( m_flDamage );
	
	m_vecEnd = GetAbsOrigin() + GetFacingDirection() * BEAM_RANGE;

	AddEffects( EF_NOSHADOW );
}


void CTripmineGrenade::Precache( void )
{
	PrecacheModel("models/Weapons/w_slam.mdl"); 

	PrecacheScriptSound( "TripmineGrenade.Place" );
	PrecacheScriptSound( "TripmineGrenade.Activate" );
}


void CTripmineGrenade::WarningThink( void  )
{
	// set to power up
	SetThink( &CTripmineGrenade::PowerupThink );
	SetNextThink( gpGlobals->curtime + 1.0f );
}


void CTripmineGrenade::PowerupThink( void  )
{
	if (gpGlobals->curtime > m_flPowerUp)
	{
		MakeBeam( );
		RemoveSolidFlags( FSOLID_NOT_SOLID );
		m_bIsLive			= true;

		// play enabled sound
		EmitSound( "TripmineGrenade.Activate" );
	}
	SetNextThink( gpGlobals->curtime + 0.1f );
}


void CTripmineGrenade::KillBeam( void )
{
	if ( m_pBeam )
	{
		UTIL_Remove( m_pBeam );
		m_pBeam = NULL;
	}
}


void CTripmineGrenade::MakeBeam( void )
{
	trace_t tr;
	UTIL_TraceLine( GetAbsOrigin(), m_vecEnd, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );

	// If I hit a living thing, send the beam through me so it turns on briefly
	// and then blows the living thing up
	if ( tr.m_pEnt != NULL )
	{
		CBaseEntity *pEntity = tr.m_pEnt;
		CBaseCombatCharacter *pBCC  = ToBaseCombatCharacter( pEntity );

		if (pBCC)
		{
			SetOwnerEntity( pBCC );
			UTIL_TraceLine( GetAbsOrigin(), m_vecEnd, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );
			m_flBeamLength = tr.fraction;
			SetOwnerEntity( NULL );
			
		}
	}
	
	
/* 	CNPC_Bullseye *pBullseye = (CNPC_Bullseye*)CreateEntityByName( "npc_bullseye" );

	ASSERT( pBullseye != NULL );

	pBullseye->AddSpawnFlags( SF_BULLSEYE_NONSOLID );
	pBullseye->SetAbsOrigin( GetAbsOrigin() );
	
	pBullseye->Spawn();
	DispatchSpawn(pBullseye);
	pBullseye->Activate();
	
	pBullseye->SetParent(this);
	pBullseye->SetHealth(1); */
		
	// set to follow laser spot
	SetThink( &CTripmineGrenade::BeamBreakThink );

	// Delay first think slightly so beam has time
	// to appear if person right in front of it
	SetNextThink( gpGlobals->curtime + 1.0f );

	m_pBeam = CBeam::BeamCreate( g_pModelNameLaser, 0.35 );
	m_pBeam->PointEntInit( tr.endpos, this );
	m_pBeam->SetColor( 255, 55, 52 );
	m_pBeam->SetScrollRate( 25.6 );
	m_pBeam->SetBrightness( 100 );
	m_pBeam->SetWidth( 0.3f - ( tr.fraction * 0.3f ) );
	m_pBeam->SetEndWidth( 0.3f );
	
	int beamAttach = LookupAttachment("beam_attach");
	m_pBeam->SetEndAttachment( beamAttach );
}


void CTripmineGrenade::BeamBreakThink( void  )
{
	// Update our angles if we're attached to a physics object.
	if ( GetMoveParent() )
	{
		m_bHasParent = true;
		m_vecEnd = GetAbsOrigin() + GetFacingDirection() * BEAM_RANGE;
		
		// Update our beam if we have one.
		if ( m_pBeam != NULL )
		{
			trace_t tr;
			UTIL_TraceLine( GetAbsOrigin(), m_vecEnd, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );
				
			m_pBeam->PointEntInit( tr.endpos, this );
			m_pBeam->SetWidth( 1.0 - tr.fraction );
				
			int beamAttach = LookupAttachment("beam_attach");
			m_pBeam->SetEndAttachment( beamAttach );
		}
	}
	
	// Our parent doesn't exist anymore, explode!
	if ( m_bHasParent && GetMoveParent() == NULL )
	{
		m_iHealth = 0;
		DelayDeathThink();
		return;
	}

	trace_t tr;

	// NOT MASK_SHOT because we want only simple hit boxes
	UTIL_TraceLine( GetAbsOrigin(), m_vecEnd, (MASK_SOLID&(~CONTENTS_GRATE)), this, COLLISION_GROUP_NONE, &tr );

	if ( tr.m_pEnt != NULL )
	{
		CBaseEntity *pEntity = tr.m_pEnt;
		CBaseCombatCharacter *pBCC = ToBaseCombatCharacter( pEntity );

		// Detect npc's and moving objects(excluding our parent)
		if ( pBCC || ( pEntity->VPhysicsGetObject() && pEntity->GetSmoothedVelocity().Length() > 10.0 && pEntity != GetParent() ) )
		{
			m_iHealth = 0;
			Event_Killed( CTakeDamageInfo( (CBaseEntity*)m_hOwner, this, 100, GIB_NORMAL ) );
			return;
		}
	}

	SetNextThink( gpGlobals->curtime + 0.01f );
}

#if 0 // FIXME: OnTakeDamage_Alive() is no longer called now that base grenade derives from CBaseAnimating
int CTripmineGrenade::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
	if (gpGlobals->curtime < m_flPowerUp && info.GetDamage() < m_iHealth)
	{
		// disable
		SetThink( &CTripmineGrenade::SUB_Remove );
		SetNextThink( gpGlobals->curtime + 0.1f );
		KillBeam();
		return FALSE;
	}
	return BaseClass::OnTakeDamage_Alive( info );
}
#endif

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CTripmineGrenade::Event_Killed( const CTakeDamageInfo &info )
{
	m_takedamage		= DAMAGE_NO;
	
	// Explode immediately if we're hit by explosives
//	if ( info.GetDamageType() & DMG_BLAST ) 
//	{
		EmitSound( "TripmineGrenade.StopSound" );
		DelayDeathThink();
//	}

	SetThink( &CTripmineGrenade::DelayDeathThink );
	SetNextThink( gpGlobals->curtime + 0.25 );

	EmitSound( "TripmineGrenade.StopSound" );
}


void CTripmineGrenade::DelayDeathThink( void )
{
	KillBeam();
	
	trace_t tr;
	UTIL_TraceLine ( GetAbsOrigin() + GetFacingDirection() * 8, GetAbsOrigin() - GetFacingDirection() * 64,  MASK_SOLID, this, COLLISION_GROUP_NONE, & tr);
	UTIL_ScreenShake( GetAbsOrigin(), 25.0, 150.0, 1.0, 750, SHAKE_START );

	ExplosionCreate( GetAbsOrigin() + GetFacingDirection() * 8, GetAbsAngles(), m_hOwner, GetDamage(), 200, 
		SF_ENVEXPLOSION_NOSPARKS | SF_ENVEXPLOSION_NODLIGHTS | SF_ENVEXPLOSION_NOSMOKE, 0.0f, this);

	UTIL_Remove( this );
}

Vector CTripmineGrenade::GetFacingDirection( void )
{
	Vector vecForward;
	GetVectors( NULL, NULL, &vecForward );
	
	return vecForward;
}