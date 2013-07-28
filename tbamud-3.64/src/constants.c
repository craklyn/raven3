/**
* @file constants.c
* Numeric and string contants used by the MUD.
*
* Part of the core tbaMUD source code distribution, which is a derivative
* of, and continuation of, CircleMUD.
*
* All rights reserved.  See license for complete information.
* Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University
* CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.
*
* @todo Come up with a standard for descriptive arrays. Either all end with
* newlines or all of them don not.
*/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "interpreter.h"	/* alias_data */

/** Current tbaMUD version.
 * @todo defined with _TBAMUD so we don't have multiple constants to change.
 * @todo cpp_extern isn't needed here (or anywhere) as the extern reserved word
 * works correctly with C compilers (at least in my Experience)
 * Jeremy Osborne 1/28/2008 */
cpp_extern const char *tbamud_version = "tbaMUD 3.64";

/* strings corresponding to ordinals/bitvectors in structs.h */
/* (Note: strings for class definitions in class.c instead of here) */

/** Description of cardinal directions.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *dirs[] =
{
  "north",
  "east",
  "south",
  "west",
  "up",
  "down",
  "northwest", /* Diagonals only used if CONFIG_DIAGONAL_DIRS is set */
  "northeast",
  "southeast",
  "southwest",
  "\n"
};

const char *autoexits[] =
{
  "North",
  "East",
  "South",
  "West",
  "Up",
  "Down",
  "Northwest",
  "Northeast",
  "Southeast",
  "Southwest",
  "\n"
};

/** Room flag descriptions.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *room_bits[] = {
  "DARK",
  "DEATH",
  "NO_MOB",
  "INDOORS",
  "PEACEFUL",
  "SOUNDPROOF",
  "NO_TRACK",
  "NO_MAGIC",
  "TUNNEL",
  "PRIVATE",
  "GODROOM",
  "HOUSE",
  "HCRSH",
  "ATRIUM",
  "OLC",
  "*",				/* The BFS Mark. */
  "WORLDMAP",
  "\n"
};

/** Room flag descriptions. (ZONE_x)
 * @pre Must be in the same order as the defines in structs.h.
 * Must end array with a single newline. */
const char *zone_bits[] = {
  "CLOSED",
  "NO_IMMORT",
  "QUEST",
  "GRID",
  "NOBUILD",
  "!ASTRAL",
  "WORLDMAP",
  "\n"
};

/** Exit bits for doors.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *exit_bits[] = {
  "DOOR",
  "CLOSED",
  "LOCKED",
  "PICKPROOF",
  "\n"
};

/** Description of the room sector type.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *sector_types[] = {
  "Inside",
  "City",
  "Field",
  "Forest",
  "Hills",
  "Mountains",
  "Water (Swim)",
  "Water (No Swim)",
  "In Flight",
  "Underwater",
  "\n"
};

/** PC and NPC sex.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *genders[] =
{
  "neutral",
  "male",
  "female",
  "\n"
};

/** Character positions.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *position_types[] = {
  "Dead",
  "Mortally wounded",
  "Incapacitated",
  "Stunned",
  "Sleeping",
  "Resting",
  "Sitting",
  "Fighting",
  "Standing",
  "\n"
};

/* Immortal Names */
const char *god_labels[NUM_GOD_LABELS + 1] = {
      " HERO ",
      " SAINT",
      " ANGEL",
      " DEITY",
      " CREAT",
      " DEMI ",
      " LGOD ",
      " GOD  ",
      " GRGOD",
      " IMP  ",
      "\n"
};

/** Player flags.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *player_bits[] = {
  "KILLER",
  "THIEF",
  "FROZEN",
  "DONTSET",
  "WRITING",
  "MAILING",
  "CSH",
  "SITEOK",
  "NOSHOUT",
  "NOTITLE",
  "DELETED",
  "LOADRM",
  "NO_WIZL",
  "NO_DEL",
  "INVST",
  "CRYO",
  "DEAD",    /* You should never see this flag on a character in game. */
  "IBT_BUG",
  "IBT_IDEA",
  "IBT_TYPO",
  "UNUSED1",
  "UNUSED2",
  "UNUSED3",
  "UNUSED4",
  "UNUSED5",
  "\n"
};

