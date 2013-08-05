/**************************************************************************
*  File: spec_procs.c                                      Part of tbaMUD *
*  Usage: Implementation of special procedures for mobiles/objects/rooms. *
*                                                                         *
*  All rights reserved.  See license for complete information.            *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
**************************************************************************/

/* For more examples: 
 * ftp://ftp.circlemud.org/pub/CircleMUD/contrib/snippets/specials */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "constants.h"
#include "act.h"
#include "spec_procs.h"
#include "class.h"
#include "fight.h"
#include "modify.h"

#define MIN_LEVEL_SPREAD 15
#define NUM_CURATIVE_SPELLS 3


/* locally defined functions of local (file) scope */
static int compare_spells(const void *x, const void *y);
static const char *how_good(int percent);
static void npc_steal(struct char_data *ch, struct char_data *victim);
static int getBestOffensiveSpell(struct char_data *mob);
static int getBestHealingSpell(struct char_data *mob);
static bool removeBadSpells(struct char_data *mob, struct char_data *affected);
static int getBestOffensiveSkill(struct char_data *mob);

/* Special procedures for mobiles. */
static int spell_sort_info[MAX_SKILLS + 1];

struct spec_mob_action {
  /* this is just for reference */
  const char *name;
  SPECIAL(*func);
};

struct spec_mob_skill {
  int skill_number;
  void  (*action)
       (struct char_data *ch, char *argument, int cmd, int subcmd);
};

struct curative_spells {
  bitvector_t affect; /* the affect that needs to cured/removed */
  int         spell;  /* the spell that can cure/remove the affect */
};

/*
 * List of class specific mob behavior.
 */
struct spec_mob_action mob_class_action[NUM_CLASSES] = {
    { "Magic User",       spec_None   },
    { "Cleric",           spec_Cleric },
    { "Thief",            spec_Thief  },
    { "Warrior",          spec_None   },
    { "Ranger",           spec_None   },
    { "Assassin",         spec_None   },
    { "Shou-lin",         spec_None   },
    { "Solamnic Knight",  spec_None   },
    { "Death Knight",     spec_None   },
    { "Shadow Dance",     spec_None   },
    { "Necromancer",      spec_None   },
    { "Druid",            spec_None   },
};

struct spec_mob_action mob_race_action[NUM_RACES] = {
    {"Human", spec_None},
    {"Plant", spec_None},
    {"Animal", spec_None},
    {"Dragon", spec_None},
    {"Undead", spec_None},
    {"Vampire", spec_None},
    {"Halfling", spec_None},
    {"Elf", spec_None},
    {"Dwarf", spec_None},
    {"Giant", spec_None},
    {"Minotaur", spec_None},
    {"Demon", spec_None},
    {"Ogre", spec_None},
    {"Troll", spec_None},
    {"Werewolf", spec_None},
    {"Elemental", spec_None},
    {"Orc", spec_Orc},
    {"Gnome", spec_None},
    {"Draconian", spec_None},
    {"Faerie", spec_None},
    {"Amara", spec_None},
    {"Izarti", spec_None},
    {"Drow", spec_None},
    {"S-Human", spec_None},
    {"S-Halfling", spec_None},
    {"S-Elf", spec_None},
    {"S-Drow", spec_None},
    {"S-Dwarf", spec_None},
    {"S-Minotaur", spec_None},
    {"S-Ogre", spec_None},
    {"S-Troll", spec_None},
    {"S-Draconian", spec_None},
    {"S-Gnome", spec_None},
    {"S-Orc", spec_Orc},
    {"Terran", spec_None},
    {"Zerg", spec_None},
    {"Protoss", spec_None}
};

struct curative_spells curative_spell_list[NUM_CURATIVE_SPELLS] = {
    { AFF_BLIND,  SPELL_CURE_BLIND    },
    { AFF_POISON, SPELL_REMOVE_POISON },
    { AFF_CURSE,  SPELL_REMOVE_CURSE  },
};

#define NUM_MOB_SKILLS 3

struct spec_mob_skill spec_mob_skill_info[NUM_MOB_SKILLS] = {
    {SKILL_KICK, do_kick},
    {SKILL_BASH, do_bash},
    {SKILL_WHIRLWIND, do_whirlwind}
};

