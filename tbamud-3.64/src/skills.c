/*
 * @author Hudas
 * @file skills.c
 * @created 05 Aug, 2013
 *
 * Implementation of new skill/spell system functions.
 *
 * Part of RavenMUD source code.
 * Extension of tbaMUD source code.
 */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "constants.h"
#include "utils.h"
#include "comm.h"
#include "spells.h"
#include "handler.h"
#include "db.h"
#include "constants.h"
#include "interpreter.h"
#include "dg_scripts.h"
#include "act.h"
#include "fight.h"
#include "class.h"
#include "skills.h"
#include "modify.h"
#include "graph.h"
#include "oasis.h"
#include "screen.h"

#define __SKILLS_C__

/* Utility macros */
#define WRITE_STR_LN(fp, field, value)  (fprintf(fp,"%-10s: %s\n", field, value))
#define WRITE_NUM_LN(fp, field, value)  (fprintf(fp,"%-10s: %d\n", field, value))
#define WRITE_CHA_LN(fp, field, value)  (fprintf(fp,"%-10s: %c\n", field, value))
#define NOT_NULL(str)                   (str ? str : "None.")

/* The global ability list */
struct list_data *abilityList;

/* file scope variables for OLC */
static int    curr_line;              /**< The current line of the skill file being read  */
static int    currVal1;               /**< Multi-purpose variable 1                       */
static int    currVal2;               /**< Multi-purpose variable 2                       */
static struct affected_type *currAff; /* The current affect edited                        */

/* local functions declaration */
static bool   equipmentSkillSuccess     (struct char_data *ch);
static bool   affectSkillSuccess        (struct char_data *ch);
static void   createAbilitySpell        (struct ability_info_type *ability);
static void   createAbilitySkill        (struct ability_info_type *ability);
static void   writeAbility              (FILE *abisFile, struct ability_info_type *ability);
static void   writeMessage              (FILE *fp, struct msg_type *msg, const char *name);
static void   readAbility               (FILE *fp, struct ability_info_type *ab, int type);
static void   parseAbility              (struct ability_info_type *ab, char *line);
static void   interpretAbilityLine      (struct ability_info_type *ab, char *line, char *value);
static void   displayGeneralAbilityInfo (struct char_data *ch, struct ability_info_type *ab);
static void   displayMessages           (struct char_data *ch, struct msg_type *msg, const char *messageName);
static int    getIndexOf                (char *str, const char **arr);
static struct ability_info_type * createAbility(int num, char *name, int type);

/* OLC local functions */
static void abeditDisplayMainMenu     (struct descriptor_data *d);
static void abeditDisplayAffMenu      (struct descriptor_data *d);
static void abeditDisplayTypes        (struct descriptor_data *d);
static void abeditDisplayPositions    (struct descriptor_data *d);
static void abeditDisplayCostMenu     (struct descriptor_data *d);
static void abeditDisplayDamMenu      (struct descriptor_data *d);
static void abeditDisplayFlags        (struct descriptor_data *d);
static void abeditDisplayRoutines     (struct descriptor_data *d);
static void abeditDisplayTargets      (struct descriptor_data *d);
static void abeditDisplayLevels       (struct descriptor_data *d);
static void abeditDisplayAffDurMenu   (struct descriptor_data *d);
static void abeditDisplayAffApplies   (struct descriptor_data *d);
static void abeditDisplayMessageMenu  (struct descriptor_data *d);
static void abeditDisplayMessageNew   (struct descriptor_data *d);
static void abeditDisplayMessages     (struct descriptor_data *d);
static void abeditDisplayTypeSpecMenu (struct descriptor_data *d);
static void abeditDisplayManualFuncs  (struct descriptor_data *d);
static void abeditDisplayValuesMenu   (struct descriptor_data *d);

/* abedit sub commands */
static ACMD(do_abedit_stat);
static ACMD(do_abedit_list);

/* Constants */
/* Manual spells table */
static struct manual_spell_info {
  char *name;     /**< The name             */
  char *funcName; /**< The function name    */
  ASPELL(*spell); /**< The function pointer */
} manual_spells [] ={
    {"charm person",    "spell_charm",           spell_charm         },
    {"identify",        "spell_identify",        spell_identify      },
    {"create water",    "spell_create_water",    spell_create_water  },
    {"recall",          "spell_recall",          spell_recall        },
    {"summon",          "spell_summon",          spell_summon        },
    {"locate object",   "spell_locate_object",   spell_locate_object },
    {"enchant weapon",  "spell_enchant_weapon",  spell_enchant_weapon},
    {"detect poison",   "spell_detect_poison",   spell_detect_poison },
    {"teleport",        "spell_teleport",        spell_teleport      },
    {"\n", "spell_null", NULL} /* always last*/
};

/* Manual skills table */
static struct manual_skill_info {
  char *name;     /**< The name             */
  char *funcName; /**< The function name    */
  int  subcmd;    /**< The sub-command      */
  ACMD(*action);  /**< The function pointer */
} manual_skills[] ={
    {"backstab",   "do_backstab",  0,         do_backstab  },
    {"bash",       "do_bash",      0,         do_bash      },
    {"hide",       "do_hide",      0,         do_hide      },
    {"kick",       "do_kick",      0,         do_kick      },
    {"pick lock",  "do_gen_door",  SCMD_PICK, do_gen_door  },
    {"rescue",     "do_rescue",    0,         do_rescue    },
    {"sneak",      "do_sneak",     0,         do_sneak     },
    {"steal",      "do_steal",     0,         do_steal     },
    {"track",      "do_track",     0,         do_track     },
    {"whirlwind",  "do_whirlwind", 0,         do_whirlwind },
    {"\n", "none", 0,  NULL},
};

/* Routines */
static const char *routine_types[NUM_ABI_ROUTINES + 1] = {
    "Damages",    /**< MAG_DAMAGES   */
    "Affects",    /**< MAG_AFFECTS   */
    "Unaffects",  /**< MAG_UNAFFECTS */
    "Points",     /**< MAG_POINTS    */
    "AlterObj",   /**< MAG_ALTER_OBJ */
    "Groups",     /**< MAG_GROUPS    */
    "Masses",     /**< MAG_MASSES    */
    "Areas",      /**< MAG_AREAS     */
    "Summons",    /**< MAG_SUMMONS   */
    "Creations",  /**< MAG_CREATIONS */
    "Manual",     /**< MAG_MANUAL    */
    "Rooms",      /**< MAG_ROOMS     */
    "\n"
};

/* Targets */
static const char *target_types[NUM_ABI_TARGETS + 1] = {
    "Ignore",       /**< TAR_IGNORE     */
    "CharInRoom",   /**< TAR_CHAR_ROOM  */
    "CharInWorld",  /**< TAR_CHAR_WORLD */
    "FightSelf",    /**< TAR_FIGHT_SELF */
    "FightVict",    /**< TAR_FIGHT_VICT */
    "SelfOnly",     /**< TAR_SELF_ONLY  */
    "NotSelf",      /**< TAR_NOT_SELF   */
    "ObjInInv",     /**< TAR_OBJ_INV    */
    "ObjInRoom",    /**< TAR_OBJ_ROOM   */
    "ObjInWorld",   /**< TAR_OBJ_WORLD  */
    "ObjEquiped",   /**< TAR_OBJ_EQUIP  */
    "\n"
};

/* Flags */
static const char *ability_flag_types[NUM_ABILITY_FLAGS + 1] = {
    "AccumDuration",  /**< ABILITY_ACCUMULATIVE_DURATION  */
    "AccumApply",     /**< ABILITY_ACCUMULATIVE_APPLY     */
    "CostsVigor",     /**< ABILITY_COSTS_VIGOR            */
    "AntiGood",       /**< ABILITY_ANTI_GOOD              */
    "AntiNeutral",    /**< ABILITY_ANTI_NEUTRAL           */
    "AntiEvil",       /**< ABILITY_ANTI_EVIL              */
    "RequireObj",     /**< ABILITY_REQUIRE_OBJ            */
    "\n"
};

/* Messeages */
static const char *ability_message_type[NUM_ABILITY_MSGS + 1] = {
    "Success",  /**< ABILITY_MSG_SUCCESS  */
    "Fail",     /**< ABILITY_MSG_FAIL     */
    "WearOff",  /**< ABILITY_MSG_WEA_ROFF */
    "GodVict",  /**< ABILITY_MSG_GOD_VICT */
    "Death",    /**< ABILITY_MSG_DEATH    */
    "\n"
};

/* Message Receivers */
static const char *ability_message_to[NUM_ABILITY_MSG_TO + 1] = {
    "ToChar", /**< ABILITY_MSG_TO_CHAR */
    "ToVict", /**< ABILITY_MSG_TO_VICT */
    "ToRoom", /**< ABILITY_MSG_TO_ROOM */
    "\n"
};

/* Ability Types */
static const char *ability_types[NUM_ABILITY_TYPES + 1] = {
    "Spell",  /**< ABILITY_TYPE_SPELL */
    "Skill",  /**< ABILITY_TYPE_SKILL */
    "\n"
};

/* Skill Stuns */
static const char *skill_stuns[NUM_SKILL_STUN + 1] = {
    "Success",  /**< SKILL_STUN_SUCCESS */
    "Fail",     /**< SKILL_STUN_FAIL    */
    "\n"
};

/* Skill Stun Receivers */
static const char *skill_stun_to[NUM_SKILL_STUN_TO + 1] = {
    "ToChar", /**< SKILL_STUN_TO_CHAR */
    "ToVict", /**< SKILL_STUN_TO_VICT */
    "\n"
};


/* new skill feature */
/*
 * Return the equivalent spell number of the given spell name. If spellName does
 * not exist in the current spell_info table, return -1.
 * @param spellName the name of the spell/skill
 */
int getSpellByName(const char *spellName) {
  int spellNum = -1, i;

  /* just to be sure */
  if(*spellName) {
    for(i = 0; i < NUM_SPELLS; i++)
      if(!strcasecmp(spell_info[i].name, spellName)){
        spellNum = i;
        break;
      }

  } else {
    log("SYSERR: NULL spellName passed in getSpellByName() in %s:%d", __FILE__, __LINE__);
  }

  return spellNum;
}

/*
 * Returns TRUE if ch is affected by the given spellNum, otherwise FALSE. This
 *  will replace the AFF_FLAGG(ch, AFF_xxx) function. Instead of just checking
 *  if a bit is set, this will take the longer route of checking ch's affects
 *  for the given spell.
 * @param ch the mob/player
 * !parac spellNum the spell/skill number
 */
bool isAffectedBySpellNum(struct char_data *ch, sh_int spellNum) {
  struct affected_type *affect;
  bool isAffected = FALSE;

  if(spellNum > -1 && spellNum < NUM_SPELLS){
    for(affect = ch->affected; affect; affect = affect->next) {
      if(affect->spell == spellNum) {
        isAffected = TRUE;
        break;
      }
    }
  } else {
    log("SYSERR: Invalid spell '%d' passed to isAffectedBySpellNum!", spellNum);
  }

  return isAffected;
}

/*
 * Returns TRUE if ch is affected by the given spellName, otherwise FALSE. This
 *  will replace the AFF_FLAGG(ch, AFF_xxx) function. Instead of just checking
 *  if a bit is set, this will take the longer route of checking ch's affects
 *  for the given spell.
 * @param ch the mob/player
 * !parac spellName the name of the spell/skill
 */
