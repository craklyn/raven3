/* ************************************************************************
*  file:  plrtoascii.c                                     Part of tbaMUD *
*  Copyright (C) 1990, 1991 - see 'license.doc' for complete information. *
*  All Rights Reserved                                                    *
************************************************************************* */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "db.h"
#include "pfdefaults.h"


// first some stock circle 3.0 defines. Change where appropriate.
#define MAX_NAME_LENGTH		20  /* Used in char_file_u *DO*NOT*CHANGE* */
#define MAX_PWD_LENGTH		10  /* Used in char_file_u *DO*NOT*CHANGE* */
#define MAX_TITLE_LENGTH	80  /* Used in char_file_u *DO*NOT*CHANGE* */
#define HOST_LENGTH		30  /* Used in char_file_u *DO*NOT*CHANGE* */
#define MAX_SKILLS		300 /* Used in char_file_u *DO*NOT*CHANGE* */
#define MAX_AFFECT		64  /* Used in char_file_u *DO*NOT*CHANGE* */

#define MAX_PWD_LENGTH        10     /**< Max PC password length */
#define PLR_DESC_LENGTH       240   /**< Max length for PC description */
#define MAX_SKILLS            300    /**< Max number of skills/spells */

#define MAX_CHORES 15

/* Char's abilities.  Used in char_file_u *DO*NOT*CHANGE* */
struct char_ability_data_plrtoascii {
    char str;
    char str_add;
    char intel;
    char wis;
    char dex;
    char con;
    char cha;
};


/* Char's points.  Used in char_file_u *DO*NOT*CHANGE* */
struct char_point_data_plrtoascii {
    sh_int mana;
    sh_int max_mana; /* Max move for PC/NPC                     */
    sh_int hit;
    sh_int max_hit; /* Max hit for PC/NPC                      */
    sh_int move;
    sh_int max_move; /* Max move for PC/NPC                     */

    sh_int armor; /* Internal -100..100, external -10..10 AC */
    int gold; /* Money carried                           */
    int bank_gold; /* Gold the char has in a bank account     */
    int exp; /* The experience of the player            */
    char hitroll; /* Any bonus or penalty to the hit roll    */
    char damroll; /* Any bonus or penalty to the damage roll */
};


/* 
 * char_special_data_saved: specials which both a PC and an NPC have in
 * common, but which must be saved to the playerfile for PC's.
 *
 * WARNING:  Do not change this structure.  Doing so will ruin the
 * playerfile.  If you want to add to the playerfile, use the spares
 * in player_special_data.
 */
struct char_special_data_saved_plrtoascii {
    int alignment; /* +-1000 for alignments                    */
    long idnum; /* player's idnum; -1 for mobiles           */
    int act[PM_ARRAY_MAX]; /* act flag for NPC's; player flag for PC's */
    int affected_by[AF_ARRAY_MAX]; /* Bitvector for spells/skills affected by */
    sh_int apply_saving_throw[5]; /* Saving throw (Bonuses)       */
};

struct player_special_data_saved_plrtoascii {
  u_char skills[MAX_SKILLS+1]; /* array of skills plus skill 0  */
  u_char skill_usage[MAX_SKILLS+1];
  u_char spare01;              /* Used to be race, moved it after purge */
  u_char spells_to_learn;      /* How many can you learn yet this level. */
  int    wimp_level;           /* Below this # of hit points, flee!    */
  u_char freeze_level;         /* Level of god who froze char, if any */
  short  invis_level;          /* level of invisibility  */
  short  load_room;            /* Which room to place char in  */
  int  pref[PR_ARRAY_MAX];     /* preference flags for PC's.  */
  u_char bad_pws;              /* number of bad password attemps   */
  char   conditions[3];        /* Drunk, full, thirsty         */
  u_char clan_id;
  u_char clan_rank;
  u_char sub_race;             /* Players sub_race */
  u_char pthief_countdown;
  u_char combat_delay;
  u_char race_modifier;
  char poofin[80];	       /* Poofin */
  char poofout[80];	       /* Poofout */
  int    pkill_countdown;
  u_char conj_countdown[4];       /* Time before character can use a conj spell */
  int    ac_remain;            /* Remainder from ac calcs in handler.c - stored in 8ths */
  int    jail_timer;           /* Flag Game jail control variable.         */
  int    quest_pts;            /* The number of quest points a player has  */
  char unused1;
  char unused2;
  char unused3;
  char mastery;                /* Mastery, currently only used for warrior weapons */
  int    olc_zone;             /* zone this immortal can use olc on.       */
  int    prayer_time;	       /* Time in ticks before can pray again */
  int    locker_num;           /* vnum of this player's locker, if any */
  int    assistant;            /* hp of divine assistant */
  u_char notell_level;
  u_char nogoss_level;
  u_char phunt_countdown;
  u_char chore_count;
  int    aggr_pref;
  int   recall;
  int    chores[MAX_CHORES];
  /*
  ** spares below for future expansion.  You can change the names from
  ** 'sparen' to something meaningful, but don't change the order.
  */
  int    arenarank;           /* Elo rating for arena kills */
  int    advance_level;
  int    specialization;
  char   cooldown[4];
  int    spares[10];
};

