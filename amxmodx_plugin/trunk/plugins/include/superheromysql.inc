// SuperHero MySQL Include File
// Version 1.2.0

//----------------------------------------------------------------------------------------------
// Abstract Methods that must be implemented by a save superhero include file...
//
// memoryTableWrite()
// loadXP(id, const savekey[32])
// cleanXP(clearAll = false)
//----------------------------------------------------------------------------------------------
/*

//Plugin will auto create the tables if they do not exist

//If shmod's mysql cvars not set plugin will try to setup
//database in amxmodx's database if one exists

//Use these to create the tables manually if necessary

CREATE TABLE `sh_savexp` (
	`SH_KEY` varchar(32) binary NOT NULL default '',
	`PLAYER_NAME` varchar(32) binary NOT NULL default '',
	`LAST_PLAY_DATE` timestamp(14) NOT NULL,
	`XP` int(10) NOT NULL default '0',
	`HUDHELP` tinyint(3) unsigned NOT NULL default '1',
	`SKILL_COUNT` tinyint(3) unsigned NOT NULL default '0',
	PRIMARY KEY  (`SH_KEY`)
) TYPE=MyISAM COMMENT='SUPERHERO XP Saving Table';

CREATE TABLE `sh_saveskills` (
	`SH_KEY` varchar(32) binary NOT NULL default '',
	`SKILL_NUMBER` tinyint(3) unsigned NOT NULL default '0',
	`HERO_NAME` varchar(25) NOT NULL default '',
	PRIMARY KEY  (`SH_KEY`,`SKILL_NUMBER`)
) TYPE=MyISAM COMMENT='SUPERHERO Skill Saving Table';

//Upgrade from prior to 1.17.5
ALTER TABLE `sh_savexp` ADD `HUDHELP` TINYINT( 3 ) UNSIGNED DEFAULT '1' NOT NULL AFTER `XP`;

//Upgraded from prior to 1.20 (XP from unsigned to signed and usage of tinyint over int)
ALTER TABLE `sh_savexp` CHANGE `XP` `XP` INT( 10 ) SIGNED NOT NULL DEFAULT '0';
ALTER TABLE `sh_savexp` CHANGE `HUDHELP` `HUDHELP` TINYINT( 3 ) UNSIGNED NOT NULL DEFAULT '1';
ALTER TABLE `sh_savexp` CHANGE `SKILL_COUNT` `SKILL_COUNT` TINYINT( 3 ) UNSIGNED NOT NULL DEFAULT '0';
ALTER TABLE `sh_saveskills` CHANGE `SKILL_NUMBER` `SKILL_NUMBER` TINYINT( 3 ) UNSIGNED NOT NULL DEFAULT '0';

*/

#if defined _shsave_included || !defined SHCORE
  #endinput
#endif
#define _shsave_included

//Comment this out to use old syntax for MySQL 3.23
#define NEW_MYSQL

#include <dbi>