bool isAffectedBySpellName(struct char_data *ch, const char *spellName) {
  return isAffectedBySpellNum(ch, getSpellByName(spellName));
}

/*
 * Return TRUE if roll is successful for ch's skillNum.
 * @param ch the actor of the skill
 * @param skill the skill number
 */
bool skillSuccess(struct char_data *ch, sh_int skillNum) {
  bool success = FALSE;
  int chance;

  if(skillNum > -1 && skillNum < MAX_SKILLS) {
    if(IS_NPC(ch)) {
      if(spell_info[skillNum].min_level[GET_CLASS(ch)] > GET_LEVEL(ch)) {
        chance = 0;
      } else {
        /**
         * Mob skill success still needs to be added soon..
         */
        chance = (GET_LEVEL(ch) + GET_SKILL(ch, skillNum)) / 2;

      }
    } else {
      chance = GET_PLAYER_SKILL(ch, skillNum);
    }

    if(chance > 0) {
      success = equipmentSkillSuccess(ch) && affectSkillSuccess(ch);
    }

    /*
     * Still not successful from eq and affects. Now calculated the chance based
     * on stances, spell affects, etc.
     */
    if(!success) {
      //for now roll chance as is

      //this should be the last roll for success
      chance = (chance > 99 ? 99 : chance);
      success = PERCENT_SUCCESS(chance);
    }

  } else {
    log("SYSERR: Invalid skill '%d' passed to isAffectedBySpellNum!", skillNum);
  }

  return success;
}

/*
 * Returns TRUE if roll is successful for ch's skillName.
 * @param ch the actor of the skill
 * @param skillName the name of the skill
 */
bool skillSuccessByName(struct char_data *ch, const char *skillName) {
  return skillSuccess(ch, getSpellByName(skillName));
}

/*
 * Load abilities from file.
 */
void loadAbilities (void) {
  struct ability_info_type *ab;
  int type = 0;
  char line[MAX_STRING_LENGTH];
  FILE *fp;

  if((fp = fopen(SKILLS_FILE, "r"))) {

    // initialize the list
    if(abilityList)
      free_list(abilityList);
    abilityList = create_list();

    curr_line = 0;
    while(!feof(fp) && (get_line(fp, line))) {

      // end of file
      if(*line == '$') break;

      if(!str_cmp(line, "Spell")) {
        type  = ABILITY_TYPE_SPELL;
      } else if (!str_cmp(line, "Skill")) {
        type = ABILITY_TYPE_SKILL;
      } else {
        log("Invalid ability type '%s' at line %d of %s", line, curr_line, SKILLS_FILE);
      }
      curr_line++;
      ab = createAbility(0, "NotSetYet", type);
      readAbility(fp, ab, type);
    }

    fclose(fp);

  } else {
    log("SYSERRR: Unable to open skills file for reading!");
    exit(1);
  }

  log("Loaded %d abilities, %ld bytes.", abilityList->iSize, sizeof(struct ability_info_type) * abilityList->iSize);

}

/*
 * Save all abilities.
 * @return the number of abilities saved
 */
int saveAbilities (void) {
  int count = 0, written;
  FILE *abisFile;
  struct ability_info_type *a;

  if(abilityList->iSize > 0) {
    if((abisFile = fopen(SKILLS_FILE, "w"))) {
      while((a = (struct ability_info_type *) simple_list(abilityList))) {
        writeAbility(abisFile, a);
        count++;
      }
      fputs("$",abisFile);
      written = ftell(abisFile);
      fclose(abisFile);
      log("SkEdit: '%s' saved, %d bytes written.", SKILLS_FILE, written);
    } else {
      log("SYSERR: SkEdit: saveSkills: Cannot open %s for writing!", SKILLS_FILE);
    }
  } else {
    log("SYSERR: SkEdit: abilityList is empty!");
  }

  return count;
}

/*
 * Returns the ability with the given num, NULL if the ability does not exist.
 */
struct ability_info_type *getAbility(int num) {
  struct ability_info_type *ability = NULL, *a;
  bool found = FALSE;

  while((a = (struct ability_info_type *) simple_list(abilityList))) {
    if(!found && a->number == num){
      ability = a;
      found = TRUE;
    }
  }

  return ability;
}

/*
 * Returns the ability with the given name, NULL if the ability does not exist.
 */
struct ability_info_type *getAbilityByName(char *name) {
  struct ability_info_type *ability = NULL, *a;
    bool found = FALSE;

    while((a = (struct ability_info_type *) simple_list(abilityList))) {
      if(!found && !str_cmp(name, a->name)){
        ability = a;
        found = TRUE;
      }
    }

    return ability;
}

/*
 * Interprets the given 'arg' and calls the appropriate functions based on the
 * current OLC mode.
 * Used by nanny() in interpreter.c
 */
