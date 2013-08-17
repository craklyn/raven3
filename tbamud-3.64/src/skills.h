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

/* Ability types */
#define ABILITY_TYPE_SPELL 0  /**< A spell */
#define ABILITY_TYPE_SKILL 1  /**< A skill */
/* Number of ability types */
#define NUM_ABILITY_TYPES  2

/* do_abedit sub commands */
#define SCMD_ABEDIT_STAT 1  /**< Sub-command for abstat */
#define SCMD_ABEDIT_LIST 2  /**< Sub-command for ablist */

/* Number of bit arry for ability flags */
#define ABF_ARRAY_MAX  4

/* Number of ability miscelleneous values */
#define NUM_ABILITY_VALUES 5

/* Ability flags */
#define ABILITY_ACCUMULATIVE_DURATION  0 /**< Ability has accumulative duration                        */
#define ABILITY_ACCUMULATIVE_APPLY     1 /**< Ability has accumulative apply modificaitons             */
#define ABILITY_COSTS_MOVE             2 /**< Ability costs movement points                            */
#define ABILITY_ANTIGOOD               3 /**< Ability can't be activated when actor is good aligned    */
#define ABILITY_ANTINEUTRAL            4 /**< Ability can't be activated when actor is neutral aligned */
#define ABILITY_ANTIEVIL               5 /**< Ability can't be activated when actor is evil aligned    */
#define ABILITY_REQUIRE_OBJ            6 /**< Ability requires objects to complete                     */
/* Number of ability flags */
#define NUM_ABILITY_FLAGS                   7

/* Ability message types */
#define ABILITY_MSG_SUCCESS    0 /**< Messages when ability succeeds              */
#define ABILITY_MSG_FAIL       1 /**< Messages when ability fails                 */
#define ABILITY_MSG_WEAR_OFF   2 /**< Messages when ability affects wear-off      */
#define ABILITY_MSG_GOD_VICT   3 /**< Messages when victim is an immortal player  */
#define ABILITY_MSG_DEATH      4 /**< Messages when ability causes victim's death */
/* Number of ability messages */
#define NUM_ABILITY_MSGS       5

/* Ability message receiver types */
#define ABILITY_MSG_TO_CHAR    0 /**< Message to actor/char */
#define ABILITY_MSG_TO_VICT    1 /**< Message to victim/target */
#define ABILITY_MSG_TO_ROOM    2 /**< Message to room */
/* Number of messag  receiver types */
#define NUM_ABILITY_MSG_TO     3

/* OLC modes */
#define ABEDIT_MAIN_MENU        0
#define ABEDIT_NAME             1
#define ABEDIT_TYPE             2
#define ABEDIT_MINPOS           3
#define ABEDIT_COST             4
#define ABEDIT_COST_MIN         5
#define ABEDIT_COST_MAX         6
#define ABEDIT_COST_CHG         7
#define ABEDIT_COST_EXPR        8
#define ABEDIT_DAM              9
#define ABEDIT_DAM_NUM         10
#define ABEDIT_DAM_SIZE        11
#define ABEDIT_DAM_DR          12
#define ABEDIT_DAM_EXPR        13
#define ABEDIT_FLAGS           14
#define ABEDIT_ROUTINES        15
#define ABEDIT_TARGETS         16
#define ABEDIT_LEVELS          17
#define ABEDIT_AFF_MENU        18
#define ABEDIT_AFF_APPLY       19
#define ABEDIT_AFF_MOD         20
#define ABEDIT_AFF_DUR         21
#define ABEDIT_AFF_DUR_EXPR    22
#define ABEDIT_AFF_DUR_VAL     23
#define ABEDIT_AFF_DELETE      24
#define ABEDIT_MESSAGE         25
#define ABEDIT_MESSAGE_NEW     26
#define ABEDIT_MESSAGE_EDIT    27
#define ABEDIT_MESSAGE_DEL     28
#define ABEDIT_MESSAGE_MSG     29
#define ABEDIT_TYPE_SPEC       30
#define ABEDIT_FUNCTION        31
#define ABEDIT_SKILL_STUN      32
#define ABEDIT_SKILL_STUN_CHAR 33
#define ABEDIT_SKILL_STUN_VICT 34
#define ABEDIT_VALUES          35
#define ABEDIT_VALUES_VAL      36
#define ABEDIT_QUIT            37
#define ABEDIT_CONFIRM_SAVE    38
#define ABEDIT_CONFIRM_ADD     39
#define ABEDIT_CONFIRM_DELETE  40