// Global to this save class
new Sql:gMySQL = SQL_FAILED
new gVaultFile[128]
new bool:gPersistentTemp
new bool:CreateTables = true
new sh_mysql_host, sh_mysql_user, sh_mysql_pass
new sh_mysql_db, sh_mysql_persistent
//----------------------------------------------------------------------------------------------
saving_init()
{
	sh_mysql_host = register_cvar("sh_mysql_host", "")
	sh_mysql_user = register_cvar("sh_mysql_user", "")
	sh_mysql_pass = register_cvar("sh_mysql_pass", "")
	sh_mysql_db = register_cvar("sh_mysql_db", "")
	sh_mysql_persistent = register_cvar("sh_mysql_persistent", "0")

	register_concmd("amx_shvaulttosql", "adminCopyVault", ADMIN_RCON, "- Copies vault XP to MySQL, should only be done once.")

	get_localinfo("amxx_vault", gVaultFile, 127)
}
//----------------------------------------------------------------------------------------------
public adminCopyVault(id, level, cid)
{
	if ( !cmd_access(id, level, cid, 1) ) return

	mySQLConnect()

	if ( gMySQL <= SQL_FAILED ) return

	//Turn Persistant on for this function
	gPersistentTemp = true

	// Open up the vault file - read line by line
	if ( !file_exists(gVaultFile) ) return

	console_print(id, "Please wait while the XP data is copied")

	// Read through the file looking for shinfo entries
	new parm[2]
	parm[0] = id
	parm[1] = 0
	adminCopyVHelper(parm)

	//Rest of this is done in the helper function
}
//----------------------------------------------------------------------------------------------
public adminCopyVHelper(parm[])
{
	//Get data from parm
	new id = parm[0]
	new nextLine = parm[1]

	// Read through the file looking for shinfo entries
	new lengthRead
	new data[512]

	while ((nextLine = read_file(gVaultFile,nextLine,data,511,lengthRead)) != 0) {
		if ( lengthRead > 0 && containi(data, "shinfo") == 0 ) {
			parseXP(gMemoryTableSize - 1, data)
			if (nextLine % 200 == 0) {
				parm[1] = nextLine
				console_print(id,"Delaying a few seconds before copying the next set of entries")
				set_task(3.0, "adminCopyVHelper", _, parm, 2)
				return
			}
		}
	}

	//If it makes it this far it must be done copying

	// OK now load the XP for any players playing
	new savekey[32]

	for ( new x = 1; x <= SH_MAXSLOTS; x++ ) {
		if ( !is_user_connected(x) ) continue
		getSaveKey(x, savekey)
		loadXP(x, savekey)
	}
	console_print(id, "Saved XP has been copied from the vault to the MySQL Database Successfully")

	//Set Persistent back how it was
	gPersistentTemp = false

	//Closes the connection if set not to be persistent
	close_mysql()
}
//----------------------------------------------------------------------------------------------
parseXP(id, const data[])
{
	// "name" epochtime XP HUDHELP "SKILL1" "SKILL2" ...
	new heroIndex
	static Left[39], Right[1501]
	Left[0] = '^0'
	Right[0] = '^0'

	copy(Right, 1500, data)

	//Get the key out
	strbrkqt(Right, Left, 38, Right, 1500)
	replace(Left, 38, "shinfo.", "" )
	copy(gMemoryTableKeys[id], 31, Left)

	//Get the name out
	strbrkqt(Right, Left, 38, Right, 1500)
	copy(gMemoryTableNames[id], 31, Left)

	//Get the epochtime out
	strbrkqt(Right, Left, 38, Right, 1500)

	//Get the XP out and save it
	strbrkqt(Right, Left, 38, Right, 1500)
	gMemoryTableXP[id] = str_to_num(Left)

	//Get the HUD Status out
	strbrkqt(Right, Left, 38, Right, 1500)
	gMemoryTableFlags[id] = str_to_num(Left)

	//Now load in all the skills
	new powerCount = 0
	new MaxPowers = min(get_pcvar_num(sh_maxpowers), SH_MAXLEVELS)

	while ( Right[0] != '^0' && powerCount < MaxPowers ) {
		strbrkqt(Right, Left, 38, Right, 1500)
		heroIndex = getHeroID(Left)
		if ( heroIndex >= 0 )  {
			gMemoryTablePowers[id][0] = ++powerCount
			gMemoryTablePowers[id][powerCount] = heroIndex
		}
	}

	// Now save it to the SQL database
	writeData(id)
}
//----------------------------------------------------------------------------------------------
public memoryTableWrite()
{
	if ( !get_pcvar_num(sh_savexp) ) return

	debugMsg(0, 3, "Writing XP Data to SQL Database")

	//Turn Persistant on for this function
	gPersistentTemp = true

	// Write Memory Table to File
	for (new x = 1; x < gMemoryTableCount; x++) {
		if ( gMemoryTableKeys[x][0] != '^0' ) {
			writeData(x)
		}
		// Can even clear the MemoryKey - if a player is disconnected no reason to save again and again...
		gMemoryTableKeys[x][0] = '^0'
	}

	// No need to keep disconnected player around...
	gMemoryTableCount = 33

	//Set Persistent back how it was
	gPersistentTemp = false

	//Closes the connection if set not to be persistent
	close_mysql()
}
//----------------------------------------------------------------------------------------------
//
//AMXX SQL API Code
//
//----------------------------------------------------------------------------------------------
mySQLConnect()
{
	if ( gMySQL >= SQL_OK ) {
		if ( !get_pcvar_num(sh_mysql_persistent) && !gPersistentTemp ) close_mysql()
		else return
	}

	static host[64], user[32], pass[32], db[32], error[128]

	get_pcvar_string(sh_mysql_host, host, 63)
	get_pcvar_string(sh_mysql_user, user, 31)
	get_pcvar_string(sh_mysql_pass, pass, 31)
	get_pcvar_string(sh_mysql_db, db, 31)

	if ( host[0] == '^0' && user[0] == '^0' && db[0] == '^0' ) {
		get_cvar_string("amx_mysql_host", host, 63)
		get_cvar_string("amx_mysql_user", user, 31)
		get_cvar_string("amx_mysql_pass", pass, 31)
		get_cvar_string("amx_mysql_db", db, 31)
	}

	gMySQL = dbi_connect(host, user, pass, db, error, 127)
	if ( gMySQL <= SQL_FAILED ) {
		debugMsg(0, 0, "MySQL connect error: '%s' (%s,%s,%s)", error, host, user, db)
		return
	}
	else if ( CreateTables ) {
		// Only try to create the tables once
		static sql[512]
		new Result:retval

		copy(sql, 511, "CREATE TABLE IF NOT EXISTS `sh_savexp` ( `SH_KEY` varchar(32) binary NOT NULL default '', `PLAYER_NAME` varchar(32) binary NOT NULL default '', `LAST_PLAY_DATE` timestamp(14) NOT NULL, `XP` int(10) NOT NULL default '0', `HUDHELP` tinyint(3) unsigned NOT NULL default '1', `SKILL_COUNT` tinyint(3) unsigned NOT NULL default '0', PRIMARY KEY  (`SH_KEY`) ) TYPE=MyISAM COMMENT='SUPERHERO XP Saving Table'")
		retval = dbi_query(gMySQL, sql)
		if ( retval <= RESULT_FAILED ) {
			dbi_error(gMySQL, error, 127)
			debugMsg(0, 0, "Error Making Tables: '%s' - '%s'", error, sql)
			return
		}

		sql[0] = '^0'
		copy(sql, 511, "CREATE TABLE IF NOT EXISTS `sh_saveskills` ( `SH_KEY` varchar(32) binary NOT NULL default '', `SKILL_NUMBER` tinyint(3) unsigned NOT NULL default '0', `HERO_NAME` varchar(25) NOT NULL default '', PRIMARY KEY  (`SH_KEY`,`SKILL_NUMBER`) ) TYPE=MyISAM COMMENT='SUPERHERO Skill Saving Table'")
		retval = dbi_query(gMySQL, sql)
		if ( retval <= RESULT_FAILED ) {
			dbi_error(gMySQL, error, 127)
			debugMsg(0, 0, "Error Making Tables: '%s' - '%s'", error, sql)
			return
		}

		CreateTables = false
	}
}
//----------------------------------------------------------------------------------------------
// Flushes data in memory table position x to database...
writeData(x)
{
	mySQLConnect()

	if ( gMySQL <= SQL_FAILED ) return

	debugMsg(x, 8, "Trying to save XP data to MySQL database")

	static error[128], sql[512]
	new Result:retval, Result:retvalins

	static savekey[63]
	copy(savekey, 62, gMemoryTableKeys[x])
	replace_all(savekey, 62, "`", "\`")
	replace_all(savekey, 62, "'", "\'")

	// Thanks to HC for help with writing more efficient queries
	// Check if this user has an entry already, if not make one
	formatex(sql, 511, "SELECT * FROM `sh_savexp` WHERE `SH_KEY` = '%s'", savekey)
	retval = dbi_query(gMySQL, sql)

	if ( retval <= RESULT_FAILED ) {
		dbi_error(gMySQL, error, 127)
		debugMsg(0, 0, "Error Querying MySQL DB for %s: '%s' - '%s'", savekey, error, sql)
		return
	}
	else if ( retval == RESULT_NONE || !dbi_nextrow(retval) ) {
		formatex(sql, 511, "INSERT INTO `sh_savexp` (SH_KEY) VALUES ('%s')", savekey)
		retvalins = dbi_query(gMySQL, sql)
		if ( retvalins <= RESULT_FAILED ) {
			dbi_error(gMySQL, error, 127)
			debugMsg(0, 0, "Error Writing MySQL XP for %s: '%s' - '%s'", savekey, error, sql)
			return
		}
	}

	if ( retval >= RESULT_OK ) dbi_free_result(retval)

	static bkqtname[63]
	copy(bkqtname, 62, gMemoryTableNames[x])
	replace_all(bkqtname, 62, "`", "\`")
	replace_all(bkqtname, 62, "'", "\'")

	//Update users entry with current data
	formatex(sql, 511, "UPDATE `sh_savexp` SET `PLAYER_NAME`='%s', `LAST_PLAY_DATE`=SYSDATE(), `XP`='%d', `HUDHELP`='%d', `SKILL_COUNT`='%d' WHERE SH_KEY='%s'", bkqtname, gMemoryTableXP[x], gMemoryTableFlags[x], gMemoryTablePowers[x][0], savekey)
	retval = dbi_query(gMySQL, sql)
	if (retval <= RESULT_FAILED) {
		dbi_error(gMySQL, error, 127)
		debugMsg(0, 0, "Error Writing MySQL XP for %s: '%s' - '%s'", savekey, error, sql)
		return
	}

	if ( !is_user_connected(x) || gChangedHeroes[x] ) {

		// Remove all saved powers for this user
		formatex(sql, 511, "DELETE FROM `sh_saveskills` WHERE SH_KEY='%s'", savekey)
		retval = dbi_query(gMySQL, sql)
		if (retval <= RESULT_FAILED) {
			dbi_error(gMySQL, error, 127)
			debugMsg(0, 0, "Error Deleting MySQL Skills for %s: '%s' - '%s'", savekey, error, sql)
			return
		}
		new heroIndex, numHeroes
		numHeroes = gMemoryTablePowers[x][0]
		// Saving by SuperHeroName since the hero order in the plugin.ini can change...
		for ( new p = 1; p <= numHeroes; p++ ) {
			heroIndex = gMemoryTablePowers[x][p]
			formatex(sql, 511, "INSERT INTO `sh_saveskills` VALUES ('%s','%d','%s')", savekey, p, gSuperHeros[heroIndex][hero])
			retval = dbi_query(gMySQL, sql)
			if ( retval <= RESULT_FAILED ) {
				dbi_error(gMySQL, error, 127)
				debugMsg(0, 0, "Error Writing MySQL XP for %s: '%s' - '%s'", savekey, error, sql)
				return
			}
		}

		if ( 0 < x <= SH_MAXSLOTS ) {
			gChangedHeroes[x] = false
		}
	}

	//Closes the connection if set not to be persistent
	close_mysql()
}
//----------------------------------------------------------------------------------------------
loadXP(id, const savekey[32])
{
	mySQLConnect()
	if ( gMySQL <= SQL_FAILED ) return false

	debugMsg(id, 8, "Trying to load XP data from MySQL database")

	static sql[512], error[128], shkey[63]
	new Result:retvalxp

	copy(shkey, 62, savekey)
	replace_all(shkey, 62, "`", "\`")
	replace_all(shkey, 62, "'", "\'")

	formatex(sql, 511, "SELECT `XP`, `HUDHELP`, `SKILL_COUNT` FROM `sh_savexp` WHERE `SH_KEY` = '%s'", shkey)
	retvalxp = dbi_query(gMySQL, sql)

	if ( retvalxp <= RESULT_FAILED ) {
		dbi_error(gMySQL, error, 127)
		debugMsg(0, 0, "Error Loading MySQL XP for %s: '%s' - '%s'", shkey, error, sql)
		return false
	}
	else if ( retvalxp == RESULT_NONE || !dbi_nextrow(retvalxp) ) {
		dbi_error(gMySQL, error, 127)
		debugMsg(id, 1, "No Saved XP Loaded for %s: '%s'", shkey, error)

		if ( retvalxp >= RESULT_OK ) dbi_free_result(retvalxp)

		//Closes the connection if set not to be persistent
		close_mysql()

		return true
	}

	static xp[12], skillCount[4], userFlags[4]
	new playerLevel, Result:retvalskill

	dbi_field(retvalxp, 1, xp, 11)
	gPlayerXP[id] = str_to_num(xp)
	gPlayerLevel[id] = playerLevel = getPlayerLevel(id)
	setLevel(id, playerLevel)

	dbi_field(retvalxp, 2, userFlags, 3)
	gPlayerFlags[id] = str_to_num(userFlags)

	dbi_field(retvalxp, 3, skillCount, 3)

	if ( retvalxp >= RESULT_OK ) dbi_free_result(retvalxp)

	formatex(sql, 511, "SELECT `HERO_NAME` FROM `sh_saveskills` WHERE `SH_KEY` = '%s' AND `SKILL_NUMBER` <= '%s' ORDER BY `SKILL_NUMBER` ASC", shkey, skillCount)
	retvalskill = dbi_query(gMySQL, sql)

	if ( retvalskill <= RESULT_FAILED ) {
		dbi_error(gMySQL, error, 127)
		debugMsg(0, 0, "Error Loading MySQL XP for %s: '%s' - '%s'", shkey, error, sql)
		return false
	}

	if ( retvalskill != RESULT_NONE ) {
		static heroName[25]
		new heroIndex, skills
		gPlayerPowers[id][0] = 0

		while ( dbi_nextrow(retvalskill) ) {
			heroName[0] = '^0'
			dbi_field(retvalskill, 1, heroName, 24)
			heroIndex = getHeroID(heroName)
			if ( -1 < heroIndex < gSuperHeroCount && getHeroLevel(heroIndex) <= playerLevel ) {
				gPlayerPowers[id][0] = ++skills
				gPlayerPowers[id][skills] = heroIndex
				initHero(id, heroIndex, SH_HERO_ADD)
			}
		}
	}

	if ( retvalskill >= RESULT_OK ) dbi_free_result(retvalskill)

	memoryTableUpdate(id)

	//Closes the connection if set not to be persistent
	close_mysql()

	return true
}
//----------------------------------------------------------------------------------------------
cleanXP(bool:clearAll)
{
	new xpSaveDays = get_pcvar_num(sh_xpsavedays)

	if ( !clearAll && (!get_pcvar_num(sh_savexp) || xpSaveDays < 0) ) return

	static error[128], sql[255]
	new Result:retval

	mySQLConnect()
	if ( gMySQL <= SQL_FAILED ) return

	if ( clearAll ) {
		formatex(sql, 254, "TRUNCATE TABLE `sh_saveskills`")
		retval = dbi_query(gMySQL, sql)
		if ( retval <= RESULT_FAILED ) {
			dbi_error(gMySQL, error, 127)
			debugMsg(0, 0, "Couldn't Erase XP: '%s' - '%s'", error, sql)
			return
		}

		formatex(sql, 254, "TRUNCATE TABLE `sh_savexp`")
		retval = dbi_query(gMySQL, sql)
		if ( retval <= RESULT_FAILED ) {
			dbi_error(gMySQL, error, 127)
			debugMsg(0, 0, "Couldn't Erase XP: '%s' - '%s'", error, sql)
			return
		}
	}
	else {

#if defined NEW_MYSQL

		formatex(sql, 254, "DELETE FROM `sh_saveskills` USING `sh_saveskills`, `sh_savexp` WHERE sh_savexp.`SH_KEY` = sh_saveskills.`SH_KEY` AND sh_savexp.`LAST_PLAY_DATE` < (SYSDATE() - INTERVAL '%d' DAY) AND NOT (sh_savexp.`HUDHELP` & '%d')", xpSaveDays, SH_FLAG_XPIMMUNE)
		retval = dbi_query(gMySQL, sql)
		if ( retval <= RESULT_FAILED ) {
			dbi_error(gMySQL, error, 127)
			debugMsg(0, 0, "Couldn't Clean Powers: '%s' - '%s'", error, sql)
			return
		}

		formatex(sql, 254, "DELETE FROM `sh_savexp` WHERE `LAST_PLAY_DATE` < (SYSDATE() - INTERVAL '%d' DAY) AND NOT (`HUDHELP` & '%d')", xpSaveDays, SH_FLAG_XPIMMUNE)
		retval = dbi_query(gMySQL, sql)
		if ( retval <= RESULT_FAILED ) {
			dbi_error(gMySQL, error, 127)
			debugMsg(0, 0, "Couldn't Clean XP: '%s' - '%s'", error, sql)
			return
		}

#else

		formatex(sql, 254, "SELECT `SH_KEY` FROM `sh_savexp` WHERE `LAST_PLAY_DATE` < (SYSDATE() - INTERVAL '%d' DAY) AND NOT (`HUDHELP` & '%d')", xpSaveDays, SH_FLAG_XPIMMUNE)
		retval = dbi_query(gMySQL, sql)
		if ( retval <= RESULT_FAILED ) {
			dbi_error(gMySQL, error, 127)
			debugMsg(0, 0, "Couldn't SELECT to Clean XP: '%s' - '%s'", error, sql)
			return
		}

		new Result:retvaldel

		if ( retval != RESULT_NONE ) {
			static savekey[63]
			while ( dbi_nextrow(retval) ) {
				savekey[0] = '^0'
				dbi_field(retval, 1, savekey, 62)
				if ( savekey[0] == '^0' ) return

				replace_all(savekey, 62, "`", "\`")
				replace_all(savekey, 62, "'", "\'")

				formatex(sql, 511, "DELETE FROM `sh_savexp` WHERE SH_KEY='%s'", savekey)
				retvaldel = dbi_query(gMySQL, sql)
				if ( retvaldel <= RESULT_FAILED ) {
					dbi_error(gMySQL, error, 127)
					debugMsg(0, 0, "Couldn't Clean XP: '%s' - '%s'", error, sql)
					return
				}

				formatex(sql, 511, "DELETE FROM `sh_saveskills` WHERE SH_KEY='%s'", savekey)
				retvaldel = dbi_query(gMySQL, sql)
				if ( retvaldel <= RESULT_FAILED ) {
					dbi_error(gMySQL, error, 127)
					debugMsg(0, 0, "Couldn't Clean Powers: '%s' - '%s'", error, sql)
					return
				}
			}
		}
		if ( retval >= RESULT_OK ) dbi_free_result(retval)

#endif
	}

	//Closes the connection if set not to be persistent
	close_mysql()
}
//----------------------------------------------------------------------------------------------
close_mysql()
{
	if ( gMySQL <= SQL_FAILED || get_pcvar_num(sh_mysql_persistent) || gPersistentTemp ) return

	dbi_close(gMySQL)
}
//----------------------------------------------------------------------------------------------
saving_end()
{
	if ( gMySQL <= SQL_FAILED ) return

	dbi_close(gMySQL)
}
//----------------------------------------------------------------------------------------------