void abeditParse(struct descriptor_data *d, char *arg) {
  bool quit = FALSE;
  struct ability_info_type *ab = OLC_ABILITY(d);
  struct affected_type *aff, *prev;
  struct msg_type *msg;
  int num = -1, cls = -1, i;
  char val[MAX_INPUT_LENGTH];

  if(*arg)
    num = atoi(arg);

  switch(OLC_MODE(d)) {
  case ABEDIT_MAIN_MENU: {
    if(!*arg) {
      abeditDisplayMainMenu(d);
      return;
    } else {
      switch(*arg) {
      case '1':
        write_to_output(d,"Name: \r\n");
        OLC_MODE(d) = ABEDIT_NAME;
        break;
      case '2':
        abeditDisplayTypes(d);
        break;
      case '3':
        abeditDisplayPositions(d);
        break;
      case '4':
        // just toggle
        ab->violent = !ab->violent;
        break;
      case '5':
        abeditDisplayCostMenu(d);
        break;
      case '6':
        if(IS_SET(ab->routines, MAG_DAMAGE)) {
          abeditDisplayDamMenu(d);
        }
        else
          write_to_output(d, "Please set the 'Damages' routine first.\r\n");
        break;
      case 'A':
      case 'a':
        abeditDisplayAffMenu(d);
        break;
      case 'F':
      case 'f':
        abeditDisplayFlags(d);
        break;
      case 'L':
      case 'l':
        abeditDisplayLevels(d);
        break;
      case 'M':
      case 'm':
        abeditDisplayMessageMenu(d);
        break;
      case 'R':
      case 'r':
        abeditDisplayRoutines(d);
        break;
      case 'S':
      case 's':
        abeditDisplayTypeSpecMenu(d);
        break;
      case 'T':
      case 't':
        abeditDisplayTargets(d);
        break;
      case 'V':
      case 'v':
        abeditDisplayValuesMenu(d);
        break;
      case 'X':
      case 'x':
        if(getAbility(ab->number) != NULL) {
          write_to_output(d, "Are you sure you want to delete ability [%s%d%s]%s%s%s (Y/N)?\r\n",
              cyn, ab->number, nrm, grn, ab->name, nrm);
          OLC_MODE(d) = ABEDIT_CONFIRM_DELETE;
        } else {
          abeditDisplayMainMenu(d);
          return;
        }
        break;
      case '?':
        write_to_output(d, "To be implemented soon...\r\n");
        break;
      case 'Q':
      case 'q':
        if(OLC_VAL(d) == 1) {
          quit = TRUE;
          if(getAbility(ab->number) != NULL) {
            write_to_output(d, "Do you want to save your changes to file (Y/N)?\r\n");
            OLC_MODE(d) = ABEDIT_CONFIRM_SAVE;
          } else {
            // this is a new ability confirm to add!
            write_to_output(d, "Are you sure you want to add this new ability (Y/N)? :\r\n");
            OLC_MODE(d) = ABEDIT_CONFIRM_ADD;
          }
        } else {
          write_to_output(d,"Exiting ability editor.\r\n");
          cleanup_olc(d, CLEANUP_ALL);
          return;
        }
        break;
      default:
        abeditDisplayMainMenu(d);
        return;
      }
    }
    break;
  }
  case ABEDIT_CONFIRM_DELETE:
    if(LOWER(*arg) == 'y') {
      remove_from_list(ab, abilityList);
      write_to_output(d,"Exiting ability editor.\r\n");
      cleanup_olc(d, CLEANUP_ALL);
      free(ab);
    } else if(LOWER(*arg) == 'n') {
      abeditDisplayMainMenu(d);
    } else {
      write_to_output(d, "Invalid Option!\r\nAre you sure you want to delete ability [%s%d%s]%s%s%s (Y/N)?\r\n",
          cyn, ab->number, nrm, grn, ab->name, nrm);
    }
    return;
  case ABEDIT_CONFIRM_SAVE:
    if(LOWER(*arg) == 'y') {
      num = saveAbilities();
      write_to_output(d, "%d abilities saved to file.\r\n", num);
      write_to_output(d,"Exiting ability editor.\r\n");
      cleanup_olc(d, CLEANUP_ALL);
    } else if(LOWER(*arg) == 'n') {
      write_to_output(d,"Exiting ability editor.\r\n");
      cleanup_olc(d, CLEANUP_ALL);
    } else {
      write_to_output(d, "Invalid option!\r\nDo you want to save your changes to file (Y/N)?\r\n");
    }
    return;
  case ABEDIT_CONFIRM_ADD:
    if(LOWER(*arg) == 'y') {
      add_to_list(ab, abilityList);
      write_to_output(d, "New ability [%s%d%s]%s%s%s added to list.\r\n", cyn, ab->number, nrm, grn, ab->name, nrm);
      write_to_output(d, "Do you want to save your changes to file (Y/N)?\r\n");
      OLC_MODE(d) = ABEDIT_CONFIRM_SAVE;
    } else if (LOWER(*arg) == 'n') {
      write_to_output(d,"Exiting ability editor.\r\n");
      cleanup_olc(d, CLEANUP_ALL);
      free(ab);
    } else {
      write_to_output(d, "Invalid option!\r\nDo you want to save your changes to file (Y/N)?\r\n");
    }
    return;
  case ABEDIT_AFF_MENU: {
    if(!*arg) {
      abeditDisplayAffMenu(d);
    } else {
      switch(*arg) {
      case 'D':
      case 'd':
        abeditDisplayAffDurMenu(d);
        break;
      case 'N':
      case 'n':
        abeditDisplayAffApplies(d);
        break;
      case 'Q':
      case 'q':
        abeditDisplayMainMenu(d);
        break;
      case 'X':
      case 'x':
        if(ab->affects != NULL) {
          write_to_output(d, "Enter affect number :\r\n");
          OLC_MODE(d) = ABEDIT_AFF_DELETE;
        } else {
          abeditDisplayAffMenu(d);
        }
        break;
      default:
        if(is_number(arg) && ((num = atoi(arg)) > 0)) {
          for(aff = ab->affects, i = 1; aff != NULL && i != num; aff = aff->next, i++);

          if(aff != NULL) {
            currAff = aff;
            abeditDisplayAffApplies(d);
          } else {
            write_to_output(d, "That affect doest not exist!!!\r\n");
            abeditDisplayAffMenu(d);
          }
        } else {
          abeditDisplayAffMenu(d);
        }
      }
    }
    break;
  }
  case ABEDIT_NAME:
    if(*arg) {
      ab->name = strdup(arg);
    }
    abeditDisplayMainMenu(d);
    break;
  case ABEDIT_TYPE:
    if(num == 0) {
      abeditDisplayMainMenu(d);
    } else if(num > 0 && num <= NUM_ABILITY_TYPES) {
      ab->type = num-1;
      abeditDisplayMainMenu(d);
    } else {
      write_to_output(d, "Invalid selection!\r\nType (0 to quit) :\r\n");
    }
    break;
  case ABEDIT_MINPOS:
    if(num == 0) {
      abeditDisplayMainMenu(d);
    } else if(num > 0 && num <= NUM_POSITIONS) {
      ab->minPosition = num-1;
      abeditDisplayMainMenu(d);
    } else {
      write_to_output(d, "Invalid selection!\r\nPosition (0 to quit):\r\n");
    }
    break;
  case ABEDIT_COST:
    switch(*arg){
    case '1':
      write_to_output(d, "(%s?%s) For help on variables\r\n", grn, nrm);
      write_to_output(d, "Cost Expression (blank to clear) :\r\n");
      OLC_MODE(d) = ABEDIT_COST_EXPR;
      break;
    case '2':
      write_to_output(d, "Minimun Cost(1 - 999) :\r\n");
      OLC_MODE(d) = ABEDIT_COST_MIN;
      break;
    case 'Q':
    case 'q':
    default:
      abeditDisplayMainMenu(d);
      return;
    }
    break;
    case ABEDIT_COST_MIN:
      if(num > 0 && num <= 999) {
        ab->minCost = num;
        write_to_output(d, "Maximum Cost(%d - 999) :\r\n", num + 1);
        OLC_MODE(d) = ABEDIT_COST_MAX;
      } else {
        write_to_output(d, "Enter value between 1 - 999 :\r\n");
      }
      return;
    case ABEDIT_COST_MAX:
      if(num > 0 && num <= 999 && num > ab->minCost) {
        ab->maxCost = num;
        OLC_MODE(d) = ABEDIT_COST_CHG;
        write_to_output(d, "Cost Change(1 - 999) :\r\n");
      } else {
        write_to_output(d, "Enter value between %d - 999 :\r\n", ab->minCost + 1);
      }
      return;
    case ABEDIT_COST_CHG:
      if(num > 0 && num <= 999) {
        ab->costChange = num;
        abeditDisplayCostMenu(d);
      } else {
        write_to_output(d, "Enter value between 0 - 999 :\r\n");
      }
      return;
    case ABEDIT_COST_EXPR:
      if(*arg) {
        if(*arg == '?') {
          write_to_output(d, "To be implemented soon...\r\n");
          write_to_output(d, "Cost Expression (blank to clear) :\r\n");
        } else {
          ab->costFormula = strdup(arg);
          abeditDisplayCostMenu(d);
        }
      } else {
        ab->costFormula = NULL;
        ab->minCost = 0;
        ab->maxCost = 0;
        ab->costChange = 0;
        abeditDisplayCostMenu(d);
      }
      return;
    case ABEDIT_DAM:
      switch(*arg){
      case '1':
        write_to_output(d, "(%s?%s) For help on variables\r\n", grn, nrm);
        write_to_output(d, "Damage Expression (blank to clear) :\r\n");
        OLC_MODE(d) = ABEDIT_DAM_EXPR;
        break;
      case '2':
        if(ab->damDice == NULL) {
          CREATE(ab->damDice, struct dam_dice_info, 1);
        }
        write_to_output(d, "Number of Dice(1 - 999) :\r\n");
        OLC_MODE(d) = ABEDIT_DAM_NUM;
        break;
      case 'Q':
      case 'q':
      default:
        abeditDisplayMainMenu(d);
      }
      break;
    case ABEDIT_DAM_NUM:
      if(num > 0 && num <= 999) {
        ab->damDice->numDice = num;
        write_to_output(d, "Dice Size(1 - 999) :\r\n");
        OLC_MODE(d) = ABEDIT_DAM_SIZE;
      } else {
        write_to_output(d, "Enter value between 1 - 99 :\r\n");
      }
      return;
    case ABEDIT_DAM_SIZE:
      if(num > 0 && num <= 999) {
        ab->damDice->diceSize = num;
        write_to_output(d, "Damroll(1 - 999) :\r\n");
        OLC_MODE(d) = ABEDIT_DAM_DR;
      } else {
        write_to_output(d, "Enter value between 1 - 99 :\r\n");
      }
      return;
    case ABEDIT_DAM_DR:
      if(num > 0 && num <= 999) {
        ab->damDice->damRoll = num;
        abeditDisplayDamMenu(d);
      } else {
        write_to_output(d, "Enter value between 1 - 99 :\r\n");
      }
      return;
    case ABEDIT_DAM_EXPR:
      if(*arg) {
        if(*arg == '?') {
          write_to_output(d, "To be implemented soon...\r\n");
          write_to_output(d, "Damage Expression (blank to clear) :\r\n");
        } else {
          ab->damDiceFormula = strdup(arg);
          abeditDisplayMainMenu(d);
        }
      } else {
        ab->damDiceFormula = NULL;
        ab->damDice->numDice = 0;
        ab->damDice->diceSize = 0;
        ab->damDice->damRoll = 0;
        abeditDisplayMainMenu(d);
      }
      return;
    case ABEDIT_FLAGS:
      if(num  == 0) {
        abeditDisplayMainMenu(d);
      } else if (num > 0 && num < NUM_ABILITY_FLAGS + 1) {
        TOGGLE_BIT_AR(ab->flags, num - 1);
        abeditDisplayFlags(d);
      } else {
        write_to_output(d, "Please select from the flags below :\r\n");
        abeditDisplayFlags(d);
      }
      return;
    case ABEDIT_ROUTINES:
      if(num  == 0) {
        abeditDisplayMainMenu(d);
      } else if (num > 0 && num < NUM_ABI_ROUTINES + 1) {
        TOGGLE_BIT(ab->routines, (1 << (num - 1)));
        abeditDisplayRoutines(d);
      } else {
        write_to_output(d, "Please select from the flags below :\r\n");
        abeditDisplayRoutines(d);
      }
      return;
    case ABEDIT_TARGETS:
      if(num  == 0) {
        abeditDisplayMainMenu(d);
      } else if (num > 0 && num < NUM_ABI_TARGETS + 1) {
        TOGGLE_BIT(ab->targets, (1 << (num - 1)));
        abeditDisplayTargets(d);
      } else {
        write_to_output(d, "Please select from the flags below :\r\n");
        abeditDisplayTargets(d);
      }
      return;
    case ABEDIT_LEVELS:
      if(*arg) {
        if(LOWER(*arg) == 'q') {
          abeditDisplayMainMenu(d);
        } else if((sscanf(arg,"%s %d", val, &num)) == 2) {
          cls = getIndexOf(val, class_abbrevs);
          if((cls >= 0 && cls < NUM_CLASSES) && (num > 0 && num <= LVL_IMMORT)) {
            ab->minLevels[cls] = num;
            abeditDisplayLevels(d);
          } else {
            write_to_output(d, "Invalid class or level!\r\n");
            write_to_output(d, "Enter class and level (e.g. wa 1) or Q to quit :\r\n");
          }
        } else {
          write_to_output(d, "Enter class and level (e.g. wa 1) or Q to quit :\r\n");
        }
      } else {
        write_to_output(d, "Enter class and level (e.g. wa 1) or Q to quit :\r\n");
        abeditDisplayLevels(d);
      }
      return;
    case ABEDIT_AFF_DUR:
      if(*arg) {
        switch(*arg){
        case '1':
          write_to_output(d, "(%s?%s) For help on variables\r\n", grn, nrm);
          write_to_output(d, "Duration Expression (blank to clear) :\r\n");
          OLC_MODE(d) = ABEDIT_AFF_DUR_EXPR;
          break;
        case '2':
          write_to_output(d, "Duration (1 - 999) :\r\n");
          OLC_MODE(d) = ABEDIT_AFF_DUR_VAL;
          break;
        case 'Q':
        case 'q':
          abeditDisplayAffMenu(d);
          break;
        default:
          abeditDisplayAffDurMenu(d);
          break;
        }
      } else {
        abeditDisplayAffDurMenu(d);
      }
      return;

    case ABEDIT_AFF_DUR_EXPR:
      if(*arg) {
        if(*arg == '?') {
          write_to_output(d, "To be implemented soon....\r\n");
          write_to_output(d, "Duration Expression (blank to clear) :\r\n");
        } else {
          ab->affDurationFormula = strdup(arg);
          abeditDisplayAffDurMenu(d);
        }
      } else {
        ab->affDurationFormula = NULL;
        ab->affDuration = 0;
        abeditDisplayAffDurMenu(d);
      }
      return;
    case ABEDIT_AFF_DUR_VAL:
      if(num > 0 && num <= 999) {
        ab->affDuration = num;
        abeditDisplayAffDurMenu(d);
      } else {
        write_to_output(d, "Invalid duration!\r\nDuration (1 - 999) :\r\n");
      }
      return;
    case ABEDIT_AFF_APPLY:
      if(num == 0) {
        abeditDisplayAffMenu(d);
      } else if((num = get_flag_by_number(num, NUM_APPLIES, NUM_ILLEGAL_APPLIES, illegal_applies)) != -1) {
        //create the new aff here to avoid memory leaks if the the user enters
        //0 and cancels the new affect
        if(currAff == NULL) {
          CREATE(currAff, struct affected_type, 1);
          currAff->next = NULL;
        }
        currAff->location = num;

        write_to_output(d, "Modifier (-999 - 999) :\r\n");
        OLC_MODE(d) = ABEDIT_AFF_MOD;
      } else {
        write_to_output(d, "Invalid location!\r\n");
        abeditDisplayAffApplies(d);
      }
      return;
    case ABEDIT_AFF_MOD:
      if(num >= -999 && num <= 999) {
        currAff->modifier = num;
        if(ab->affects == NULL) {
          // this is the first affect
          ab->affects = currAff;
          ab->affects->next = NULL;
        } else {
          //find first if it's already in the list
          // add end of affect list if new
          for(aff = ab->affects; aff->next != NULL || aff != currAff; aff = aff->next);
          // this is indeed a new affect
          if(aff != currAff) {
            aff->next = currAff;
          }
        }
        currAff = NULL;
        abeditDisplayAffMenu(d);
      } else {
        write_to_output(d, "Invalid modifier!\r\nModifier (-999 - 999) :\r\n");
      }
      return;
    case ABEDIT_AFF_DELETE:
      if(num > 0) {
        //find the aff we want to delete
        for(aff = ab->affects, i = 1; aff != NULL && i != num; aff = aff->next, i++);

        //found it
        if(aff != NULL) {

          if(ab->affects == aff) {
            // it's the first affect
            ab->affects = aff->next;
            aff = NULL;
            free(aff);
          } else {
            //find the affect before aff
            for(prev = ab->affects; prev != NULL && prev->next != aff; prev = prev->next);

            prev->next = aff->next;
            aff = NULL;
            free(aff);
          }
          abeditDisplayAffMenu(d);

        } else {
          // it doesn't exist
          write_to_output(d, "That affect does not exist!!!\r\n");
          abeditDisplayAffMenu(d);
        }
      } else {
        write_to_output(d, "Maybe you should start with 1???\r\n");
        abeditDisplayAffMenu(d);
      }
      return;
    case ABEDIT_MESSAGE:
      if(*arg) {
        switch(*arg) {
        case 'Q':
        case 'q':
          abeditDisplayMainMenu(d);
          return;
        case 'N':
        case 'n':
          abeditDisplayMessageNew(d);
          return;
        case 'X':
        case 'x':
          write_to_output(d, "Message number :\r\n");
          OLC_MODE(d) = ABEDIT_MESSAGE_DEL;
          return;
        default:
          if((num = atoi(arg)) > 0 && num < NUM_ABILITY_MSGS + 1) {
            currVal2 = num - 1;
            abeditDisplayMessages(d);
          } else {
            write_to_output(d, "Invalid message number!\r\n");
            abeditDisplayMessageMenu(d);
          }
          return;
        }
      } else {
        abeditDisplayMessageMenu(d);
        return;
      }
      break;
    case ABEDIT_MESSAGE_NEW:
      if(num == 0) {
        abeditDisplayMessageMenu(d);
      }else if(num > 0 && num < NUM_ABILITY_MSGS + 1) {
        currVal1 = num - 1;
        if(ab->messages[currVal1] != NULL) {
          write_to_output(d, "Message is not empty, switching to edit.\r\n");
        } else {
          CREATE(ab->messages[currVal1], struct msg_type, 1);
        }
        abeditDisplayMessages(d);
      } else {
        write_to_output(d, "Invalid message number!\r\nEnter message number (0 to quit) :");
      }
      return;
    case ABEDIT_MESSAGE_DEL:
      if(num > 0) {
        msg = NULL;
        for(i = 0; i < NUM_ABILITY_MSGS; i++) {
          if(i == (num - 1))
            msg = ab->messages[i];
        }

        if(msg != NULL) {
          free(msg->attacker_msg);
          free(msg->room_msg);
          free(msg->victim_msg);
          ab->messages[num - 1] = NULL;
          abeditDisplayMessageMenu(d);
        } else {
          write_to_output(d, "That message does not exist!\r\n");
          abeditDisplayMessageMenu(d);
        }
      } else {
        write_to_output(d, "Invalid message number!\r\n");
        abeditDisplayMessageMenu(d);
      }
      return;
    case ABEDIT_MESSAGE_EDIT:
      if(num == 0) {
        abeditDisplayMessageMenu(d);
      } else if(num > 0 && num < NUM_ABILITY_MSG_TO + 1) {
        write_to_output(d, "Message (blank to clear):\r\n");
        currVal2 = num - 1;
        OLC_MODE(d) = ABEDIT_MESSAGE_MSG;
      } else {
        write_to_output(d, "Invalid message tp number!\r\nMessage to number (0 to quit) :\r\n");
      }
      return;
    case ABEDIT_MESSAGE_MSG:
      if(*arg) {
        if(currVal2 == ABILITY_MSG_TO_CHAR) {
          ab->messages[currVal1]->attacker_msg = strdup(arg);
        } else if(currVal2 == ABILITY_MSG_TO_ROOM) {
          ab->messages[currVal1]->room_msg = strdup(arg);
        } else if(currVal2 == ABILITY_MSG_TO_VICT) {
          ab->messages[currVal1]->victim_msg = strdup(arg);
        }
      } else {
        if(currVal2 == ABILITY_MSG_TO_CHAR) {
          ab->messages[currVal1]->attacker_msg = NULL;
        } else if(currVal2 == ABILITY_MSG_TO_ROOM) {
          ab->messages[currVal1]->room_msg = NULL;
        } else if(currVal2 == ABILITY_MSG_TO_VICT) {
          ab->messages[currVal1]->victim_msg = NULL;
        }
      }
      abeditDisplayMessages(d);
      return;
    case ABEDIT_TYPE_SPEC:
      if(*arg) {
        switch(*arg) {
        case 'F':
        case 'f':
          abeditDisplayManualFuncs(d);
          return;
        case 'Q':
        case 'q':
          abeditDisplayMainMenu(d);
          return;
        default:
          if(num > 0) {
            if(IS_SKILL(ab) && num < NUM_SKILL_STUN + 1) {
              if((num - 1) == SKILL_STUN_SUCCESS) {
                currVal1 = SKILL_STUN_SUCCESS;
              } else if((num - 1) == SKILL_STUN_FAIL) {
                currVal1 = SKILL_STUN_FAIL;
              }
              write_to_output(d, "Enter %s stun %s value (0 - 10) :\r\n", skill_stuns[currVal1], skill_stun_to[SKILL_STUN_TO_CHAR]);
              OLC_MODE(d) = ABEDIT_SKILL_STUN_CHAR;
            }
          } else {
            abeditDisplayTypeSpecMenu(d);
          }
          return;
        }
      } else {
        abeditDisplayTypeSpecMenu(d);
      }
      break;
    case ABEDIT_SKILL_STUN_CHAR:
      if(*arg) {
        if(num >= 0 && num <= 10) {
          ab->skillInfo->stunChar[currVal1] = num;
          write_to_output(d, "Enter %s stun %s value (0 - 10) :\r\n", skill_stuns[currVal1], skill_stun_to[SKILL_STUN_TO_VICT]);
          OLC_MODE(d) = ABEDIT_SKILL_STUN_VICT;
        } else {
          write_to_output(d, "Enter %s stun %s value (0 - 10) :\r\n", skill_stuns[currVal1], skill_stun_to[SKILL_STUN_TO_CHAR]);
        }
      } else {
        write_to_output(d, "Enter %s stun %s value (0 - 10) :\r\n", skill_stuns[currVal1], skill_stun_to[SKILL_STUN_TO_CHAR]);
      }
      return;
    case ABEDIT_SKILL_STUN_VICT:
      if(*arg) {
        if(num >= 0 && num <= 10) {
          ab->skillInfo->stunVict[currVal1] = num;
          abeditDisplayTypeSpecMenu(d);
        } else {
          write_to_output(d, "Enter %s stun %s value (0 - 10) :\r\n", skill_stuns[currVal1], skill_stun_to[SKILL_STUN_TO_VICT]);
        }
      } else {
        write_to_output(d, "Enter %s stun %s value (0 - 10) :\r\n", skill_stuns[currVal1], skill_stun_to[SKILL_STUN_TO_VICT]);
      }
      return;
    case ABEDIT_FUNCTION:
      if(num == 0) {
        abeditDisplayTypeSpecMenu(d);
      } else if(num > 0) {
        if(IS_SPELL(ab) && num < (sizeof(manual_spells)/sizeof(manual_spells[0]))) {
          ab->spellInfo->spell = manual_spells[num - 1].spell;
          ab->spellInfo->spellFunc = manual_spells[num - 1].funcName;
        } else if(IS_SKILL(ab) && num < (sizeof(manual_skills)/sizeof(manual_skills[0]))) {
          ab->skillInfo->action = manual_skills[num -  1].action;
          ab->skillInfo->skillFunc = manual_skills[num - 1].funcName;
          ab->skillInfo->subcmd = manual_skills[num - 1].subcmd;
        } else {
          write_to_output(d, "Invalid selection!\r\nEnter selection (0 - quit):\r\n");
          return;
        }
        abeditDisplayTypeSpecMenu(d);
      } else {
        write_to_output(d, "Invalid selection!\r\nEnter selection (0 - quit):\r\n");
        return;
      }
      return;
    case ABEDIT_VALUES:
      if(num == 0) {
        abeditDisplayMainMenu(d);
      } else if(num > 0 && num < NUM_ABILITY_VALUES + 1) {
        currVal1 = num - 1;
        write_to_output(d, "Enter value for %d (0 to quit) :\r\n", currVal1 + 1);
        OLC_MODE(d) = ABEDIT_VALUES_VAL;
      } else {
        write_to_output(d, "Invalid value!\r\nEnter value for %d (0 to quit) :\r\n", currVal1 + 1);
      }
      return;
    case ABEDIT_VALUES_VAL:
      if(num == 0) {
        abeditDisplayValuesMenu(d);
      } else if(num > 0) {
        if(HAS_ROUTINE(ab, MAG_SUMMONS) && real_mobile(num) == NOBODY) {
          write_to_output(d, "That mobile does not exist!\r\nEnter value for %d (0 - quit) :\r\n", currVal1 + 1);
        } else if(HAS_ROUTINE(ab, MAG_CREATIONS) && real_object(num) == NOTHING) {
          write_to_output(d, "That object does not exist!\r\nEnter value for %d (0 - quit) :\r\n", currVal1 + 1);
        } else {
          ab->miscValues[currVal1] = num;
          abeditDisplayValuesMenu(d);
        }
      } else {
        write_to_output(d, "Invalid value!!!\r\nEnter value for %d (0 to quit) :\r\n", currVal1 + 1);
      }
      return;
    default:
      abeditDisplayMainMenu(d);
      return;
  }

  if(!quit) {
    OLC_VAL(d) = 1;
  }
}

