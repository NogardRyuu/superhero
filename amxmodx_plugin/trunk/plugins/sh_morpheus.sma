// Morpheus! - The Main Man of the Matrix

/* CVARS - copy and paste to shconfig.cfg

//Morpheus
morpheus_level 8
morpheus_gravity 0.35		//Gravity Morpheus has
morpheus_mp5mult 2.0		//Damage multiplyer for his MP5
morpheus_rldmode 0		//Endless Ammo mode: 0-server default, 1-no reload, 2-reload, 3-drop wpn (Default 1)

*/

//---------- User Changeable Defines --------//

// 0-server default, 1-no reload, 2-reload, 3-drop wpn, 4-normal cs [Default 0]
#define AMMO_MODE 0

// Comment out to force not using the model, will result in a very small reduction in code/checks
// Note: If you change anything here from default setting you must recompile the plugin
#define USE_MODEL

//------- Do not edit below this point ------//

#include <superheromod>

// GLOBAL VARIABLES
new gHeroID
new const gHeroName[] = "Morpheus"
new bool:gHasMorpheus[SH_MAXSLOTS+1]
new pCvarMP5Mult

#if defined USE_MODEL
	new const gModelMP5[] = "models/shmod/morpheus_mp5.mdl"
	new bool:gModelLoaded
#endif
//----------------------------------------------------------------------------------------------
public plugin_init()
{
	// Plugin Info
	register_plugin("SUPERHERO Morpheus", SH_VERSION_STR, "RadidEskimo/Freecode")

	// DO NOT EDIT THIS FILE TO CHANGE CVARS, USE THE SHCONFIG.CFG
	new pcvarLevel = register_cvar("morpheus_level", "8")
	new pcvarGravity = register_cvar("morpheus_gravity", "0.35")
	pCvarMP5Mult = register_cvar("morpheus_mp5mult", "2.0")

	// FIRE THE EVENT TO CREATE THIS SUPERHERO!
	gHeroID = sh_create_hero(gHeroName, pcvarLevel)
	sh_set_hero_info(gHeroID, "Dual MP5's", "Lower Gravity/Dual MP5's/Unlimited Ammo")
	sh_set_hero_grav(gHeroID, pcvarGravity, {CSW_MP5NAVY})
	sh_set_hero_shield(gHeroID, true)

	// REGISTER EVENTS THIS HERO WILL RESPOND TO!
	register_event("CurWeapon", "weapon_change", "be", "1=1")
}
//----------------------------------------------------------------------------------------------
public plugin_precache()
{
	// Method servers 2 purposes, moron check and optional way to not use the model
	if ( file_exists(gModelMP5) ) {
		precache_model(gModelMP5)
		gModelLoaded = true
	}
	else {
		sh_debug_message(0, 0, "Aborted loading ^"%s^", file does not exist on server", gModelMP5)
		gModelLoaded = false
	}
}
//----------------------------------------------------------------------------------------------
public sh_hero_init(id, heroID, mode)
{
	if ( gHeroID != heroID ) return

	switch(mode) {
		case SH_HERO_ADD: {
			gHasMorpheus[id] = true

			morpheus_weapons(id)
#if defined USE_MODEL
			if ( gModelLoaded ) {
				switchmodel(id)
			}
#endif
		}

		case SH_HERO_DROP: {
			gHasMorpheus[id] = false
			if ( is_user_alive(id) ) {
				sh_drop_weapon(id, CSW_MP5NAVY, true)
			}
		}
	}

	sh_debug_message(id, 1, "%s %s", gHeroName, mode ? "ADDED" : "DROPPED")
}
//----------------------------------------------------------------------------------------------
public sh_client_spawn(id)
{
	if ( gHasMorpheus[id] ) {
		morpheus_weapons(id)
	}
}
//----------------------------------------------------------------------------------------------
morpheus_weapons(id)
{
	if ( sh_is_active() && is_user_alive(id) && gHasMorpheus[id] ) {
		sh_give_weapon(id, CSW_MP5NAVY)
	}
}
//----------------------------------------------------------------------------------------------
#if defined USE_MODEL
switchmodel(id)
{
	if ( !sh_is_active() || !is_user_alive(id) || !gHasMorpheus[id] ) return

	if ( get_user_weapon(id) == CSW_MP5NAVY ) {
		set_pev(id, pev_viewmodel2, gModelMP5)
	}
}
//----------------------------------------------------------------------------------------------
#endif
public weapon_change(id)
{
	if ( !sh_is_active() || !gHasMorpheus[id] ) return

	//weaponID = read_data(2)
	if ( read_data(2) != CSW_MP5NAVY ) return

#if defined USE_MODEL
	if ( gModelLoaded ) {
		switchmodel(id)
	}
#endif

#if AMMO_MODE < 4
	// Never Run Out of Ammo!
	//clip = read_data(3)
	if ( read_data(3) == 0 ) {
		sh_reload_ammo(id, AMMO_MODE)
	}
#endif
}
//----------------------------------------------------------------------------------------------
public client_damage(attacker, victim, damage, wpnindex, hitplace)
{
	if ( !sh_is_active() ) return
	if ( !is_user_alive(victim) || !is_user_connected(attacker) ) return

	if ( gHasMorpheus[attacker] && wpnindex == CSW_MP5NAVY ) {
		new headshot = hitplace == 1 ? 1 : 0

		// do extra damage
		new extraDamage = floatround(damage * get_pcvar_float(pCvarMP5Mult) - damage)
		if ( extraDamage > 0) sh_extra_damage(victim, attacker, extraDamage, "mp5navy", headshot)
	}
}
//----------------------------------------------------------------------------------------------