struct affected_type_plrtoascii {
    short type; /* The type of spell that caused this       */
    short duration; /* For how long its effects will last       */
    char modifier; /* This is added to apropriate ability      */
    u_char location; /* Tells which ability to change(APPLY_XXX) */
    u_int bitvector; /* Tells which bits to set (AFF_XXX)        */
    char level;
    char unused1;
    short unused2;
};

/* ==================== File Structure for Player ======================= */
/*             BEWARE: Changing it will ruin the playerfile		  */
struct char_file_u_plrtoascii {
   /* char_player_data */
   char	name[MAX_NAME_LENGTH+1];
   char	description[PLR_DESC_LENGTH];
   char	title[MAX_TITLE_LENGTH+1];
   char sex;
   char chclass;
   char race;
   char level;
   char lostlevels;
   char orcs;
   time_t birth;   /* Time of birth of character     */
   int	played;    /* Number of secs played in total */
   ubyte weight;
   ubyte height;
   char	pwd[MAX_PWD_LENGTH+1];    /* character's password */

   struct char_special_data_saved_plrtoascii char_specials_saved;
   struct player_special_data_saved_plrtoascii player_specials_saved;
   struct char_ability_data_plrtoascii abilities;
   struct char_point_data_plrtoascii points;
   struct affected_type_plrtoascii affected[MAX_AFFECT];

   time_t last_logon;		/* Time (in secs) of last logon */
   char host[HOST_LENGTH+1];	/* host of last logon */
};
/* ====================================================================== */

int sprintascii(char *out, bitvector_t bits);
int plr_filename(char *orig_name, char *filename);