/*
 * Local functions definition
 */

/*
 * Return TRUE if ch's worn items has skill success and a roll is successful
 * @param ch the player
 */
static bool equipmentSkillSuccess(struct char_data *ch) {
  bool success = FALSE;
  int j, i;

  for( j = 0; j < NUM_WEARS; j++ ) {
    if( ch->equipment[j]) {
      for (i = 0; i < MAX_OBJ_AFFECT; i++) {
        if((success = PERCENT_SUCCESS(ch->equipment[j]->affected[i].location)))
          break;
      }
    }
  }

  return success;
}

/*
 * Returns TRUE if ch has affections with skill success and if a roll is successful.
 * @param ch the player
 */
static bool affectSkillSuccess (struct char_data *ch) {
  struct affected_type *af;
  bool success = FALSE;

  for (af = ch->affected; af; af = af->next) {
    if(af->location == APPLY_SKILL_SUCCESS) {
      if(af->modifier > 0 && PERCENT_SUCCESS(af->modifier)) {
        success = TRUE;
        break;
      }
      else if(af->modifier < 0 && PERCENT_SUCCESS(-(af->modifier))) {
        success = FALSE;
        break;
      }
    }
  }

  return success;
}

/*
 * Utility function to return the index of 'str' from the given 'arr'.
 */
static int getIndexOf(char *str, const char **arr) {
  int i, index = -1;

  for(i = 0; *arr[i] != '\n'; i++) {
    if(!str_cmp(arr[i], str)) {
      index = i;
      break;
    }
  }

  return index;
}

#define STR_TO_INDEX(str, arr) ((num_val = getIndexOf(str, arr)) < 0 ? 0 : num_val)
#define CASE(test)  \
  if (value && !matched && !str_cmp(field, test) && (matched = TRUE))

#define FIELD_STR_VAL(key, field) CASE(key) field = strdup(value)
#define FIELD_NUM_VAL(key, field) CASE(key) field = num_val
#define ERROR_FIELD_VAL(field) (log("SYSERR: Invalid value format for field %s at %s:%d", field, SKILLS_FILE, curr_line))

/*
 * Interprets the current line from the skill file.
 * @param ab the ability
 * @param field the field
 * @param the value
 */