/** Mob action flags.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *action_bits[] = {
    "SPEC",
    "SENTINEL",
    "SCVNGR",
    "ISNPC",
    "AWARE",
    "AGGR",
    "STAYZ",
    "WIMP",
    "AGG_E",
    "AGG_G",
    "AGG_N",
    "SEEKR",
    "HELPER",
    "!CHRM",
    "!SUMN",
    "!SLEP",
    "!BASH",
    "!BLND",
    "CNJRD",
    "MEMORY",
    "SUPAGG",
    "MOUNT",
    "CLONE",
    "PREDATOR",
    "GUARD_CLASS",
    "GUARD_RACE",
    "GUARD_BOTH",
    "QUESTMASTER",
    "TELEPORTS",
    "NPC_KILLER",
    "!NOX",
    "GRENADER",
    "EVASIVE",
    "DEAD",
    "!KILL",
    "\n"
};

/** PC Preference flags.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *preference_bits[] = {
  "BRIEF",
  "COMPACT",
  "NO_SHOUT",
  "NO_TELL",
  "D_HP",
  "D_MANA",
  "D_MOVE",
  "AUTOEX",
  "NO_HASS",
  "QUEST",
  "SUMN",
  "NO_REP",
  "LIGHT",
  "C1",
  "C2",
  "NO_WIZ",
  "L1",
  "L2",
  "NO_AUC",
  "NO_GOS",
  "NO_GTZ",
  "RMFLG",
  "D_AUTO",
  "CLS",
  "BLDWLK",
  "AFK",
  "AUTOLOOT",
  "AUTOGOLD",
  "AUTOSPLIT",
  "AUTOSAC",
  "AUTOASSIST",
  "AUTOMAP",
  "AUTOKEY",
  "AUTODOOR",
  "\n"
};

/** Affected bits.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *affected_bits[] =
{
    "DONTSET",
    "INVIS",
    "DET-ALIGN",
    "DET-INVIS",
    "DET-MAGIC",
    "SENSE-LIFE",
    "SHAWALK",
    "SANCT",
    "GROUP",
    "CURSE",
    "INFRA",
    "POISON",
    "PROT-EVIL",
    "PROT-GOOD",
    "SLEEP",
    "!TRACK",
    "FLY",
    "REGEN",
    "SNEAK",
    "HIDE",
    "HASTE",
    "CHARM",
    "SHIELD",
    "PARA",
    "AIRSPHERE",
    "PLAGUE",
    "SHIELDBLOCK",
    "SILENCE",
    "MOUNTED",
    "WARD",
    "SHASPH",
    "BERSERK",
    "!HOT",
    "!COLD",
    "!DRY",
    "BLIND",
    "WEBBED",
    "BLINK",
    "FEEBLEMIND",
    "ASSISTANT",
    "FORTIFY",
    "HAMSTRUNG",
    "PULSE-HIT",
    "PULSE-MANA",
    "DISTRACT",
    "CRUSADE",
    "APOCALYPSE",
    "DIVINE-MISSION",
    "!UNUSED!",
    "FOREST-LORE",
    "LEARNING",
    "FLAMING",
    "!UNUSED!",
    "WRAITHFORM",
    "DISEASE",
    "WATERWALK",
    "\n"
};

/** Connection type descriptions.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *connected_types[] = {
  "Playing",
  "Disconnecting",
  "Get name",
  "Confirm name",
  "Get password",
  "Get new PW",
  "Confirm new PW",
  "Select sex",
  "Select race"
  "Select class",
  "Reading MOTD",
  "Main Menu",
  "Get descript.",
  "Changing PW 1",
  "Changing PW 2",
  "Changing PW 3",
  "Self-Delete 1",
  "Self-Delete 2",
  "Disconnecting",
  "Object edit",
  "Room edit",
  "Zone edit",
  "Mobile edit",
  "Shop edit",
  "Text edit",
  "Config edit",
  "Social edit",
  "Trigger edit",
  "Help edit",
  "Quest edit",
  "Preference edit",
  "IBT edit",
  "Message edit",
  "Protocol Detection",
  "Select breath",
  "\n"
};

/** Describes the position in the equipment listing.
 * @pre Must be in the same order as the defines.
 * Not used in sprinttype() so no \n. */