/* Skill stun types */
#define SKILL_STUN_SUCCESS  0 /**< Stun when skill succeeds */
#define SKILL_STUN_FAIL     1 /**< Stun when skill fails */
/* Number of skill stun types */
#define NUM_SKILL_STUN      2

/* Skill stun receiver */
#define SKILL_STUN_TO_CHAR 0 /**< Stun to actor/char */
#define SKILL_STUN_TO_VICT 1 /**< Stun to victim/target */
/* Number of skill stun receiver */
#define NUM_SKILL_STUN_TO  2

/* Utility macros */
/* TRUE if 'routine' is set in ability's routines */
#define HAS_ROUTINE(ability, routine)   (IS_SET(ability->routines, routine))
/* TRUE if 'target' is set in ability's targets */
#define CAN_TARGET(ability, target)     (IS_SET(ability->targets, target))
/* TRUE if 'flag' is set in ability's flags */
#define ABILITY_FLAGGED(ability, flag)  (IS_SET_VAR(ability-flags, flag))
/* TRUE if ability is of type ABILITY_SKILL */
#define IS_SKILL(ability)               ((ability)->type == ABILITY_TYPE_SKILL && (ability)->skillInfo != NULL)
/* TRUE if ability is of type ABILITY_SPELL */
#define IS_SPELL(ability)               ((ability)->type == ABILITY_TYPE_SPELL && (ability)->spellInfo != NULL)
/* TRUE if 'MAG_MANUAL' is set in ability's routines */
#define IS_MANUAL(ability)              HAS_ROUTINE(ability, MAG_MANUAL)

/* from skills.c*/
ACMD(do_abedit);

/* OLC functions */
void abeditParse(struct descriptor_data *d, char *arg);

/* Global ability list */
extern struct list_data *abilityList;

/* Damage dice roll structure */
struct dam_dice_info {
  int diceSize; /**< The dice size                */
  int numDice;  /**< The number of dice           */
  int damRoll;  /**< The addition constant damage */
};

/* Spell specific structure */
struct ability_spell_info_type {
  char *spellFunc; /**< The spell function name    */
  ASPELL(*spell);  /**< The spell function pointer */
};

/* Skill specific structure */
struct ability_skill_info_type {
  int stunChar[NUM_SKILL_STUN]; /**< The stuns to actor/char    */
  int stunVict[NUM_SKILL_STUN]; /**< The stuns to victim/target */
  int subcmd;                   /**< The subcommand             */
  char *skillFunc;              /**< The skill function name    */
  ACMD(*action);                /**< The skill function pointer */
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
  char *name;                                   /**> The name                                           */
  bool violent;                                 /**> The violent ability indicator                      */
  int type;                                     /**> The type                                           */
  int number;                                   /**> The number                                         */
  int minPosition;                              /**> The minimum position required to use this ability  */
  int routines;                                 /**> The routines                                       */
  int targets;                                  /**> The targets                                        */
  int minLevels[NUM_CLASSES];                   /**> The minimum class levels                           */
  int miscValues[NUM_ABILITY_VALUES];           /**> The miscellaneous/other values                     */
  int flags[ABF_ARRAY_MAX];                     /**> The ability flags                                  */
  struct ability_spell_info_type *spellInfo;    /**> The spell specific info                            */
  struct ability_skill_info_type *skillInfo;    /**> The skill specific info                            */
  struct msg_type *messages[NUM_ABILITY_MSGS];  /**> The messages                                       */
  int minCost;                                  /**> The minimum cost                                   */
  int maxCost;                                  /**> The maximum cost                                   */
  int costChange;                               /**> The cost change                                    */
  char *costFormula;                            /**> The cost formula/expression                        */
  char *affDurationFormula;                     /**> The affects duration formula/expression            */
  char *damDiceFormula;                         /**> The damage formula/expression                      */
  struct affected_type *affects;                /**> The affects                                        */
  int affDuration;                              /**> The standard affect duration                       */
  struct dam_dice_info *damDice;                /**> The standard damage dice roll                      */
};

#endif /* SKILLS_H_ */
#endif