static void interpretAbilityLine(struct ability_info_type *ab, char *field, char *value) {
  int num_val = 0, i, nVal1 = -1, nVal2 = -1;
  bool matched = FALSE;
  char val1[MAX_STRING_LENGTH], val2[MAX_STRING_LENGTH];
  char *var;
  struct affected_type *aff;
  struct msg_type *msg;

  if(value)
    num_val = atoi(value);

  FIELD_NUM_VAL("Number", ab->number);
  FIELD_STR_VAL("Name",   ab->name);
  FIELD_STR_VAL("CostExpr", ab->costFormula);
  FIELD_STR_VAL("DamExpr", ab->damDiceFormula);

  CASE("Cost") {
    if((sscanf(value,"%d %d %d",&ab->minCost, &ab->maxCost, &ab->costChange)) < 3){
      ERROR_FIELD_VAL("Cost");
    }
  }
  CASE("DamDice") {
    if(!ab->damDice)
      CREATE(ab->damDice, struct dam_dice_info, 1);
    if((sscanf(value,"%d %d %d",&ab->damDice->diceSize,
        &ab->damDice->numDice, &ab->damDice->damRoll)) < 3) {
      ERROR_FIELD_VAL("DamDice");
    }
   }

  CASE("MinPos") {
    ab->minPosition = STR_TO_INDEX(value, position_types);
  }

  CASE("Violent") {
    // other than Yes, its FALSE
    ab->violent = (!str_cmp(value, "Yes") ? TRUE : FALSE);
  }

  CASE("Affects") {
    if((sscanf(value, "%s %d", val1, &nVal1)) < 2) {
      ERROR_FIELD_VAL("Affects");
    } else {
      CREATE(aff, struct affected_type, 1);
      aff->location = STR_TO_INDEX(val1, apply_types);
      aff->modifier = nVal1;

      if(!ab->affects) {
        ab->affects = aff;
      } else {
        // put in order of occurrence in file
        ab->affects->next = aff;
      }

    }
  }

  CASE("AffDurtn") {
    if(*value == 'F') {
      var = one_argument(value, val2);
      ab->affDurationFormula = strdup(trim_str(var));
    } else if(is_number(value)){
        ab->affDuration = atoi(value);
    } else {
      ERROR_FIELD_VAL("AffDurtn");
    }
  }

  CASE("Level") {
    if((sscanf(value, "%s %d", val1, &nVal1)) < 2) {
      ERROR_FIELD_VAL("Level");
    } else {
      if((nVal2 = STR_TO_INDEX(val1, class_abbrevs)) > -1) {
        ab->minLevels[nVal2] = nVal1;
      }
    }
  }

  CASE("Routines") {
    if(!str_cmp(value, "NOBITS")) {
      ab->routines = 0;
    } else {
      var = one_argument(value, val1);
      while(strlen(val1) > 0) {
        for(i = 0; *routine_types[i] != '\n'; i++) {
          if(!str_cmp(val1, routine_types[i]))
            SET_BIT(ab->routines, (1 << i));
        }
        var = one_argument(var, val1);
      }
    }
  }

  CASE("Targets") {
    if(!str_cmp(value, "NOBITS")) {
      ab->routines = 0;
    } else {
      var = one_argument(value, val1);
      while(strlen(val1) > 0) {
        for(i = 0; *routine_types[i] != '\n'; i++) {
          if(!str_cmp(val1,target_types[i]))
            SET_BIT(ab->targets, (1 << i));
        }
        var = one_argument(var, val1);
      }
    }
  }

  CASE("Flags") {
    if(!str_cmp(value, "NOBITS")) {
      ab->routines = 0;
    } else {
      var = one_argument(value, val1);
      while(strlen(val1) > 0) {
        for(i = 0; *ability_flag_types[i] != '\n'; i++) {
          if(!str_cmp(val1, ability_flag_types[i]))
            SET_BIT_AR(ab->flags, i);
        }
        var = one_argument(var, val1);
      }
    }
  }

  CASE("Manual") {
    if(ab->type == ABILITY_TYPE_SPELL && ab->spellInfo != NULL) {
      for(i = 0; *(manual_spells[i].name) != '\n'; i++) {
        if(!str_cmp(value, manual_spells[i].funcName)) {
          ab->spellInfo->spell = manual_spells[i].spell;
          ab->spellInfo->spellFunc = manual_spells[i].funcName;
          break;
        }
      }
      if(ab->spellInfo == NULL || ab->spellInfo->spellFunc == NULL)
        log("SYSERR: Function '%s' for manual ability '%s' does not exist!", value, ab->name);
    } else if(ab->type == ABILITY_TYPE_SKILL && ab->skillInfo != NULL) {
      for(i = 0; *(manual_skills[i].name) != '\n'; i++) {
        if(!str_cmp(value, manual_skills[i].funcName)) {
          ab->skillInfo->action = manual_skills[i].action;
          ab->skillInfo->skillFunc = manual_skills[i].funcName;
          break;
        }
      }
      if(ab->skillInfo == NULL || ab->skillInfo->skillFunc == NULL)
        log("SYSERR: Function '%s' for manual ability '%s' does not exist!", value, ab->name);
    }
  }

  CASE("Message") {
    // message type
    var = one_argument(value, val1);
    // message to
    if(var != NULL && val1 != NULL)
      var = one_argument(var, val2);
    else
      ERROR_FIELD_VAL("Message");
    // the message is what's remaining of var
    if(var == NULL)
      ERROR_FIELD_VAL("Message");

    nVal1 = STR_TO_INDEX(val1, ability_message_type);
    nVal2 = STR_TO_INDEX(val2, ability_message_to);

    if(nVal1 > -1 && nVal2 > -1 && var != NULL) {
      if(ab->messages[nVal1] == NULL) {
        CREATE(msg, struct msg_type, 1);
        ab->messages[nVal1] = msg;
      }
      else {
        msg = ab->messages[nVal1];
      }

      var = trim_str(var);
      switch(nVal2) {
      case ABILITY_MSG_TO_CHAR:
        msg->attacker_msg = strdup(var);
        break;
      case ABILITY_MSG_TO_VICT:
        msg->victim_msg = strdup(var);
        break;
      case ABILITY_MSG_TO_ROOM:
        msg->room_msg = strdup(var);
        break;
      default:
        ERROR_FIELD_VAL("Message");
      }
    }
  }

  if(!matched) {
    log("SYSERR: Warning: Unknown field '%s' with value '%s' in line %d or skills file.", field, value, curr_line);
  }
}

#undef STR_TO_INDEX
#undef CASE
#undef FIELD_STR_VAL
#undef FIELD_NUM_VAL
#undef ERROR_FIELD_VAL

/*
 * Extracts the field and value in the current line from the skills file.
 * Format in skill file should be <field> : <value>.
 * @param ab the ability
 * @param line the current line
 */
static void parseAbility(struct ability_info_type *ab, char *line) {
  char *ptr;

  if ((ptr = strchr(line, ':')) != NULL) {
    *(ptr++) = '\0';
    while (isspace(*ptr))
      ptr++;
  }

  interpretAbilityLine(ab, trim_str(line), ptr);
}

/*
 * Reads one block of ability definition from the skills file.
 * @param fp the file pointer
 * @param ab the ability
 * @param type the ability type
 */
static void readAbility (FILE *fp, struct ability_info_type *ab, int type) {
  char line[MAX_STRING_LENGTH];

  while(get_line(fp, line)) {
    /* End ability definition move to next */
    if(!str_cmp(line, "End"))
      break;
    curr_line++;
    parseAbility(ab, line);
  }

  add_to_list(ab, abilityList);
}

/*
 * Writes ability messages to the skill file.
 * @param fp the file pointer
 * @param msg the message
 * @param name the message type
 */
static void writeMessage (FILE *fp, struct msg_type *msg, const char *name) {
  char buf[MAX_STRING_LENGTH];
  if(msg) {
    if(msg->attacker_msg) {
      sprintf(buf,"%s %s %s", name, ability_message_to[ABILITY_MSG_TO_CHAR], msg->attacker_msg);
      WRITE_STR_LN(fp,"Message", buf);
    }

    if(msg->victim_msg) {
      sprintf(buf,"%s %s %s", name, ability_message_to[ABILITY_MSG_TO_VICT],msg->victim_msg);
      WRITE_STR_LN(fp,"Message", buf);
    }

    if(msg->room_msg) {
      sprintf(buf,"%s %s %s", name, ability_message_to[ABILITY_MSG_TO_ROOM],msg->room_msg);
      WRITE_STR_LN(fp,"Message", buf);
    }
  } else {
    log("WARNING: NULL '%s' msg passed to writeMessage!", name);
  }
}

/*
 * Writes an ability into the skill file.
 * @param aFile the file pointer
 * @param ability the ability
 */
static void writeAbility (FILE *aFile, struct ability_info_type *ability) {
  char buf[MAX_STRING_LENGTH];
  struct affected_type *af;
  int i;

  fprintf(aFile, "%s\n", ability_types[ability->type]);
  WRITE_NUM_LN(aFile, "Number", ability->number);
  WRITE_STR_LN(aFile, "Name"  , ability->name);
  WRITE_STR_LN(aFile, "MinPos" , position_types[ability->minPosition]);
  WRITE_STR_LN(aFile, "Violent", ability->violent ? "Yes" : "No");

  sprintbit(ability->routines, routine_types, buf, sizeof(buf));
  WRITE_STR_LN(aFile, "Routines", buf);
  sprintbit(ability->targets, target_types, buf, sizeof(buf));
  WRITE_STR_LN(aFile, "Targets", buf);
  sprintbitarray(ability->flags, ability_flag_types, ABF_ARRAY_MAX, buf);
  WRITE_STR_LN(aFile, "Flags", buf);

  if(HAS_ROUTINE(ability, MAG_MANUAL)) {
    if(ability->type == ABILITY_TYPE_SPELL && ability->spellInfo != NULL
        && ability->spellInfo->spellFunc != NULL) {
      WRITE_STR_LN(aFile, "Manual", ability->spellInfo->spellFunc);
    } else if(ability->type == ABILITY_TYPE_SKILL && ability->skillInfo != NULL
        && ability->skillInfo->skillFunc != NULL) {
      WRITE_STR_LN(aFile, "Manual", ability->skillInfo->skillFunc);
    }
  }
  sprintf(buf, "%d %d %d", ability->minCost, ability->maxCost, ability->costChange);
  WRITE_STR_LN(aFile, "Cost", buf);

  if(ability->costFormula)
    WRITE_STR_LN(aFile, "CostExpr", ability->costFormula);


  if(ability->damDice) {
    sprintf(buf, "%d %d %d", ability->damDice->diceSize, ability->damDice->numDice, ability->damDice->damRoll);
    WRITE_STR_LN(aFile, "DamDice", buf);
  }
  if(ability->damDiceFormula)
    WRITE_STR_LN(aFile, "DamExpr", ability->damDiceFormula);

  for(i = 0; i < NUM_ABILITY_MSGS; i++) {
    if(ability->messages[i]) {
      writeMessage(aFile, ability->messages[i], ability_message_type[i]);
    }
  }

  if(ability->affects) {
    for(af = ability->affects; af; af = af->next) {
      if(af->location > 0) {
        sprintf(buf,"%s %d",apply_types[af->location], af->modifier);
        WRITE_STR_LN(aFile, "Affects", buf);
      }
    }
    if(ability->affDurationFormula) {
      sprintf(buf, "F %s", ability->affDurationFormula);
      WRITE_STR_LN(aFile, "AffDurtn", buf);
    } else {
      WRITE_NUM_LN(aFile, "AffDurtn", ability->affDuration);
    }
  }

  //levels
  for(i = 0; i < NUM_CLASSES; i++) {
    sprintf(buf,"%s %d", class_abbrevs[i], ability->minLevels[i]);
    WRITE_STR_LN(aFile, "Level", buf);
  }

  fputs("End\n\n", aFile);
}

/*
 * Creates a spell specific info for the given ability
 * @param ability the ability
 */
static void createAbilitySpell(struct ability_info_type *ability) {
  struct ability_spell_info_type *sp;

  CREATE(sp, struct ability_spell_info_type, 1);
  ability->spellInfo = sp;

}

/*
 * Creates a skill specific infor for the given ability
 * @param abitility the ability
 */