const char *wear_where[] = {
    "<used as light>      ",
    "<worn on finger>     ",
    "<worn on finger>     ",
    "<worn as cloak>      ",
    "<worn around neck>   ",
    "<worn on body>       ",
    "<worn on head>       ",
    "<worn on ears>       ",
    "<worn on face>       ",
    "<worn on legs>       ",
    "<worn on ankles>     ",
    "<worn on feet>       ",
    "<worn on hands>      ",
    "<worn on arms>       ",
    "<worn as shield>     ",
    "<worn about body>    ",
    "<worn about waist>   ",
    "<worn around wrist>  ",
    "<worn around wrist>  ",
    "<wielded>            ",
    "<held>               ",
    "<orbitting head>     "
};

/* Describes where an item can be worn.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *equipment_types[] = {
    "Used as light",
    "Worn on right finger",
    "Worn on left finger",
    "Worn as cloak",
    "Worn around Neck",
    "Worn on body",
    "Worn on head",
    "Worn on ears",
    "Worn on face",
    "Worn on legs",
    "Worn on ankles",
    "Worn on feet",
    "Worn on hands",
    "Worn on arms",
    "Worn as shield",
    "Worn about body",
    "Worn around waist",
    "Worn around right wrist",
    "Worn around left wrist",
    "Wielded",
    "Held",
    "Orbitting Head",
    "\n"
};

/** Describes the type of object.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *item_types[] = {
  "UNDEFINED",
  "LIGHT",
  "SCROLL",
  "WAND",
  "STAFF",
  "WEAPON",
  "FURNITURE",
  "FREE",
  "TREASURE",
  "ARMOR",
  "POTION",
  "WORN",
  "OTHER",
  "TRASH",
  "FREE2",
  "CONTAINER",
  "NOTE",
  "LIQ CONTAINER",
  "KEY",
  "FOOD",
  "MONEY",
  "PEN",
  "BOAT",
  "FOUNTAIN",
  "PORTAL",
  "SCRIBE",
  "AFFECT",
  "DUST",
  "POLE",
  "\n"
};

/** Describes the wear flags set on an item.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *wear_bits[] = {
    "TAKE",
    "FINGER",
    "CLOAK",
    "BODY",
    "HEAD",
    "LEGS",
    "FEET",
    "HANDS",
    "ARMS",
    "SHIELD",
    "ABOUT",
    "WAIST",
    "WRIST",
    "WIELD",
    "HOLD",
    "NECK",
    "ORBIT",
    "ANKLES",
    "EARS",
    "FACE",
    "\n"
};

/** Describes the extra flags applied to an item.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *extra_bits[] = {
    "GLOW",        /*  0 */
    "HUM",
    "!RENT",
    "!DONATE",
    "!INVIS",
    "INVISIBLE",   /*  5 */
    "MAGIC",
    "CURSED",
    "BLESS",
    "!GOOD",
    "!EVIL",        /* 10 */
    "!NEUTRAL",
    "!MAGE",
    "!CLERIC",
    "!THIEF",
    "!WARRIOR",     /* 15 */
    "!SELL",
    "!RANGER",
    "!ASSASSIN",
    "!SHOU_LIN",
    "!SOLAMN_KN",   /* 20 */
    "!DEATH_KN",
    "!SHAD_DANC",
    "TIMED",
    "EXPLODES",
    "ARTIFACT",   /* 25 */
    "NO_LOCATE",
    "!MINOTAUR",
    "!GNOME",
    "!ORC",
    "!ELF", /* 30 */
    "!DRACONIAN",
    "!HALFLING",
    "!OGRE",
    "!TROLL",
    "!DWARF", /* 35 */
    "!HUMAN",
    "!NECRO",
    "MAIN_PART",
    "INSERT_PART",
    "ARENA",     /* 40 */
    "!DEMON",
    "!IZARTI",
    "!VAMPIRE",
    "!WEREWOLF",
    "!ELEMENTAL", /* 45 */
    "!GIANT",
    "!FAERIE",
    "!AMARA",
    "!UNDEAD",
    "!DROW",      /* 50 */
    "LOOTED",
    "IDENTIFIED",
    "RANDOMIZED",
    "SOULBOUND",
    "!REMORT",    /* 55 */
    "!PREMORT",
    "TROPHY",
    "QUESTITEM",
    "\n"
};