/*
 * Return the best possible offensive spell for the given mob.
 * @param mob the mobile
 */
static int getBestOffensiveSpell(struct char_data *mob) {
  int i, spellNum = -1, spellCount = 0, offensiveSpells[NUM_SPELLS];
  int minLevel   = ( GET_LEVEL(mob) <= MIN_LEVEL_SPREAD ? 1 : GET_LEVEL(mob) - MIN_LEVEL_SPREAD );
  int maxLevel   = ( GET_LEVEL(mob) >=  LVL_IMMORT ?  LVL_IMMORT - 1 : GET_LEVEL(mob));

  /* get the offensive spells */
  for(i = 0; i < NUM_SPELLS; i++) {
    if(spell_info[i].violent
        /* spells within a level range based on mob's level */
        && spell_info[i].min_level[(int)GET_CLASS(mob)] >= minLevel
        && spell_info[i].min_level[(int)GET_CLASS(mob)] <= maxLevel) {
      offensiveSpells[spellCount++] = i;
    }
  }

  if(spellCount > 0) {
    spellNum = offensiveSpells[rand_number(0, spellCount-1)];
  }

  return spellNum;
}

static int getBestOffensiveSkill(struct char_data *mob) {
  int i, skillNum = -1, skillCount = 0, offensiveSkills[NUM_SPELLS];

  /* get the offensive spells */
  for(i = 0; i < NUM_MOB_SKILLS; i++) {
    /* spells within a level range based on mob's level */
    if(spell_info[spec_mob_skill_info[i].skill_number].min_level[(int)GET_CLASS(mob)] <= GET_LEVEL(mob)) {
      offensiveSkills[skillCount++] = i;
    }
  }

  if(skillCount > 0) {
    skillNum = offensiveSkills[rand_number(0, skillCount-1)];
  }

  return skillNum;
}

/*
 * Return the best possible healing spell for the given mob.
 * @param mob the mobile
 */
static int getBestHealingSpell(struct char_data *mob) {
  int i, healingSpell = -1;
  const int numHealSpells = 3;
  /*
   * Put the healing spells here in ascending order so we don't need to compare
   * the levels below.
   */
  int healingSpells[] = {
      SPELL_CURE_LIGHT,
      SPELL_CURE_CRITIC,
      SPELL_HEAL
  };

  for(i = 0; i < numHealSpells; i++)
    /* continue as long as the skill is within the mob's level */
    if(spell_info[healingSpells[i]].min_level[(int)GET_CLASS(mob)] <= GET_LEVEL(mob))
        healingSpell = healingSpells[i];

  return healingSpell;
}

/*
 * Attempt to remove bad spells (e.g. blind, poison etc.) from 'affected'. Returns TRUE if a
 * spell was cast (even if it failed), other wise FALSE.
 * @param mob the mobile
 */
static bool removeBadSpells(struct char_data *mob, struct char_data *affected) {
  bool doneCasting = FALSE;
  int i;

  for(i = 0; i < NUM_CURATIVE_SPELLS; i++)
    if(!doneCasting && IS_AFFECTED(affected, curative_spell_list[i].affect) && mob_cast(mob, affected, curative_spell_list[i].spell))
      doneCasting = TRUE;

  return doneCasting;
}

static int compare_spells(const void *x, const void *y)
{
  int	a = *(const int *)x,
	b = *(const int *)y;

  return strcmp(spell_info[a].name, spell_info[b].name);
}

void sort_spells(void)
{
  int a;

  /* initialize array, avoiding reserved. */
  for (a = 1; a <= MAX_SKILLS; a++)
    spell_sort_info[a] = a;

  qsort(&spell_sort_info[1], MAX_SKILLS, sizeof(int), compare_spells);
}

static const char *how_good(int percent)
{
  if (percent < 0)
    return " error)";
  if (percent == 0)
    return " (@Wnot learned@n)";
  if (percent < 15)
    return " (@Rinept@n)";
  if (percent < 30)
    return " (@Rawful@n)";
  if (percent < 40)
    return " (@Mpoor@n)";
  if (percent < 55)
    return " (@Bfair@n)";
  if (percent < 65)
    return " (@Bpromising@n)";
  if (percent < 70)
    return " (@Bgood@n)";
  if (percent < 75)
    return " (@Cvery good@n)";
  if (percent < 80)
    return " (@Csuperb@n)";
  if (percent < 85)
    return " (@Bexcellent@n)";
  if (percent < 90)
    return " (@Badept@n)";
  if (percent <= 99)
    return " (@Btrue meaning@n)";
  
  return " (@R@=godlike@n)";
}