static void createAbilitySkill(struct ability_info_type *ability) {
  struct ability_skill_info_type *sk;
  int i;

  CREATE(sk, struct ability_skill_info_type, 1);

  for(i = 0; i < NUM_SKILL_STUN; i++) {
    sk->stunChar[i] = 0;
    sk->stunVict[i] = 0;
  }
  ability->skillInfo = sk;
}

/*
 * Creates a basic ability with the given num, name and type.
 * @param num the ability number
 * @param name the ability name
 * @param type the ability type
 */
static struct ability_info_type *createAbility(int num, char *name, int type) {
  struct ability_info_type *ability;
  int i;

  CREATE(ability, struct ability_info_type, 1);
  ability->name = name;
  ability->number = num;
  ability->type = type;
  ability->violent = FALSE;
  ability->affDuration = 0;
  ability->maxCost = 0;
  ability->minCost = 0;
  ability->costChange = 0;

  for(i = 0; i < NUM_ABILITY_VALUES; i++) {
    ability->miscValues[i] = NOTHING;
  }

  for(i = 0; i < NUM_CLASSES; i++)
    ability->minLevels[i] = LVL_IMMORT;

  if(ability->type == ABILITY_TYPE_SPELL)
    createAbilitySpell(ability);
  else if(ability->type == ABILITY_TYPE_SKILL)
    createAbilitySkill(ability);

  return ability;
}

/*
 * Displays the ability messages to ch.
 * @param ch to whom the messages will be shown
 * @param msg the message
 * @param messageName the message type
 */
static void displayMessages(struct char_data *ch, struct msg_type *msg, const char *messageName) {
  send_to_char(ch, "\tC%s Messages\tn:\r\n", messageName);
  send_to_char(ch, "  \tcTo Char\tn  : \ty%s\tn\r\n", NOT_NULL(msg->attacker_msg));
  send_to_char(ch, "  \tcTo Target\tn: \ty%s\tn\r\n", NOT_NULL(msg->victim_msg));
  send_to_char(ch, "  \tcTo Room\tn  : \ty%s\tn\r\n", NOT_NULL(msg->room_msg));
}

/*
 * Displays the ability information to ch.
 * @param ch to whom the information will be shown
 * @param ab the ability
 */
static void displayGeneralAbilityInfo(struct char_data *ch, struct ability_info_type *ab) {
  char buf[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];
  int i,len = 0, len2 = 0;
  struct affected_type *aff;
  bool hasMessages = FALSE;

  send_to_char(ch, "----------------------------------------------------------------------\r\n");
  send_to_char(ch, "\tc%-10s\tn: \ty%-10d\tn \tc%-10s\tn: \ty%-15s\tn \tc%-10s\tn: \ty%s\tn\r\n",
      "Number",  ab->number,
      "Name",    ab->name,
      "Type",    ability_types[ab->type]);

  if(!ab->costFormula) {
    send_to_char(ch, "\tc%-10s\tn: \ty%-10d\tn \tc%-10s\tn: \ty%-15d\tn \tc%-10s\tn: \ty%d\tn\r\n",
        "Min. Cost",  ab->minCost,
        "Max. Cost",  ab->maxCost,
        "Cost Delta",ab->costChange);
  } else {
    send_to_char(ch, "\tc%-10s\tn: \tG%s\tn\r\n", "Cost Expr.", ab->costFormula);
  }

  if(HAS_ROUTINE(ab, MAG_DAMAGE)) {
    if(!ab->damDiceFormula) {
      if(ab->damDice) {
        sprintf(buf,"%dd%d+%d", ab->damDice->diceSize, ab->damDice->numDice, ab->damDice->damRoll);
        send_to_char(ch, "\tc%-10s\tn: \ty%-10d\tn \tc%-10s\tn: \ty%-15s\tn \tc%-10s\tn: %s\r\n",
            "Avg. Dam.", (((ab->damDice->diceSize + 1) / 2) * ab->damDice->numDice) + ab->damDice->damRoll,
            "Dam. Dice",   buf,
            "Violent", (ab->violent ? "\tRYes\tn" : "\tDNo\tn"));
      } else {
        send_to_char(ch, "\tc%-10s\tn: \ty%-10s\tn \tc%-10s\tn: \ty%-15s\tn \tc%-10s\tn: %s \r\n",
            "Avg. Dam.", "N/A",
            "Dam. Dice", "N/A",
            "Violent", (ab->violent ? "\tRYes\tn" : "\tDNo\tn"));
      }
    } else {
      send_to_char(ch, "\tc%-10s\tn: \tG%-38s\tn \tc%-10s\tn: \ty%s\tn\r\n",
          "Dam. Expr.", ab->damDiceFormula,
          "Violent",    (ab->violent ? "\tRYes\tn" : "\tDNo\tn"));
    }
  }

  sprintbitarray(ab->flags, ability_flag_types, ABF_ARRAY_MAX, buf);
  send_to_char(ch, "\tc%-10s\tn: \ty%s\tn\r\n", "Flags", buf);
  sprintbit(ab->routines, routine_types, buf, sizeof(buf));
  send_to_char(ch, "\tc%-10s\tn: \ty%s\tn\r\n", "Routines", buf);
  sprintbit(ab->targets, target_types, buf, sizeof(buf));
  send_to_char(ch, "\tc%-10s\tn: \ty%s\tn\r\n", "Targets", buf);

  for(i = 0; i < NUM_CLASSES; i++) {
    len += sprintf(buf + len,  "%-3s", class_abbrevs[i]);
    len2 += sprintf(buf2 + len2, "%s%-3d\tn",
        ab->minLevels[i] < LVL_IMMORT ? "\tG" : "\tg",
        ab->minLevels[i]);
  }
  send_to_char(ch,"\tc%-10s\tn: \ty%s\tn\r\n", "Levels", buf);
  send_to_char(ch,"%-10s: %s\r\n", " ",buf2);

  if(HAS_ROUTINE(ab, MAG_MANUAL)) {
    //lets try to print the function address
    if(ab->type == ABILITY_TYPE_SPELL && ab->spellInfo != NULL
        && ab->spellInfo->spellFunc != NULL) {
      sprintf(buf, "\tc%-10s\tn: \tC%s\tn @\tY%p\tn","Function",ab->spellInfo->spellFunc, ab->spellInfo->spell);
      send_to_char(ch,"%s\r\n", buf);
    } else if (ab->type == ABILITY_TYPE_SKILL && ab->skillInfo != NULL
        && ab->skillInfo->skillFunc != NULL) {
      sprintf(buf, "\tc%-10s\tn: \tC%s\tn @\tY%p\tn","Function",ab->skillInfo->skillFunc, ab->skillInfo->action);
      send_to_char(ch,"%s\r\n", buf);
    }
  }

  if(ab->affects) {
    i = 1;
    send_to_char(ch, "----------------------------------------------------------------------\r\n");
    if(ab->affDurationFormula) {
      send_to_char(ch, "\tcAffects the following for the evaluated duration expression\tn '\tG%s\tn':\r\n", ab->affDurationFormula);
    } else {
      send_to_char(ch, "\tcAffects the following for\tn \ty%d\tn\tchrs\tn:\r\n", ab->affDuration);
    }
    for(aff = ab->affects; aff; aff = aff->next) {
      if(aff->location != APPLY_NONE) {
        send_to_char(ch, "   \tg%d\tn) \tcmodifies\tn \ty%-15s\tn \tcby\tn \tC%d\tn\r\n", i++, apply_types[aff->location], aff->modifier);
      }
    }
  }

  for(i = 0; i < NUM_ABILITY_MSGS; i++) if(ab->messages[i]) hasMessages = TRUE;

  if(hasMessages) {
    send_to_char(ch, "----------------------------------------------------------------------\r\n");
    for(i = 0; i < NUM_ABILITY_MSGS; i++) {
      if(ab->messages[i])
        displayMessages(ch, ab->messages[i], ability_message_type[i]);
    }
  }
  send_to_char(ch, "----------------------------------------------------------------------\r\n");

}

/*
 * Macro for printing the command syntax to ch
 */
#define PRINT_SYNTAX(ch)                                      \
do {                                                          \
 send_to_char(ch, "Syntax: ABEDIT <option/ability number or name>\r\n\r\n"); \
 send_to_char(ch, "Options:\r\n");                            \
 send_to_char(ch, " save                       - save all abilities to file\r\n");  \
 send_to_char(ch, " stat <ability number/name> - display the ability information\r\n");  \
 send_to_char(ch, " list                       - list all abilities and their basic information\r\n");  \
 send_to_char(ch, " new                        - create a new ability\r\n");  \
} while (0)

/*
 * ABEDIT subcommand. Lists the abilities defined.
 */
static ACMD(do_abedit_list) {
  struct ability_info_type *ab;
  char buf[MAX_STRING_LENGTH];
  int len = 0, nlen = 0;

  len = sprintf(buf, "\tc%-7s %-25s %-10s %s\tr\r\n","Number", "Name", "Type", "Min. Position");
  len += sprintf(buf + len, "\tW============================================================\r\n");
  if(abilityList && abilityList->iSize > 0) {
    while((ab = (struct ability_info_type *) simple_list(abilityList))) {
      len += snprintf(buf + len, sizeof(buf) - len, "\tg%-7d\tn \ty%-25s\tn %-10s %s\r\n",
          ab->number, ab->name, ability_types[ab->type], position_types[ab->minPosition]);
      if(nlen + len >= sizeof(buf)) {
        page_string(ch->desc, buf, TRUE);
        nlen = 0;
        len = 0;
      }
      nlen += len;
    }
    len += sprintf(buf + len, "\tW============================================================\r\n");
    nlen = snprintf(buf + len, sizeof(buf) - len, "\tC%d abilities listed.\r\n", abilityList->iSize);
    page_string(ch->desc, buf, TRUE);
  }

}

/*
 * ABEDIT subcommand. Displays information of a given ability.
 */
static ACMD(do_abedit_stat) {
  struct ability_info_type *ab;
  skip_spaces(&argument);

  if(*argument) {
    if(is_number(argument))
      ab = getAbility(atoi(argument));
    else
      ab = getAbilityByName(argument);

    if(ab) {
      displayGeneralAbilityInfo(ch, ab);
    } else {
      send_to_char(ch, "That ability name or number does not exist.\r\n");
    }
  } else {
    PRINT_SYNTAX(ch);
  }
}

/*
 * ABEDIT command implementation.
 */