/** Describes the apply types.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *apply_types[] = {
    "NONE",
    "STR",
    "DEX",
    "INT",
    "WIS",
    "CON",
    "CHA",
    "CLASS",
    "LEVEL",
    "AGE",
    "CHAR_WEIGHT",
    "CHAR_HEIGHT",
    "MAXMANA",
    "MAXHIT",
    "MAXMOVE",
    "GOLD",
    "EXP",
    "ARMOR",
    "HITROLL",
    "DAMROLL",
    "SAVING_PARA",
    "SAVING_ROD",
    "SAVING_PETRI",
    "SAVING_BREATH",
    "SAVING_SPELL",
    "POISON",
    "PLAGUE",
    "MANA_COST",
    "SPELL_SAVES",
    "SPELL_DAMAGE",
    "SPELL_DURATION",
    "SKILL_SUCCESS",
    "!UNUSED!",
    "USELEVEL",
    "\n"
};

/** Describes the closure mechanism for a container.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *container_bits[] = {
  "CLOSEABLE",
  "PICKPROOF",
  "CLOSED",
  "LOCKED",
  "\n",
};

/** Describes the liquid description.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *drinks[] =
{
  "water",
  "beer",
  "wine",
  "ale",
  "dark ale",
  "whisky",
  "lemonade",
  "firebreather",
  "local speciality",
  "slime mold juice",
  "milk",
  "tea",
  "coffee",
  "blood",
  "salt water",
  "clear water",
  "\n"
};

/** Describes a one word alias for each type of liquid.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *drinknames[] =
{
  "water",
  "beer",
  "wine",
  "ale",
  "ale",
  "whisky",
  "lemonade",
  "firebreather",
  "local",
  "juice",
  "milk",
  "tea",
  "coffee",
  "blood",
  "salt",
  "water",
  "\n"
};

/** Define the effect of liquids on hunger, thirst, and drunkenness, in that
 * order. See values.doc for more information.
 * @pre Must be in the same order as the defines. */
int drink_aff[][3] = {
  {0, 1, 10},
  {3, 2, 5},
  {5, 2, 5},
  {2, 2, 5},
  {1, 2, 5},
  {6, 1, 4},
  {0, 1, 8},
  {10, 0, 0},
  {3, 3, 3},
  {0, 4, -8},
  {0, 3, 6},
  {0, 1, 6},
  {0, 1, 6},
  {0, 2, -1},
  {0, 1, -2},
  {0, 0, 13}
};

/** Describes the color of the various drinks.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *color_liquid[] =
{
  "clear",
  "brown",
  "clear",
  "brown",
  "dark",
  "golden",
  "red",
  "green",
  "clear",
  "light green",
  "white",
  "brown",
  "black",
  "red",
  "clear",
  "crystal clear",
  "\n"
};

/** Used to describe the level of fullness of a drink container. Not used in
 * sprinttype() so no \n. */
const char *fullness[] =
{
  "less than half ",
  "about half ",
  "more than half ",
  ""
};

/** Strength attribute affects.
 * The fields are hit mod, damage mod, weight carried mod, and weight wielded
 * mod. */
cpp_extern const struct str_app_type str_app[] = {
  {-5, -4, 0, 0},	/* str = 0 */
  {-5, -4, 3, 1},	/* str = 1 */
  {-3, -2, 3, 2},
  {-3, -1, 10, 3},
  {-2, -1, 25, 4},
  {-2, -1, 55, 5},	/* str = 5 */
  {-1, 0, 80, 6},
  {-1, 0, 90, 7},
  {0, 0, 100, 8},
  {0, 0, 100, 9},
  {0, 0, 115, 10},	/* str = 10 */
  {0, 0, 115, 11},
  {0, 0, 140, 12},
  {0, 0, 140, 13},
  {0, 0, 170, 14},
  {0, 0, 170, 15},	/* str = 15 */
  {0, 1, 195, 16},
  {1, 1, 220, 18},
  {1, 2, 255, 20},	/* str = 18 */
  {3, 7, 640, 40},
  {3, 8, 700, 40},	/* str = 20 */
  {4, 9, 810, 40},
  {4, 10, 970, 40},
  {5, 11, 1130, 40},
  {6, 12, 1440, 40},
  {7, 14, 1750, 40},	/* str = 25 */
  {1, 3, 280, 22},	/* str = 18/0 - 18-50 */
  {2, 3, 305, 24},	/* str = 18/51 - 18-75 */
  {2, 4, 330, 26},	/* str = 18/76 - 18-90 */
  {2, 5, 380, 28},	/* str = 18/91 - 18-99 */
  {3, 6, 480, 30}	/* str = 18/100 */
};

