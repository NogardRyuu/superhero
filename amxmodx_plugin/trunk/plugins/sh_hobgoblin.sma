// Hob Goblin - Extra Nade Damage/Refill Nade

/* CVARS - copy and paste to shconfig.cfg

//Hob Goblin
goblin_level 0
goblin_grenademult 1.5		//Damage multiplyer from orginal damage amount
goblin_grenadetimer 10		//How many second delay for new grenade

*/

// v1.17 - JTP - Fixed giving new genades using more reliable event

#include <superheromod>

// GLOBAL VARIABLES
new gHeroID
new const gHeroName[]= "Hobgoblin"
new bool:gHasHobgoblin[SH_MAXSLOTS+1]
new bool:gBlockGiveTask[SH_MAXSLOTS+1]
new gPcvarGrenadeMult, gPcvarGrenadeTimer

#define AMMOX_HEGRENADE 12
//----------------------------------------------------------------------------------------------
public plugin_init()
{
	// Plugin Info
	register_plugin("SUPERHERO Hobgoblin", SH_VERSION_STR, "{HOJ} Batman/JTP10181")

	// DO NOT EDIT THIS FILE TO CHANGE CVARS, USE THE SHCONFIG.CFG
	new pcvarLevel = register_cvar("goblin_level", "0")
	gPcvarGrenadeMult = register_cvar("goblin_grenademult", "1.5")
	gPcvarGrenadeTimer = register_cvar("goblin_grenadetimer", "10")

	// FIRE THE EVENTS TO CREATE THIS SUPERHERO!
	gHeroID = sh_create_hero(gHeroName, pcvarLevel)
	sh_set_hero_info(gHeroID, "Hobgoblin Grenades", "Extra Nade Damage/Refill Nade")

	// REGISTER EVENTS THIS HERO WILL RESPOND TO!
	register_event("AmmoX", "on_ammox", "b")
}
//----------------------------------------------------------------------------------------------
public sh_hero_init(id, heroID, mode)
{
	if ( gHeroID != heroID ) return

	switch(mode) {
		case SH_HERO_ADD: {
			gHasHobgoblin[id] = true
			give_grenade(id)
		}
		case SH_HERO_DROP: {
			gHasHobgoblin[id] = false
		}
	}

	sh_debug_message(id, 1, "%s %s", gHeroName, mode ? "ADDED" : "DROPPED")
}
//----------------------------------------------------------------------------------------------
public sh_client_spawn(id)
{
	if ( gHasHobgoblin[id] ) {
		//Block Ammox nade give task on spawn, since you are given a nade on spawn.
		//This must not be delayed, it must catch before inital ammox called.
		gBlockGiveTask[id] = true

		give_grenade(id)
	}
}
//----------------------------------------------------------------------------------------------
public give_grenade(id)
{
	if ( sh_is_active() && is_user_alive(id) && gHasHobgoblin[id] ) {
		sh_give_weapon(id, CSW_HEGRENADE)
	}
}
//----------------------------------------------------------------------------------------------
public client_damage(attacker, victim, damage, wpnindex)
{
	if ( !sh_is_active() || damage <= 0 ) return
	if ( !is_user_alive(victim) || !is_user_connected(attacker) ) return

	if ( gHasHobgoblin[attacker] && wpnindex == CSW_HEGRENADE ) {
		// do extra damage
		new extraDamage = floatround(damage * get_pcvar_float(gPcvarGrenadeMult) - damage)
		if ( extraDamage > 0 ) sh_extra_damage(victim, attacker, extraDamage, "grenade")
	}
}
//----------------------------------------------------------------------------------------------
public on_ammox(id)
{
	//Ammox is used in case other heroes give nades so the task can be removed when nade is refilled.
	if ( !sh_is_active() || !is_user_alive(id) || !gHasHobgoblin[id] ) return

	//new iAmmoType = read_data(1)
	if ( read_data(1) == AMMOX_HEGRENADE ) {
		new iAmmoCount = read_data(2)

		if ( iAmmoCount == 0 && !gBlockGiveTask[id] ) {
			//This will be called on spawn as well as when nade is thrown, block this on spawn.
			//Nade was thrown set task to give another.
			set_task(get_pcvar_float(gPcvarGrenadeTimer), "give_grenade", id)
		}
		else if ( iAmmoCount > 0 ) {
			gBlockGiveTask[id] = false
			remove_task(id)
		}
	}
}
//----------------------------------------------------------------------------------------------
public client_connect(id)
{
	gHasHobgoblin[id] = false
}
//----------------------------------------------------------------------------------------------