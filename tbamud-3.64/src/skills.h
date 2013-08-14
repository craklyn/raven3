/*
 * @author Hudas
 * @file spells.h
 * Constants and function prototypes for the new skill/spell system.
 *
 * Part of RavenMUD source code.
 * Extension of tbaMUD source code.
 */

#ifndef SKILLS_H_
#define SKILLS_H_

#ifndef __SKILLS_C__

#define ABILITY_TYPE_SPELL 0
#define ABILITY_TYPE_SKILL 1

#define SCMD_ABEDIT_STAT 1
#define SCMD_ABEDIT_LIST 2

#define ABF_ARRAY_MAX  4
#define NUM_ABILITY_VALUES 5

#define AB_FLAG_ACCUMULATIVE_DURATION  0
#define AB_FLAG_ACCUMULATIVE_APPLY     1
#define AB_FLAG_COSTS_MOVE             2
#define AB_FLAG_ANTIGOOD               3
#define AB_FLAG_ANTINEUTRAL            4
#define AB_FLAG_ANTIEVIL               5
#define AB_FLAG_REQUIRE_OBJ            6

#define NUM_AB_FLAGS                   7

#define AB_MSG_SUCCESS    0
#define AB_MSG_FAIL       1
#define AB_MSG_WEAR_OFF   2
#define AB_MSG_GOD        3
#define AB_MSG_DEATH      4
#define NUM_AB_MSGS       5

#define AB_MSG_TO_CHAR    0
#define AB_MSG_TO_VICT    1
#define AB_MSG_TO_ROOM    3
#define NUM_AB_MSG_TO     4

/* OLC modes */
#define ABEDIT_MAIN_MENU  0
#define ABEDIT_NAME       1
#define ABEDIT_TYPE       2
#define ABEDIT_MINPOS     3
#define ABEDIT_COST       4
#define ABEDIT_COST_MIN   5
#define ABEDIT_COST_MAX   6
#define ABEDIT_COST_CHG   7
#define ABEDIT_DAM        8
#define ABEDIT_DAM_NUM    9
#define ABEDIT_DAM_DICE   10
#define ABEDIT_DAM_DR     11
#define ABEDIT_AFF_MENU   12


ACMD(do_abedit);
ASPELL(spell_none);

/* OLC functions */
void abeditParse(struct descriptor_data *d, char *arg);

extern struct list_data *abilityList;

struct dam_dice_info {
  int diceSize;
  int numDice;
  int damRoll;
};

struct ability_spell_info_type {
  char *spellFunc;
  ASPELL(*spell); /* for manual spells */
};

struct ability_skill_info_type {
  int stunChar; /* multiplied by PULSE_VIOLENCE */
  int stunVict;
  char *skillFunc; /* for manual skills */
  ACMD(*action);
};

/*
 * Skill/Spell utility functions
 */
bool  isAffectedBySpellName (struct char_data *ch, const char *spellName);
bool  isAffectedBySpellNum  (struct char_data *ch, sh_int spellNum);
bool  skillSuccessByName    (struct char_data *ch, const char *skillName);
bool  skillSuccess     (struct char_data *ch, sh_int skillNum);
int   getSpellByName        (const char *spellName);
struct ability_info_type *getAbility(int num);
struct ability_info_type *getAbilityByName(char *name);

/* skills loading and saving */
int   saveAbilities (void);
void  loadAbilities (void);

/* New skill/spell structure
 * Lets call it ability to generalize spell and skill.
 */
struct ability_info_type {
  char *name;
  bool violent;
  int type; /* skill or spell or something else in the future */
  int number;
  int minPosition;
  int routines;
  int targets;
  int minLevels[NUM_CLASSES];
  int miscValues[NUM_ABILITY_VALUES]; /* other values (e.g. vnum for gate mobs)*/
  int flags[ABF_ARRAY_MAX];
  struct ability_spell_info_type *spellInfo;
  struct ability_skill_info_type *skillInfo;
  struct msg_type *messages[NUM_AB_MSGS];

  int minCost;
  int maxCost;
  int costChange;
  char *costFormula;
  char *affDurationFormula;
  char *damDiceFormula;
  struct affected_type *affects; /* for affects and unaffect routine */
  int affDuration; /* durration if affect or unaffect spell */
  struct dam_dice_info *damDice;
};

#endif /* SKILLS_H_ */
#endif