/** Dexterity skill modifiers for thieves.
 * The fields are for pick pockets, pick locks, find traps, sneak and hide. */
cpp_extern const struct dex_skill_type dex_app_skill[] = {
  {-99, -99, -90, -99, -60},	/* dex = 0 */
  {-90, -90, -60, -90, -50},	/* dex = 1 */
  {-80, -80, -40, -80, -45},
  {-70, -70, -30, -70, -40},
  {-60, -60, -30, -60, -35},
  {-50, -50, -20, -50, -30},	/* dex = 5 */
  {-40, -40, -20, -40, -25},
  {-30, -30, -15, -30, -20},
  {-20, -20, -15, -20, -15},
  {-15, -10, -10, -20, -10},
  {-10, -5, -10, -15, -5},	/* dex = 10 */
  {-5, 0, -5, -10, 0},
  {0, 0, 0, -5, 0},
  {0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0},		/* dex = 15 */
  {0, 5, 0, 0, 0},
  {5, 10, 0, 5, 5},
  {10, 15, 5, 10, 10},		/* dex = 18 */
  {15, 20, 10, 15, 15},
  {15, 20, 10, 15, 15},		/* dex = 20 */
  {20, 25, 10, 15, 20},
  {20, 25, 15, 20, 20},
  {25, 25, 15, 20, 20},
  {25, 30, 15, 25, 25},
  {25, 30, 15, 25, 25}		/* dex = 25 */
};

/** Dexterity attribute affects.
 * The fields are reaction, missile attacks, and defensive (armor class). */
cpp_extern const struct dex_app_type dex_app[] = {
  {-7, -7, 6},		/* dex = 0 */
  {-6, -6, 5},		/* dex = 1 */
  {-4, -4, 5},
  {-3, -3, 4},
  {-2, -2, 3},
  {-1, -1, 2},		/* dex = 5 */
  {0, 0, 1},
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, 0},		/* dex = 10 */
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, -1},		/* dex = 15 */
  {1, 1, -2},
  {2, 2, -3},
  {2, 2, -4},		/* dex = 18 */
  {3, 3, -4},
  {3, 3, -4},		/* dex = 20 */
  {4, 4, -5},
  {4, 4, -5},
  {4, 4, -5},
  {5, 5, -6},
  {5, 5, -6}		/* dex = 25 */
};

/** Constitution attribute affects.
 * The field referenced is for hitpoint bonus. */
cpp_extern const struct con_app_type con_app[] = {
  {-4},		/* con = 0 */
  {-3},		/* con = 1 */
  {-2},
  {-2},
  {-1},
  {-1},		/* con = 5 */
  {-1},
  {0},
  {0},
  {0},
  {0},		/* con = 10 */
  {0},
  {0},
  {0},
  {0},
  {1},		/* con = 15 */
  {2},
  {2},
  {3},		/* con = 18 */
  {3},
  {4},		/* con = 20 */
  {5},
  {5},
  {5},
  {6},
  {6}		/* con = 25 */
};

/** Intelligence attribute affects.
 * The field shows how much practicing affects a skill/spell. */
cpp_extern const struct int_app_type int_app[] = {
  {3},		/* int = 0 */
  {5},		/* int = 1 */
  {7},
  {8},
  {9},
  {10},		/* int = 5 */
  {11},
  {12},
  {13},
  {15},
  {17},		/* int = 10 */
  {19},
  {22},
  {25},
  {30},
  {35},		/* int = 15 */
  {40},
  {45},
  {50},		/* int = 18 */
  {53},
  {55},		/* int = 20 */
  {56},
  {57},
  {58},
  {59},
  {60}		/* int = 25 */
};

/** Wisdom attribute affects.
 * The field represents how many extra practice points are gained per level. */
cpp_extern const struct wis_app_type wis_app[] = {
  {0},	/* wis = 0 */
  {0},  /* wis = 1 */
  {0},
  {0},
  {0},
  {0},  /* wis = 5 */
  {0},
  {0},
  {0},
  {0},
  {0},  /* wis = 10 */
  {0},
  {2},
  {2},
  {3},
  {3},  /* wis = 15 */
  {3},
  {4},
  {5},	/* wis = 18 */
  {6},
  {6},  /* wis = 20 */
  {6},
  {6},
  {7},
  {7},
  {7}  /* wis = 25 */
};

