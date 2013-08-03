/**
* @file spec_procs.h
* Header file for special procedure modules. This file groups a lot of the
* legacy special procedures found in spec_procs.c and castle.c.
* 
* Part of the core tbaMUD source code distribution, which is a derivative
* of, and continuation of, CircleMUD.
*                                                                        
* All rights reserved.  See license for complete information.                                                                
* Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University 
* CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.
* 
*/
#ifndef _SPEC_PROCS_H_
#define _SPEC_PROCS_H_

/*****************************************************************************
 * Begin Functions and defines for castle.c 
 ****************************************************************************/
void assign_kings_castle(void);

/*****************************************************************************
 * Begin Functions and defines for spec_assign.c 
 ****************************************************************************/
void assign_mobiles(void);
void assign_objects(void);
void assign_rooms(void);

/*****************************************************************************
 * Begin Functions and defines for spec_procs.c 
 ****************************************************************************/
/* Utility functions */
void sort_spells(void);
void list_skills(struct char_data *ch);
/* Special functions */
SPECIAL(guild);
SPECIAL(dump);
SPECIAL(mayor);
SPECIAL(snake);
SPECIAL(guild_guard);
SPECIAL(puff);
SPECIAL(fido);
SPECIAL(janitor);
SPECIAL(cityguard);
SPECIAL(pet_shops);
SPECIAL(bank);

/****************************************************************************
 * Class/Race based behaviour of mobiles
 ****************************************************************************/
SPECIAL(spec_Thief);
SPECIAL(spec_Warrior);
SPECIAL(spec_Cleric);
SPECIAL(spec_None);
SPECIAL(spec_OffensiveAction);

SPECIAL(spec_Orc);

void mobCombatAction(struct char_data *mob);
void mobNormalAction(struct char_data *mob);

#endif /* _SPEC_PROCS_H_ */
