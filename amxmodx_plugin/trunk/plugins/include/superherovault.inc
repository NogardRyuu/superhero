// SuperHero Vault Include File
// Version 1.2.0

//----------------------------------------------------------------------------------------------
// Abstract Methods that must be implemented by a save superhero include file...
//
// memoryTableWrite()
// loadXP(id, const savekey[32])
// cleanXP(clearAll = false)
//----------------------------------------------------------------------------------------------

#if defined _shsave_included || !defined SHCORE
  #endinput
#endif
#define _shsave_included

// Global to this save class
new gVaultFile[128]
//----------------------------------------------------------------------------------------------
saving_init()
{
	get_localinfo("amxx_vault", gVaultFile, 127)
}
//----------------------------------------------------------------------------------------------
parseXP(id, const data[])
{
	// "name" epochtime XP HUDHELP "SKILL1" "SKILL2" ...
	new heroIndex
	static Left[32], Right[1501]
	Left[0] = '^0'
	Right[0] = '^0'

	copy(Right, 1500, data)

	//Get the name out
	strbrkqt(Right, Left, 31, Right, 1500)

	//Get the epochtime out
	strbrkqt(Right, Left, 31, Right, 1500)

	//Get the XP out and save it
	strbrkqt(Right, Left, 31, Right, 1500)
	gPlayerXP[id] = str_to_num(Left)
	gPlayerLevel[id] = getPlayerLevel(id)
	setLevel(id, gPlayerLevel[id])

	//Get the User Flags out and save them
	strbrkqt(Right, Left, 31, Right, 1500)
	gPlayerFlags[id] = str_to_num(Left)

	//Now load in all the skills
	new powerCount = 0
	new MaxPowers = min(get_pcvar_num(sh_maxpowers), SH_MAXLEVELS)

	while ( Right[0] != '^0' && powerCount < MaxPowers ) {
		strbrkqt(Right, Left, 31, Right, 1500)
		heroIndex = getHeroID(Left)
		if ( -1 < heroIndex < gSuperHeroCount )  {
			if ( getHeroLevel(heroIndex) <= gPlayerLevel[id] ) {
				gPlayerPowers[id][0] = ++powerCount
				gPlayerPowers[id][powerCount] = heroIndex
				initHero(id, heroIndex, SH_HERO_ADD)
			}
		}
	}
}
//----------------------------------------------------------------------------------------------
public memoryTableWrite()
{
	if ( !get_cvar_num("sh_savexp") ) return

	debugMsg(0, 3, "Writing XP Data to Vault file")

	// Write Memory Table to File
	for ( new x = 1; x < gMemoryTableCount; x++ ) {
		if ( gMemoryTableKeys[x][0] != '^0' ) {
			writeData(x)
		}
		// Can even clear the MemoryKey - if a player is disconnected no reason to save again and again...
		gMemoryTableKeys[x][0] = '^0'
	}

	// No need to keep disconnected players around if this has been flushed...
	gMemoryTableCount = 33
}
//----------------------------------------------------------------------------------------------
writeData(id)
{
	// Formats Player Information and writes to file:
	// "name" epochtime XP USERFLAGS "SKILL1" "SKILL2" ...
	static data[1501], vaultSaveKey[39]
	new heroIndex, len
	new powerCount = gMemoryTablePowers[id][0]
	new maxlen = 1500

	data[0] = '^0'
	vaultSaveKey[0] = '^0'

	len += formatex(data[len], maxlen-len, "^"%s^" %d %d %d", gMemoryTableNames[id], get_systime(), gMemoryTableXP[id], gMemoryTableFlags[id])

	// x is not a hero index but the amount of heroes to get gNumLevels
	for ( new x = 1; x <= powerCount && x <= gSuperHeroCount && len < maxlen; x++ ) {
		heroIndex = gMemoryTablePowers[id][x]
		if ( -1 < heroIndex < gSuperHeroCount ) {
			len += formatex(data[len], maxlen-len, " ^"%s^"", gSuperHeros[heroIndex][hero])
		}
	}

	formatex(vaultSaveKey, 38, "shinfo.%s", gMemoryTableKeys[id])

	set_vaultdata(vaultSaveKey, data)

	if ( 0 < id <= SH_MAXSLOTS ) {
		gChangedHeroes[id] = false
	}
}
//----------------------------------------------------------------------------------------------
loadXP(id, const savekey[32])
{
	static data[1501], vaultSaveKey[39]
	data[0] = '^0'
	vaultSaveKey[0] = '^0'

	formatex(vaultSaveKey, 38, "shinfo.%s", savekey)
	get_vaultdata(vaultSaveKey, data, 1500)

	if ( strlen(data) > 12 ) {
		parseXP(id, data)
	}

	memoryTableUpdate(id)

	return true
}
//----------------------------------------------------------------------------------------------
cleanXP(bool:clearAll)
{
	// Open up the vault file - read line by line
	// Use vault to delete to any data over a certain age
	if ( !file_exists(gVaultFile) ) return

	new xpSaveDays = get_pcvar_num(sh_xpsavedays)

	if ( !clearAll && (!get_pcvar_num(sh_savexp) || xpSaveDays < 0) ) return

	// Create a copy of savekeys to remove
	new temp[128]
	formatex(temp, 127, "%s~", gVaultFile)

	new vaultFile = fopen(gVaultFile, "r")
	new tempFile = fopen(temp, "w+")

	if ( !vaultFile ) {
		debugMsg(0, 0, "Failed to open vault.ini, please verify file/folder permissions")
		return
	}

	if ( !tempFile ) {
		debugMsg(0, 0, "Failed to create temp file for cleaning vault.ini, please verify file/folder permissions")
		return
	}

	// Read through the file looking for shinfo entries
	// Check the epoch time and immune flag to see if deletion should happen
	new data[128], vaultSaveKey[39], name[32], epoch[16], xp[12], flags[4]
	new maxTime = xpSaveDays * 24 * 3600
	new curTime = get_systime()
	new bool:dataFound

	while ( !feof(vaultFile) ) {
		data[0] = '^0'

		// Read the line in vault and if sh save info check to see if we should copy vault key to temp file
		// Don't need to read whole line only enough for what we need to parse
		fgets(vaultFile, data, 127)

		if ( equal(data, "shinfo", 6) ) {
			vaultSaveKey[0] = '^0'

			if ( clearAll ) {
				// Copy all keys to be cleared
				parse(data, vaultSaveKey, 38)
				fprintf(tempFile, "%s^n", vaultSaveKey)
			}
			else {
				// Only copy keys if older than max save days and not immune to reset
				epoch[0] = '^0'
				flags[0] = '^0'
				parse(data, vaultSaveKey, 38, name, 31, epoch, 15, xp, 11, flags, 3)

				if ( (str_to_num(epoch) + maxTime) < curTime && !(str_to_num(flags)&SH_FLAG_XPIMMUNE) ) {
					fprintf(tempFile, "%s^n", vaultSaveKey)
					dataFound = true
				}
			}
		}
	}

	// Finished reading vault close it
	fclose(vaultFile)

	if ( dataFound ) {
		// Reset to top of temp file
		fseek(tempFile, 0, SEEK_SET)

		// Remove all entries in temp file from vault.ini
		while ( !feof(tempFile) ) {
			vaultSaveKey[0] = '^0'

			fgets(tempFile, vaultSaveKey, 38)
			trim(vaultSaveKey)

			if ( vaultSaveKey[0] != '^0' ) remove_vaultdata(vaultSaveKey)
		}
	}

	fclose(tempFile)
	delete_file(temp)
}
//----------------------------------------------------------------------------------------------
saving_end() {/*Don't need to clean up vault saving, but keep this here*/}
//----------------------------------------------------------------------------------------------