const char *prac_types[] = {
  "spell",
  "skill"
};

#define LEARNED_LEVEL	0	/* % known which is considered "learned" */
#define MAX_PER_PRAC	1	/* max percent gain in skill per practice */
#define MIN_PER_PRAC	2	/* min percent gain in skill per practice */
#define PRAC_TYPE	3	/* should it say 'spell' or 'skill'?	 */

#define LEARNED(ch) (prac_params[LEARNED_LEVEL][(int)GET_CLASS(ch)])
#define MINGAIN(ch) (prac_params[MIN_PER_PRAC][(int)GET_CLASS(ch)])
#define MAXGAIN(ch) (prac_params[MAX_PER_PRAC][(int)GET_CLASS(ch)])
#define SPLSKL(ch) (prac_types[prac_params[PRAC_TYPE][(int)GET_CLASS(ch)]])

void list_skills(struct char_data *ch)
{
  const char *overflow = "\r\n**OVERFLOW**\r\n";
  int i, sortpos;
  size_t len = 0, nlen;
  char buf2[MAX_STRING_LENGTH];

  len = snprintf(buf2, sizeof(buf2), "You have %d practice session%s remaining.\r\n"
	"You know of the following %ss:\r\n", GET_PRACTICES(ch),
	GET_PRACTICES(ch) == 1 ? "" : "s", SPLSKL(ch));

  int cnt = 0;
  for (sortpos = 1; sortpos <= MAX_SKILLS; sortpos++) {
    i = spell_sort_info[sortpos];
    if (GET_LEVEL(ch) >= spell_info[i].min_level[(int) GET_CLASS(ch)]) {
      cnt += 1;
      nlen = snprintf(buf2 + len, sizeof(buf2) - len, (cnt%2) ? "%-20s %s | " : "%-20s %s\r\n", spell_info[i].name, how_good(GET_PLAYER_SKILL(ch, i)));
      if (len + nlen >= sizeof(buf2) || nlen < 0)
        break;
      len += nlen;
    }
  }
  if (len >= sizeof(buf2))
    strcpy(buf2 + sizeof(buf2) - strlen(overflow) - 1, overflow); /* strcpy: OK */

  parse_at(buf2);
  page_string(ch->desc, buf2, TRUE);
}

SPECIAL(guild)
{
  int skill_num, percent;

  if (IS_NPC(ch) || !CMD_IS("practice"))
    return (FALSE);

  skip_spaces(&argument);

  if (!*argument) {
    list_skills(ch);
    return (TRUE);
  }
  if (GET_PRACTICES(ch) <= 0) {
    send_to_char(ch, "You do not seem to be able to practice now.\r\n");
    return (TRUE);
  }

  skill_num = find_skill_num(argument);

  if (skill_num < 1 ||
      GET_LEVEL(ch) < spell_info[skill_num].min_level[(int) GET_CLASS(ch)]) {
    send_to_char(ch, "You do not know of that %s.\r\n", SPLSKL(ch));
    return (TRUE);
  }
  if (GET_PLAYER_SKILL(ch, skill_num) >= LEARNED(ch)) {
    send_to_char(ch, "You are already learned in that area.\r\n");
    return (TRUE);
  }
  send_to_char(ch, "You practice for a while...\r\n");
  GET_PRACTICES(ch)--;

  percent = GET_PLAYER_SKILL(ch, skill_num);
  percent += MIN(MAXGAIN(ch), MAX(MINGAIN(ch), int_app[GET_INT(ch)].learn));

  SET_PLAYER_SKILL(ch, skill_num, MIN(LEARNED(ch), percent));

  if (GET_PLAYER_SKILL(ch, skill_num) >= LEARNED(ch))
    send_to_char(ch, "You are now learned in that area.\r\n");

  return (TRUE);
}