void convert(char *filename)
{
  FILE *fl, *outfile, *index_file;
  struct char_file_u_plrtoascii player;
  char index_name[40], outname[40], bits[127];
  int i, j;
  struct char_special_data_saved_plrtoascii *csds;
  struct player_special_data_saved_plrtoascii *psds;
  struct char_ability_data_plrtoascii *cad;
  struct char_point_data_plrtoascii *cpd;
  struct affected_type_plrtoascii *aff;

  if (!(fl = fopen(filename, "r+"))) {
    perror("error opening playerfile");
    exit(1);
  }
  sprintf(index_name, "%s%s", LIB_PLRFILES, INDEX_FILE);
  if (!(index_file = fopen(index_name, "w"))) {
    perror("error opening index file");
    exit(1);
  }
  
  int idnum = 0;
  for (;;) {
    idnum += 1;
    j = fread(&player, sizeof(struct char_file_u_plrtoascii), 1, fl);
    if (feof(fl)) {
      fclose(fl);
      fclose(index_file);
      exit(1);
    }

    if (!plr_filename(player.name, outname))
      exit(1);
    printf("writing: %s\n", outname);

    fprintf(index_file, "%ld %s %d %s %ld\n", 
	idnum, player.name, player.level, bits,
	(long)player.last_logon);

    if (!(outfile = fopen(outname, "w"))) {
      printf("error opening output file");
      exit(1);
    }

/* char_file_u */
    if (player.name)
      fprintf(outfile, "Name: %s\n", player.name);
    if (player.pwd)
      fprintf(outfile, "Pass: %s\n", player.pwd);
    if (player.title)
      fprintf(outfile, "Titl: %s\n", player.title);
    if (player.description && *player.description)
      fprintf(outfile, "Desc:\n%s~\n", player.description);
    if (player.sex != PFDEF_SEX)
      fprintf(outfile, "Sex : %d\n", (int)player.sex); 
    if (player.chclass != PFDEF_CLASS)
      fprintf(outfile, "Clas: %d\n", (int)player.chclass); 
//      fprintf(outfile, "Clas: %d\n", 1); 
    if (player.race  != PFDEF_RACE)  		
      fprintf(outfile, "Race: %d\n", (int)player.race);
    if (player.player_specials_saved.sub_race != PFDEF_SUBRACE)  	
      fprintf(outfile, "SubR: %d\n", player.player_specials_saved.sub_race);
    if (player.level != PFDEF_LEVEL)
      fprintf(outfile, "Levl: %d\n", (int)player.level); 
    fprintf(outfile, "Brth: %d\n", (int)player.birth);
    fprintf(outfile, "Plyd: %d\n", (int)player.played);
    fprintf(outfile, "Last: %d\n", (int)player.last_logon);
    fprintf(outfile, "Host: %s\n", player.host);
    if (player.height != PFDEF_HEIGHT)
      fprintf(outfile, "Hite: %d\n", (int)player.height);
    if (player.weight != PFDEF_WEIGHT)
      fprintf(outfile, "Wate: %d\n", (int)player.weight);

/* char_special_data_saved */
    csds = &(player.char_specials_saved);
    if (csds->alignment != PFDEF_ALIGNMENT)
      fprintf(outfile, "Alin: %d\n", csds->alignment);
    fprintf(outfile, "Id  : %d\n", idnum);
    if (csds->act != PFDEF_PLRFLAGS)
//      fprintf(outfile, "Act : %d\n", (int)csds->act);
      fprintf(outfile, "Act : %s\n", "0 0 0 0");
    if (csds->affected_by != PFDEF_AFFFLAGS) {
      sprintascii(bits, csds->affected_by);
//      fprintf(outfile, "Aff : %s\n", bits);
      fprintf(outfile, "Aff : %s\n", "0 0 0 0");
    }
    if (csds->apply_saving_throw[0] != PFDEF_SAVETHROW)
      fprintf(outfile, "Thr1: %d\n", csds->apply_saving_throw[0]);
    if (csds->apply_saving_throw[1] != PFDEF_SAVETHROW)
      fprintf(outfile, "Thr2: %d\n", csds->apply_saving_throw[1]);
    if (csds->apply_saving_throw[2] != PFDEF_SAVETHROW)
      fprintf(outfile, "Thr3: %d\n", csds->apply_saving_throw[2]);
    if (csds->apply_saving_throw[3] != PFDEF_SAVETHROW)
      fprintf(outfile, "Thr4: %d\n", csds->apply_saving_throw[3]);
    if (csds->apply_saving_throw[4] != PFDEF_SAVETHROW)
      fprintf(outfile, "Thr5: %d\n", csds->apply_saving_throw[4]);

/* player_special_data_saved */
    psds = &(player.player_specials_saved);
    if (player.level < LVL_IMMORT) {
      fprintf(outfile, "Skil:\n");
      for (i = 1; i <= MAX_SKILLS; i++) {
	if (psds->skills[i])
	  fprintf(outfile, "%d %d\n", i, (int)psds->skills[i]);
      }
      fprintf(outfile, "0 0\n");
    }
    if (psds->wimp_level != PFDEF_WIMPLEV)
      fprintf(outfile, "Wimp: %d\n", psds->wimp_level);
    if (psds->freeze_level != PFDEF_FREEZELEV)
      fprintf(outfile, "Frez: %d\n", (int)psds->freeze_level);
    if (psds->invis_level != PFDEF_INVISLEV)
      fprintf(outfile, "Invs: %d\n", (int)psds->invis_level);
    if (psds->load_room != PFDEF_LOADROOM)
//      fprintf(outfile, "Room: %d\n", (int)psds->load_room);
      fprintf(outfile, "Room: %d\n", 1204);
    if (psds->pref != PFDEF_PREFFLAGS) {
      sprintascii(bits, psds->pref);
      fprintf(outfile, "Pref: %s\n", "efghno 0 0 0");
    }
    if (psds->conditions[HUNGER] && player.level < LVL_IMMORT &&
	psds->conditions[HUNGER] != PFDEF_HUNGER)
      fprintf(outfile, "Hung: %d\n", (int)psds->conditions[0]);
    if (psds->conditions[THIRST] && player.level < LVL_IMMORT &&
	psds->conditions[THIRST] != PFDEF_THIRST)
      fprintf(outfile, "Thir: %d\n", (int)psds->conditions[1]);
    if (psds->conditions[2] && player.level < LVL_IMMORT &&
	psds->conditions[DRUNK] != PFDEF_DRUNK)
      fprintf(outfile, "Drnk: %d\n", (int)psds->conditions[2]);
    if (psds->spells_to_learn != PFDEF_PRACTICES)
      fprintf(outfile, "Lern: %d\n", (int)psds->spells_to_learn);

/* char_ability_data */
    cad = &(player.abilities);
    if (cad->str != PFDEF_STR || cad->str_add != PFDEF_STRADD)
      fprintf(outfile, "Str : %d/%d\n", cad->str, cad->str_add);
    if (cad->intel != PFDEF_INT)
      fprintf(outfile, "Int : %d\n", cad->intel);
    if (cad->wis != PFDEF_WIS)
      fprintf(outfile, "Wis : %d\n", cad->wis);
    if (cad->dex != PFDEF_DEX)
      fprintf(outfile, "Dex : %d\n", cad->dex);
    if (cad->con != PFDEF_CON)
      fprintf(outfile, "Con : %d\n", cad->con);
    if (cad->cha != PFDEF_CHA)
      fprintf(outfile, "Cha : %d\n", cad->cha);

/* char_point_data */
    cpd = &(player.points);
    if (cpd->hit != PFDEF_HIT || cpd->max_hit != PFDEF_MAXHIT)
      fprintf(outfile, "Hit : %d/%d\n", cpd->hit, cpd->max_hit);
    if (cpd->mana != PFDEF_MANA || cpd->max_mana != PFDEF_MAXMANA)
      fprintf(outfile, "Mana: %d/%d\n", cpd->mana, cpd->max_mana);
    if (cpd->move != PFDEF_MOVE || cpd->max_move != PFDEF_MAXMOVE)
      fprintf(outfile, "Move: %d/%d\n", cpd->move, cpd->max_move);
    if (cpd->armor != PFDEF_AC)
      fprintf(outfile, "Ac  : %d\n", cpd->armor);
    if (cpd->gold != PFDEF_GOLD)
      fprintf(outfile, "Gold: %d\n", cpd->gold);
    if (cpd->bank_gold != PFDEF_BANK)
      fprintf(outfile, "Bank: %d\n", cpd->bank_gold);
    if (cpd->exp != PFDEF_EXP)
      fprintf(outfile, "Exp : %d\n", cpd->exp);
    if (cpd->hitroll != PFDEF_HITROLL)
      fprintf(outfile, "Hrol: %d\n", cpd->hitroll);
    if (cpd->damroll != PFDEF_DAMROLL)
      fprintf(outfile, "Drol: %d\n", cpd->damroll);

/* affected_type */
    fprintf(outfile, "Affs:\n");
    for (i = 0; i < MAX_AFFECT; i++) {
      aff = &(player.affected[i]);
      if (aff->type)
	fprintf(outfile, "%d %d %d %d %d\n", aff->type, aff->duration,
	  aff->modifier, aff->location, (int)aff->bitvector);
    }
    fprintf(outfile, "0 0 0 0 0\n");

    fclose(outfile);
  }
}


