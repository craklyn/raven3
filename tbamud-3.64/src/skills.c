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

#define __SKILLS_C__

#define WRITE_STR_LN(fp, field, value) (fprintf(fp,"%-10s: %s\n", field, value))
#define WRITE_NUM_LN(fp, field, value) (fprintf(fp,"%-10s: %d\n", field, value))
#define WRITE_CHA_LN(fp, field, value) (fprintf(fp,"%-10s: %c\n", field, value))
#define NOT_NULL(str) (str ? str : "None.")

struct list_data *abilityList;

static int curr_line;

/* local functions */
static bool equipmentSkillSuccess (struct char_data *ch);
static bool affectSkillSuccess    (struct char_data *ch);
static void writeSkill            (FILE *skillsFile, int skillNum);
static struct ability_info_type * createAbility(int num, char *name, int type);
static void createAbilitySpell(struct ability_info_type *ability);
static void createAbilitySkill(struct ability_info_type *ability);
static void writeAbility (FILE *abisFile, struct ability_info_type *ability);
static void writeMessage (FILE *fp, struct msg_type *msg, const char *name);
static void readAbility (FILE *fp, struct ability_info_type *ab, int type);
static void parseAbility(struct ability_info_type *ab, char *line);
static void interpretAbilityLine(struct ability_info_type *ab, char *line, char *value);
static void displayGeneralAbilityInfo(struct char_data *ch, struct ability_info_type *ab);
static void displayMessages(struct char_data *ch, struct msg_type *msg, const char *messageName);
static int getIndexOf(char *str, const char **arr);

static ACMD(do_skedit_stat);
static ACMD(do_skedit_list);

static struct manual_spell_info {
  int spellNum;
  char *name;
  char *funcName;
  ASPELL(*spell);
} manual_spells [] ={
    {SPELL_CHARM,"charm person","spell_charm", spell_charm},
    {SPELL_IDENTIFY,"identify", "spell_identify", spell_identify},
    {SPELL_CREATE_WATER,"create water", "spell_create_water", spell_create_water},
    {SPELL_WORD_OF_RECALL,"recall", "spell_recall", spell_recall},
    {SPELL_SUMMON,"summon", "spell_summon", spell_summon},
    {SPELL_LOCATE_OBJECT,"locate object", "spell_locate_object", spell_locate_object},
    {SPELL_ENCHANT_WEAPON,"enchant weapon", "spell_enchant_weapon" ,spell_enchant_weapon},
    {SPELL_DETECT_POISON,"detect poison", "spell_detect_poison", spell_detect_poison},
    {0,"none", "spell_none", spell_none} /* always last*/
};

static struct manual_skill_info {
  int skillNum;
  char *name;
  char *funcName;
  ACMD(*action);
} manual_skills[] ={
    {SKILL_BACKSTAB, "backstab", "do_backstab", do_backstab},
    {SKILL_BASH, "bash", "do_bash", do_bash},
    {SKILL_HIDE, "hide", "do_hide", do_hide},
    {SKILL_KICK, "kick", "do_kick", do_kick},
    {SKILL_PICK_LOCK, "pick lock", "do_gen_door", do_gen_door}, /* SCMD_PICK */
    {SKILL_RESCUE, "rescue", "do_rescue", do_rescue},
    {SKILL_SNEAK, "sneak", "do_sneak", do_sneak},
    {SKILL_STEAL, "steal", "do_steal", do_steal},
    {SKILL_TRACK, "track", "do_track", do_track},
    {SKILL_WHIRLWIND, "whirlwind", "do_whirlwind", do_whirlwind},
    {0, "none", "do_not_here", do_not_here},
};

static const char *routine_types[NUM_ABI_ROUTINES + 1] = {
    "Damages",
    "Affects",
    "Unaffects",
    "Points",
    "AlterObj",
    "Groups",
    "Masses",
    "Areas",
    "Summons",
    "Creations",
    "Manual",
    "Rooms",
    "\n"
};

static const char *target_types[NUM_ABI_TARGETS + 1] = {
    "Ignore",
    "CharInRoom",
    "CharInWorld",
    "FightSelf",
    "FightVict",
    "SelfOnly",
    "NotSelf",
    "ObjInInv",
    "ObjInRoom",
    "ObjInWorld",
    "ObjEquiped",
    "\n"
};

static const char *ability_flag_types[NUM_AB_FLAGS + 1] = {
    "AccumDuration",
    "AccumApply",
    "CostsVigor",
    "AntiGood",
    "AntiNeutral",
    "AntiEvil",
    "RequireObj",
    "\n"
};

static const char *ability_message_type[NUM_AB_MSGS + 1] = {
    "Success",
    "Fail",
    "WearOff",
    "GodVict",
    "Death",
    "\n"
};

