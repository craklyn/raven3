/*
 * race.c
 *
 *  Created on: Jul 22, 2013
 *      Author: root
 */
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "spells.h"
#include "interpreter.h"

#define __RACE_C__

const char *pc_race_types[] = {
	    "Human",
	    "Plant",
	    "Animal",
	    "Dragon",
	    "Undead",
	    "Vampire",
	    "Halfling",
	    "Elf",
	    "Dwarf",
	    "Giant",
	    "Minotaur",
	    "Demon",
	    "Ogre",
	    "Troll",
	    "Werewolf",
	    "Elemental",
	    "Orc",
	    "Gnome",
	    "Draconian",
	    "Faerie",
	    "Amara",
	    "Izarti",
	    "Drow",
	    "Ascended Human",
	    "Halfling Sneak",
	    "Elf Ancient",
	    "Dark Drow",
	    "Firstborn Dwarf",
	    "Minotaur Warchampion",
	    "Ogre Magi",
	    "Rootfellow Troll",
	    "Dragonspawn",
	    "Feyborn Gnome",
	    "Orc Blooddrinker",
	    "Terran",
	    "Zerg",
	    "Protoss",
	    "\n"
	};

const char *race_abbrevs[NUM_RACES + 1] = {
    "Hum",		/* (0) - HUMAN		*/
    "\tGPlt\tn",	/* (1) - PLANT		*/
    "Ani",		/* (2) - ANIMAL		*/
    "\tR\t=\tYDrg\tn",	/* (3) - DRAGON		*/
    "\twUnd\tn",	/* (4) - UNDEAD		*/
    "\tRVam\tn",	/* (5) - VAMPIRE	*/
    "Hlf",		/* (6) - HALFLING	*/
    "Elf",		/* (7) - ELF		*/
    "Dwf",		/* (8) - DWARF		*/
    "\tmGia\tn",	/* (9) - GIANT		*/
    "Min",		/* (10) - MINOTAUR	*/
    "\tcDem\tn",	/* (11) - DEMON		*/
    "Ogr",		/* (12) - OGRE		*/
    "Tro",		/* (13) - TROLL		*/
    "\tyWer\tn",	/* (14) - WEREWOLF	*/
    "\tWEle\tn",	/* (15) - ELEMENTAL	*/
    "Orc",		/* (16) - ORC		*/
    "Gnm",		/* (17) - GNOME		*/
    "Drc",		/* (18) - DRACONIAN	*/
    "\tCFae\tn",	/* (19) - FAERIE	*/
    "\tBAma\tn",	/* (20) - AMARA 	*/
    "\tYIza\tn",	/* (21) - IZARTI	*/
    "Drw",              /* (22) - DROW		*/
    "\tBHum\tn",        /* (23) - SHUMAN	*/
    "\tB\t=\tGHlf\tn",     /* (24) - SHALFLING	*/
    "\tGElf\tn",        /* (25) - SELF		*/
    "\twDrw\tn",        /* (26) - SDROW		*/
    "\tyDwf\tn",        /* (27) - SDWARF	*/
    "\tR\t=\tWMin\tn",     /* (28) - SMINOTAUR	*/
    "\tWOgr\tn",        /* (29) - SOGRE		*/
    "\tGTro\tn",        /* (30) - STROLL	*/
    "\tRDrc\tn",        /* (31) - SDRACONIAN	*/
    "\tbGnm\tn",        /* (32) - SGNOME	*/
    "\trOrc\tn",        /* (33) - SORC		*/
    "\tB\t=\twTrn\tn",     /*  34    TERRAN        */
    "\tG\t=\tWZrg\tn",
    "\tW\t=\tDPro\tn",
    "\n"
};

const char *race_menu =
"\r\n"
"Your choice of race will restrict which classes you can pick.  A character who\r\n"
"has advanced sufficiently can be reborn as any of these races or a variety of\r\n"
"more powerful races.\r\n"
"\r\n"
"Select a race : \r\n"
"  [H] - Human\r\n"
"  [B] - Gnome\r\n"
"  [C] - Orc\r\n"
"  [E] - Elf\r\n"
"  [I] - Draconian\r\n"
"  [L] - Halfling\r\n"
"  [M] - Minotaur\r\n"
"  [O] - Ogre\r\n"
"  [T] - Troll\r\n"
"  [W] - Dwarf\r\n"
"  [X] - Drow\r\n"
;