SPECIAL(dump)
{
  struct obj_data *k;
  int value = 0;

  for (k = world[IN_ROOM(ch)].contents; k; k = world[IN_ROOM(ch)].contents) {
    act("$p vanishes in a puff of smoke!", FALSE, 0, k, 0, TO_ROOM);
    extract_obj(k);
  }

  if (!CMD_IS("drop"))
    return (FALSE);

  do_drop(ch, argument, cmd, SCMD_DROP);

  for (k = world[IN_ROOM(ch)].contents; k; k = world[IN_ROOM(ch)].contents) {
    act("$p vanishes in a puff of smoke!", FALSE, 0, k, 0, TO_ROOM);
    value += MAX(1, MIN(50, GET_OBJ_COST(k) / 10));
    extract_obj(k);
  }

  if (value) {
    send_to_char(ch, "You are awarded for outstanding performance.\r\n");
    act("$n has been awarded for being a good citizen.", TRUE, ch, 0, 0, TO_ROOM);

    if (GET_LEVEL(ch) < 3)
      gain_exp(ch, value);
    else
      increase_gold(ch, value);
  }
  return (TRUE);
}

SPECIAL(mayor)
{
  char actbuf[MAX_INPUT_LENGTH];

  const char open_path[] =
	"W3a3003b33000c111d0d111Oe333333Oe22c222112212111a1S.";
  const char close_path[] =
	"W3a3003b33000c111d0d111CE333333CE22c222112212111a1S.";

  static const char *path = NULL;
  static int path_index;
  static bool move = FALSE;

  if (!move) {
    if (time_info.hours == 6) {
      move = TRUE;
      path = open_path;
      path_index = 0;
    } else if (time_info.hours == 20) {
      move = TRUE;
      path = close_path;
      path_index = 0;
    }
  }
  if (cmd || !move || (GET_POS(ch) < POS_SLEEPING) ||
      (GET_POS(ch) == POS_FIGHTING))
    return (FALSE);

  switch (path[path_index]) {
  case '0':
  case '1':
  case '2':
  case '3':
    perform_move(ch, path[path_index] - '0', 1);
    break;

  case 'W':
    GET_POS(ch) = POS_STANDING;
    act("$n awakens and groans loudly.", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'S':
    GET_POS(ch) = POS_SLEEPING;
    act("$n lies down and instantly falls asleep.", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'a':
    act("$n says 'Hello Honey!'", FALSE, ch, 0, 0, TO_ROOM);
    act("$n smirks.", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'b':
    act("$n says 'What a view!  I must get something done about that dump!'",
	FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'c':
    act("$n says 'Vandals!  Youngsters nowadays have no respect for anything!'",
	FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'd':
    act("$n says 'Good day, citizens!'", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'e':
    act("$n says 'I hereby declare the bazaar open!'", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'E':
    act("$n says 'I hereby declare Midgaard closed!'", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'O':
    do_gen_door(ch, strcpy(actbuf, "gate"), 0, SCMD_UNLOCK);	/* strcpy: OK */
    do_gen_door(ch, strcpy(actbuf, "gate"), 0, SCMD_OPEN);	/* strcpy: OK */
    break;

  case 'C':
    do_gen_door(ch, strcpy(actbuf, "gate"), 0, SCMD_CLOSE);	/* strcpy: OK */
    do_gen_door(ch, strcpy(actbuf, "gate"), 0, SCMD_LOCK);	/* strcpy: OK */
    break;

  case '.':
    move = FALSE;
    break;

  }

  path_index++;
  return (FALSE);
}

/* General special procedures for mobiles. */

static void npc_steal(struct char_data *ch, struct char_data *victim)
{
  int gold;

  if (IS_NPC(victim))
    return;
  if (GET_LEVEL(victim) >= LVL_IMMORT)
    return;
  if (!CAN_SEE(ch, victim))
    return;

  if (AWAKE(victim) && (rand_number(0, GET_LEVEL(ch)) == 0)) {
    act("You discover that $n has $s hands in your wallet.", FALSE, ch, 0, victim, TO_VICT);
    act("$n tries to steal gold from $N.", TRUE, ch, 0, victim, TO_NOTVICT);
  } else {
    /* Steal some gold coins */
    gold = (GET_GOLD(victim) * rand_number(1, 10)) / 100;
    if (gold > 0) {
      increase_gold(ch, gold);
	  decrease_gold(victim, gold);
    }
  }
}

/* Quite lethal to low-level characters. */
SPECIAL(snake)
{
  if (cmd || GET_POS(ch) != POS_FIGHTING || !FIGHTING(ch))
    return (FALSE);

  if (IN_ROOM(FIGHTING(ch)) != IN_ROOM(ch) || rand_number(0, GET_LEVEL(ch)) != 0)
    return (FALSE);

  act("$n bites $N!", 1, ch, 0, FIGHTING(ch), TO_NOTVICT);
  act("$n bites you!", 1, ch, 0, FIGHTING(ch), TO_VICT);
  call_magic(ch, FIGHTING(ch), 0, SPELL_POISON, GET_LEVEL(ch), CAST_SPELL);
  return (TRUE);
}

/* Special procedures for mobiles. */
SPECIAL(guild_guard) 
{ 
  int i, direction; 
  struct char_data *guard = (struct char_data *)me; 
  const char *buf = "The guard humiliates you, and blocks your way.\r\n"; 
  const char *buf2 = "The guard humiliates $n, and blocks $s way."; 

  if (!IS_MOVE(cmd) || AFF_FLAGGED(guard, AFF_BLIND)) 
    return (FALSE); 
     
  if (GET_LEVEL(ch) >= LVL_IMMORT) 
    return (FALSE); 
   
  /* find out what direction they are trying to go */ 
  for (direction = 0; direction < NUM_OF_DIRS; direction++)
    if (!strcmp(cmd_info[cmd].command, dirs[direction]))
      for (direction = 0; direction < DIR_COUNT; direction++)
		if (!strcmp(cmd_info[cmd].command, dirs[direction]) ||
			!strcmp(cmd_info[cmd].command, autoexits[direction]))
	      break; 

  for (i = 0; guild_info[i].guild_room != NOWHERE; i++) { 
    /* Wrong guild. */ 
    if (GET_ROOM_VNUM(IN_ROOM(ch)) != guild_info[i].guild_room) 
      continue; 

    /* Wrong direction. */ 
    if (direction != guild_info[i].direction) 
      continue; 

    /* Allow the people of the guild through. */ 
    if (!IS_NPC(ch) && GET_CLASS(ch) == guild_info[i].pc_class) 
      continue; 

    send_to_char(ch, "%s", buf); 
    act(buf2, FALSE, ch, 0, 0, TO_ROOM); 
    return (TRUE); 
  } 
  return (FALSE); 
} 

SPECIAL(puff)
{
  char actbuf[MAX_INPUT_LENGTH];

  if (cmd)
    return (FALSE);

  switch (rand_number(0, 60)) {
    case 0:
      do_say(ch, strcpy(actbuf, "My god!  It's full of stars!"), 0, 0);	/* strcpy: OK */
      return (TRUE);
    case 1:
      do_say(ch, strcpy(actbuf, "How'd all those fish get up here?"), 0, 0);	/* strcpy: OK */
      return (TRUE);
    case 2:
      do_say(ch, strcpy(actbuf, "I'm a very female dragon."), 0, 0);	/* strcpy: OK */
      return (TRUE);
    case 3:
      do_say(ch, strcpy(actbuf, "I've got a peaceful, easy feeling."), 0, 0);	/* strcpy: OK */
      return (TRUE);
    default:
      return (FALSE);
  }
}

SPECIAL(fido)
{
  struct obj_data *i, *temp, *next_obj;

  if (cmd || !AWAKE(ch))
    return (FALSE);

  for (i = world[IN_ROOM(ch)].contents; i; i = i->next_content) {
    if (!IS_CORPSE(i))
      continue;

    act("$n savagely devours a corpse.", FALSE, ch, 0, 0, TO_ROOM);
    for (temp = i->contains; temp; temp = next_obj) {
      next_obj = temp->next_content;
      obj_from_obj(temp);
      obj_to_room(temp, IN_ROOM(ch));
    }
    extract_obj(i);
    return (TRUE);
  }
  return (FALSE);
}

SPECIAL(janitor)
{
  struct obj_data *i;

  if (cmd || !AWAKE(ch))
    return (FALSE);

  for (i = world[IN_ROOM(ch)].contents; i; i = i->next_content) {
    if (!CAN_WEAR(i, ITEM_WEAR_TAKE))
      continue;
    if (GET_OBJ_TYPE(i) != ITEM_DRINKCON && GET_OBJ_COST(i) >= 15)
      continue;
    act("$n picks up some trash.", FALSE, ch, 0, 0, TO_ROOM);
    obj_from_room(i);
    obj_to_char(i, ch);
    return (TRUE);
  }
  return (FALSE);
}

SPECIAL(cityguard)
{
  struct char_data *tch, *evil, *spittle;
  int max_evil, min_cha;

  if (cmd || !AWAKE(ch) || FIGHTING(ch))
    return (FALSE);

  max_evil = 1000;
  min_cha = 6;
  spittle = evil = NULL;

  for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room) {
    if (!CAN_SEE(ch, tch))
      continue;
    if (!IS_NPC(tch) && PLR_FLAGGED(tch, PLR_KILLER)) {
      act("$n screams 'HEY!!!  You're one of those PLAYER KILLERS!!!!!!'", FALSE, ch, 0, 0, TO_ROOM);
      hit(ch, tch, TYPE_UNDEFINED);
      return (TRUE);
    }

    if (!IS_NPC(tch) && PLR_FLAGGED(tch, PLR_THIEF)) {
      act("$n screams 'HEY!!!  You're one of those PLAYER THIEVES!!!!!!'", FALSE, ch, 0, 0, TO_ROOM);
      hit(ch, tch, TYPE_UNDEFINED);
      return (TRUE);
    }

    if (FIGHTING(tch) && GET_ALIGNMENT(tch) < max_evil && (IS_NPC(tch) || IS_NPC(FIGHTING(tch)))) {
      max_evil = GET_ALIGNMENT(tch);
      evil = tch;
    }

    if (GET_CHA(tch) < min_cha) {
      spittle = tch;
      min_cha = GET_CHA(tch);
    }
  }

  if (evil && GET_ALIGNMENT(FIGHTING(evil)) >= 0) {
    act("$n screams 'PROTECT THE INNOCENT!  BANZAI!  CHARGE!  ARARARAGGGHH!'", FALSE, ch, 0, 0, TO_ROOM);
    hit(ch, evil, TYPE_UNDEFINED);
    return (TRUE);
  }

  /* Reward the socially inept. */
  if (spittle && !rand_number(0, 9)) {
    static int spit_social;

    if (!spit_social)
      spit_social = find_command("spit");

    if (spit_social > 0) {
      char spitbuf[MAX_NAME_LENGTH + 1];
      strncpy(spitbuf, GET_NAME(spittle), sizeof(spitbuf));	/* strncpy: OK */
      spitbuf[sizeof(spitbuf) - 1] = '\0';
      do_action(ch, spitbuf, spit_social, 0);
      return (TRUE);
    }
  }
  return (FALSE);
}

#define PET_PRICE(pet) (GET_LEVEL(pet) * 300)
SPECIAL(pet_shops)
{
  char buf[MAX_STRING_LENGTH], pet_name[256];
  room_rnum pet_room;
  struct char_data *pet;

  /* Gross. */
  pet_room = IN_ROOM(ch) + 1;

  if (CMD_IS("list")) {
    send_to_char(ch, "Available pets are:\r\n");
    for (pet = world[pet_room].people; pet; pet = pet->next_in_room) {
      /* No, you can't have the Implementor as a pet if he's in there. */
      if (!IS_NPC(pet))
        continue;
      send_to_char(ch, "%8d - %s\r\n", PET_PRICE(pet), GET_NAME(pet));
    }
    return (TRUE);
  } else if (CMD_IS("buy")) {

    two_arguments(argument, buf, pet_name);

    if (!(pet = get_char_room(buf, NULL, pet_room)) || !IS_NPC(pet)) {
      send_to_char(ch, "There is no such pet!\r\n");
      return (TRUE);
    }
    if (GET_GOLD(ch) < PET_PRICE(pet)) {
      send_to_char(ch, "You don't have enough gold!\r\n");
      return (TRUE);
    }
    decrease_gold(ch, PET_PRICE(pet));

    pet = read_mobile(GET_MOB_RNUM(pet), REAL);
    GET_EXP(pet) = 0;
    SET_BIT_AR(AFF_FLAGS(pet), AFF_CHARM);

    if (*pet_name) {
      snprintf(buf, sizeof(buf), "%s %s", pet->player.name, pet_name);
      /* free(pet->player.name); don't free the prototype! */
      pet->player.name = strdup(buf);

      snprintf(buf, sizeof(buf), "%sA small sign on a chain around the neck says 'My name is %s'\r\n",
	      pet->player.description, pet_name);
      /* free(pet->player.description); don't free the prototype! */
      pet->player.description = strdup(buf);
    }
    char_to_room(pet, IN_ROOM(ch));
    add_follower(pet, ch);

    /* Be certain that pets can't get/carry/use/wield/wear items */
    IS_CARRYING_W(pet) = 1000;
    IS_CARRYING_N(pet) = 100;

    send_to_char(ch, "May you enjoy your pet.\r\n");
    act("$n buys $N as a pet.", FALSE, ch, 0, pet, TO_ROOM);

    return (TRUE);
  }

  /* All commands except list and buy */
  return (FALSE);
}

/* Special procedures for objects. */
SPECIAL(bank)
{
  int amount;

  if (CMD_IS("balance")) {
    if (GET_BANK_GOLD(ch) > 0)
      send_to_char(ch, "Your current balance is %d coins.\r\n", GET_BANK_GOLD(ch));
    else
      send_to_char(ch, "You currently have no money deposited.\r\n");
    return (TRUE);
  } else if (CMD_IS("deposit")) {
    if ((amount = atoi(argument)) <= 0) {
      send_to_char(ch, "How much do you want to deposit?\r\n");
      return (TRUE);
    }
    if (GET_GOLD(ch) < amount) {
      send_to_char(ch, "You don't have that many coins!\r\n");
      return (TRUE);
    }
    decrease_gold(ch, amount);
	increase_bank(ch, amount);
    send_to_char(ch, "You deposit %d coins.\r\n", amount);
    act("$n makes a bank transaction.", TRUE, ch, 0, FALSE, TO_ROOM);
    return (TRUE);
  } else if (CMD_IS("withdraw")) {
    if ((amount = atoi(argument)) <= 0) {
      send_to_char(ch, "How much do you want to withdraw?\r\n");
      return (TRUE);
    }
    if (GET_BANK_GOLD(ch) < amount) {
      send_to_char(ch, "You don't have that many coins deposited!\r\n");
      return (TRUE);
    }
    increase_gold(ch, amount);
	decrease_bank(ch, amount);
    send_to_char(ch, "You withdraw %d coins.\r\n", amount);
    act("$n makes a bank transaction.", TRUE, ch, 0, FALSE, TO_ROOM);
    return (TRUE);
  } else
    return (FALSE);
}

/******************************************************************************
 * Class and Racial behaviors
 ******************************************************************************/
/*
 * Place holder for unimplemented special behaviour
 */
SPECIAL(spec_None) {
  return FALSE;
}

SPECIAL(spec_Thief)
{
  struct char_data *cons;

  if (cmd || GET_POS(ch) != POS_STANDING)
    return (FALSE);

  for (cons = world[IN_ROOM(ch)].people; cons; cons = cons->next_in_room)
    if (!IS_NPC(cons) && GET_LEVEL(cons) < LVL_IMMORT && !rand_number(0, 4)) {
      npc_steal(ch, cons);
      return (TRUE);
    }

  return (FALSE);
}

SPECIAL(spec_Cleric) {
  int healSpell = -1;
  bool doneCasting = FALSE;
  struct char_data *tch;

  /* Fighting or not a Cleric will always first check if they need some healing */
  if((GET_HIT(ch) < (GET_MAX_HIT(ch)/4))
      || (GET_HIT(ch) < (GET_MAX_HIT(ch)/2) && !rand_number(0, 7))
      || (GET_HIT(ch) < (GET_MAX_HIT(ch)/3) && !rand_number(0, 4)))
    if((healSpell = getBestHealingSpell(ch)) != -1)
      doneCasting = mob_cast(ch, ch, healSpell);

  /* spec_OffensiveSpellCaster has covered the fighting behavior */
  if(GET_POS(ch) == POS_STANDING) {
    /* Try to remove bad spells */
    doneCasting = removeBadSpells(ch, ch);

    /* Find for mobs that might need some help */
    if(!doneCasting && MOB_FLAGGED(ch, MOB_HELPER))
    for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room) {
        if (CAN_SEE(ch, tch) && GET_HIT(tch) < GET_MAX_HIT(tch) && (healSpell = getBestHealingSpell(ch)) != -1)
          doneCasting = mob_cast(ch, tch, healSpell);
        if (!doneCasting && CAN_SEE(ch, tch) && GET_HIT(tch) < GET_MAX_HIT(tch))
          doneCasting = removeBadSpells(ch, tch);
    }
  }

  return doneCasting;
}

SPECIAL(spec_OffensiveAction)
{
  struct char_data *vict;
  int offensiveSpell, offensiveSkill;

  if (GET_POS(ch) != POS_FIGHTING)
    return (FALSE);

  /* pseudo-randomly choose someone in the room who is fighting me */
  for (vict = world[IN_ROOM(ch)].people; vict; vict = vict->next_in_room)
    if (FIGHTING(vict) == ch && !rand_number(0, 4))
      break;

  /* if I didn't pick any of those, then just slam the guy I'm fighting */
  if (vict == NULL && IN_ROOM(FIGHTING(ch)) == IN_ROOM(ch))
    vict = FIGHTING(ch);

  /* Hm...didn't pick anyone...I'll wait a round. */
  if (vict == NULL)
    return (TRUE);

  if((offensiveSpell = getBestOffensiveSpell(ch)) > 0) {
    return mob_cast(ch, vict, offensiveSpell);
  } else if((offensiveSkill = getBestOffensiveSkill(ch)) >= 0 && GET_WAIT_STATE(ch) < 1){
    ((*spec_mob_skill_info[offensiveSkill].action)(ch, "", 0, 0));
  }

  return (TRUE);
}

/*
 * This is to demonstrate mobile behavior based on race. Nothing really special.
 * Them Orcs hate the Elves!
 */
SPECIAL(spec_Orc) {
  struct char_data *elf;
  bool ok = FALSE;
  char *action = NULL, buf[MAX_STRING_LENGTH];

  if(!FIGHTING(ch) && (GET_RACE(ch) == RACE_ORC || GET_RACE(ch) == RACE_SORC)) {
    for (elf = world[IN_ROOM(ch)].people; elf; elf = elf->next_in_room) {
      if(CAN_SEE(ch, elf) && (GET_RACE(elf) == RACE_ELF || GET_RACE(elf) == RACE_SELF) && !rand_number(0, 4)
          ) {
        switch(rand_number(0, 4)) {
        case 0:
          action = strdup("sneer");
          break;
        case 1:
          action = strdup("hate");
          break;
        case 2:
          action = strdup("rebuke");
          break;
        case 3:
          action = strdup("challenge");
          break;
        }

        if(action != NULL) {
          do_action(ch, GET_NAME(elf), find_command(action), 0);
        } else {
          sprintf(buf,"To be with %s in the same room is not something I desire.", GET_NAME(elf));
          do_say(ch, buf, 0, 0);
        }
        continue;
      }
    }
  }

  return ok;
}

/*
 * Class and/or race specific mobile combat behavior.
 */
void mobCombatAction(struct char_data *mob) {
  /* First do class specific spells
   */
  if(!(*mob_class_action[(int)GET_CLASS(mob)].func)(mob, NULL, 0, NULL)
      && !(*mob_race_action[(int)GET_RACE(mob)].func)(mob, NULL, 0, NULL)) {
    spec_OffensiveAction(mob, NULL, 0, NULL);
  }
}

/*
 *
 */
void mobNormalAction(struct char_data *mob) {
  if(!(*mob_class_action[(int)GET_CLASS(mob)].func)(mob, NULL, 0, NULL))
    (*mob_race_action[(int)GET_RACE(mob)].func)(mob, NULL, 0, NULL);
}
