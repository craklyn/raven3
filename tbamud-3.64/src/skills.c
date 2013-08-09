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

#define __SKILLS_C__

/* local functions */
static bool equipmentSkillSuccess (struct char_data *ch);
static bool affectSkillSuccess    (struct char_data *ch);

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

bool skillSuccessByNum(struct char_data *ch, sh_int skillNum) {
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


bool skillSuccessByName(struct char_data *ch, const char *skillName) {
  return skillSuccessByNum(ch, getSpellByName(skillName));
}

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


