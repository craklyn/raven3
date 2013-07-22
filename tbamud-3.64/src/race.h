/*
 * race.h
 *
 *  Created on: Jul 22, 2013
 *      Author: Hudas
 */

#ifndef RACE_H_
#define RACE_H_

bitvector_t find_race_bitvector	(const char *arg);
int 		invalid_race		(struct char_data *ch, struct obj_data *obj);
int			parse_race			(char arg);

#ifndef __RACE_C__

extern const char *race_abbrevs[NUM_RACES];
extern const char *pc_race_types[NUM_RACES];
extern const char *race_menu;
extern       int  classRaceAllowed[NUM_RACES][NUM_CLASSES];
extern const int  raceStatsLimit[NUM_RACES][NUM_STATS];

#endif /* RACE_H_ */
#endif /* __RACE_C__ */
