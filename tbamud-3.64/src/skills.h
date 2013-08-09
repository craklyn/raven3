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
/*
 * Skill/Spell utility functions
 */
bool  isAffectedBySpellName (struct char_data *ch, const char *spellName);
bool  isAffectedBySpellNum  (struct char_data *ch, sh_int spellNum);
bool  skillSuccessByName    (struct char_data *ch, const char *skillName);
bool  skillSuccess     (struct char_data *ch, sh_int skillNum);
int   getSpellByName        (const char *spellName);

#endif /* SKILLS_H_ */
#endif