int main(int argc, char **argv)
{
  if (argc != 2)
    printf("Usage: %s playerfile-name\n", argv[0]);
  else
    convert(argv[1]);

  return 0;
}


int sprintascii(char *out, bitvector_t bits)
{
  int i, j = 0;
  /* 32 bits, don't just add letters to try to get more unless your bitvector_t is also as large. */
  char *flags = "abcdefghijklmnopqrstuvwxyzABCDEF";

  for (i = 0; flags[i] != '\0'; i++)
    if (bits & (1 << i))
      out[j++] = flags[i];

  if (j == 0) /* Didn't write anything. */
    out[j++] = '0';

  /* NUL terminate the output string. */
  out[j++] = '\0';
  return j;
}


int plr_filename(char *orig_name, char *filename)
{
  const char *middle;
  char name[64], *ptr;

  if (orig_name == NULL || *orig_name == '\0' || filename == NULL) {
    perror("error getting player file name");
    return (0);
  }

  strcpy(name, orig_name);
  for (ptr = name; *ptr; ptr++)
    *ptr = LOWER(*ptr);

  switch (LOWER(*name)) {
  case 'a':  case 'b':  case 'c':  case 'd':  case 'e':
    middle = "A-E";
    break;
  case 'f':  case 'g':  case 'h':  case 'i':  case 'j':
    middle = "F-J";
    break;
  case 'k':  case 'l':  case 'm':  case 'n':  case 'o':
    middle = "K-O";
    break;
  case 'p':  case 'q':  case 'r':  case 's':  case 't':
    middle = "P-T";
    break;
  case 'u':  case 'v':  case 'w':  case 'x':  case 'y':  case 'z':
    middle = "U-Z";
    break;
  default:
    middle = "ZZZ";
    break;
  }

  sprintf(filename, "%s%s"SLASH"%s.%s", LIB_PLRFILES, middle, name, SUF_PLR);
  return (1);
}
