# Created by...: Kevin tkm@gamersource.net
# Description..: SuperHeroMod MySQL Install Script
# Orig Date....: 08/01/03
# Modified Date: 08/11/08
#
# Reference:  Linux: See MySQL_README_LINUX - SECTION 2.0
#	      Windows: See MySQL_README_WINDOWS - SECTION 2.0
#

# Start: Create the database
CREATE DATABASE IF NOT EXISTS SHeroDB;
USE SHeroDB;
GRANT SELECT ON `SHeroDB`.* TO SuperHeroModUser@localhost;


#----------------------------------------------------
# Start: Create the tables
#----------------------------------------------------

CREATE TABLE IF NOT EXISTS `sh_savexp` (
	`SH_KEY` varchar(32) binary NOT NULL default '',
	`PLAYER_NAME` varchar(32) binary NOT NULL default '',
	`LAST_PLAY_DATE` timestamp(14) NOT NULL,
	`XP` int(10) NOT NULL default '0',
	`HUDHELP` tinyint(3) unsigned NOT NULL default '1',
	`SKILL_COUNT` tinyint(3) unsigned NOT NULL default '0',
	PRIMARY KEY  (`SH_KEY`)
) TYPE=MyISAM COMMENT='SUPERHERO XP Saving Table';

GRANT SELECT, INSERT, UPDATE, DELETE, CREATE, ALTER ON `sh_savexp` TO SuperHeroModUser@localhost;

CREATE TABLE IF NOT EXISTS `sh_saveskills` (
	`SH_KEY` varchar(32) binary NOT NULL default '',
	`SKILL_NUMBER` tinyint(3) unsigned NOT NULL default '0',
	`HERO_NAME` varchar(25) NOT NULL default '',
	PRIMARY KEY  (`SH_KEY`,`SKILL_NUMBER`)
) TYPE=MyISAM COMMENT='SUPERHERO Skill Saving Table';

GRANT SELECT, INSERT, UPDATE, DELETE, CREATE, ALTER ON `sh_saveskills` TO SuperHeroModUser@localhost;

FLUSH PRIVILEGES;

#----------------------------------------------------
# Stop
#----------------------------------------------------