const int  raceStatsLimit[NUM_RACES][NUM_STATS] = {
/*	 S   %    I   W   D   C   Ch				*/
	{18, 100, 18, 18, 18, 18, 18}, /* 0 - RACE_HUMAN	*/
	{20, 100, 16, 16, 18, 20, 16}, /* 1 - RACE_PLANT 	*/
	{19, 100, 12, 12, 19, 19, 18}, /* 2 - RACE_ANIMAL	*/
	{25, 100, 20, 20, 20, 25, 20}, /* 3 - RACE_DRAGON	*/
	{20, 100, 16, 16, 16, 20,  3}, /* 4 - RACE_UNDEAD	*/
	{21, 100, 18, 18, 22, 20, 23}, /* 5 - RACE_VAMPIRE	*/
	{18,  50, 18, 17, 20, 18, 18}, /* 6 - RACE_HALFLING	*/
	{18,  70, 19, 20, 19, 16, 19}, /* 7 - RACE_ELF		*/
	{18, 100, 17, 19, 16, 21, 16}, /* 8 - RACE_DWARF	*/
	{24, 100, 10, 10, 12, 24, 14}, /* 9 - RACE_GIANT	*/
	{20, 100, 13, 12, 16, 20,  4}, /*10 - RACE_MINOTAUR	*/
	{22, 100, 19, 19, 20, 21, 20}, /*11 - RACE_DEMON	*/
	{21, 100, 12, 12, 15, 20,  5}, /*12 - RACE_OGRE		*/
	{20, 100,  6,  6, 12, 22,  3}, /*13 - RACE_TROLL	*/
	{21, 100, 18, 18, 20, 22, 14}, /*14 - RACE_WEREWOLF	*/
	{20, 100, 20, 20, 18, 20, 19}, /*15 - RACE_ELEMENTAL	*/
	{19, 100, 16, 16, 17, 19, 12}, /*16 - RACE_ORC		*/
	{18,  50, 20, 16, 18, 17, 18}, /*17 - RACE_GNOME	*/
	{19, 100, 17, 16, 16, 19,  8}, /*18 - RACE_DRACONIAN	*/
    {13,   0, 24, 24, 22, 16, 24}, /*19 - RACE_FAERIE       */
    {18, 100, 23, 23, 22, 18, 12}, /*20 - RACE_AMARA        */
    {19, 100, 21, 21, 21, 19, 22}, /*21 - RACE_IZARTI       */
    {18, 100, 19, 17, 21, 14, 18}, /*22 - RACE_DROW         */
    {20, 100, 20, 20, 20, 20, 20}, /*23 - RACE_SHUMAN       */
    {18,  90, 20, 18, 22, 20, 20}, /*24 - RACE_SHALFLING    */
    {19, 100, 21, 22, 21, 18, 21}, /*25 - RACE_SELF         */
    {20, 100, 20, 18, 23, 15, 20}, /*26 - RACE_SDROW        */
    {20, 100, 18, 20, 18, 23, 18}, /*27 - RACE_SDWARF       */
    {22, 100, 15, 14, 18, 22,  6}, /*28 - RACE_SMINOTAUR     */
    {24, 100, 14, 14, 16, 21,  7}, /*29 - RACE_SOGRE         */
    {22, 100,  8,  8, 14, 24,  5}, /*30 - RACE_STROLL       */
    {21, 100, 19, 17, 18, 21, 10}, /*31 - RACE_SDRACONIAN   */
    {18,  80, 22, 17, 20, 19, 20}, /*32 - RACE_SGNOME       */
    {21, 100, 17, 17, 19, 21, 14}, /*33 - RACE_SORC         */
    {18, 100, 18, 18, 18, 18, 18}, /*34 - RACE_TERRAN	*/
    {18, 100, 18, 18, 18, 15, 18}, /*35 - RACE_ZERG 	*/
    {20, 100, 18, 18, 14, 19, 18}, /*36 - RACE_PROTOSS	*/
};