static const char *ability_message_to[NUM_AB_MSG_TO + 1] = {
    "ToChar", "ToVict", "ToRoom", "\n"
};

static const char *ability_types[] = {
    "Spell","Skill","\n"
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

void loadAbilities (void) {
  struct ability_info_type *ab, *manual;
  int type = 0, i;
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

  log("Assigning spells functions to manual spells.");
  for(i = 0; manual_spells[i].spellNum > 0;i++) {
    manual = getAbility(manual_spells[i].spellNum);

    if(manual != NULL) {
      manual->spellInfo->spell = manual_spells[i].spell;
      manual->spellInfo->spellFunc = manual_spells[i].funcName;
      SET_BIT(manual->routines, MAG_MANUAL);
    }
  }

  for(i = 0; manual_skills[i].skillNum > 0; i++) {
    manual = getAbility(manual_skills[i].skillNum);

    if(manual != NULL) {
      manual->skillInfo->action = manual_skills[i].action;
      manual->skillInfo->skillFunc = manual_skills[i].funcName;
      SET_BIT(manual->routines, MAG_MANUAL);
    }
  }

  log("Loaded %d abilities of %ld bytes.", abilityList->iSize, sizeof(struct ability_info_type) * abilityList->iSize);

}

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
      case AB_MSG_TO_CHAR:
        msg->attacker_msg = strdup(var);
        break;
      case AB_MSG_TO_VICT:
        msg->victim_msg = strdup(var);
        break;
      case AB_MSG_TO_ROOM:
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

static void parseAbility(struct ability_info_type *ab, char *line) {
  char *ptr;

  if ((ptr = strchr(line, ':')) != NULL) {
    *(ptr++) = '\0';
    while (isspace(*ptr))
      ptr++;
  }

  interpretAbilityLine(ab, trim_str(line), ptr);
}

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

static void writeMessage (FILE *fp, struct msg_type *msg, const char *name) {
  char buf[MAX_STRING_LENGTH];
  if(msg) {
    if(msg->attacker_msg) {
      sprintf(buf,"%s %s %s", name, ability_message_to[AB_MSG_TO_CHAR], msg->attacker_msg);
      WRITE_STR_LN(fp,"Message", buf);
    }

    if(msg->victim_msg) {
      sprintf(buf,"%s %s %s", name, ability_message_to[AB_MSG_TO_VICT],msg->victim_msg);
      WRITE_STR_LN(fp,"Message", buf);
    }

    if(msg->room_msg) {
      sprintf(buf,"%s %s %s", name, ability_message_to[AB_MSG_TO_ROOM],msg->room_msg);
      WRITE_STR_LN(fp,"Message", buf);
    }
  } else {
    log("WARNING: NULL '%s' msg passed to writeMessage!", name);
  }
}


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

  for(i = 0; i < NUM_AB_MSGS; i++) {
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

void writeSkill(FILE *skillsFile, int skillNum) {
  int i;
  char buf[MAX_STRING_LENGTH];

  buf[0] = '\0';
  fprintf(skillsFile, "SNumber   : %d\n", skillNum);
  fprintf(skillsFile, "SName     : %s\n", spell_info[skillNum].name);
  fprintf(skillsFile, "MinPos    : %s\n", position_types[spell_info[skillNum].min_position]);
  fprintf(skillsFile, "MinMana   : %d\n", spell_info[skillNum].mana_min);
  fprintf(skillsFile, "MaxMana   : %d\n", spell_info[skillNum].mana_max);
  fprintf(skillsFile, "ManaChange: %d\n", spell_info[skillNum].mana_change);
  fprintf(skillsFile, "Violent   : %s\n", spell_info[skillNum].violent ? "Yes" : "No");
  sprintbit(spell_info[skillNum].routines, routine_types, buf, sizeof(buf));
  fprintf(skillsFile, "Routines  : %s\n", buf);
  sprintbit(spell_info[skillNum].targets, target_types, buf, sizeof(buf));
  fprintf(skillsFile, "Targets   : %s\n", buf);
  if(spell_info[skillNum].wear_off_msg)
    fprintf(skillsFile, "Message   : WearOff ToChar %s\n", spell_info[skillNum].wear_off_msg);


  for(i = 0; i < NUM_CLASSES; i++) {
    fprintf(skillsFile, "MinLevel  : %s %d\n", class_abbrevs[i], spell_info[skillNum].min_level[i]);
  }

  fputs("End\n\n", skillsFile);
}

static void createAbilitySpell(struct ability_info_type *ability) {
  struct ability_spell_info_type *sp;

  CREATE(sp, struct ability_spell_info_type, 1);
  ability->spellInfo = sp;

}

static void createAbilitySkill(struct ability_info_type *ability) {
  struct ability_skill_info_type *sk;

  CREATE(sk, struct ability_skill_info_type, 1);
  sk->stunChar = 0;
  sk->stunVict = 0;
  ability->skillInfo = sk;
}

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

  for(i = 0; i < NUM_CLASSES; i++)
    ability->minLevels[i] = LVL_IMMORT;

  if(ability->type == ABILITY_TYPE_SPELL)
    createAbilitySpell(ability);
  else if(ability->type == ABILITY_TYPE_SKILL)
    createAbilitySkill(ability);

  return ability;
}

static void displayMessages(struct char_data *ch, struct msg_type *msg, const char *messageName) {
  send_to_char(ch, "\tC%s Messages\tn:\r\n", messageName);
  send_to_char(ch, "  \tcTo Char\tn  : \ty%s\tn\r\n", NOT_NULL(msg->attacker_msg));
  send_to_char(ch, "  \tcTo Target\tn: \ty%s\tn\r\n", NOT_NULL(msg->victim_msg));
  send_to_char(ch, "  \tcTo Room\tn  : \ty%s\tn\r\n", NOT_NULL(msg->room_msg));
}

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

  if(!ab->damDiceFormula) {
    if(ab->damDice) {
      sprintf(buf,"%dd%d+%d", ab->damDice->numDice, ab->damDice->diceSize, ab->damDice->damRoll);
      send_to_char(ch, "\tc%-10s\tn: \ty%-10d\tn \tc%-10s\tn: \ty%-15s\tn \tc%-10s\tn: %s\r\n",
          "Avg. Dam.", (((ab->damDice->numDice + 1) / 2) * ab->damDice->diceSize) + ab->damDice->damRoll,
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

  if(IS_SET(ab->routines, MAG_MANUAL)) {
    send_to_char(ch,"\tc%-10s\tn: \tC%s\tn\r\n", "Function",
        ab->type == ABILITY_TYPE_SPELL ?
            ab->spellInfo->spellFunc : ab->skillInfo->skillFunc);
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

  for(i = 0; i < NUM_AB_MSGS; i++) if(ab->messages[i]) hasMessages = TRUE;

  if(hasMessages) {
    send_to_char(ch, "----------------------------------------------------------------------\r\n");
    for(i = 0; i < NUM_AB_MSGS; i++) {
      if(ab->messages[i])
        displayMessages(ch, ab->messages[i], ability_message_type[i]);
    }
  }
  send_to_char(ch, "----------------------------------------------------------------------\r\n");

}

#define PRINT_SYNTAX(ch)                                      \
do {                                                          \
 send_to_char(ch, "Syntax: ABEDIT save\r\n\r\n");             \
 send_to_char(ch, "Options:\r\n");                            \
 send_to_char(ch, " save                       - save all abilities to file\r\n");  \
 send_to_char(ch, " stat <ability number/name> - display the ability information\r\n");  \
} while (0)

static ACMD(do_skedit_list) {
  struct ability_info_type *ab;
  char buf[MAX_STRING_LENGTH];
  int len = 0, nlen = 0;

  len = sprintf(buf, "\tc%-7s %-25s %-10s %s\tr\r\n","Number", "Name", "Type", "Min. Position");
  len += sprintf(buf + len, "\tW============================================================\r\n");
  if(abilityList && abilityList->iSize > 0) {
    while((ab = (struct ability_info_type *) simple_list(abilityList))) {
      len += snprintf(buf + len, sizeof(buf) - len, "\tC%-7d %-25s %-10s %s\tn\r\n",
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
static ACMD(do_skedit_stat) {
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

ACMD(do_skedit) {
  char arg1[MAX_INPUT_LENGTH];
  char *args;
  int count = 0;

  if(!IS_NPC(ch) && ch->desc && STATE(ch->desc) == CON_PLAYING) {

    if(subcmd == 0) {
      if(*argument) {
        args = one_argument(argument, arg1);
        if(*arg1 && !str_cmp(arg1, "save")) {
          count = saveAbilities();
          send_to_char(ch, "Saved %d abilities to file.\r\n", count);
        } else if(!str_cmp(arg1, "stat")) {
          do_skedit_stat(ch, args, cmd, subcmd);
        } else if(!str_cmp(arg1, "list")) {
          do_skedit_list(ch, args, cmd, subcmd);
        }
        else {
          PRINT_SYNTAX(ch);
        }
      } else {
        PRINT_SYNTAX(ch);
      }
    } else {
      switch(subcmd) {
      case SCMD_ABEDIT_STAT:
        do_skedit_stat(ch, argument, cmd, subcmd);
        break;
      case SCMD_ABEDIT_LIST:
        do_skedit_list(ch, argument, cmd, subcmd);
      }
    }
  }

  return;
}
#undef PRINT_SYNTAX

ASPELL(spell_none) {
  return;
}