/** Define a set of opposite directions from the cardinal directions. */
int rev_dir[] =
{
  SOUTH,
  WEST,
  NORTH,
  EAST,
  DOWN,
  UP,
  SOUTHEAST,
  SOUTHWEST,
  NORTHWEST,
  NORTHEAST
};

/** How much movement is lost moving through a particular sector type. */
int movement_loss[] =
{
  1,	/* Inside     */
  1,	/* City       */
  2,	/* Field      */
  3,	/* Forest     */
  4,	/* Hills      */
  6,	/* Mountains  */
  4,	/* Swimming   */
  1,	/* Unswimable */
  1,	/* Flying     */
  5   /* Underwater */
};

/** The names of the days of the mud week. Not used in sprinttype(). */
const char *weekdays[] = {
  "the Day of the Moon",
  "the Day of the Bull",
  "the Day of the Deception",
  "the Day of Thunder",
  "the Day of Freedom",
  "the Day of the Great Gods",
  "the Day of the Sun"
};

/** The names of the mud months. Not used in sprinttype(). */
const char *month_name[] = {
  "Month of Winter",		/* 0 */
  "Month of the Winter Wolf",
  "Month of the Frost Giant",
  "Month of the Old Forces",
  "Month of the Grand Struggle",
  "Month of the Spring",
  "Month of Nature",
  "Month of Futility",
  "Month of the Dragon",
  "Month of the Sun",
  "Month of the Heat",
  "Month of the Battle",
  "Month of the Dark Shades",
  "Month of the Shadows",
  "Month of the Long Shadows",
  "Month of the Ancient Darkness",
  "Month of the Great Evil"
};

/** Names for mob trigger types. */
const char *trig_types[] = {
  "Global",
  "Random",
  "Command",
  "Speech",
  "Act",
  "Death",
  "Greet",
  "Greet-All",
  "Entry",
  "Receive",
  "Fight",
  "HitPrcnt",
  "Bribe",
  "Load",
  "Memory",
  "Cast",
  "Leave",
  "Door",
  "UNUSED",
  "Time",
  "\n"
};

/** Names for object trigger types. */
const char *otrig_types[] = {
  "Global",
  "Random",
  "Command",
  "UNUSED1",
  "UNUSED2",
  "Timer",
  "Get",
  "Drop",
  "Give",
  "Wear",
  "UNUSED3",
  "Remove",
  "UNUSED4",
  "Load",
  "UNUSED5",
  "Cast",
  "Leave",
  "UNUSED6",
  "Consume",
  "Time",
  "\n"
};

/** Names for world (room) trigger types. */
const char *wtrig_types[] = {
  "Global",
  "Random",
  "Command",
  "Speech",
  "UNUSED1",
  "Zone Reset",
  "Enter",
  "Drop",
  "UNUSED2",
  "UNUSED3",
  "UNUSED4",
  "UNUSED5",
  "UNUSED6",
  "UNUSED7",
  "UNUSED8",
  "Cast",
  "Leave",
  "Door",
  "Login",
  "Time",
  "\n"
};

/** The names of the different channels that history is stored for.
 * @todo Only referenced by do_history at the moment. Should be moved local
 * to that function. */
const char *history_types[] = {
  "all",
  "say",
  "gossip",
  "wiznet",
  "tell",
  "shout",
  "grats",
  "holler",
  "auction",
  "\n"
};

/** Flag names for Ideas, Bugs and Typos (defined in ibt.h) */
const char *ibt_bits[] = {
  "Resolved",
  "Important",
  "InProgress",
  "\n"
};

const char *chaString[27] = {
    "NONE!",
    "Nightmare",  "Horrifying",  "Monstrous",   /* 01 - 03 */
    "Sickening",  "Hideous",     "Deformed",    /* 04 - 06 */
    "Ugly",       "Ugly",        "Unattractive",   /* 07 - 09 */
    "Below Avg",  "Plain",       "Average",     /* 10 - 12 */
    "Appealing",  "Attractive",  "Comely",      /* 13 - 15 */
    "Charming",   "Stunning",    "Gorgeous",    /* 16 - 18 */
    "Breathtaking","Majestic",   "Perfect",     /* 19 - 21 */
    "Unearthly",  "Godlike",     "Godlike",	/* 22 - 24 */
    "Divine",
    "-*END*-"
};