ACMD(do_abedit) {
  char arg1[MAX_INPUT_LENGTH], *args;
  int count = 0, abNum = -1;
  struct ability_info_type *ab = NULL;
  struct descriptor_data *d;
  bool edit = FALSE, canEdit = TRUE, new = FALSE;

  if(!IS_NPC(ch) && ch->desc && STATE(ch->desc) == CON_PLAYING) {
    if(subcmd == 0) {
      if(*argument) {
        args = one_argument(argument, arg1);
        if(!str_cmp(arg1, "save")) {
          mudlog(BRF, LVL_IMMORT, TRUE, "WARN: do_abedit: %s saving all abilities", GET_NAME(ch));
          count = saveAbilities();
          mudlog(BRF, LVL_IMMORT, TRUE, "WARN: do_abedit: %s saved %d abilities to file.", GET_NAME(ch), count);
          send_to_char(ch, "Saved %d abilities to file.\r\n", count);
        } else if(!str_cmp(arg1, "stat")) {
          do_abedit_stat(ch, args, cmd, subcmd);
        } else if(!str_cmp(arg1, "list")) {
          do_abedit_list(ch, args, cmd, subcmd);
        } else if(!str_cmp(arg1, "new")) {
          new = TRUE;
        } else if(is_number(arg1)) {
          abNum = atoi(arg1);
          edit = TRUE;
        } else {
          edit = TRUE;
        }

        if(new) {
          //lets find an available number
          for(abNum = 1; getAbility(abNum) != NULL; abNum++);
          ab = createAbility(abNum, "new ability", ABILITY_TYPE_SPELL);

        } else if(edit) {
          for(d = descriptor_list; d; d = d->next) {
            if(STATE(d) == CON_ABEDIT && OLC_ABILITY(d) && OLC_NUM(d) == abNum) {
              send_to_char(ch,"Someone is already editing that ability.\r\n");
              canEdit = FALSE;
            }
          }
          if(canEdit) {
            if((abNum > -1 && (ab = getAbility(abNum)) != NULL) || ((ab = getAbilityByName(trim_str(argument))) != NULL)) {

            } else {
              send_to_char(ch, "That ability name or number does not exist.\r\n");
            }
          }
        }

        if((new || canEdit) && ab != NULL) {
          d = ch->desc;

          if(d->olc != NULL) {
            mudlog(BRF, LVL_IMMORT, TRUE, "SYSERR: do_abedit: Player already had olc structure.");
            free(d->olc);
          }

          CREATE(d->olc, struct oasis_olc_data, 1);
          OLC_NUM(d) = abNum;
          OLC_VAL(d) = 0;

          OLC_ABILITY(d) = ab;
          abeditDisplayMainMenu(d);
          STATE(d) = CON_ABEDIT;

          act("$n starts using OLC.", TRUE, ch, NULL, NULL, TO_ROOM);
          SET_BIT_AR(PLR_FLAGS(ch), PLR_WRITING);

          mudlog(CMP, LVL_IMMORT, TRUE, "OLC: %s starts editing skill (%d):%s",
              GET_NAME(ch), ab->number, ab->name);
        }

      } else {
        PRINT_SYNTAX(ch);
      }
    } else {
      switch(subcmd) {
      case SCMD_ABEDIT_STAT:
        do_abedit_stat(ch, argument, cmd, subcmd);
        break;
      case SCMD_ABEDIT_LIST:
        do_abedit_list(ch, argument, cmd, subcmd);
      }
    }
  }

  return;
}
#undef PRINT_SYNTAX

/********************** OLC local functions ***********************************/
static void abeditDisplayValuesMenu(struct descriptor_data *d) {
  struct ability_info_type *ab = OLC_ABILITY(d);
  int i;

  if(HAS_ROUTINE(ab, MAG_SUMMONS)) {
    //mob vnums for conjuring spells?
    write_to_output(d, "%sConjured Mobiles' VNUM%s:\r\n", cyn, nrm);
    for(i = 0; i < NUM_ABILITY_VALUES; i++) {
      write_to_output(d, "%s%d%s) [%s%d%s] %s\r\n",
          grn, (i+1), nrm,
          cyn,  ab->miscValues[i], nrm,
          real_mobile(ab->miscValues[i]) == NOBODY ? "Invalid Mob" : mob_proto[real_mobile(ab->miscValues[i])].player.short_descr);
    }
  } else if(HAS_ROUTINE(ab, MAG_CREATIONS)){
    //for portal, create warding, prepare camp, etc..
    write_to_output(d, "%sObjects' VNUM%s:\n\r", cyn, nrm);
    for(i = 0; i < NUM_ABILITY_VALUES; i++) {
      write_to_output(d, "%s%d%s) [%s%d%s] %s\r\n", grn, (i+1), nrm,
          cyn, ab->miscValues[i], nrm,
          real_object(ab->miscValues[i]) == NOTHING ? "Invalid Object" : obj_proto[real_object(ab->miscValues[i])].short_description);
    }
  }
  write_to_output(d, "Enter selection (0 to quit) :\r\n");
  OLC_MODE(d) = ABEDIT_VALUES;
}

#define LIST_MANUAL(list) \
    do { \
      for(i = 0; *list[i].name != '\n'; i++) { \
        write_to_output(d, "%s%d%s) %-15.15s ", grn, (i + 1), nrm, list[i].funcName);\
        if(!((i+1) % 3)) write_to_output(d, "\r\n");\
      }\
    } while (0)

static void abeditDisplayManualFuncs(struct descriptor_data *d) {
  int i;
  struct ability_info_type *ab = OLC_ABILITY(d);

  if(IS_SPELL(ab)) {
    LIST_MANUAL(manual_spells);
  } else if (IS_SKILL(ab)) {
    LIST_MANUAL(manual_skills);
  }

  write_to_output(d, "\r\nEnter selection (0 - quit):\r\n");
  OLC_MODE(d) = ABEDIT_FUNCTION;
}

#undef LIST_MANUAL

static void abeditDisplayTypeSpecMenu(struct descriptor_data *d) {
  struct ability_info_type *ab = OLC_ABILITY(d);
  int i = 0;

  if(IS_SPELL(ab)) {
    if(IS_MANUAL(ab)) {
      write_to_output(d, "%sF%s) Function: %s%s%s %s@%p%s\r\n",
          grn, nrm, yel, ab->spellInfo->spellFunc, nrm,
          cyn, ab->spellInfo->spell, nrm);
    }
  } else if(IS_SKILL(ab)) {
    if(IS_MANUAL(ab)) {
      write_to_output(d, "%sF%s) Function: %s%s%s %s@%p%s\r\n",
          grn, nrm, yel, ab->skillInfo->skillFunc, nrm,
          cyn, ab->skillInfo->action, nrm);
    }
    for(i = 0; *skill_stuns[i] != '\n'; i++) {
      write_to_output(d, "%s%d%s) Stun %s\r\n",
          grn, (i+1), nrm, skill_stuns[i]);
      write_to_output(d, "    %s%s%s: %s%d%s\r\n",
          yel, skill_stun_to[SKILL_STUN_TO_CHAR], nrm,
          cyn, ab->skillInfo->stunChar[i], nrm);
      write_to_output(d, "    %s%s%s: %s%d%s\r\n",
          yel, skill_stun_to[SKILL_STUN_TO_VICT], nrm,
          cyn, ab->skillInfo->stunVict[i], nrm);
    }
  }

  write_to_output(d, "%sQ%s) Return to Main Menu\r\n", grn, nrm);
  write_to_output(d, "Enter selection or number :\r\n");
  OLC_MODE(d) = ABEDIT_TYPE_SPEC;
}

static void abeditDisplayMessages(struct descriptor_data *d) {
  struct ability_info_type *ab = OLC_ABILITY(d);

  write_to_output(d, "%s%s%s Messages:\r\n", cyn, ability_message_type[currVal1], nrm);
  write_to_output(d, "%s1%s) %s%s%s: %s%s%s\r\n",
      grn, nrm, cyn, ability_message_to[ABILITY_MSG_TO_CHAR], nrm,
      yel, NOT_NULL(ab->messages[currVal1]->attacker_msg), nrm);
  write_to_output(d, "%s2%s) %s%s%s: %s%s%s\r\n",
      grn, nrm, cyn, ability_message_to[ABILITY_MSG_TO_VICT], nrm,
      yel, NOT_NULL(ab->messages[currVal1]->victim_msg), nrm);
  write_to_output(d, "%s3%s) %s%s%s: %s%s%s\r\n",
      grn, nrm, cyn, ability_message_to[ABILITY_MSG_TO_ROOM], nrm,
      yel, NOT_NULL(ab->messages[currVal1]->room_msg), nrm);
  write_to_output(d, "Enter message to number (0 to quit) :\r\n");
  OLC_MODE(d) = ABEDIT_MESSAGE_EDIT;
}

static void abeditDisplayMessageNew(struct descriptor_data *d) {
  struct ability_info_type *ab = OLC_ABILITY(d);
  int i;

  //display the empty slots only
  clear_screen(d);
  write_to_output(d, "%sAvailable message slot%s :\r\n", grn, nrm);
  for(i = 0; i < NUM_ABILITY_MSGS; i++) {
    if(ab->messages[i] == NULL)
      write_to_output(d, "%s%d%s) %s%s%s\r\n", grn, (i + 1), nrm, cyn, ability_message_type[i], nrm);
  }
  write_to_output(d,"\r\nEnter message number (0 to quit) :\r\n");
  OLC_MODE(d) = ABEDIT_MESSAGE_NEW;
}

static void abeditDisplayMessageMenu(struct descriptor_data *d) {
  struct ability_info_type *ab = OLC_ABILITY(d);
  char buf[MAX_STRING_LENGTH], *msg = '\0';
  bool hasMessage = FALSE;
  int i, j, len = 0;

  get_char_colors(d->character);
  for(i = 0; i < NUM_ABILITY_MSGS; i++) {
    if(ab->messages[i] != NULL) {
      hasMessage = TRUE;

      len += sprintf(buf + len, "%s%d%s) %s%-10s Messages%s:\r\n",
          grn, (i+1), nrm, cyn, ability_message_type[i], nrm);
      for(j = 0; j < NUM_ABILITY_MSG_TO; j++) {
        len += sprintf(buf + len, "     %s%6s%s: ",
            grn, ability_message_to[j], nrm);

        if(j == ABILITY_MSG_TO_CHAR) {
          msg = ab->messages[i]->attacker_msg;
          len += sprintf(buf + len, "%s%s%s\r\n", yel, NOT_NULL(msg), nrm);
        } else if(j == ABILITY_MSG_TO_VICT) {
          msg = ab->messages[i]->victim_msg;
          len += sprintf(buf + len, "%s%s%s\r\n", yel, NOT_NULL(msg), nrm);
        } else if(j == ABILITY_MSG_TO_ROOM) {
          msg = ab->messages[i]->room_msg;
          len += sprintf(buf + len, "%s%s%s\r\n", yel, NOT_NULL(msg), nrm);
        }
      }
    }
  }

  if(!hasMessage) {
    write_to_output(d, "%sNo messages defined, create%s (%sN%s)%sew!%s\r\n",yel, nrm, grn, nrm, yel, nrm);
  } else {
    write_to_output(d, "%s\r\n", buf);
  }
  write_to_output(d, "(%sN%s) New Message\r\n", grn, nrm);
  write_to_output(d, "(%sX%s) Delete\r\n", grn, nrm);
  write_to_output(d, "(%sQ%s) Return to Main Menu\r\n", grn, nrm);
  write_to_output(d, "Enter options or number to edit :\r\n");
  OLC_MODE(d) = ABEDIT_MESSAGE;
}

static void abeditDisplayAffApplies(struct descriptor_data *d) {
  int i, count = 0;
  const char *allowed_applies[NUM_APPLIES];

  clear_screen(d);

  for (i = 0; i < NUM_APPLIES; i++) {
    if (is_illegal_flag(i, NUM_ILLEGAL_APPLIES, illegal_applies)) continue;
    allowed_applies[count++] = strdup(apply_types[i]);
  }
  column_list(d->character, 4, allowed_applies, count, TRUE);
  write_to_output(d, "Enter apply location (0 no apply) :\r\n");
  OLC_MODE(d) = ABEDIT_AFF_APPLY;
}