int  classRaceAllowed[NUM_RACES][NUM_CLASSES] = {
/* 	 Ma Cl Th Wa Ra As Sl Kn Dk Sd Nm Dr*/
	{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},	/* 0 - RACE_HUMAN */
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},	/* 1 - RACE_PLANT */
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},	/* 2 - RACE_ANIMAL */
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},	/* 3 - RACE_DRAGON */
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},	/* 4 - RACE_UNDEAD */
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},	/* 5 - RACE_VAMPIRE */
	{0, 1, 1, 1, 0, 0, 0, 0, 0, 1, 0, 0},	/* 6 - RACE_HALFLING */
	{1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 0, 1},	/* 7 - RACE_ELF */
	{0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0},	/* 8 - RACE_DWARF */
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},	/* 9 - RACE_GIANT */
	{0, 1, 0, 1, 1, 0, 0, 0, 0, 0, 1, 1},	/* 10 - RACE_MINOTAUR */
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},	/* 11 - RACE_DEMON */
	{1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1},	/* 12 - RACE_OGRE */
	{0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1},	/* 13 - RACE_TROLL */
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},	/* 14 - RACE_WEREWOLF */
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},	/* 15 - RACE_ELEMENTAL */
	{1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 0},	/* 16 - RACE_ORC */
	{1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1},	/* 17 - RACE_GNOME */
	{1, 1, 0, 1, 0, 1, 1, 0, 0, 0, 1, 1},	/* 18 - RACE_DRACONIAN */
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},  /* 19 - RACE_FAERIE */
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},  /* 20 - RACE_AMARA */
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},  /* 21 - RACE_IZARTI */
    {1, 1, 1, 1, 0, 1, 0, 0, 0, 1, 1, 1},  /* 22 - RACE_DROW */
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},  /* 23 - RACE_SHUMAN */
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},  /* 24 - RACE_SHALFLING   */
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},  /* 25 - RACE_SELF */
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},  /* 26 - RACE_SDROW */
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},  /* 27 - RACE_SDWARF */
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},  /* 28 - RACE_SMINOTAUR */
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},  /* 29 - RACE_SOGRE */
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},  /* 30 - RACE_STROLL */
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},  /* 31 - RACE_SDRACONIAN */
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},  /* 32 - RACE_SGNOME */
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},  /* 33 - RACE_SORC */
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},  /* 33 - RACE_TERRAN */
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},  /* 33 - RACE_ZERG */
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},  /* 33 - RACE_PROTOSS */
};

/*
 * Returns the race code equivalent of the given arg.
 * This is used in character creation.
 */
int	parse_race (char arg) {
	int race = RACE_UNDEFINED;

	switch (LOWER(arg))
	{
	case 'b': return RACE_GNOME;
	case 'c': return RACE_ORC;
	case 'e': return RACE_ELF;
	case 'x': return RACE_DROW;
	case 'i': return RACE_DRACONIAN;
	case 'l': return RACE_HALFLING;
	case 'm': return RACE_MINOTAUR;
	case 'o': return RACE_OGRE;
	case 't': return RACE_TROLL;
	case 'w': return RACE_DWARF;
	case 'h': return RACE_HUMAN;
	default:
		return RACE_UNDEFINED;
	}

	return race;
}

/**
 * bitvectors (i.e., powers of two) for each race, mainly for use in do_who
 * and do_users.  Add new races at the end so that all races use sequential
 * powers of two (1 << 0, 1 << 1, 1 << 2, 1 << 3, 1 << 4, 1 << 5, etc.) up to
 * the limit of your bitvector_t, typically 0-31.
 *
 */
bitvector_t find_race_bitvector(const char *arg) {
    size_t rpos, ret = 0;

    for (rpos = 0; rpos < strlen(arg); rpos++)
        ret |= (1 << parse_race(*arg));

    return (ret);
}