const char *conString[27] = {
    "NONE!",
    "Simp",       "Sickly",      "Anemic",      /* 01 - 03 */
    "Anemic",     "Fragile",     "Frail",       /* 04 - 06 */
    "Feeble",     "Feeble",      "Below Avg",   /* 07 - 09 */
    "Below Avg",  "Average",     "Average",     /* 10 - 12 */
    "Above Avg",  "Healthy",     "Husky",       /* 13 - 15 */
    "Hardy",      "Sturdy",      "Rugged",      /* 16 - 18 */
    "Tough",      "Tireless",    "Primordial",  /* 19 - 21 */
    "Primordial", "Godlike",     "Godlike",	/* 22 - 24 */
    "Divine",
    "-*END*-"
};

/*
 * Below are the string representations of attributes
 */

const char *dexString[27] = {
    "NONE!",
    "Immobile",   "Slugish",     "Arthritic",   /* 01 - 03 */
    "Brick",      "Clutz",       "Clutz",       /* 04 - 06 */
    "Clumsy",     "Clumsy",      "Below Avg",   /* 07 - 09 */
    "Below Avg",  "Average",     "Average",     /* 10 - 12 */
    "Above Avg",  "Deft",        "Adroit",      /* 13 - 15 */
    "Smooth",     "Nimble",      "Agile",       /* 16 - 18 */
    "Graceful",   "Cat-like",    "Incredible",  /* 19 - 21 */
    "Incredible", "Godlike",	 "Godlike",	/* 22 - 24 */
    "Divine",					/* 25	   */
    "-*END*-"
};

const char *strString[32] = {
    "NONE!",
    "Child",      "Child",       "Wimpy",       /* 01 - 03 */
    "Wimpy",      "Pencil-neck", "Weak",        /* 04 - 06 */
    "Weak",       "Weak",        "Pipsqueak",   /* 07 - 09 */
    "Piwi",       "Below Avg",   "Average",     /* 10 - 12 */
    "Average",    "Above Avg",   "Vigorous",    /* 13 - 15 */
    "Wiry",       "Sinewy",      "Muscular",    /* 16 - 18 */
    "Potent",     "Strong",      "Powerful",    /* 18/0 , 18/41,  18/61 */
    "Heroic",     "Atlantian",   "Herculean",   /* 18/81, 18/100, 19    */
    "Gigantic",   "Colossal",    "Gargantuan",  /* 20 - 22 */
    "Titanic",    "Godlike",	 "Divine",	/* 23 - 25 */
    "-*END*-"
};

const char *intString[27] = {
    "NONE!",
    "Vegetable",  "Animal",      "Idiot",
    "Imbecile",   "Moron",       "Dolt",
    "Dumb",       "Below Avg",   "Average",
    "Average",    "Average",     "Apt",
    "Smart",      "Clever",      "Intellignt",
    "Exceptnl",   "Brilliant",   "Gifted",
    "Genius",     "Genius",      "Sup-Genius",
    "Sup-Genius", "Godlike",	 "Godlike",
    "Divine",
    "-*END*-"
};

const char *wisString[27] = {
    "NONE!",
    "Ninkempoop",        "Fool",              "Fool",
    "Fool",              "Twit",              "Gullible",
    "Gullible",          "Gullible",          "Naive",
    "Naive",             "Average",           "Average",
    "Above Avg",         "Keen",              "Crafty",
    "Shrewd",            "Astute",            "Wise",
    "Sagacious",         "Transcendent",      "Infinite",
    "Omniscient",	 "Godlike",	      "Godlike",
    "Divine",
    "-*END*-"
};
/* --- End of constants arrays. --- */

/* Various arrays we count so we can check the world files.  These
 * must be at the bottom of the file so they're pre-declared. */
  /** Number of defined room bit descriptions. */
  size_t	room_bits_count = sizeof(room_bits) / sizeof(room_bits[0]) - 1,
  /** Number of defined action bit descriptions. */
	action_bits_count = sizeof(action_bits) / sizeof(action_bits[0]) - 1,
	/** Number of defined affected bit descriptions. */
	affected_bits_count = sizeof(affected_bits) / sizeof(affected_bits[0]) - 1,
	/** Number of defined extra bit descriptions. */
	extra_bits_count = sizeof(extra_bits) / sizeof(extra_bits[0]) - 1,
	/** Number of defined wear bit descriptions. */
	wear_bits_count = sizeof(wear_bits) / sizeof(wear_bits[0]) - 1;