static void abeditDisplayAffDurMenu(struct descriptor_data *d) {
  clear_screen(d);
  write_to_output(d,"%sNOTE: 'Duration Expression' will always be used if it has a value. If the\r\n", cyn);
  write_to_output(d,"expression is un-parsable it will have a value of 1.%s\r\n", nrm);
  write_to_output(d, "%s1%s) Duration Expression\r\n", grn, nrm);
  write_to_output(d, "%s2%s) Standard Duration Value\r\n", grn, nrm);
  write_to_output(d, "%sQ%s) Return To Affects Menu\r\n", grn, nrm);
  write_to_output(d, "Enter Option :\r\n");
  OLC_MODE(d) = ABEDIT_AFF_DUR;
}

static void abeditDisplayLevels(struct descriptor_data *d) {
  char buf[MAX_STRING_LENGTH];
  int i, len = 0;
  struct ability_info_type *ab = OLC_ABILITY(d);

  for(i = 0; i < NUM_CLASSES; i++) {
    len += sprintf(buf + len, "%s%2d%s) %s%2s%s - %s%-3d%s ",
        grn, (i + 1), nrm,
        yel, class_abbrevs[i], nrm,
        cyn, ab->minLevels[i], nrm);
    if(!((i+1) % 3))
      len += sprintf(buf + len, "\r\n");
  }
  write_to_output(d, "%s\r\n", buf);
  write_to_output(d, "%sQ%s) Return to Main Menu\r\n", grn, nrm);
  write_to_output(d, "Enter Option (class level) :\r\n");
  OLC_MODE(d) = ABEDIT_LEVELS;
}

static void abeditDisplayTargets(struct descriptor_data *d) {
  char buf[MAX_STRING_LENGTH];
  struct ability_info_type *ab = OLC_ABILITY(d);

  sprintbit(ab->targets, target_types, buf, sizeof(buf));
  column_list(d->character, 2, target_types, NUM_ABI_TARGETS, TRUE);
  write_to_output(d, "Current targets: %s%s%s\r\n", cyn, buf, nrm);
  write_to_output(d, "Enter Option (0 to quit) :\r\n");
  OLC_MODE(d) = ABEDIT_TARGETS;
}

static void abeditDisplayRoutines(struct descriptor_data *d) {
  char buf[MAX_STRING_LENGTH];
  struct ability_info_type *ab = OLC_ABILITY(d);

  sprintbit(ab->routines, routine_types, buf, sizeof(buf));
  column_list(d->character, 2, routine_types, NUM_ABI_ROUTINES, TRUE);
  write_to_output(d, "Current routines: %s%s%s\r\n", cyn, buf, nrm);
  write_to_output(d, "Enter Option (0 to quit) :\r\n");
  OLC_MODE(d) = ABEDIT_ROUTINES;
}

static void abeditDisplayFlags(struct descriptor_data *d) {
  char buf[MAX_STRING_LENGTH];
  struct ability_info_type *ab = OLC_ABILITY(d);

  sprintbitarray(ab->flags, ability_flag_types, ABF_ARRAY_MAX, buf);
  column_list(d->character, 2, ability_flag_types, NUM_ABILITY_FLAGS, TRUE);
  write_to_output(d, "Current flags: %s%s%s\r\n", cyn, buf, nrm);
  write_to_output(d, "Enter Option (0 to quit) :\r\n");
  OLC_MODE(d) = ABEDIT_FLAGS;
}

static void abeditDisplayDamMenu(struct descriptor_data *d) {
  write_to_output(d,"%sNOTE: 'Damage Expression' will always be used if it has a value. If the\r\n", cyn);
  write_to_output(d,"expression is un-parsable it will have a value of 1.%s\r\n", nrm);
  write_to_output(d, "%s1%s) Damage Expression\r\n", grn, nrm);
  write_to_output(d, "%s2%s) Damage Dice Values\r\n", grn, nrm);
  write_to_output(d, "%sQ%s) Return To Main Menu\r\n", grn, nrm);
  write_to_output(d, "Enter Option :\r\n");
  OLC_MODE(d) = ABEDIT_DAM;
}

static void abeditDisplayCostMenu(struct descriptor_data *d) {
  write_to_output(d,"%sNOTE: 'Cost Expression' will always be used if it has a value. If the\r\n", cyn);
  write_to_output(d,"expression is un-parsable it will have a value of 1.%s\r\n", nrm);
  write_to_output(d, "%s1%s) Cost Expression\r\n", grn, nrm);
  write_to_output(d, "%s2%s) Standard Cost Value\r\n", grn, nrm);
  write_to_output(d, "%sQ%s) Return To Main Menu\r\n", grn, nrm);
  write_to_output(d, "Enter Option :\r\n");
  OLC_MODE(d) = ABEDIT_COST;
}

static void abeditDisplayPositions(struct descriptor_data *d) {
  column_list(d->character, 0, position_types, NUM_POSITIONS, TRUE);
  write_to_output(d, "Position (0 to quit):\r\n");
  OLC_MODE(d) = ABEDIT_MINPOS;
}

static void abeditDisplayTypes(struct descriptor_data *d) {
  column_list(d->character, 0, ability_types, NUM_ABILITY_TYPES, TRUE);
  write_to_output(d, "Type (0 to quit) :\r\n");
  OLC_MODE(d) = ABEDIT_TYPE;
}

static void abeditDisplayAffMenu(struct descriptor_data *d) {
  char buf[MAX_STRING_LENGTH];
  int len = 0, i = 0;
  struct affected_type *aff;
  struct ability_info_type *ab = OLC_ABILITY(d);

  if(ab->affects != NULL) {
    for(aff = ab->affects; aff; aff = aff->next) {
      len += sprintf(buf + len, "  %s%2d%s) modifies %s%-15s%s by %s%-3d%s\r\n",
          grn, ++i, nrm, cyn, apply_types[aff->location], nrm, yel, aff->modifier, nrm);
    }
    write_to_output(d, "Applies the following:\r\n%s\r\n", buf);
    if(ab->affDurationFormula) {
      sprintf(buf, "%sD%s) Duration expression: %s%s%s",
          grn, nrm, yel, ab->affDurationFormula, nrm);
    } else {
      sprintf(buf, "%sD%s) Duration: %s%d%s", grn, nrm, yel, ab->affDuration, nrm);
    }
    write_to_output(d, "%s\r\n", buf);

  } else {
    write_to_output(d, "%sNo affects defined, create (%sN%s)ew!.%s\r\n", yel, grn, yel, nrm);
  }
  write_to_output(d, "%sN%s) New affect\r\n", grn, nrm);
  if(ab->affects != NULL)
    write_to_output(d, "%sX%s) Delete affect\r\n", grn, nrm);
  write_to_output(d, "%s?%s) Help\r\n", grn, nrm);
  write_to_output(d, "%sQ%s) Return to Main Menu\r\n", grn, nrm);
  write_to_output(d, "Enter Selection or the affect number to edit :\r\n");

  OLC_MODE(d) = ABEDIT_AFF_MENU;
}

static void abeditDisplayMainMenu(struct descriptor_data *d) {
  char buf[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];
  int len = 0, len2 = 0, i;
  struct ability_info_type *ab = OLC_ABILITY(d);


  get_char_colors(d->character);
  write_to_output(d, "%s1%s) %-10s: [%s%3d%s] %s%-15s%s %s2%s) %-10s: %s%s%s\r\n",
      grn, nrm, "Name", cyn, ab->number, nrm, yel, ab->name, nrm,
      grn, nrm, "Type", yel, ability_types[ab->type], nrm);

  write_to_output(d, "%s3%s) %-10s: %s%-21s%s %s4%s) %-10s: %s%-3s%s\r\n",
      grn, nrm, "Min. Pos.", yel, position_types[ab->minPosition], nrm,
      grn, nrm, "Violent", yel, (ab->violent ? "Yes" : "No"), nrm);

  if(ab->costFormula != NULL) {
    sprintf(buf, "%s%-5s%s [%s%-15s%s]",
        grn,"Expr", nrm, cyn, ab->costFormula, nrm);
  } else {
    sprintf(buf, "%s%s%s[%s%3d%s] %s%s%s[%s%3d%s] %s%s%s[%s%3d%s]",
        grn, "Min", nrm, cyn, ab->minCost, nrm,
        grn, "Max", nrm, cyn, ab->maxCost, nrm,
        grn, "Chg", nrm, cyn, ab->costChange, nrm);
  }

  write_to_output(d, "%s5%s) %-10s: %s\r\n",
        grn, nrm, "Cost", buf);

  if(IS_SET(ab->routines, MAG_DAMAGE)) {

    if(ab->damDiceFormula == NULL) {
      if(ab->damDice != NULL) {
        sprintf(buf, "%s%dd%d+%d%s  %s%10s%s[%s%-3d%s]",
            yel, ab->damDice->diceSize, ab->damDice->numDice, ab->damDice->damRoll, nrm,
            grn, "Avg", nrm,
            cyn,
            ((((ab->damDice->diceSize + 1) / 2) * ab->damDice->numDice + ab->damDice->damRoll)),
            nrm);
      } else {
        sprintf(buf, "%s%dd%d+%d%s  %s%10s%s[%s%-3d%s]",
            yel, 0, 0, 0, nrm,
            grn, "Avg", nrm,
            cyn,0, nrm);
      }
    } else {
      sprintf(buf, "%s%-5s%s [%s%-15s%s]",
          grn,"Expr", nrm, cyn, ab->damDiceFormula, nrm);
    }
    write_to_output(d, "%s6%s) %-10s: %s\r\n",
        grn, nrm, "Damage", buf);
  }

  sprintbitarray(ab->flags, ability_flag_types, NUM_ABILITY_FLAGS, buf);
  write_to_output(d, "%sF%s) %-10s: %s%s%s\r\n",
      grn, nrm, "Flags", yel, buf, nrm);
  sprintbit(ab->routines, routine_types, buf, sizeof(buf));
  write_to_output(d, "%sR%s) %-10s: %s%s%s\r\n",
      grn, nrm, "Routines", yel, buf, nrm);
  sprintbit(ab->targets, target_types, buf, sizeof(buf));
  write_to_output(d, "%sT%s) %-10s: %s%s%s\r\n",
      grn, nrm, "Targets", yel, buf, nrm);

  for(i = 0; i < NUM_CLASSES; i++) {
    len += sprintf(buf + len, "%-3s", class_abbrevs[i]);
    len2 += sprintf(buf2 + len2, "%3d", ab->minLevels[i]);
  }
  write_to_output(d, "%sL%s) %-10s: %s%s%s\r\n",
      grn, nrm, "Levels", yel, buf, nrm);
  write_to_output(d, "%-13s %s%s%s\r\n", " ", cyn, buf2, nrm);

  write_to_output(d, "%sA%s) Affects Menu\r\n", grn, nrm);
  write_to_output(d, "%sM%s) Messages Menu\r\n", grn, nrm);
  write_to_output(d, "%sS%s) %s Specific Menu\r\n", grn, nrm, ability_types[ab->type]);
  if(HAS_ROUTINE(ab, MAG_SUMMONS) || HAS_ROUTINE(ab, MAG_CREATIONS)) {
    write_to_output(d, "%sV%s) Other Values\r\n", grn, nrm);
  }

  if(getAbility(ab->number) != NULL) {
    write_to_output(d, "%sX%s) Delete\r\n", grn, nrm);
  }
  write_to_output(d, "%s?%s) Help\r\n", grn, nrm);
  write_to_output(d, "%sQ%s) Quit\r\nEnter Selection :", grn, nrm);

  OLC_MODE(d) = ABEDIT_MAIN_MENU;
}
