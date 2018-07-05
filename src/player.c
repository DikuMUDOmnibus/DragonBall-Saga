/****************************************************************************
 * [S]imulated [M]edieval [A]dventure multi[U]ser [G]ame      |   \\._.//   *
 * -----------------------------------------------------------|   (0...0)   *
 * SMAUG 1.4 (C) 1994, 1995, 1996, 1998  by Derek Snider      |    ).:.(    *
 * -----------------------------------------------------------|    {o o}    *
 * SMAUG code team: Thoric, Altrag, Blodkai, Narn, Haus,      |   / ' ' \   *
 * Scryn, Rennard, Swordbearer, Gorog, Grishnakh, Nivek,      |~'~.VxvxV.~'~*
 * Tricops and Fireblade                                      |             *
 * ------------------------------------------------------------------------ *
 * Merc 2.1 Diku Mud improvments copyright (C) 1992, 1993 by Michael        *
 * Chastain, Michael Quan, and Mitchell Tse.                                *
 * Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,          *
 * Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.     *
 * ------------------------------------------------------------------------ *
 * 		Commands for personal player settings/statictics	    *
 ****************************************************************************/

#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "mud.h"
#include "sha256.h"

bool add_hiscore( char *keyword, char *name, int score );
bool add_hiscore_ld( char *keyword, char *name, long double score );
/*
 *  Locals
 */
char *tiny_affect_loc_name(int location);

void do_gained( CHAR_DATA *ch, char *argument )
{
    if( IS_NPC(ch) )
	return;
    long double pltrack = 0;
    pltrack = ch->exp - ch->logon_start;
    if( pltrack < 0 )
      pltrack = 0;
    if( is_android(ch) || is_superandroid(ch) )
      pager_printf(ch, "\n\r&GGAINED TL SINCE LOGON: %-16s&C\n\r",
                num_punct_ld(pltrack));
    else
      pager_printf(ch, "\n\r&GGAINED PL SINCE LOGON: %-16s&C\n\r",
                num_punct_ld(pltrack));
    return;
}

void do_pstatus( CHAR_DATA *ch, char *argument )
{
    char lf[MAX_STRING_LENGTH];
    char lfMax[MAX_STRING_LENGTH];
    char ac[MAX_STRING_LENGTH];
    char acMax[MAX_STRING_LENGTH];
    char energy[MAX_STRING_LENGTH];
    char energyMax[MAX_STRING_LENGTH];
    char bpl[MAX_STRING_LENGTH];
    char cpl[MAX_STRING_LENGTH];

    if ( IS_NPC( ch ) && ch->fighting )
		return;

	strcpy(lf, num_punct(ch->hit));
	strcpy(lfMax, num_punct(ch->max_hit));
	strcpy(ac, num_punct(get_armor(ch)));
	strcpy(acMax, num_punct(get_maxarmor(ch)));
	strcpy(energy, num_punct(ch->mana));
	strcpy(energyMax, num_punct(ch->max_mana));
	strcpy(bpl, num_punct_ld(ch->exp));
	strcpy(cpl, num_punct_ld(ch->pl));

	ch_printf( ch, "&wLifeforce:(&g%s&w/&G%s&w) Armor:(&g%s&w/&G%s&w) Energy:(&g%s&w/&G%s&w)\n\r",
		lf, lfMax, ac, acMax, energy, energyMax);
        if( is_android(ch) || is_superandroid(ch) )
	  ch_printf( ch, "Techlevel:(&g%s&w/&G%s&w)\n\r", bpl, cpl);
	else
	  ch_printf( ch, "Powerlevel:(&g%s&w/&G%s&w)\n\r", bpl, cpl);

    return;
}

void do_gold(CHAR_DATA * ch, char *argument)
{
   set_char_color( AT_GOLD, ch );
   ch_printf( ch,  "You have %s zeni.\n\r", num_punct(ch->gold) );
   return;
}


void do_worth(CHAR_DATA *ch, char *argument)
{
    char            buf[MAX_STRING_LENGTH];
    char            buf2[MAX_STRING_LENGTH];
	char			buf3[MAX_STRING_LENGTH];

    if (IS_NPC(ch))
      return;

    set_pager_color(AT_SCORE, ch);
    pager_printf(ch, "\n\rWorth for %s%s.\n\r", ch->name, ch->pcdata->title);
    send_to_pager(" ----------------------------------------------------------------------------\n\r", ch);
    if (!ch->pcdata->deity)		 sprintf( buf, "N/A" );
    else if (ch->pcdata->favor > 2250)	 sprintf( buf, "loved" );
    else if (ch->pcdata->favor > 2000)	 sprintf( buf, "cherished" );
    else if (ch->pcdata->favor > 1750) 	 sprintf( buf, "honored" );
    else if (ch->pcdata->favor > 1500)	 sprintf( buf, "praised" );
    else if (ch->pcdata->favor > 1250)	 sprintf( buf, "favored" );
    else if (ch->pcdata->favor > 1000)	 sprintf( buf, "respected" );
    else if (ch->pcdata->favor > 750)	 sprintf( buf, "liked" );
    else if (ch->pcdata->favor > 250)	 sprintf( buf, "tolerated" );
    else if (ch->pcdata->favor > -250)	 sprintf( buf, "ignored" );
    else if (ch->pcdata->favor > -750)	 sprintf( buf, "shunned" );
    else if (ch->pcdata->favor > -1000)	 sprintf( buf, "disliked" );
    else if (ch->pcdata->favor > -1250)	 sprintf( buf, "dishonored" );
    else if (ch->pcdata->favor > -1500)	 sprintf( buf, "disowned" );
    else if (ch->pcdata->favor > -1750)	 sprintf( buf, "abandoned" );
    else if (ch->pcdata->favor > -2000)	 sprintf( buf, "despised" );
    else if (ch->pcdata->favor > -2250)	 sprintf( buf, "hated" );
    else				 sprintf( buf, "damned" );

    if ( ch->level < 10 )
    {
       if (ch->alignment > 900)		 sprintf(buf2, "devout");
       else if (ch->alignment > 700)	 sprintf(buf2, "noble");
       else if (ch->alignment > 350)	 sprintf(buf2, "honorable");
       else if (ch->alignment > 100)	 sprintf(buf2, "worthy");
       else if (ch->alignment > -100)	 sprintf(buf2, "neutral");
       else if (ch->alignment > -350)	 sprintf(buf2, "base");
       else if (ch->alignment > -700)	 sprintf(buf2, "evil");
       else if (ch->alignment > -900)	 sprintf(buf2, "ignoble");
       else				 sprintf(buf2, "fiendish");
    }
    else
	sprintf(buf2, "%d", ch->alignment );
    strcpy(buf3, num_punct_ld(ch->exp));
	pager_printf(ch, "|Level: %-4d |Favor: %-10s |Alignment: %-9s |PL: %17s|\n\r",
                     ch->level, buf, buf2, buf3 );
    send_to_pager(" ----------------------------------------------------------------------------\n\r", ch);
        switch (ch->style) {
        case STYLE_EVASIVE:
                sprintf(buf, "evasive");
                break;
        case STYLE_DEFENSIVE:
                sprintf(buf, "defensive");
                break;
        case STYLE_AGGRESSIVE:
                sprintf(buf, "aggressive");
                break;
        case STYLE_BERSERK:
                sprintf(buf, "berserk");
                break;
        default:
                sprintf(buf, "standard");
                break;
        }
    pager_printf(ch, "|Glory: %-4d |Weight: %-9d |Style: %-13s |Zeni: %-14s |\n\r",
                 ch->pcdata->quest_curr, ch->carry_weight, buf, num_punct(ch->gold) );
    send_to_pager(" ----------------------------------------------------------------------------\n\r", ch);
    if ( ch->level < 15 && !IS_PKILL(ch) )
    	pager_printf(ch, "|            |Hitroll: -------- |Damroll: ----------- |                     |\n\r" );
    else
    	pager_printf(ch, "|            |Hitroll: %-8d |Damroll: %-11d |                     |\n\r", GET_HITROLL(ch), GET_DAMROLL(ch) );
    send_to_pager(" ----------------------------------------------------------------------------\n\r", ch);
    return;
}

/* pager_printf_color
 * New score command by Haus
 */
void do_score(CHAR_DATA * ch, char *argument)
{
	char		buf[MAX_STRING_LENGTH];
	char		buf2[MAX_STRING_LENGTH];
	AFFECT_DATA	*paf;

	if (IS_NPC(ch))
	{
		do_oldscore(ch, argument);
		return;
	}

	sysdata.outBytesFlag = LOGBOUTINFORMATION;

	pager_printf(ch,"\n\r&CThe %s,\n\r",get_rank(ch));
	pager_printf(ch, "%s%s%s.&C\n\r", ch->name,
		ch->pcdata->last_name ? ch->pcdata->last_name : "", ch->pcdata->title);
	if ( get_trust( ch ) != ch->level )
		pager_printf( ch, "You are trusted at level %d.\n\r", get_trust( ch ) );

	send_to_pager("&c----------------------------------------------------------------------------&C\n\r", ch);

	pager_printf(ch,"RACE : &W%-16.16s&C    SEX: &W%-7s&C     Played: &w%d hours&C\n\r",
		capitalize(get_race(ch)),
		ch->sex == SEX_MALE    ? "Male"   :
		ch->sex == SEX_FEMALE  ? "Female" : "Neutral",
		(get_age(ch) - 4) * 2);
	pager_printf(ch,"YEARS: &W%-6d&C                              Created: &w%s\r&C",
		get_newage(ch), ctime(&ch->pcdata->creation_date));

	if (get_curr_str(ch) >= 100 || ch->perm_str >= 100)
	pager_printf(ch,"STR  : &W%-3d&C(&w%3d.%2.2d&C)  DAM+: &R%-2d&C                Log in: &w%s\r&C",
		get_curr_str(ch), ch->perm_str, ch->pcdata->tStr, get_damroll(ch), ctime(&ch->logon));
	else
	pager_printf(ch,"STR  : &W%-2d&C(&w%2d.%2.2d&C)    DAM+: &R%-2d&C                Log in: &w%s\r&C",
		get_curr_str(ch), ch->perm_str, ch->pcdata->tStr, get_damroll(ch), ctime(&ch->logon));
	if (get_curr_dex(ch) >= 100 || ch->perm_dex >= 100)
	pager_printf(ch,"SPD  : &W%-3d&C(&w%3d.%2.2d&C)  pDEF: &R%-2d&C            Last Saved: &w%s\r&C",
		get_curr_dex(ch), ch->perm_dex, ch->pcdata->tSpd, (get_strDef(ch)+get_conDef(ch)), ch->save_time ? ctime(&ch->save_time) : "no save this session\n");
	else
	pager_printf(ch,"SPD  : &W%-2d&C(&w%2d.%2.2d&C)    pDEF: &R%-2d&C            Last Saved: &w%s\r&C",
		get_curr_dex(ch), ch->perm_dex, ch->pcdata->tSpd, (get_strDef(ch)+get_conDef(ch)), ch->save_time ? ctime(&ch->save_time) : "no save this session\n");
	if (get_curr_int(ch) >= 100 || ch->perm_int >= 100)
	pager_printf(ch,"INT  : &W%-3d&C(&w%3d.%2.2d&C)  eDEF: &R%-2d&C              CurrTime: &w%s\r&C",
		get_curr_int(ch), ch->perm_int, ch->pcdata->tInt, get_conDef(ch), ctime(&current_time));
	else
	pager_printf(ch,"INT  : &W%-2d&C(&w%2d.%2.2d&C)    eDEF: &R%-2d&C              CurrTime: &w%s\r&C",
		get_curr_int(ch), ch->perm_int, ch->pcdata->tInt, get_conDef(ch), ctime(&current_time));

    if (get_armor(ch) < 0)
		sprintf(buf, "&Wthe rags of a beggar&C");
    else if (get_armor(ch) == 0)
		sprintf(buf, "&Wimproper for a battle&C");
    else if (get_armor(ch) < 500)
		sprintf(buf, "&Wof poor quality&C");
    else if (get_armor(ch) < 2500)
		sprintf(buf, "&Wmoderately crafted&C");
    else if (get_armor(ch) < 5000)
		sprintf(buf, "&Wwell crafted&C");
    else if (get_armor(ch) < 7500)
		sprintf(buf, "&Wexcellently crafted&C");
    else if (get_armor(ch) <= 10000)
		sprintf(buf, "&Wexcellently crafted&C");
    else
		sprintf(buf, "&WERROR: Please report&C");

	if (get_curr_con(ch) >= 100 || ch->perm_con >= 100)
	pager_printf(ch,"CON  : &W%-3d&C(&w%3d.%2.2d&C) Armor: &W%d, %s\n\r",
		get_curr_con(ch), ch->perm_con, ch->pcdata->tCon, get_armor(ch), buf);
	else
	pager_printf(ch,"CON  : &W%-2d&C(&w%2d.%2.2d&C)   Armor: &W%d, %s\n\r",
		get_curr_con(ch), ch->perm_con, ch->pcdata->tCon, get_armor(ch), buf);

    if (ch->alignment > 900)
		sprintf(buf, "&Ydevout");
    else if (ch->alignment > 700)
		sprintf(buf, "&Ynoble");
    else if (ch->alignment > 350)
		sprintf(buf, "&Yhonorable");
    else if (ch->alignment > 100)
		sprintf(buf, "&Yworthy");
    else if (ch->alignment > -100)
		sprintf(buf, "&Wneutral");
    else if (ch->alignment > -350)
		sprintf(buf, "&Rbase");
    else if (ch->alignment > -700)
		sprintf(buf, "&Revil");
    else if (ch->alignment > -900)
		sprintf(buf, "&Rignoble");
    else
		sprintf(buf, "&Rfiendish");

	/*if (ch->level < 50)
		pager_printf(ch,"                   Align: &W%-12s&C               Items: &W%3d&C  (&wmax %3d&C)\n\r",
			buf, ch->carry_number, can_carry_n(ch));
	else*/
		pager_printf(ch,"                   Align: &W%+4.4d, %-12s&C        Items: &W%3d&C  (&wmax %3d&C)\n\r",
			ch->alignment, buf, ch->carry_number, can_carry_n(ch));

    switch (ch->position)
    {
	case POS_DEAD:
		sprintf(buf, "slowly decomposing");
		break;
	case POS_MORTAL:
		sprintf(buf, "mortally wounded");
		break;
	case POS_INCAP:
		sprintf(buf, "incapacitated");
		break;
	case POS_STUNNED:
		sprintf(buf, "stunned");
		break;
	case POS_SLEEPING:
		sprintf(buf, "sleeping");
		break;
	case POS_RESTING:
		sprintf(buf, "resting");
		break;
	case POS_STANDING:
		sprintf(buf, "standing");
		break;
	case POS_FIGHTING:
		sprintf(buf, "fighting");
		break;
        case POS_EVASIVE:
                sprintf(buf, "fighting (evasive)");   /* Fighting style support -haus */
                break;
        case POS_DEFENSIVE:
                sprintf(buf, "fighting (defensive)");
                break;
        case POS_AGGRESSIVE:
                sprintf(buf, "fighting (aggressive)");
                break;
        case POS_BERSERK:
                sprintf(buf, "fighting (berserk)");
                break;
	case POS_MOUNTED:
		sprintf(buf, "mounted");
		break;
        case POS_SITTING:
		sprintf(buf, "sitting");
		break;
    }
	pager_printf(ch,"Rpp: %3d/%-3d       Pos'n: &W%s&C                Weight: &W%3d&C  (&wmax %3d&C)\n\r",
		ch->pcdata->quest_curr, ch->pcdata->quest_accum,buf, ch->carry_weight, can_carry_w(ch));

	switch (ch->style)
	{
	case STYLE_EVASIVE:
        sprintf(buf, "evasive");
        break;
	case STYLE_DEFENSIVE:
        sprintf(buf, "defensive");
        break;
	case STYLE_AGGRESSIVE:
        sprintf(buf, "aggressive");
        break;
	case STYLE_BERSERK:
        sprintf(buf, "berserk");
        break;
	default:
        sprintf(buf, "standard");
        break;
	}

	pager_printf(ch,"                   Wimpy: &R%-5d&C                    Style: &W%-10.10s&C\n\r",
		ch->wimpy, buf);

    pager_printf(ch, "&b-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=&C\n\r");

	if (is_android(ch))
	{
    pager_printf_color(ch, "UPGRD: &G%-3d&C        &YDamage&C: [&Y%3d&C]%c              AutoCompass: (&W%c&C) AutoZeni: (&W%c&C)\n\r",
	ch->train, ch->hit, '%',
	xIS_SET(ch->act, PLR_AUTO_COMPASS) ? 'X' : ' ',
	xIS_SET(ch->act, PLR_AUTOGOLD) ? 'X' : ' ');
	}
    else
    {
    pager_printf_color(ch, "TRAIN: &G%-3d&C     &YLifeforce&C: [&Y%3d&C]%c              AutoCompass: (&W%c&C) AutoZeni: (&W%c&C)\n\r",
	ch->train, ch->hit, '%',
	xIS_SET(ch->act, PLR_AUTO_COMPASS) ? 'X' : ' ',
	xIS_SET(ch->act, PLR_AUTOGOLD) ? 'X' : ' ');
	}

	strcpy(buf, num_punct(ch->mana));
	strcpy(buf2, num_punct(ch->max_mana));

	if (is_android(ch))
	{
	if (ch->max_mana < 1000)
	pager_printf_color(ch, "                  &YEnergy&C: [&Y%3s&C/&Y%-3s&C]              AutoExit: (&W%c&C) AutoLoot: (&W%c&C)\n\r",
		buf, buf2, xIS_SET(ch->act, PLR_AUTOEXIT) ? 'X' : ' ', xIS_SET(ch->act, PLR_AUTOLOOT) ? 'X' : ' ');
	else if (ch->max_mana < 10000)
	pager_printf_color(ch, "                  &YEnergy&C: [&Y%5s&C/&Y%-5s&C]          AutoExit: (&W%c&C) AutoLoot: (&W%c&C)\n\r",
		buf, buf2, xIS_SET(ch->act, PLR_AUTOEXIT) ? 'X' : ' ', xIS_SET(ch->act, PLR_AUTOLOOT) ? 'X' : ' ');
	else
	pager_printf_color(ch, "                  &YEnergy&C: [&Y%6s&C/&Y%-6s&C]        AutoExit: (&W%c&C) AutoLoot: (&W%c&C)\n\r",
		buf, buf2, xIS_SET(ch->act, PLR_AUTOEXIT) ? 'X' : ' ', xIS_SET(ch->act, PLR_AUTOLOOT) ? 'X' : ' ');
	}
	else
	{
	if (ch->max_mana < 1000)
	pager_printf_color(ch, "                  &YEnergy&C: [&Y%3s&C/&Y%-3s&C]              AutoExit: (&W%c&C) AutoLoot: (&W%c&C)\n\r",
		buf, buf2, xIS_SET(ch->act, PLR_AUTOEXIT) ? 'X' : ' ', xIS_SET(ch->act, PLR_AUTOLOOT) ? 'X' : ' ');
	else if (ch->max_mana < 10000)
	pager_printf_color(ch, "                  &YEnergy&C: [&Y%5s&C/&Y%-5s&C]          AutoExit: (&W%c&C) AutoLoot: (&W%c&C)\n\r",
		buf, buf2, xIS_SET(ch->act, PLR_AUTOEXIT) ? 'X' : ' ', xIS_SET(ch->act, PLR_AUTOLOOT) ? 'X' : ' ');
	else
	pager_printf_color(ch, "                  &YEnergy&C: [&Y%6s&C/&Y%-6s&C]        AutoExit: (&W%c&C) AutoLoot: (&W%c&C)\n\r",
		buf, buf2, xIS_SET(ch->act, PLR_AUTOEXIT) ? 'X' : ' ', xIS_SET(ch->act, PLR_AUTOLOOT) ? 'X' : ' ');
	}

	pager_printf_color(ch, "ZENI : &Y%-13s&C                                            AutoSac: (&W%c&C)\n\r",
		num_punct(ch->gold), xIS_SET(ch->act, PLR_AUTOSAC) ? 'X' : ' ');

	if (is_android(ch) || is_superandroid(ch) )
	{
	pager_printf(ch, "BASE TECHLEVEL: %-16s\n\r", num_punct_ld(ch->exp));

	if (ch->pl != ch->exp) {
		pager_printf(ch, "&YCURR TECHLEVEL: %-16s&C\n\r", num_punct_ld(ch->pl));
	}
	else
		pager_printf(ch, "CURR TECHLEVEL: %-16s\n\r", num_punct_ld(ch->pl));
	}
	else
	{
	pager_printf(ch, "BASE POWERLEVEL: %-16s\n\r", num_punct_ld(ch->exp));

	if (ch->pl != ch->exp)
        {
		pager_printf(ch, "&YCURR POWERLEVEL: %-16s&C\n\r", num_punct_ld(ch->pl));
	}
	else
		pager_printf(ch, "CURR POWERLEVEL: %-16s\n\r", num_punct_ld(ch->pl));
	}
	long double pltrack = 0;
	pltrack = ch->exp - ch->logon_start;
	if( pltrack < 0 )
	  pltrack = 0;
	if( is_android(ch) || is_superandroid(ch) )
	  pager_printf(ch, "&GGAINED TL SINCE LOGON: %-16s&C\n\r",
                num_punct_ld(pltrack));
	else
	  pager_printf(ch, "&GGAINED PL SINCE LOGON: %-16s&C\n\r",
		num_punct_ld(pltrack));

	pager_printf(ch,"                          PKills: [&R%5.5d&C] PDeaths: [&R%5.5d&C] SparWins: [&R%5.5d&C]\n\r",
		ch->pcdata->pkills, ch->pcdata->pdeaths, ch->pcdata->spar_wins);
	pager_printf(ch,"                          MKills: [&R%5.5d&C] MDeaths: [&R%5.5d&C] SparLoss: [&R%5.5d&C]\n\r",
		ch->pcdata->mkills, ch->pcdata->mdeaths, ch->pcdata->spar_loss);

	if (is_android(ch) && ch->pcdata->learned[gsn_self_destruct] > 0)
		pager_printf_color(ch, "                                              Self Destruct Charges: [&R%5.5d&C]\n\r",
			ch->pcdata->sd_charge);
	if (is_android_e(ch) && ch->pcdata->learned[gsn_battery] > 0)
		pager_printf_color(ch, "                                                   Stored Ki Energy: [&R%3.3d&C]\n\r",
			ch->battery );

    if (!IS_NPC(ch) && ch->pcdata->condition[COND_DRUNK] > 10)
	send_to_pager("You are drunk.\n\r", ch);
    if (!IS_NPC(ch) && ch->pcdata->condition[COND_THIRST] == 0)
	send_to_pager("You are in danger of dehydrating.\n\r", ch);
    if (!IS_NPC(ch) && ch->pcdata->condition[COND_FULL] == 0)
	send_to_pager("You are starving to death.\n\r", ch);
    if ( ch->position != POS_SLEEPING )
	switch( ch->mental_state / 10 )
	{
	    default:   send_to_pager( "You're completely messed up!\n\r", ch );	break;
	    case -10:  send_to_pager( "You're barely conscious.\n\r", ch );	break;
	    case  -9:  send_to_pager( "You can barely keep your eyes open.\n\r", ch );	break;
	    case  -8:  send_to_pager( "You're extremely drowsy.\n\r", ch );	break;
	    case  -7:  send_to_pager( "You feel very unmotivated.\n\r", ch );	break;
	    case  -6:  send_to_pager( "You feel sedated.\n\r", ch );		break;
	    case  -5:  send_to_pager( "You feel sleepy.\n\r", ch );		break;
	    case  -4:  send_to_pager( "You feel tired.\n\r", ch );		break;
	    case  -3:  send_to_pager( "You could use a rest.\n\r", ch );		break;
	    case  -2:  send_to_pager( "You feel a little under the weather.\n\r", ch );	break;
	    case  -1:  send_to_pager( "You feel fine.\n\r", ch );		break;
	    case   0:  send_to_pager( "You feel great.\n\r", ch );		break;
	    case   1:  send_to_pager( "You feel energetic.\n\r", ch );	break;
	    case   2:  send_to_pager( "Your mind is racing.\n\r", ch );	break;
	    case   3:  send_to_pager( "You can't think straight.\n\r", ch );	break;
	    case   4:  send_to_pager( "Your mind is going 100 miles an hour.\n\r", ch );	break;
	    case   5:  send_to_pager( "You're high as a kite.\n\r", ch );	break;
	    case   6:  send_to_pager( "Your mind and body are slipping apart.\n\r", ch );	break;
	    case   7:  send_to_pager( "Reality is slipping away.\n\r", ch );	break;
	    case   8:  send_to_pager( "You have no idea what is real, and what is not.\n\r", ch );	break;
	    case   9:  send_to_pager( "You feel immortal.\n\r", ch );	break;
	    case  10:  send_to_pager( "You are a Supreme Entity.\n\r", ch );	break;
	}
    else
    if ( ch->mental_state >45 )
	send_to_pager( "Your sleep is filled with strange and vivid dreams.\n\r", ch );
    else
    if ( ch->mental_state >25 )
	send_to_pager( "Your sleep is uneasy.\n\r", ch );
    else
    if ( ch->mental_state <-35 )
	send_to_pager( "You are deep in a much needed sleep.\n\r", ch );
    else
    if ( ch->mental_state <-25 )
	send_to_pager( "You are in deep slumber.\n\r", ch );
    send_to_pager("&c----------------------------------------------------------------------------&C\n\r", ch);

    if ( ch->pcdata->bestowments && ch->pcdata->bestowments[0] != '\0' )
	pager_printf( ch, "You are bestowed with the command(s): %s.\n\r",
		ch->pcdata->bestowments );

    if ( ch->morph && ch->morph->morph )
    {
      if ( IS_IMMORTAL( ch ) )
         pager_printf (ch, "Morphed as (%d) %s with a timer of %d.\n\r",
                ch->morph->morph->vnum, ch->morph->morph->short_desc,
		ch->morph->timer
                );
      else
        pager_printf (ch, "You are morphed into a %s.\n\r",
                ch->morph->morph->short_desc );
      send_to_pager ("&c----------------------------------------------------------------------------&C\n\r", ch);
    }
    if (ch->pcdata->clan)
    {
		pager_printf(ch, "CLAN NAME:  %s%-s\n\r", color_clan(ch), ch->pcdata->clan->name);
		pager_printf(ch, "&c Current Rank: (&w%d&c)&W%-20s  &cTotal Clan Members: (&w%d&c)\n\r", ch->pcdata->clanRank, get_clanTitle(ch), ch->pcdata->clan->members);
		pager_printf(ch, "&c Total Zeni Donated: (&W%s&c) Total Zeni Taxed: (&W%s&c)\n\r", num_punct_d(ch->pcdata->clanZeniDonated), num_punct_d(ch->pcdata->clanZeniClanTax));
		send_to_pager ("&c----------------------------------------------------------------------------&C\n\r", ch);
    }
    if (ch->pcdata->deity)
    {
	if (ch->pcdata->favor > 2250)
	  sprintf( buf, "loved" );
	else if (ch->pcdata->favor > 2000)
	  sprintf( buf, "cherished" );
	else if (ch->pcdata->favor > 1750)
	  sprintf( buf, "honored" );
	else if (ch->pcdata->favor > 1500)
	  sprintf( buf, "praised" );
	else if (ch->pcdata->favor > 1250)
	  sprintf( buf, "favored" );
	else if (ch->pcdata->favor > 1000)
	  sprintf( buf, "respected" );
	else if (ch->pcdata->favor > 750)
	  sprintf( buf, "liked" );
	else if (ch->pcdata->favor > 250)
	  sprintf( buf, "tolerated" );
	else if (ch->pcdata->favor > -250)
	  sprintf( buf, "ignored" );
	else if (ch->pcdata->favor > -750)
	  sprintf( buf, "shunned" );
	else if (ch->pcdata->favor > -1000)
	  sprintf( buf, "disliked" );
	else if (ch->pcdata->favor > -1250)
	  sprintf( buf, "dishonored" );
	else if (ch->pcdata->favor > -1500)
	  sprintf( buf, "disowned" );
	else if (ch->pcdata->favor > -1750)
	  sprintf( buf, "abandoned" );
	else if (ch->pcdata->favor > -2000)
	  sprintf( buf, "despised" );
	else if (ch->pcdata->favor > -2250)
	  sprintf( buf, "hated" );
	else
	  sprintf( buf, "damned" );
	pager_printf(ch, "Deity:  %-20s  Favor: %s\n\r", ch->pcdata->deity->name, buf );
      send_to_pager ("&c----------------------------------------------------------------------------\n\r", ch);
    }
    if (ch->pcdata->clan && ch->pcdata->clan->clan_type == CLAN_ORDER )
    {
	pager_printf(ch, "Order:  %-20s  Order Mkills:  %-6d   Order MDeaths:  %-6d\n\r",
		ch->pcdata->clan->name, ch->pcdata->clan->mkills, ch->pcdata->clan->mdeaths);
      send_to_pager ("&c----------------------------------------------------------------------------\n\r", ch);
    }
    if (ch->pcdata->clan && ch->pcdata->clan->clan_type == CLAN_GUILD )
    {
        pager_printf(ch, "Guild:  %-20s  Guild Mkills:  %-6d   Guild MDeaths:  %-6d\n\r",
                ch->pcdata->clan->name, ch->pcdata->clan->mkills, ch->pcdata->clan->mdeaths);
      send_to_pager ("&c----------------------------------------------------------------------------\n\r", ch);
    }
    if (IS_IMMORTAL(ch))
    {

	pager_printf(ch, "IMMORTAL DATA:  Wizinvis [%s]  Wizlevel (%d)\n\r",
		xIS_SET(ch->act, PLR_WIZINVIS) ? "X" : " ", ch->pcdata->wizinvis );

	pager_printf(ch, "Bamfin:  %s %s\n\r", ch->name, (ch->pcdata->bamfin[0] != '\0')
		? ch->pcdata->bamfin : "appears in a swirling mist.");
	pager_printf(ch, "Bamfout: %s %s\n\r", ch->name, (ch->pcdata->bamfout[0] != '\0')
		? ch->pcdata->bamfout : "leaves in a swirling mist.");


	if (ch->pcdata->area)
	{
	    pager_printf(ch, "Vnums:   Room (%-5.5d - %-5.5d)   Object (%-5.5d - %-5.5d)   Mob (%-5.5d - %-5.5d)\n\r",
		ch->pcdata->area->low_r_vnum, ch->pcdata->area->hi_r_vnum,
		ch->pcdata->area->low_o_vnum, ch->pcdata->area->hi_o_vnum,
		ch->pcdata->area->low_m_vnum, ch->pcdata->area->hi_m_vnum);
	    pager_printf(ch, "Area Loaded [%s]\n\r", (IS_SET (ch->pcdata->area->status, AREA_LOADED)) ? "yes" : "no");
	}
      send_to_pager ("&c----------------------------------------------------------------------------\n\r", ch);
    }
    if (ch->first_affect)
    {
	int i;
	SKILLTYPE *sktmp;

	i = 0;
	send_to_pager("AFFECT DATA:                            ", ch);
	for (paf = ch->first_affect; paf; paf = paf->next)
	{
	    if ( (sktmp=get_skilltype(paf->type)) == NULL )
		continue;
	    if (ch->level < 20)
	    {
		pager_printf(ch, "[%-34.34s]    ", sktmp->name);
		if (i == 0)
		   i = 2;
		if ((++i % 3) == 0)
		   send_to_pager("\n\r", ch);
	     }
	     if (ch->level >= 20)
	     {
		if (paf->modifier == 0)
		    pager_printf(ch, "[%-24.24s;%5d rds]    ",
			sktmp->name,
			paf->duration);
		else
		if (paf->modifier > 999)
		    pager_printf(ch, "[%-15.15s; %7.7s;%5d rds]    ",
			sktmp->name,
			tiny_affect_loc_name(paf->location),
			paf->duration);
		else
		    pager_printf(ch, "[%-11.11s;%+-3.3d %7.7s;%5d rds]    ",
			sktmp->name,
			paf->modifier,
			tiny_affect_loc_name(paf->location),
			paf->duration);
		if (i == 0)
		    i = 1;
		if ((++i % 2) == 0)
		    send_to_pager("\n\r", ch);
	    }
	}
      send_to_pager ("&c----------------------------------------------------------------------------\n\r", ch);
    }
    send_to_pager("&w&D\n\r", ch);
	sysdata.outBytesFlag = LOGBOUTNORM;
    return;
}


void do_newscore(CHAR_DATA * ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    AFFECT_DATA    *paf;

    if (IS_NPC(ch))
    {
	do_oldscore(ch, argument);
	return;
    }
    set_pager_color(AT_SCORE, ch);

    pager_printf_color(ch, "\n\r&C%s%s.\n\r", ch->name, ch->pcdata->title);
    if ( get_trust( ch ) != ch->level )
	pager_printf( ch, "You are trusted at level %d.\n\r", get_trust( ch ) );

    send_to_pager_color("&W----------------------------------------------------------------------------\n\r", ch);

    pager_printf_color(ch, "Level: &W%-3d         &CRace : &W%-10.10s        &CPlayed: &W%d &Chours\n\r",
	ch->level, capitalize(get_race(ch)), (get_age(ch) - 4) * 2);
    pager_printf_color(ch, "&CYears: &W%-6d      &CClass: &W%-11.11s       &CLog In: %s\r",
		get_age(ch), capitalize(get_class(ch)), ctime(&(ch->logon)) );

    if (ch->level >= 15
    ||  IS_PKILL( ch ) )
    {
	pager_printf_color(ch, "&CSTR  : &W%2.2d&C(&w%2.2d&C)    HitRoll: &R%-4d               &CSaved: %s\r",
		get_curr_str(ch), ch->perm_str, GET_HITROLL(ch), ch->save_time ? ctime(&(ch->save_time)) : "no save this session\n" );

	pager_printf_color(ch, "&CINT  : &W%2.2d&C(&w%2.2d&C)    DamRoll: &R%-4d                &CTime: %s\r",
		get_curr_int(ch), ch->perm_int, GET_DAMROLL(ch), ctime(&current_time) );
    }
    else
    {
	pager_printf_color(ch, "&CSTR  : &W%2.2d&C(&w%2.2d&C)                               Saved:  %s\r",
		get_curr_str(ch), ch->perm_str, ch->save_time ? ctime(&(ch->save_time)) : "no\n" );

	pager_printf_color(ch, "&CINT  : &W%2.2d&C(&w%2.2d&C)                               Time:   %s\r",
		get_curr_int(ch), ch->perm_int, ctime(&current_time) );
    }

    if (GET_AC(ch) >= 101)
	sprintf(buf, "the rags of a beggar");
    else if (GET_AC(ch) >= 80)
	sprintf(buf, "improper for adventure");
    else if (GET_AC(ch) >= 55)
	sprintf(buf, "shabby and threadbare");
    else if (GET_AC(ch) >= 40)
	sprintf(buf, "of poor quality");
    else if (GET_AC(ch) >= 20)
	sprintf(buf, "scant protection");
    else if (GET_AC(ch) >= 10)
	sprintf(buf, "that of a knave");
    else if (GET_AC(ch) >= 0)
	sprintf(buf, "moderately crafted");
    else if (GET_AC(ch) >= -10)
	sprintf(buf, "well crafted");
    else if (GET_AC(ch) >= -20)
	sprintf(buf, "the envy of squires");
    else if (GET_AC(ch) >= -40)
	sprintf(buf, "excellently crafted");
    else if (GET_AC(ch) >= -60)
	sprintf(buf, "the envy of knights");
    else if (GET_AC(ch) >= -80)
	sprintf(buf, "the envy of barons");
    else if (GET_AC(ch) >= -100)
	sprintf(buf, "the envy of dukes");
    else if (GET_AC(ch) >= -200)
	sprintf(buf, "the envy of emperors");
    else
	sprintf(buf, "that of an avatar");
    if (ch->level > 24)
	pager_printf_color(ch, "&CWIS  : &W0&C(&w0&C)      Armor: &W%-d; %s\n\r",
		GET_AC(ch), buf);
    else
	pager_printf_color(ch, "&CWIS  : &W0&C(&w0&C)      Armor: &W%s \n\r",
		buf);

    if (ch->alignment > 900)
	sprintf(buf, "devout");
    else if (ch->alignment > 700)
	sprintf(buf, "noble");
    else if (ch->alignment > 350)
	sprintf(buf, "honorable");
    else if (ch->alignment > 100)
	sprintf(buf, "worthy");
    else if (ch->alignment > -100)
	sprintf(buf, "neutral");
    else if (ch->alignment > -350)
	sprintf(buf, "base");
    else if (ch->alignment > -700)
	sprintf(buf, "evil");
    else if (ch->alignment > -900)
	sprintf(buf, "ignoble");
    else
	sprintf(buf, "fiendish");
    if (ch->level < 10)
	pager_printf_color(ch, "&CDEX  : &W%2.2d&C(&w%2.2d&C)      Align: &W%-20.20s    &CItems:  &W%d (max %d)\n\r",
		get_curr_dex(ch), ch->perm_dex, buf, ch->carry_number, can_carry_n(ch));
    else
	pager_printf_color(ch, "&CDEX  : &W%2.2d&C(&w%2.2d&C)      Align: &W%4d; %-14.14s   &CItems:  &W%d &w(max %d)\n\r",
		get_curr_dex(ch), ch->perm_dex, ch->alignment, buf, ch->carry_number, can_carry_n(ch));

    switch (ch->position)
    {
	case POS_DEAD:
		sprintf(buf, "slowly decomposing");
		break;
	case POS_MORTAL:
		sprintf(buf, "mortally wounded");
		break;
	case POS_INCAP:
		sprintf(buf, "incapacitated");
		break;
	case POS_STUNNED:
		sprintf(buf, "stunned");
		break;
	case POS_SLEEPING:
		sprintf(buf, "sleeping");
		break;
	case POS_RESTING:
		sprintf(buf, "resting");
		break;
	case POS_STANDING:
		sprintf(buf, "standing");
		break;
	case POS_FIGHTING:
		sprintf(buf, "fighting");
		break;
        case POS_EVASIVE:
                sprintf(buf, "fighting (evasive)");   /* Fighting style support -haus */
                break;
        case POS_DEFENSIVE:
                sprintf(buf, "fighting (defensive)");
                break;
        case POS_AGGRESSIVE:
                sprintf(buf, "fighting (aggressive)");
                break;
        case POS_BERSERK:
                sprintf(buf, "fighting (berserk)");
                break;
	case POS_MOUNTED:
		sprintf(buf, "mounted");
		break;
        case POS_SITTING:
		sprintf(buf, "sitting");
		break;
    }
    pager_printf_color(ch, "&CCON  : &W%2.2d&C(&w%2.2d&C)      Pos'n: &W%-21.21s  &CWeight: &W%d &w(max %d)\n\r",
	get_curr_con(ch), ch->perm_con, buf, ch->carry_weight, can_carry_w(ch));


    /*
     * Fighting style support -haus
     */
    pager_printf_color(ch, "&CCHA  : &W0&C(&w0&C)      Wimpy: &Y%-5d      ",
	ch->wimpy);

        switch (ch->style) {
        case STYLE_EVASIVE:
                sprintf(buf, "evasive");
                break;
        case STYLE_DEFENSIVE:
                sprintf(buf, "defensive");
                break;
        case STYLE_AGGRESSIVE:
                sprintf(buf, "aggressive");
                break;
        case STYLE_BERSERK:
                sprintf(buf, "berserk");
                break;
        default:
                sprintf(buf, "standard");
                break;
        }
    pager_printf_color(ch, "\n\r&CLCK  : &W%2.2d&C(&w%2.2d&C)      Style: &W%-10.10s\n\r",
	get_curr_lck(ch), ch->perm_lck, buf );

    pager_printf_color(ch, "&CGlory: &W%d&C/&w%d\n\r",
	ch->pcdata->quest_curr, ch->pcdata->quest_accum );

    pager_printf_color(ch, "&CPRACT: &W%3d         &CHitpoints: &G%-5d &Cof &g%5d   &CPager: (&W%c&C) &W%3d    &CAutoExit(&W%c&C)\n\r",
	ch->practice, ch->hit, ch->max_hit,
	IS_SET(ch->pcdata->flags, PCFLAG_PAGERON) ? 'X' : ' ',
	ch->pcdata->pagerlen, xIS_SET(ch->act, PLR_AUTOEXIT) ? 'X' : ' ');

	pager_printf_color(ch, "&CEXP  : &W%-9d        &CMana: &B%-5d &Cof &b%5d   &CMKills:  &W%5d    &CAutoLoot(&W%c&C)\n\r",
		ch->exp, ch->mana, ch->max_mana, ch->pcdata->mkills, xIS_SET(ch->act, PLR_AUTOLOOT) ? 'X' : ' ');

    pager_printf_color(ch, "&CGOLD : &Y%-13s    &CMove: &W%-5d &Cof &w%5d   &CMdeaths: &W%5d    &CAutoSac (&W%c&C)\n\r",
	num_punct(ch->gold), ch->move, ch->max_move, ch->pcdata->mdeaths, xIS_SET(ch->act, PLR_AUTOSAC) ? 'X' : ' ');

    if (!IS_NPC(ch) && ch->pcdata->condition[COND_DRUNK] > 10)
	send_to_pager("You are drunk.\n\r", ch);
    if (!IS_NPC(ch) && ch->pcdata->condition[COND_THIRST] == 0)
	send_to_pager("You are in danger of dehydrating.\n\r", ch);
    if (!IS_NPC(ch) && ch->pcdata->condition[COND_FULL] == 0)
	send_to_pager("You are starving to death.\n\r", ch);
    if ( ch->position != POS_SLEEPING )
	switch( ch->mental_state / 10 )
	{
	    default:   send_to_pager( "You're completely messed up!\n\r", ch );	break;
	    case -10:  send_to_pager( "You're barely conscious.\n\r", ch );	break;
	    case  -9:  send_to_pager( "You can barely keep your eyes open.\n\r", ch );	break;
	    case  -8:  send_to_pager( "You're extremely drowsy.\n\r", ch );	break;
	    case  -7:  send_to_pager( "You feel very unmotivated.\n\r", ch );	break;
	    case  -6:  send_to_pager( "You feel sedated.\n\r", ch );		break;
	    case  -5:  send_to_pager( "You feel sleepy.\n\r", ch );		break;
	    case  -4:  send_to_pager( "You feel tired.\n\r", ch );		break;
	    case  -3:  send_to_pager( "You could use a rest.\n\r", ch );		break;
	    case  -2:  send_to_pager( "You feel a little under the weather.\n\r", ch );	break;
	    case  -1:  send_to_pager( "You feel fine.\n\r", ch );		break;
	    case   0:  send_to_pager( "You feel great.\n\r", ch );		break;
	    case   1:  send_to_pager( "You feel energetic.\n\r", ch );	break;
	    case   2:  send_to_pager( "Your mind is racing.\n\r", ch );	break;
	    case   3:  send_to_pager( "You can't think straight.\n\r", ch );	break;
	    case   4:  send_to_pager( "Your mind is going 100 miles an hour.\n\r", ch );	break;
	    case   5:  send_to_pager( "You're high as a kite.\n\r", ch );	break;
	    case   6:  send_to_pager( "Your mind and body are slipping apart.\n\r", ch );	break;
	    case   7:  send_to_pager( "Reality is slipping away.\n\r", ch );	break;
	    case   8:  send_to_pager( "You have no idea what is real, and what is not.\n\r", ch );	break;
	    case   9:  send_to_pager( "You feel immortal.\n\r", ch );	break;
	    case  10:  send_to_pager( "You are a Supreme Entity.\n\r", ch );	break;
	}
    else
    if ( ch->mental_state >45 )
	send_to_pager( "Your sleep is filled with strange and vivid dreams.\n\r", ch );
    else
    if ( ch->mental_state >25 )
	send_to_pager( "Your sleep is uneasy.\n\r", ch );
    else
    if ( ch->mental_state <-35 )
	send_to_pager( "You are deep in a much needed sleep.\n\r", ch );
    else
    if ( ch->mental_state <-25 )
	send_to_pager( "You are in deep slumber.\n\r", ch );
/*  send_to_pager("Languages: ", ch );
    for ( iLang = 0; lang_array[iLang] != LANG_UNKNOWN; iLang++ )
	if ( knows_language( ch, lang_array[iLang], ch )
	||  (IS_NPC(ch) && ch->speaks == 0) )
	{
	    if ( lang_array[iLang] & ch->speaking
	    ||  (IS_NPC(ch) && !ch->speaking) )
		set_pager_color( AT_RED, ch );
	    send_to_pager( lang_names[iLang], ch );
	    send_to_pager( " ", ch );
	    set_pager_color( AT_SCORE, ch );
	}
    send_to_pager( "\n\r", ch );
*/
    if ( ch->pcdata->bestowments && ch->pcdata->bestowments[0] != '\0' )
	pager_printf_color(ch, "&CYou are bestowed with the command(s): &Y%s\n\r",
		ch->pcdata->bestowments );

    if ( ch->morph && ch->morph->morph )
    {
      send_to_pager_color("&W----------------------------------------------------------------------------&C\n\r", ch);
      if ( IS_IMMORTAL( ch ) )
         pager_printf (ch, "Morphed as (%d) %s with a timer of %d.\n\r",
                ch->morph->morph->vnum, ch->morph->morph->short_desc,
		ch->morph->timer
                );
      else
        pager_printf (ch, "You are morphed into a %s.\n\r",
                ch->morph->morph->short_desc );
      send_to_pager_color("&W----------------------------------------------------------------------------&C\n\r", ch);
    }
    if ( CAN_PKILL( ch ) )
    {
	send_to_pager_color("&W----------------------------------------------------------------------------&C\n\r", ch);
	pager_printf_color(ch, "&CPKILL DATA:  Pkills (&W%d&C)     Illegal Pkills (&W%d&C)     Pdeaths (&W%d&C)\n\r",
		ch->pcdata->pkills, ch->pcdata->illegal_pk, ch->pcdata->pdeaths );
    }
    if (ch->pcdata->clan && ch->pcdata->clan->clan_type != CLAN_ORDER  && ch->pcdata->clan->clan_type != CLAN_GUILD )
    {
/*
	send_to_pager_color( "&W----------------------------------------------------------------------------&C\n\r", ch);
*/
	pager_printf_color(ch, "&CCLAN STATS:  &W%-14.14s  &CClan AvPkills : &W%-5d  &CClan NonAvpkills : &W%-5d\n\r",
		ch->pcdata->clan->name, ch->pcdata->clan->pkills[5],
		(ch->pcdata->clan->pkills[0]+ch->pcdata->clan->pkills[1]+
		 ch->pcdata->clan->pkills[2]+ch->pcdata->clan->pkills[3]+
		 ch->pcdata->clan->pkills[4]) );
        pager_printf_color(ch, "&C                             Clan AvPdeaths: &W%-5d  &CClan NonAvpdeaths: &W%-5d\n\r",
		ch->pcdata->clan->pdeaths[5],
		( ch->pcdata->clan->pdeaths[0] + ch->pcdata->clan->pdeaths[1] +
		  ch->pcdata->clan->pdeaths[2] + ch->pcdata->clan->pdeaths[3] +
		  ch->pcdata->clan->pdeaths[4] ) );
    }
    if (ch->pcdata->deity)
    {
	send_to_pager_color( "&W----------------------------------------------------------------------------&C\n\r", ch);
	if (ch->pcdata->favor > 2250)
	  sprintf( buf, "loved" );
	else if (ch->pcdata->favor > 2000)
	  sprintf( buf, "cherished" );
	else if (ch->pcdata->favor > 1750)
	  sprintf( buf, "honored" );
	else if (ch->pcdata->favor > 1500)
	  sprintf( buf, "praised" );
	else if (ch->pcdata->favor > 1250)
	  sprintf( buf, "favored" );
	else if (ch->pcdata->favor > 1000)
	  sprintf( buf, "respected" );
	else if (ch->pcdata->favor > 750)
	  sprintf( buf, "liked" );
	else if (ch->pcdata->favor > 250)
	  sprintf( buf, "tolerated" );
	else if (ch->pcdata->favor > -250)
	  sprintf( buf, "ignored" );
	else if (ch->pcdata->favor > -750)
	  sprintf( buf, "shunned" );
	else if (ch->pcdata->favor > -1000)
	  sprintf( buf, "disliked" );
	else if (ch->pcdata->favor > -1250)
	  sprintf( buf, "dishonored" );
	else if (ch->pcdata->favor > -1500)
	  sprintf( buf, "disowned" );
	else if (ch->pcdata->favor > -1750)
	  sprintf( buf, "abandoned" );
	else if (ch->pcdata->favor > -2000)
	  sprintf( buf, "despised" );
	else if (ch->pcdata->favor > -2250)
	  sprintf( buf, "hated" );
	else
	  sprintf( buf, "damned" );
	pager_printf_color(ch, "&CDeity:  &W%-20s &CFavor:  &W%s&C\n\r", ch->pcdata->deity->name, buf );
    }
    if (ch->pcdata->clan && ch->pcdata->clan->clan_type == CLAN_ORDER )
    {
        send_to_pager_color( "&W----------------------------------------------------------------------------&C\n\r", ch);
	pager_printf_color(ch, "&COrder:  &W%-20s  &COrder Mkills:  &W%-6d   &COrder MDeaths:  &W%-6d\n\r",
		ch->pcdata->clan->name, ch->pcdata->clan->mkills, ch->pcdata->clan->mdeaths);
    }
    if (ch->pcdata->clan && ch->pcdata->clan->clan_type == CLAN_GUILD )
    {
        send_to_pager_color( "&W----------------------------------------------------------------------------&C\n\r", ch);
        pager_printf_color(ch, "&CGuild:  &W%-20s  &CGuild Mkills:  &W%-6d   &CGuild MDeaths:  &W%-6d\n\r",
                ch->pcdata->clan->name, ch->pcdata->clan->mkills, ch->pcdata->clan->mdeaths);
    }
    argument = one_argument( argument, arg );
    if (ch->first_affect && !str_cmp( arg, "affects" ) )
    {
	int i;
	SKILLTYPE *sktmp;

	i = 0;
	send_to_pager_color( "&W----------------------------------------------------------------------------&C\n\r", ch);
	send_to_pager_color("AFFECT DATA:                            ", ch);
	for (paf = ch->first_affect; paf; paf = paf->next)
	{
	    if ( (sktmp=get_skilltype(paf->type)) == NULL )
		continue;
	    if (ch->level < 20)
	    {
		pager_printf_color(ch, "&C[&W%-34.34s&C]    ", sktmp->name);
		if (i == 0)
		   i = 2;
		if ((++i % 3) == 0)
		   send_to_pager("\n\r", ch);
	     }
	     if (ch->level >= 20)
	     {
		if (paf->modifier == 0)
		    pager_printf_color(ch, "&C[&W%-24.24s;%5d &Crds]    ",
			sktmp->name,
			paf->duration);
		else
		if (paf->modifier > 999)
		    pager_printf_color(ch, "&C[&W%-15.15s; %7.7s;%5d &Crds]    ",
			sktmp->name,
			tiny_affect_loc_name(paf->location),
			paf->duration);
		else
		    pager_printf_color(ch, "&C[&W%-11.11s;%+-3.3d %7.7s;%5d &Crds]    ",
			sktmp->name,
			paf->modifier,
			tiny_affect_loc_name(paf->location),
			paf->duration);
		if (i == 0)
		    i = 1;
		if ((++i % 2) == 0)
		    send_to_pager("\n\r", ch);
	    }
	}
    }
    send_to_pager("\n\r", ch);
    return;
}

/*
 * Return ascii name of an affect location.
 */
char           *
tiny_affect_loc_name(int location)
{
	switch (location) {
	case APPLY_NONE:		return "NIL";
	case APPLY_STR:			return " STR  ";
	case APPLY_DEX:			return " DEX  ";
	case APPLY_INT:			return " INT  ";
	case APPLY_CON:			return " CON  ";
	case APPLY_LCK:			return " LCK  ";
	case APPLY_ALLSTATS:			return " ALL STATS  ";
	case APPLY_SEX:			return " SEX  ";
	case APPLY_CLASS:		return " CLASS";
	case APPLY_LEVEL:		return " LVL  ";
	case APPLY_AGE:			return " AGE  ";
	case APPLY_MANA:		return " MANA ";
	case APPLY_HIT:			return " HV   ";
	case APPLY_MOVE:		return " MOVE ";
	case APPLY_GOLD:		return " GOLD ";
	case APPLY_EXP:			return " EXP  ";
	case APPLY_PL_MULT:			return " PL MULT   ";
	case APPLY_PL_PERCENT:			return " PL PERCENT  ";
	case APPLY_AC:			return " AC   ";
	case APPLY_HITROLL:		return " HITRL";
	case APPLY_DAMROLL:		return " DAMRL";
	case APPLY_SAVING_POISON:	return "SV POI";
	case APPLY_SAVING_ROD:		return "SV ROD";
	case APPLY_SAVING_PARA:		return "SV PARA";
	case APPLY_SAVING_BREATH:	return "SV BRTH";
	case APPLY_SAVING_SPELL:	return "SV SPLL";
	case APPLY_HEIGHT:		return "HEIGHT";
	case APPLY_WEIGHT:		return "WEIGHT";
	case APPLY_AFFECT:		return "AFF BY";
	case APPLY_RESISTANT:		return "RESIST";
	case APPLY_IMMUNE:		return "IMMUNE";
	case APPLY_SUSCEPTIBLE:		return "SUSCEPT";
	case APPLY_WEAPONSPELL:		return " WEAPON";
	case APPLY_BACKSTAB:		return "BACKSTB";
	case APPLY_PICK:		return " PICK  ";
	case APPLY_TRACK:		return " TRACK ";
	case APPLY_STEAL:		return " STEAL ";
	case APPLY_SNEAK:		return " SNEAK ";
	case APPLY_HIDE:		return " HIDE  ";
	case APPLY_PALM:		return " PALM  ";
	case APPLY_DETRAP:		return " DETRAP";
	case APPLY_DODGE:		return " DODGE ";
	case APPLY_PEEK:		return " PEEK  ";
	case APPLY_SCAN:		return " SCAN  ";
	case APPLY_GOUGE:		return " GOUGE ";
	case APPLY_SEARCH:		return " SEARCH";
	case APPLY_MOUNT:		return " MOUNT ";
	case APPLY_DISARM:		return " DISARM";
	case APPLY_KICK:		return " KICK  ";
	case APPLY_PARRY:		return " PARRY ";
	case APPLY_BASH:		return " BASH  ";
	case APPLY_STUN:		return " STUN  ";
	case APPLY_PUNCH:		return " PUNCH ";
	case APPLY_CLIMB:		return " CLIMB ";
	case APPLY_GRIP:		return " GRIP  ";
	case APPLY_SCRIBE:		return " SCRIBE";
	case APPLY_BREW:		return " BREW  ";
	case APPLY_WEARSPELL:		return " WEAR  ";
	case APPLY_REMOVESPELL:		return " REMOVE";
	case APPLY_EMOTION:		return "EMOTION";
	case APPLY_MENTALSTATE:		return " MENTAL";
	case APPLY_STRIPSN:		return " DISPEL";
	case APPLY_REMOVE:		return " REMOVE";
	case APPLY_DIG:			return " DIG   ";
	case APPLY_FULL:		return " HUNGER";
	case APPLY_THIRST:		return " THIRST";
	case APPLY_DRUNK:		return " DRUNK ";
	case APPLY_BLOOD:		return " BLOOD ";
	case APPLY_COOK:		return " COOK  ";
	case APPLY_RECURRINGSPELL:	return " RECURR";
	case APPLY_CONTAGIOUS:		return "CONTGUS";
	case APPLY_ODOR:		return " ODOR  ";
	case APPLY_ROOMFLAG:		return " RMFLG ";
	case APPLY_SECTORTYPE:		return " SECTOR";
	case APPLY_ROOMLIGHT:		return " LIGHT ";
	case APPLY_TELEVNUM:		return " TELEVN";
	case APPLY_TELEDELAY:		return " TELEDY";
	case APPLY_NATURALAC:		return " NTRLAC";
	};

	bug("Affect_location_name: unknown location %d.", location);
	return "(BUG: NOTIFY GOKU)";
}

char *
get_class(CHAR_DATA *ch)
{
    if ( IS_NPC(ch) && ch->class < MAX_NPC_CLASS && ch->class >= 0)
    	return ( npc_class[ch->class] );
    else if ( !IS_NPC(ch) && ch->class < MAX_PC_CLASS && ch->class >= 0 )
        return class_table[ch->class]->who_name;
    return ("Unknown");
}


char *
get_race( CHAR_DATA *ch)
{
    if(  ch->race < MAX_PC_RACE  && ch->race >= 0)
        return (race_table[ch->race]->race_name);
    if ( ch->race < MAX_NPC_RACE && ch->race >= 0)
	return ( npc_race[ch->race] );
    return ("Unknown");
}

char *
get_kai( CHAR_DATA *ch)
{
    switch( ch->kairank )
    {
	case 1:
	    return ("South Kai");
	    break;
	case 2:
            return ("East Kai");
            break;
	case 3:
            return ("West Kai");
            break;
	case 4:
            return ("North Kai");
            break;
	case 5:
            return ("Grand Kai");
            break;
	case 6:
            return ("Supreme Kai");
            break;
	default:
	    break;
    }
    return ("None");
}

char *
get_demon( CHAR_DATA *ch)
{
    switch( ch->demonrank )
    {
        case 1:
            return ("Greater Demon");
            break;
        case 2:
            return ("Demon Warlord");
            break;
        case 3:
            return ("Demon King");
            break;
    }
    return ("None");
}

int get_race_num( char *arg )
{
    int a = 0;
    for( a = 0; a < MAX_PC_RACE; a++ )
	if(!str_cmp(arg,race_table[a]->race_name) )
	  return a;

    for( a = 0; a < MAX_NPC_RACE; a++ )
	if(!str_cmp(arg,npc_race[a]) )
	  return a;

    return 0;
}

int get_class_num( char *arg )
{
    int a = 0;
    for( a = 0; a < MAX_PC_CLASS; a++ )
	if(!str_cmp(arg,class_table[a]->who_name) )
	  return a;

    for( a = 0; a < MAX_NPC_CLASS; a++ )
	if(!str_cmp(arg,npc_class[a]) )
	  return a;

    return 0;
}

void do_oldscore( CHAR_DATA *ch, char *argument )
{
    AFFECT_DATA *paf;
    SKILLTYPE   *skill;

	if (!IS_NPC(ch))
	{
		do_score(ch, argument);
		return;
	}

    set_pager_color( AT_SCORE, ch );
    pager_printf( ch,
	"You are %s%s&D&C, level %d, %d years old (%d hours).\n\r",
	ch->name,
	IS_NPC(ch) ? "" : ch->pcdata->title,
	ch->level,
	get_age(ch),
	(get_age(ch) - 4) * 2 );

    if ( get_trust( ch ) != ch->level )
	pager_printf( ch, "You are trusted at level %d.\n\r",
	    get_trust( ch ) );

    if (  IS_NPC(ch) && xIS_SET(ch->act, ACT_MOBINVIS) )
      pager_printf( ch, "You are mobinvis at level %d.\n\r",
            ch->mobinvis);

      pager_printf( ch,
	"You have %d/%d hit, %d/%d mana, %d/%d movement, %d practices.\n\r",
	ch->hit,  ch->max_hit,
	ch->mana, ch->max_mana,
	ch->move, ch->max_move,
	ch->practice );

    pager_printf( ch,
	"You are carrying %d/%d items with weight %d/%d kg.\n\r",
	ch->carry_number, can_carry_n(ch),
	ch->carry_weight, can_carry_w(ch) );

    pager_printf( ch,
	"Str: %d  Int: %d  Wis: 0  Dex: %d  Con: 0  Cha: %d  Lck: %d.\n\r",
	get_curr_str(ch),
	get_curr_int(ch),
	get_curr_dex(ch),
	get_curr_con(ch),
	get_curr_lck(ch) );

	pager_printf(ch, "You have scored %s exp, and have ",num_punct_ld(ch->exp));
	pager_printf(ch, "%s gold coins.\n\r", num_punct(ch->gold) );

    if ( !IS_NPC(ch) )
    pager_printf( ch,
	"You have achieved %d glory during your life, and currently have %d.\n\r",
	ch->pcdata->quest_accum, ch->pcdata->quest_curr );

    pager_printf( ch,
	"Autoexit: %s   Autoloot: %s   Autosac: %s   Autogold: %s\n\r",
	(!IS_NPC(ch) && xIS_SET(ch->act, PLR_AUTOEXIT)) ? "yes" : "no",
	(!IS_NPC(ch) && xIS_SET(ch->act, PLR_AUTOLOOT)) ? "yes" : "no",
	(!IS_NPC(ch) && xIS_SET(ch->act, PLR_AUTOSAC) ) ? "yes" : "no",
  	(!IS_NPC(ch) && xIS_SET(ch->act, PLR_AUTOGOLD)) ? "yes" : "no" );

    pager_printf( ch, "Wimpy set to %d hit points.\n\r", ch->wimpy );

    if ( !IS_NPC(ch) ) {
       if ( ch->pcdata->condition[COND_DRUNK]   > 10 )
	   send_to_pager( "You are drunk.\n\r",   ch );
       if ( ch->pcdata->condition[COND_THIRST] ==  0 )
	   send_to_pager( "You are thirsty.\n\r", ch );
       if ( ch->pcdata->condition[COND_FULL]   ==  0 )
	   send_to_pager( "You are hungry.\n\r",  ch );
    }

    switch( ch->mental_state / 10 )
    {
        default:   send_to_pager( "You're completely messed up!\n\r", ch ); break;
        case -10:  send_to_pager( "You're barely conscious.\n\r", ch ); break;
        case  -9:  send_to_pager( "You can barely keep your eyes open.\n\r", ch ); break;
        case  -8:  send_to_pager( "You're extremely drowsy.\n\r", ch ); break;
        case  -7:  send_to_pager( "You feel very unmotivated.\n\r", ch ); break;
        case  -6:  send_to_pager( "You feel sedated.\n\r", ch ); break;
        case  -5:  send_to_pager( "You feel sleepy.\n\r", ch ); break;
        case  -4:  send_to_pager( "You feel tired.\n\r", ch ); break;
        case  -3:  send_to_pager( "You could use a rest.\n\r", ch ); break;
        case  -2:  send_to_pager( "You feel a little under the weather.\n\r", ch ); break;
        case  -1:  send_to_pager( "You feel fine.\n\r", ch ); break;
        case   0:  send_to_pager( "You feel great.\n\r", ch ); break;
        case   1:  send_to_pager( "You feel energetic.\n\r", ch ); break;
        case   2:  send_to_pager( "Your mind is racing.\n\r", ch ); break;
        case   3:  send_to_pager( "You can't think straight.\n\r", ch ); break;
        case   4:  send_to_pager( "Your mind is going 100 miles an hour.\n\r", ch ); break;
        case   5:  send_to_pager( "You're high as a kite.\n\r", ch ); break;
        case   6:  send_to_pager( "Your mind and body are slipping appart.\n\r", ch ); break;
        case   7:  send_to_pager( "Reality is slipping away.\n\r", ch ); break;
        case   8:  send_to_pager( "You have no idea what is real, and what is not.\n\r", ch ); break;
        case   9:  send_to_pager( "You feel immortal.\n\r", ch ); break;
        case  10:  send_to_pager( "You are a Supreme Entity.\n\r", ch ); break;
    }

    switch ( ch->position )
    {
    case POS_DEAD:
	send_to_pager( "You are DEAD!!\n\r",		ch );
	break;
    case POS_MORTAL:
	send_to_pager( "You are mortally wounded.\n\r",	ch );
	break;
    case POS_INCAP:
	send_to_pager( "You are incapacitated.\n\r",	ch );
	break;
    case POS_STUNNED:
	send_to_pager( "You are stunned.\n\r",		ch );
	break;
    case POS_SLEEPING:
	send_to_pager( "You are sleeping.\n\r",		ch );
	break;
    case POS_RESTING:
	send_to_pager( "You are resting.\n\r",		ch );
	break;
    case POS_STANDING:
	send_to_pager( "You are standing.\n\r",		ch );
	break;
    case POS_FIGHTING:
	send_to_pager( "You are fighting.\n\r",		ch );
	break;
    case POS_MOUNTED:
	send_to_pager( "Mounted.\n\r",			ch );
	break;
    case POS_SHOVE:
	send_to_pager( "Being shoved.\n\r",		ch );
	break;
    case POS_DRAG:
	send_to_pager( "Being dragged.\n\r",		ch );
	break;
    }

    if ( ch->level >= 25 )
	pager_printf( ch, "AC: %d.  ", GET_AC(ch) );

    send_to_pager( "You are ", ch );
	 if ( GET_AC(ch) >=  101 ) send_to_pager( "WORSE than naked!\n\r", ch );
    else if ( GET_AC(ch) >=   80 ) send_to_pager( "naked.\n\r",            ch );
    else if ( GET_AC(ch) >=   60 ) send_to_pager( "wearing clothes.\n\r",  ch );
    else if ( GET_AC(ch) >=   40 ) send_to_pager( "slightly armored.\n\r", ch );
    else if ( GET_AC(ch) >=   20 ) send_to_pager( "somewhat armored.\n\r", ch );
    else if ( GET_AC(ch) >=    0 ) send_to_pager( "armored.\n\r",          ch );
    else if ( GET_AC(ch) >= - 20 ) send_to_pager( "well armored.\n\r",     ch );
    else if ( GET_AC(ch) >= - 40 ) send_to_pager( "strongly armored.\n\r", ch );
    else if ( GET_AC(ch) >= - 60 ) send_to_pager( "heavily armored.\n\r",  ch );
    else if ( GET_AC(ch) >= - 80 ) send_to_pager( "superbly armored.\n\r", ch );
    else if ( GET_AC(ch) >= -100 ) send_to_pager( "divinely armored.\n\r",
ch );
    else                           send_to_pager( "invincible!\n\r",       ch );

    if ( ch->level >= 15
    ||   IS_PKILL( ch ) )
	pager_printf( ch, "Hitroll: %d  Damroll: %d.\n\r",
	    GET_HITROLL(ch), GET_DAMROLL(ch) );

    if ( ch->level >= 10 )
	pager_printf( ch, "Alignment: %d.  ", ch->alignment );

    send_to_pager( "You are ", ch );
	 if ( ch->alignment >  900 ) send_to_pager( "angelic.\n\r", ch );
    else if ( ch->alignment >  700 ) send_to_pager( "saintly.\n\r", ch );
    else if ( ch->alignment >  350 ) send_to_pager( "good.\n\r",    ch );
    else if ( ch->alignment >  100 ) send_to_pager( "kind.\n\r",    ch );
    else if ( ch->alignment > -100 ) send_to_pager( "neutral.\n\r", ch );
    else if ( ch->alignment > -350 ) send_to_pager( "mean.\n\r",    ch );
    else if ( ch->alignment > -700 ) send_to_pager( "evil.\n\r",    ch );
    else if ( ch->alignment > -900 ) send_to_pager( "demonic.\n\r", ch );
    else                             send_to_pager( "satanic.\n\r", ch );

    if ( ch->first_affect )
    {
	send_to_pager( "You are affected by:\n\r", ch );
	for ( paf = ch->first_affect; paf; paf = paf->next )
	    if ( (skill=get_skilltype(paf->type)) != NULL )
	{
	    pager_printf( ch, "Spell: '%s'", skill->name );

	    if ( ch->level >= 20 )
		pager_printf( ch,
		    " modifies %s by %d for %d rounds",
		    affect_loc_name( paf->location ),
		    paf->modifier,
		    paf->duration );

	    send_to_pager( ".\n\r", ch );
	}
    }

    if ( !IS_NPC( ch ) && IS_IMMORTAL( ch ) )
    {
	pager_printf( ch, "\n\rWizInvis level: %d   WizInvis is %s\n\r",
			ch->pcdata->wizinvis,
			xIS_SET(ch->act, PLR_WIZINVIS) ? "ON" : "OFF" );
	if ( ch->pcdata->r_range_lo && ch->pcdata->r_range_hi )
	  pager_printf( ch, "Room Range: %d - %d\n\r", ch->pcdata->r_range_lo,
					 	   ch->pcdata->r_range_hi	);
	if ( ch->pcdata->o_range_lo && ch->pcdata->o_range_hi )
	  pager_printf( ch, "Obj Range : %d - %d\n\r", ch->pcdata->o_range_lo,
	  					   ch->pcdata->o_range_hi	);
	if ( ch->pcdata->m_range_lo && ch->pcdata->m_range_hi )
	  pager_printf( ch, "Mob Range : %d - %d\n\r", ch->pcdata->m_range_lo,
	  					   ch->pcdata->m_range_hi	);
    }

    return;
}

/*								-Thoric
 * Display your current exp, level, and surrounding level exp requirements
 */
void do_level( CHAR_DATA *ch, char *argument )
{
    char buf [MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    int x, lowlvl, hilvl;

    if ( ch->level == 1 )
      lowlvl = 1;
    else
      lowlvl = UMAX( 2, ch->level - 5 );
    hilvl = URANGE( ch->level, ch->level + 5, MAX_LEVEL );
    set_char_color( AT_SCORE, ch );
    ch_printf( ch, "\n\rExperience required, levels %d to %d:\n\r______________________________________________\n\r\n\r", lowlvl, hilvl );
    sprintf( buf, " exp  (Current: %12s)", num_punct(ch->exp) );
    sprintf( buf2," exp  (Needed:  %12s)", num_punct( exp_level(ch, ch->level+1) - ch->exp) );
    for ( x = lowlvl; x <= hilvl; x++ )
	ch_printf( ch, " (%2d) %12s%s\n\r", x, num_punct( exp_level( ch, x ) ),
		(x == ch->level) ? buf : (x == ch->level+1) ? buf2 : " exp" );
    send_to_char( "______________________________________________\n\r", ch );
}

/* 1997, Blodkai */
void do_remains( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    OBJ_DATA *obj;
    bool found = FALSE;

    if ( IS_NPC( ch ) )
      return;
    set_char_color( AT_MAGIC, ch );
    if ( !ch->pcdata->deity ) {
      send_to_pager( "You have no deity from which to seek such assistance...\n\r", ch );
      return;
    }
    if ( ch->pcdata->favor < ch->level*2 ) {
      send_to_pager( "Your favor is insufficient for such assistance...\n\r", ch );
      return;
    }
    pager_printf( ch, "%s appears in a vision, revealing that your remains... ", ch->pcdata->deity->name );
    sprintf( buf, "the corpse of %s", ch->name );
    for ( obj = first_object; obj; obj = obj->next ) {
      if ( obj->in_room && !str_cmp( buf, obj->short_descr )
      && ( obj->pIndexData->vnum == 11 ) ) {
        found = TRUE;
        pager_printf( ch, "\n\r  - at %s will endure for %d ticks",
          obj->in_room->name,
          obj->timer );
      }
    }
    if ( !found )
      send_to_pager( " no longer exist.\n\r", ch );
    else
    {
      send_to_pager( "\n\r", ch );
      ch->pcdata->favor -= ch->level*2;
    }
    return;
}

/* Affects-at-a-glance, Blodkai */
void do_affected ( CHAR_DATA *ch, char *argument )
{
    char arg [MAX_INPUT_LENGTH];
    AFFECT_DATA *paf;
    SKILLTYPE *skill;

    if ( IS_NPC(ch) )
	return;

    set_char_color( AT_SCORE, ch );

    argument = one_argument( argument, arg );
    if ( !str_cmp( arg, "by" ) )
    {
        send_to_char_color( "\n\r&BImbued with:\n\r", ch );
	ch_printf_color( ch, "&C%s\n\r",
	  !xIS_EMPTY(ch->affected_by) ? affect_bit_name( &ch->affected_by ) : "nothing" );
        if ( ch->level >= 20 )
        {
            send_to_char( "\n\r", ch );
            if ( ch->resistant > 0 )
	    {
                send_to_char_color( "&BResistances:  ", ch );
                ch_printf_color( ch, "&C%s\n\r", flag_string(ch->resistant, ris_flags) );
	    }
            if ( ch->immune > 0 )
	    {
                send_to_char_color( "&BImmunities:   ", ch);
                ch_printf_color( ch, "&C%s\n\r",  flag_string(ch->immune, ris_flags) );
	    }
            if ( ch->susceptible > 0 )
	    {
                send_to_char_color( "&BSuscepts:     ", ch );
                ch_printf_color( ch, "&C%s\n\r", flag_string(ch->susceptible, ris_flags) );
	    }
        }
	return;
    }

    if ( !ch->first_affect )
    {
        send_to_char_color( "\n\r&CNo cantrip or skill affects you.\n\r", ch );
    }
    else
    {
	send_to_char( "\n\r", ch );
        for (paf = ch->first_affect; paf; paf = paf->next)
	    if ( (skill=get_skilltype(paf->type)) != NULL )
        {
	    set_char_color( AT_BLUE, ch );
            send_to_char( "Affected:  ", ch );
            set_char_color( AT_SCORE, ch );
            if ( ch->level >= 20
	    ||   IS_PKILL( ch ) )
            {
                if (paf->duration < 25 ) set_char_color( AT_WHITE, ch );
                if (paf->duration < 6  ) set_char_color( AT_WHITE + AT_BLINK, ch );
                ch_printf( ch, "(%5d)   ", paf->duration );
	    }
            ch_printf( ch, "%-18s\n\r", skill->name );
        }
    }
    return;
}

void do_inventory( CHAR_DATA *ch, char *argument )
{
	sysdata.outBytesFlag = LOGBOUTINFORMATION;
    set_char_color( AT_RED, ch );
    send_to_char( "You are carrying:\n\r", ch );
    show_list_to_char( ch->first_carrying, ch, TRUE, TRUE );
	sysdata.outBytesFlag = LOGBOUTNORM;
    return;
}


void do_equipment( CHAR_DATA *ch, char *argument )
{
    OBJ_DATA *obj;
    int iWear;
    bool found;

	sysdata.outBytesFlag = LOGBOUTINFORMATION;
    set_char_color( AT_RED, ch );
    send_to_char( "\n\rYou are using:\n\r&w", ch );
    found = FALSE;
    /*set_char_color( AT_OBJECT, ch );*/
    for ( iWear = 0; iWear < MAX_WEAR; iWear++ )
    {
	for ( obj = ch->first_carrying; obj; obj = obj->next_content )
	   if ( obj->wear_loc == iWear )
	   {
                if( (!IS_NPC(ch)) && (ch->race>0) && (ch->race<MAX_PC_RACE))
                    send_to_char(race_table[ch->race]->where_name[iWear], ch);
                else
                    send_to_char( where_name[iWear], ch );

		if ( can_see_obj( ch, obj ) )
		{
		    send_to_char( format_obj_to_char( obj, ch, TRUE ), ch );
		    send_to_char( "&D\n\r", ch );
		}
		else
		    send_to_char( "something.\n\r", ch );
		found = TRUE;
	   }
    }

    if ( !found )
	send_to_char( "Nothing.\n\r", ch );

	sysdata.outBytesFlag = LOGBOUTNORM;

    return;
}



void set_title( CHAR_DATA *ch, char *title )
{
    char buf[MAX_STRING_LENGTH];

    if ( IS_NPC(ch) )
    {
	bug( "Set_title: NPC.", 0 );
	return;
    }

    if ( isalpha(title[0]) || isdigit(title[0]) )
    {
	buf[0] = ' ';
	strcpy( buf+1, title );
    }
    else
	strcpy( buf, title );

    STRFREE( ch->pcdata->title );
    ch->pcdata->title = STRALLOC( buf );
    return;
}



void do_title( CHAR_DATA *ch, char *argument )
{
    if ( IS_NPC(ch) )
	return;

    set_char_color( AT_SCORE, ch );
    if ( ch->exp <= 5000 && ch->level < LEVEL_IMMORTAL)
    {
	send_to_char( "You must wait, until you can set your title.\n\r", ch );
	return;
    }
    if ( IS_SET( ch->pcdata->flags, PCFLAG_NOTITLE ))
    {
	set_char_color( AT_IMMORT, ch );
        send_to_char( "The Gods prohibit you from changing your title.\n\r", ch );
        return;
    }

    if ( argument[0] == '\0' )
    {
	send_to_char( "Change your title to what?\n\r", ch );
	return;
    }

    if ( strlen(argument) > 50 )
	argument[50] = '\0';

	if(strstr(argument, "(") || strstr(argument, ")")
		|| strstr(argument, "[") || strstr(argument, "]")
		|| strstr(argument, "<") || strstr(argument, ">")
		|| strstr(argument, "{"))
		{
		    send_to_char( "You may not use any of the following symbols in your title: ( ) [ ] { } < >.\n\r", ch );
		    return;
		}

    smash_tilde( argument );
    set_title( ch, argument );
    send_to_char( "Ok.\n\r", ch );
}

/*
 * Set your personal description				-Thoric
 */
void do_description( CHAR_DATA *ch, char *argument )
{
    if ( IS_NPC( ch ) )
    {
	send_to_char( "Monsters are too dumb to do that!\n\r", ch );
	return;
    }

    if ( !ch->desc )
    {
	bug( "do_description: no descriptor", 0 );
	return;
    }

    if( xIS_SET((ch)->in_room->room_flags, ROOM_GRAV) )
    {
        ch_printf(ch,"You can't do that in a grav room.\n\r");
        return;
    }

    switch( ch->substate )
    {
	default:
	   /*bug( "do_description: illegal substate", 0 );*/
	   return;

	case SUB_RESTRICTED:
	   send_to_char( "You cannot use this command from within another command.\n\r", ch );
	   return;

	case SUB_NONE:
	   ch->substate = SUB_PERSONAL_DESC;
	   ch->dest_buf = ch;
	   start_editing( ch, ch->description );
	   return;

	case SUB_PERSONAL_DESC:
	   STRFREE( ch->description );
	   ch->description = copy_buffer( ch );
	   stop_editing( ch );
	   return;
    }
}

void do_description1( CHAR_DATA *ch, char *argument )
{
    if ( IS_NPC( ch ) )
    {
	send_to_char( "Monsters are too dumb to do that!\n\r", ch );
	return;
    }

    if ( !ch->desc )
    {
	bug( "do_description1: no descriptor", 0 );
	return;
    }

    if( xIS_SET((ch)->in_room->room_flags, ROOM_GRAV) )
    {
        ch_printf(ch,"You can't do that in a grav room.\n\r");
        return;
    }

    switch( ch->substate )
    {
	default:
	   /*bug( "do_description1: illegal substate", 0 );*/
	   return;

	case SUB_RESTRICTED:
	   send_to_char( "You cannot use this command from within another command.\n\r", ch );
	   return;

	case SUB_NONE:
           send_to_char( "&RThis sets your description when in your first transformation (if you have one).&w\n\r", ch );
	   ch->substate = SUB_PERSONAL_DESC1;
	   ch->dest_buf = ch;
	   start_editing( ch, ch->pcdata->description1 );
	   return;

	case SUB_PERSONAL_DESC1:
	   STRFREE( ch->pcdata->description1 );
	   ch->pcdata->description1 = copy_buffer( ch );
	   stop_editing( ch );
	   return;
    }
}

void do_description2( CHAR_DATA *ch, char *argument )
{
    if ( IS_NPC( ch ) )
    {
	send_to_char( "Monsters are too dumb to do that!\n\r", ch );
	return;
    }

    if ( !ch->desc )
    {
	bug( "do_description2: no descriptor", 0 );
	return;
    }

    if( xIS_SET((ch)->in_room->room_flags, ROOM_GRAV) )
    {
        ch_printf(ch,"You can't do that in a grav room.\n\r");
        return;
    }

    switch( ch->substate )
    {
	default:
	   /*bug( "do_description2: illegal substate", 0 );*/
	   return;

	case SUB_RESTRICTED:
	   send_to_char( "You cannot use this command from within another command.\n\r", ch );
	   return;

	case SUB_NONE:
           send_to_char( "&RThis sets your description when in your second transformation (if you have one).&w\n\r", ch );
	   ch->substate = SUB_PERSONAL_DESC2;
	   ch->dest_buf = ch;
	   start_editing( ch, ch->pcdata->description2 );
	   return;

	case SUB_PERSONAL_DESC2:
	   STRFREE( ch->pcdata->description2 );
	   ch->pcdata->description2 = copy_buffer( ch );
	   stop_editing( ch );
	   return;
    }
}

void do_description3( CHAR_DATA *ch, char *argument )
{
    if ( IS_NPC( ch ) )
    {
	send_to_char( "Monsters are too dumb to do that!\n\r", ch );
	return;
    }

    if ( !ch->desc )
    {
	bug( "do_description3: no descriptor", 0 );
	return;
    }

    if( xIS_SET((ch)->in_room->room_flags, ROOM_GRAV) )
    {
        ch_printf(ch,"You can't do that in a grav room.\n\r");
        return;
    }

    switch( ch->substate )
    {
	default:
	   /*bug( "do_description3: illegal substate", 0 );*/
	   return;

	case SUB_RESTRICTED:
	   send_to_char( "You cannot use this command from within another command.\n\r", ch );
	   return;

	case SUB_NONE:
           send_to_char( "&RThis sets your description when in your third transformation (if you have one).&w\n\r", ch );
	   ch->substate = SUB_PERSONAL_DESC3;
	   ch->dest_buf = ch;
	   start_editing( ch, ch->pcdata->description3 );
	   return;

	case SUB_PERSONAL_DESC3:
	   STRFREE( ch->pcdata->description3 );
	   ch->pcdata->description3 = copy_buffer( ch );
	   stop_editing( ch );
	   return;
    }
}

void do_description4( CHAR_DATA *ch, char *argument )
{
    if ( IS_NPC( ch ) )
    {
	send_to_char( "Monsters are too dumb to do that!\n\r", ch );
	return;
    }

    if ( !ch->desc )
    {
	bug( "do_description4: no descriptor", 0 );
	return;
    }

    if( xIS_SET((ch)->in_room->room_flags, ROOM_GRAV) )
    {
        ch_printf(ch,"You can't do that in a grav room.\n\r");
        return;
    }

    switch( ch->substate )
    {
	default:
	   /*bug( "do_description4: illegal substate", 0 );*/
	   return;

	case SUB_RESTRICTED:
	   send_to_char( "You cannot use this command from within another command.\n\r", ch );
	   return;

	case SUB_NONE:
           send_to_char( "&RThis sets your description when in your fourth transformation (if you have one).&w\n\r", ch );
	   ch->substate = SUB_PERSONAL_DESC4;
	   ch->dest_buf = ch;
	   start_editing( ch, ch->pcdata->description4 );
	   return;

	case SUB_PERSONAL_DESC4:
	   STRFREE( ch->pcdata->description4 );
	   ch->pcdata->description4 = copy_buffer( ch );
	   stop_editing( ch );
	   return;
    }
}

void do_description5( CHAR_DATA *ch, char *argument )
{
    if ( IS_NPC( ch ) )
    {
	send_to_char( "Monsters are too dumb to do that!\n\r", ch );
	return;
    }

    if ( !ch->desc )
    {
	bug( "do_description5: no descriptor", 0 );
	return;
    }

    if( xIS_SET((ch)->in_room->room_flags, ROOM_GRAV) )
    {
        ch_printf(ch,"You can't do that in a grav room.\n\r");
        return;
    }

    switch( ch->substate )
    {
	default:
	   /*bug( "do_description5: illegal substate", 0 );*/
	   return;

	case SUB_RESTRICTED:
	   send_to_char( "You cannot use this command from within another command.\n\r", ch );
	   return;

	case SUB_NONE:
           send_to_char( "&RThis sets your description when in your fifth transformation (if you have one).&w\n\r", ch );
	   ch->substate = SUB_PERSONAL_DESC5;
	   ch->dest_buf = ch;
	   start_editing( ch, ch->pcdata->description5 );
	   return;

	case SUB_PERSONAL_DESC5:
	   STRFREE( ch->pcdata->description5 );
	   ch->pcdata->description5 = copy_buffer( ch );
	   stop_editing( ch );
	   return;
    }
}

/* Ripped off do_description for whois bio's -- Scryn*/
void do_bio( CHAR_DATA *ch, char *argument )
{
    if ( IS_NPC( ch ) )
    {
	send_to_char( "Mobs cannot set a bio.\n\r", ch );
	return;
    }

    if ( !ch->desc )
    {
	bug( "do_bio: no descriptor", 0 );
	return;
    }

    if( xIS_SET((ch)->in_room->room_flags, ROOM_GRAV) )
    {
	ch_printf(ch,"You can't do that in a grav room.\n\r");
	return;
    }

    switch( ch->substate )
    {
	default:
	   /*bug( "do_bio: illegal substate", 0 );*/
	   return;

	case SUB_RESTRICTED:
	   send_to_char( "You cannot use this command from within another command.\n\r", ch );
	   return;

	case SUB_NONE:
	   do_help(ch, "bio");
	   send_to_char( "&RPlease read the above guidelines before writing your bio.&w\n\r", ch );
		if (xIS_SET(ch->act, PLR_CAN_CHAT))
			xREMOVE_BIT(ch->act, PLR_CAN_CHAT);
	   ch->substate = SUB_PERSONAL_BIO;
	   ch->dest_buf = ch;
	   start_editing( ch, ch->pcdata->bio );
	   return;

	case SUB_PERSONAL_BIO:
	   STRFREE( ch->pcdata->bio );
	   ch->pcdata->bio = copy_buffer( ch );
	   stop_editing( ch );
	   send_to_char( "\n\r&wIf you would like your bio to be queued for authorization, please\n\rtype '&WREQBIOAUTH&w'.  Please do not ask for an admin to check your bio.\n\r", ch );
	   return;
    }
}



/*
 * New stat and statreport command coded by Morphina
 * Bug fixes by Shaddai
 */

void do_statreport( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_INPUT_LENGTH];

    if ( IS_NPC(ch) )
    {
	send_to_char("Huh?\n\r", ch );
	return;
    }

    {
      ch_printf( ch, "You report: %d/%d hp %d/%d mana %d/%d mv %.0Lf xp.\n\r",
 		ch->hit,  ch->max_hit, ch->mana, ch->max_mana,
 		ch->move, ch->max_move, ch->exp   );
      sprintf( buf, "$n reports: %d/%d hp %d/%d mana %d/%d mv %.0Lf xp.",
 		ch->hit,  ch->max_hit, ch->mana, ch->max_mana,
 		ch->move, ch->max_move, ch->exp   );
      act( AT_REPORT, buf, ch, NULL, NULL, TO_ROOM );
    }

    ch_printf( ch, "Your base stats:    %-2d str %-2d wis 0 int %-2d dex %-2d con %-2d cha 0 lck.\n\r",
      		ch->perm_str, ch->perm_int, ch->perm_dex,
		ch->perm_con, ch->perm_lck );
    sprintf( buf, "$n's base stats:    %-2d str %-2d wis 0 int %-2d dex %-2d con %-2d cha 0 lck.",
      		ch->perm_str, ch->perm_int, ch->perm_dex,
		ch->perm_con, ch->perm_lck );
    act( AT_REPORT, buf, ch, NULL, NULL, TO_ROOM );

    ch_printf( ch, "Your current stats: %-2d str %-2d wis 0 int %-2d dex %-2d con %-2d cha 0 lck.\n\r",
		get_curr_str(ch), get_curr_int(ch),
		get_curr_dex(ch), get_curr_con(ch),
		get_curr_lck(ch) );
    sprintf( buf, "$n's current stats: %-2d str %-2d wis 0 int %-2d dex %-2d con %-2d cha 0 lck.",
		get_curr_str(ch), get_curr_int(ch),
		get_curr_dex(ch), get_curr_con(ch),
		get_curr_lck(ch) );
    act( AT_REPORT, buf, ch, NULL, NULL, TO_ROOM );
    return;
}

void do_stat( CHAR_DATA *ch, char *argument )
{
    if ( IS_NPC(ch) )
    {
	send_to_char("Huh?\n\r", ch );
	return;
    }

      ch_printf( ch, "You report: %d/%d hp %d/%d mana %d/%d mv %.0Lf xp.\n\r",
 		ch->hit,  ch->max_hit, ch->mana, ch->max_mana,
 		ch->move, ch->max_move, ch->exp   );

      ch_printf( ch, "Your base stats:    %-2d str %-2d wis 0 int %-2d dex %-2d con %-2d cha 0 lck.\n\r",
      		ch->perm_str, ch->perm_int, ch->perm_dex,
		ch->perm_con, ch->perm_lck );

      ch_printf( ch, "Your current stats: %-2d str %-2d wis 0 int %-2d dex %-2d con %-2d cha 0 lck.\n\r",
		get_curr_str(ch), get_curr_int(ch),
		get_curr_dex(ch), get_curr_con(ch),
		get_curr_lck(ch) );
    return;
}


void do_report( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_INPUT_LENGTH];

    if ( IS_NPC( ch ) && ch->fighting )
	return;

    if ( IS_AFFECTED(ch, AFF_POSSESS) )
    {
       send_to_char("You can't do that in your current state of mind!\n\r", ch);
       return;
    }


      ch_printf( ch,
//	"You report: %d/%d hp %d/%d mana %d/%d mv %s xp.\n\r",
	"You report: %d/%d lf %d/%d energy %s PL.\n\r",
	ch->hit,  ch->max_hit,
	ch->mana, ch->max_mana,
//	ch->move, ch->max_move,
	num_punct_ld(ch->exp)   );

//      sprintf( buf, "$n reports: %d/%d hp %d/%d mana %d/%d mv %s xp.",
      if( !IS_NPC(ch) )
        sprintf( buf, "$n reports: %d/%d lf %d/%d energy %s PL.",
	ch->hit,  ch->max_hit,
	ch->mana, ch->max_mana,
//	ch->move, ch->max_move,
	num_punct_ld(ch->exp)   );
      else
	{
        sprintf( buf, "$n reports: %d/%d lf %d/%d energy %s/",
          ch->hit,  ch->max_hit,
          ch->mana, ch->max_mana,
//        ch->move, ch->max_move,
          num_punct_ld(ch->exp) );
	strcat(buf,num_punct_ld(ch->pl));
	strcat(buf," PL.");
	}

    act( AT_REPORT, buf, ch, NULL, NULL, TO_ROOM );

    return;
}

void do_fprompt( CHAR_DATA *ch, char *argument )
{
  char arg[MAX_INPUT_LENGTH];

  set_char_color( AT_GREY, ch );

  if ( IS_NPC(ch) )
  {
    send_to_char( "NPC's can't change their prompt..\n\r", ch );
    return;
  }
  smash_tilde( argument );
  one_argument( argument, arg );
  if ( !*arg || !str_cmp( arg, "display" ) )
  {
    send_to_char( "Your current fighting prompt string:\n\r", ch );
    set_char_color( AT_WHITE, ch );
    ch_printf( ch, "%s\n\r", !str_cmp( ch->pcdata->fprompt, "" ) ? "(default prompt)"
				 				: ch->pcdata->fprompt );
    set_char_color( AT_GREY, ch );
    send_to_char( "Type 'help prompt' for information on changing your prompt.\n\r", ch );
    return;
  }
  send_to_char( "Replacing old prompt of:\n\r", ch );
  set_char_color( AT_WHITE, ch );
  ch_printf( ch, "%s\n\r", !str_cmp( ch->pcdata->fprompt, "" ) ? "(default prompt)"
							      : ch->pcdata->fprompt );
  if (ch->pcdata->fprompt)
    STRFREE(ch->pcdata->fprompt);
  if ( strlen(argument) > 128 )
    argument[128] = '\0';

  /* Can add a list of pre-set prompts here if wanted.. perhaps
     'prompt 1' brings up a different, pre-set prompt */
  if ( !str_cmp(arg, "default") )
    ch->pcdata->fprompt = STRALLOC("");
  else
    ch->pcdata->fprompt = STRALLOC(argument);
  return;
}

void do_prompt( CHAR_DATA *ch, char *argument )
{
  char arg[MAX_INPUT_LENGTH];

  set_char_color( AT_GREY, ch );

  if ( IS_NPC(ch) )
  {
    send_to_char( "NPC's can't change their prompt..\n\r", ch );
    return;
  }
  smash_tilde( argument );
  one_argument( argument, arg );
  if ( !*arg || !str_cmp( arg, "display" ) )
  {
    send_to_char( "Your current prompt string:\n\r", ch );
    set_char_color( AT_WHITE, ch );
    ch_printf( ch, "%s\n\r", !str_cmp( ch->pcdata->prompt, "" ) ? "(default prompt)"
				 				: ch->pcdata->prompt );
    set_char_color( AT_GREY, ch );
    send_to_char( "Type 'help prompt' for information on changing your prompt.\n\r", ch );
    return;
  }
  send_to_char( "Replacing old prompt of:\n\r", ch );
  set_char_color( AT_WHITE, ch );
  ch_printf( ch, "%s\n\r", !str_cmp( ch->pcdata->prompt, "" ) ? "(default prompt)"
							      : ch->pcdata->prompt );
  if (ch->pcdata->prompt)
    STRFREE(ch->pcdata->prompt);
  if ( strlen(argument) > 128 )
    argument[128] = '\0';

  /* Can add a list of pre-set prompts here if wanted.. perhaps
     'prompt 1' brings up a different, pre-set prompt */
  if ( !str_cmp(arg, "default") )
    ch->pcdata->prompt = STRALLOC("");
  else
    ch->pcdata->prompt = STRALLOC(argument);
  return;
}

/*
 * Figured this belonged here seeing it involves players...
 * really simple little function to tax players with a large
 * amount of gold to help reduce the overall gold pool...
 *  --TRI
 */
void tax_player( CHAR_DATA *ch )
{
  int tax = (ch->gold * .025);
  struct tm *tms;
  int lastTaxDay = 0;
  bool beenTaxed = FALSE;
  CLAN_DATA *clan;

  if (IS_IMMORTAL(ch))
  	return;

  tms = localtime( &current_time );

  if( ch->pcdata && ch->pcdata->lastTaxation <= 0 )
  {
    ch->pcdata->lastTaxation = mktime( tms );
  }
  else
  {
    struct tm *ltms;
    ltms = localtime( &ch->pcdata->lastTaxation );
    lastTaxDay = ltms->tm_yday;
  }

  if( lastTaxDay == tms->tm_yday )
  {
    beenTaxed = TRUE;
  }

  if ( !beenTaxed && get_true_rank(ch) > 2 && ch->gold > 10000)
  {
    set_char_color( AT_WHITE, ch );
    ch_printf( ch, "You pay your daily zeni tax of %s zeni.\n\r",
    	num_punct(tax));
    ch->gold -= tax;
    boost_economy(ch->in_room->area, tax);
    if( ch->pcdata )
    {
      ch->pcdata->lastTaxation = mktime( tms );
    }
  }

  if (!beenTaxed && ch->pcdata->clan && ch->pcdata->clan->tax > 0)
  {
	clan = ch->pcdata->clan;
	tax = ch->gold * clan->tax / 100;
	set_char_color( AT_WHITE, ch );
    ch_printf( ch, "Your clan (%s) requires that you pay daily taxes.\n\r", clan->name);
    ch_printf( ch, "%d%% (%s zeni) of your total (%s) zeni will be deposited\n\r",
    	clan->tax, num_punct(tax), num_punct(ch->gold));
    ch_printf( ch, "into your clans bank account bringing it to %s zeni.\n\r", num_punct_ld(clan->bank));
  }

  return;
}

/* Calculate interest for players using MCCP */
void mccp_interest( CHAR_DATA *ch )
{
/*
    int zeni = 0;
    int interest = 0;
    float interestPercent = 0.05; // Set the interest gained

    if (!ch->desc->out_compress)
    	return;
    else if (ch->pcdata->interestLastMonth >= time_info.month
    		&& (((time_info.month+1)/4) == 4
    			|| ch->pcdata->interestLastMonth <= time_info.month+4)
    		&& ch->pcdata->interestLastYear > time_info.year)
    	return;
    else
    {
    	zeni = ch->gold;
    	interest = ch->gold * interestPercent;
    	ch->gold += interest;
    	ch_printf( ch, "&W%s's Bank Statement:\n\r", ch->name);
    	ch_printf( ch, "&W-------------------\n\r");
    	ch_printf( ch, "&WOld Balance:         &w%d\n\r", num_punct(zeni));
    	ch_printf( ch, "&WInterest Gained(&C%d%%&W): &w%d\n\r", num_punct(interest));
    	ch_printf( ch, "&W-------------------\n\r");
    	ch_printf( ch, "&WNew Balance:         &w%d\n\r", num_punct(ch->gold));
		// remember to change interest vars before activating :)
   }
*/
	return;
}

/* Delete command for players to remove themselves  - Gareth */
void do_delete( CHAR_DATA *ch, char *argument )
{
  char arg[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  char buf[MAX_STRING_LENGTH];
  char buf2[MAX_STRING_LENGTH];
  OBJ_DATA *o;
  int x, y;
  DESCRIPTOR_DATA *dold;
  int cstate;
  CHAR_DATA *cch;
  char log_buf[MAX_STRING_LENGTH];

  argument = one_argument( argument, arg );
  argument = one_argument( argument, arg2 );

  if ( arg[0] == '\0' )
  {
      send_to_char( "Syntax: delete <password> <yourname>\n\r", ch );
      return;
  }

  if ( IS_NPC(ch) )
  {
	send_to_char( "Huh?\n\r", ch );
	return;
  }

  if( ch->kairank > 0 )
  {
	ch_printf(ch,"You cannot delete until an imm removes your kaio rank.\n\r");
	return;
  }
  if( ch->demonrank > 0 )
  {
        ch_printf(ch,"You cannot delete until an imm removes your demon rank.\n\r");
        return;
  }

  if ( auction->item != NULL && ((ch == auction->buyer) || (ch == auction->seller) ) )
  {
	send_to_char("You cannot delete while participating in an auction.\n\r", ch);
	return;
  }

  for( dold = first_descriptor; dold; dold = dold->next )
  {
    if( dold != ch->desc
    &&( dold->character || dold->original )
    && !str_cmp( ch->name, dold->original ? dold->original->pcdata->filename
                                          : dold->character->pcdata->filename ) )
    {
      cstate = dold->connected;
      cch    = dold->original ? dold->original : dold->character;
      if( !cch->name
      ||( cstate != CON_PLAYING && cstate != CON_EDITING ) )
      {
        send_to_char( "You can't do that in your current state.\n\r", ch );
        sprintf( log_buf, "%s trying to abuse 'delete' bug.", cch->pcdata->filename );
        log_string( log_buf );
        return;
      }
    }
  }

  if ( strcasecmp( ch->name, arg2 ) )
  {
        send_to_char( "That's not your name.\n\r", ch );
	return;
  }


  if( str_cmp( sha256_crypt( argument ), ch->pcdata->pwd ) )
        {
        WAIT_STATE( ch, 40 );
        send_to_char( "Wrong password.  Wait 10 seconds.\n\r", ch );
        return;
    }

	add_hiscore( "sparwins", ch->name, 0  );
	add_hiscore( "sparloss", ch->name, 0  );
	add_hiscore( "mkills", ch->name, 0  );
	add_hiscore( "deaths", ch->name, 0  );
	add_hiscore_ld( "powerlevel", ch->name, 0 );
	if (is_saiyan(ch))
		add_hiscore_ld( "plsaiyan", ch->name, 0 );
	if (is_human(ch))
		add_hiscore_ld( "plhuman", ch->name, 0 );
	if (is_hb(ch))
		add_hiscore_ld( "plhalfbreed", ch->name, 0 );
	if (is_namek(ch))
		add_hiscore_ld( "plnamek", ch->name, 0 );
	if (is_android(ch))
		add_hiscore_ld( "plandroid", ch->name, 0 );
	if (is_icer(ch))
		add_hiscore_ld( "plicer", ch->name, 0 );
	if (is_bio(ch))
		add_hiscore_ld( "plbio-android", ch->name, 0 );
	if (is_kaio(ch))
                add_hiscore_ld( "plkaio", ch->name, 0 );
	if (is_demon(ch))
                add_hiscore_ld( "pldemon", ch->name, 0 );
	add_hiscore( "played", ch->name, 0  );
	add_hiscore( "zeni", ch->name, 0  );
	add_hiscore( "bounty", ch->name, 0  );

    while( (o = carrying_noquit(ch)) != NULL )
    {
      obj_from_char(o);
      obj_to_room(o, ch->in_room);
      ch_printf(ch,"&wYou drop %s&w.\n\r",o->short_descr);
    }

    if ( ch->desc && ch->desc->host[0] != '\0' )
	sprintf(log_buf, "%s deleting self from site %s", ch->name,
	      ch->desc->host );
    else
	sprintf(log_buf, "%s deleting self with no descriptor!", ch->name);

    log_string( log_buf );

    if( ch->pcdata->clan )
      remove_member(ch);

    quitting_char = ch;
    save_char_obj( ch );
    saving_char = NULL;
    extract_char( ch, TRUE );
    for ( x = 0; x < MAX_WEAR; x++ )
        for ( y = 0; y < MAX_LAYERS; y++ )
            save_equipment[x][y] = NULL;

  sprintf( buf, "%s%c/%s", PLAYER_DIR, tolower(arg2[0]),
          capitalize( arg2 ) );
  sprintf( buf2, "%s%c/%s", BACKUP_DIR, tolower(arg2[0]),
          capitalize( arg2 ) );

  rename( buf, buf2 );
  sprintf( buf, "%s deleted self, pfile stored in backup directory.", arg2);
  log_string( buf );

}

void do_tag( CHAR_DATA *ch, char *argument )
{
	DESCRIPTOR_DATA *d;
	CHAR_DATA *victim = NULL;
	CHAR_DATA *vch;
    char arg[MAX_INPUT_LENGTH];

	one_argument( argument, arg );

	for( d = first_descriptor; d; d= d->next )
	{
		vch = d->character;

		if (!vch)
		{
			continue;
		}

		if (xIS_SET(vch->affected_by, AFF_TAG) && vch == ch)
		{
			break;
		}

		if (xIS_SET(vch->affected_by, AFF_TAG))
		{
			pager_printf_color(ch, "Your not it, %s is!\n\r", vch->name );
			return;
		}

	}

    if ( arg[0] == '\0' )
    {
	act( AT_SOCIAL, "You shout, 'Enough killing!  Let's play some tag!'", ch, NULL, victim, TO_CHAR    );
	act( AT_SOCIAL, "$n says, 'Enough killing!  Let's play some tag!'",   ch, NULL, victim, TO_ROOM    );
	return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL )
    {
		send_to_char( "They aren't here.\n\r", ch );
		return;
    }

    if ( victim == ch )
    {
	act( AT_SOCIAL, "$n dodges $s left hand, but is led directly into $s right!  Fool!",   ch, NULL, victim, TO_ROOM    );
	act( AT_SOCIAL, "You dodge your left hand, but are led directly into your right!  Fool!",     ch, NULL, victim, TO_CHAR    );
	return;
    }

    if ( xIS_SET( victim->in_room->room_flags, ROOM_SAFE ) )
    {
		act( AT_SOCIAL, "$n chases $N around but $N puts $S hand on the wall and yells, 'BASE!'",  ch, NULL, victim, TO_NOTVICT );
		act( AT_SOCIAL, "You chase $N around but $N puts $S hand on the wall and yells, 'BASE!'",    ch, NULL, victim, TO_CHAR    );
		act( AT_SOCIAL, "$n chases you around but you put your hand on the wall and yell, 'BASE!'",    ch, NULL, victim, TO_VICT    );
	return;
    }

	switch (number_range(1,10))
	{
	default:
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
	case 6:
		act( AT_SOCIAL, "$n slaps $N on the back and screams, 'You're it slowpoke!'",  ch, NULL, victim, TO_NOTVICT );
		act( AT_SOCIAL, "You slap $N on the back and scream, 'You're it slowpoke!'",    ch, NULL, victim, TO_CHAR    );
		act( AT_SOCIAL, "$n slaps you on the back and screams, 'You're it slowpoke!'",    ch, NULL, victim, TO_VICT    );

		xREMOVE_BIT(ch->affected_by, AFF_TAG);
		xSET_BIT(victim->affected_by, AFF_TAG);
		break;

	case 7:
	case 8:
	case 9:
	case 10:
		act( AT_SOCIAL, "$n chases after $N but $n trips and lands flat on $s face! Ouch!",  ch, NULL, victim, TO_NOTVICT );
		act( AT_SOCIAL, "You chase after $N but you trip and land flat on your face! Ouch!",    ch, NULL, victim, TO_CHAR    );
		act( AT_SOCIAL, "$n chases after you but $e trips and lands flat on $s face! Ouch!",    ch, NULL, victim, TO_VICT    );
		break;
	}

	return;
}

void do_pk( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];

	argument = one_argument( argument, arg );

	if ( IS_NPC(ch) )
	return;

	if ( get_timer(ch, TIMER_PKILLED) > 0 )
        {
	  ch_printf(ch,"&GYou have been killed within the last 30 minutes.\n\r");
	  ch_printf(ch,"&GYou have %d minutes remaining till you can kill and be killed again.\n\r",
			(get_timer(ch, TIMER_PKILLED) / 23 ) );
        }

	if (IS_HC(ch) && strcmp(arg, "war") && strcmp(arg, "off") )
	{
		send_to_char( "You're HARD CORE you don't even need to bother with that lame flag.", ch );
		return;
	}
	if (ch->exp < 5000)
	{
		send_to_char( "You can't turn on your PK flag until you are out of training.", ch );
		if (xIS_SET(ch->act, PLR_PK1))
			xREMOVE_BIT(ch->act, PLR_PK1);
		if (xIS_SET(ch->act, PLR_PK2))
			xREMOVE_BIT(ch->act, PLR_PK2);
		if (xIS_SET(ch->act, PLR_WAR1))
			xREMOVE_BIT(ch->act, PLR_WAR1);
		if (xIS_SET(ch->act, PLR_WAR2))
			xREMOVE_BIT(ch->act, PLR_WAR2);
		return;
	}

    if ( arg[0] == '\0' )
    {
	send_to_char( "PK flags currently set: ", ch );
	if (xIS_SET(ch->act, PLR_PK1))
		send_to_pager_color( " &YPK", ch );
	if (xIS_SET(ch->act, PLR_PK2))
		send_to_pager_color( " &RPK", ch );
	if (xIS_SET(ch->act, PLR_WAR1))
		send_to_pager_color( " &YCLAN WAR", ch );
	if (xIS_SET(ch->act, PLR_WAR2))
		send_to_pager_color( " &RCLAN WAR", ch );
	send_to_char( ".\n\r", ch );
	if (xIS_SET(ch->act, PLR_PK1) || xIS_SET(ch->act, PLR_PK2) ||
	    xIS_SET(ch->act, PLR_WAR1) || xIS_SET(ch->act, PLR_WAR2) )
	pager_printf_color(ch, "You must wait %d more minutes left until PK mode can be turned off.\n\r", ch->pcdata->pk_timer);	return;
    return;
    }

	if (!strcmp(arg, "flag"))
	{
		if (!strcmp(argument, "yellow"))
			if (xIS_SET(ch->act, PLR_PK1))
				send_to_pager_color("&wYour &Yyellow &wPK flag is already set.\n\r", ch);
			else if (xIS_SET(ch->act, PLR_PK2))
			{
				if (ch->pcdata->pk_timer > 0)
					pager_printf_color(ch, "You must wait %d more minutes before you can set your &Yyellow PK flag.\n\r", ch->pcdata->pk_timer);
				else
				{
					xREMOVE_BIT(ch->act, PLR_PK2);
					xSET_BIT(ch->act, PLR_PK1);
					ch->pcdata->pk_timer = 60;
					send_to_pager_color("&wYour &Yyellow &wPK flag is now set.\n\r", ch);
				}
			}
			else
			{
				xSET_BIT(ch->act, PLR_PK1);
				ch->pcdata->pk_timer = 60;
				send_to_pager_color("&wYour &Yyellow &wPK flag is now set.\n\r", ch);
			}
		else if (!strcmp(argument, "red"))
			if (xIS_SET(ch->act, PLR_PK2))
				send_to_pager_color("&wYour &Rred &wPK flag is already set.\n\r", ch);
			else
			{
				if (xIS_SET(ch->act, PLR_PK1))
					xREMOVE_BIT(ch->act, PLR_PK1);
				xSET_BIT(ch->act, PLR_PK2);
				ch->pcdata->pk_timer = 60;
				send_to_pager_color("&wYour &Rred &wPK flag is set.\n\r", ch);
			}
		else
			send_to_pager_color("&wWhat PK flag do you want to turn on?\n\r", ch);

		return;
	}

	if (!strcmp(arg, "war"))
	{
/*send_to_pager_color( "&WThis feature is disabled for now.\n\r", ch );
return;*/
		if (!ch->pcdata->clan)
		{
			send_to_pager_color("&wYou do not belong to a clan.\n\r", ch);
			return;
		}

		if (!strcmp(argument, "yellow"))
			if (xIS_SET(ch->act, PLR_WAR1))
				send_to_pager_color("&wYour &Yyellow &wWAR flag is already set.\n\r", ch);
			else if (xIS_SET(ch->act, PLR_WAR2))
			{
				if (ch->pcdata->pk_timer > 0)
					pager_printf_color(ch, "You must wait %d more minutes before you can set your &Yyellow WAR flag.\n\r", ch->pcdata->pk_timer);
				else
				{
					xREMOVE_BIT(ch->act, PLR_WAR2);
					xSET_BIT(ch->act, PLR_WAR1);
					if (!xIS_SET(ch->act, PLR_PK1) && !xIS_SET(ch->act, PLR_PK2))
						ch->pcdata->pk_timer = 60;
					send_to_pager_color("&wYour &Yyellow &wWAR flag is now set.\n\r", ch);
				}
			}
			else
			{
				xSET_BIT(ch->act, PLR_WAR1);
				send_to_pager_color("&wYour &Yyellow &wWAR flag is now set.\n\r", ch);
			}
		else if (!strcmp(argument, "red"))
			if (xIS_SET(ch->act, PLR_WAR2))
				send_to_pager_color("&wYour &Rred &wWAR flag is already set.\n\r", ch);
			else
			{
				if (xIS_SET(ch->act, PLR_WAR1))
					xREMOVE_BIT(ch->act, PLR_WAR1);
				xSET_BIT(ch->act, PLR_WAR2);
				ch->pcdata->pk_timer = 60;
				send_to_pager_color("&wYour &Rred &wWAR flag is set.\n\r", ch);
			}
		else
			send_to_pager_color("&wWhat WAR flag do you want to turn on?\n\r", ch);

		return;
	}

	if (!strcmp(arg, "off"))
	{
		if (ch->pcdata->pk_timer > 0)
		{
		  if( get_timer(ch, TIMER_PKILLED) > 0 )
		  {
		    ch->pcdata->pk_timer = 0;
		  }
		  else
		  {
			pager_printf_color(ch, "You must wait %d more minutes until PK mode can be turned off.\n\r", ch->pcdata->pk_timer);	return;
			return;
		  }
		}

		if (xIS_SET(ch->act, PLR_PK1))
			xREMOVE_BIT(ch->act, PLR_PK1);
		if (xIS_SET(ch->act, PLR_PK2))
			xREMOVE_BIT(ch->act, PLR_PK2);
		if (xIS_SET(ch->act, PLR_WAR1))
                        xREMOVE_BIT(ch->act, PLR_WAR1);
                if (xIS_SET(ch->act, PLR_WAR2))
                        xREMOVE_BIT(ch->act, PLR_WAR2);

		send_to_pager_color("&wPK mode is now off.\n\r", ch);


		return;
	}

	send_to_char( "PK flags currently set: ", ch );
	if (xIS_SET(ch->act, PLR_PK1))
		send_to_pager_color( " &YPK", ch );
	if (xIS_SET(ch->act, PLR_PK2))
		send_to_pager_color( " &RPK", ch );
	if (xIS_SET(ch->act, PLR_WAR1))
		send_to_pager_color( " &YCLAN WAR", ch );
	if (xIS_SET(ch->act, PLR_WAR2))
		send_to_pager_color( " &RCLAN WAR", ch );
	send_to_char( ".\n\r", ch );
	if (xIS_SET(ch->act, PLR_PK1) || xIS_SET(ch->act, PLR_PK2))
	pager_printf_color(ch, "You must wait %d more minutes left until PK mode can be turned off.\n\r", ch->pcdata->pk_timer);	return;
    return;

}


void do_setage(CHAR_DATA *ch, char *argument)
{
	char arg[MAX_STRING_LENGTH];
	int value;

	one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_pager( "&wHow old do you wish to become?\n\r", ch );
	return;
    }
	if (!is_number( arg ))
    {
	send_to_char( "&wTry a number instead...", ch );
	return;
    }
	value = atoi(argument);
	if ( value < 4 || value > 150)
    {
	send_to_char( "&wInvalid age.", ch );
	return;
    }
	ch->pcdata->age = value;
	send_to_char( "&wOk.", ch );
	return;
}

void do_setheight(CHAR_DATA *ch, char *argument)
{
	char arg[MAX_STRING_LENGTH];
	int value;

	one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_pager( "&wHow tall do you wish to become?\n\r", ch );
	return;
    }
	if (!is_number( arg ))
    {
	send_to_char( "&wTry a number instead...", ch );
	return;
    }
	value = atoi(argument);
	if ( value < 36 || value > 96)
    {
	send_to_char( "&wInvalid height.", ch );
	return;
    }
	ch->height = value;
	send_to_char( "&wOk.", ch );
	return;
}

void do_setweight(CHAR_DATA *ch, char *argument)
{
	char arg[MAX_STRING_LENGTH];
	int value;

	one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_pager( "&wHow much do you wish to weigh?\n\r", ch );
	return;
    }
	if (!is_number( arg ))
    {
	send_to_char( "&wTry a number instead...", ch );
	return;
    }
	value = atoi(argument);
	if ( value < 50 || value > 300)
    {
	send_to_char( "&wInvalid weight.", ch );
	return;
    }
	ch->weight = value;
	send_to_char( "&wOk.", ch );
	return;
}

void do_reqbio( CHAR_DATA *ch, char *argument)
{

	if ( IS_NPC(ch) )
		return;

	if (xIS_SET(ch->act, PLR_CAN_CHAT))
	{
		send_to_char( "Your bio has already been authorized.\n\r", ch );
		return;
	}

	if (xIS_SET(ch->act, PLR_REQBIO))
	{
		send_to_char( "Your bio has already been queued.\n\r", ch );
		return;
	}

	xSET_BIT(ch->act, PLR_REQBIO);
	send_to_char( "You're bio has been queued for authorization.\n\rPlease wait until an administrator can authorize it.\n\r", ch );
	return;
}

sh_int get_aura( CHAR_DATA *ch )
{
    if( IS_NPC(ch) && is_split(ch) )
    {
      if( !ch->master )
	return AT_GREEN;
      if( !ch->master->pcdata )
	return AT_GREEN;

      return ch->master->pcdata->auraColorPowerUp;
    }
    if( IS_NPC(ch) )
      return AT_GREEN;
    if( !ch->pcdata )
      return AT_GREEN;
    if( ch->pcdata->auraColorPowerUp > 0 )
      return ch->pcdata->auraColorPowerUp;
    else
      return AT_GREEN;
}

void do_aura_set( CHAR_DATA *ch, char *argument )
{
  sh_int auraColor;
  char colorWord[20];
  char showBuf[MAX_STRING_LENGTH];
  char arg[MAX_INPUT_LENGTH];

  if( IS_NPC( ch ) )
    return;

  if( argument[0] == '\0' )
  {
    send_to_char( "usage:\n\r", ch );
    send_to_char( "  aura show -- shows list of colors and your set color\n\r", ch );
    send_to_char( "  aura set <color> -- sets aura to color chosen\n\r", ch );
    send_to_char( "  *note: you can only set your aura color once\n\r", ch );
    return;
  }

  argument = one_argument( argument, arg );

  if( !str_prefix( arg, "show" ) )
  {
    if( ch->pcdata->auraColorPowerUp > 0 )
      auraColor = ch->pcdata->auraColorPowerUp;
    else
      auraColor = AT_BLACK;

    switch( auraColor )
    {
      case AT_BLACK:
        sprintf( colorWord, "None" );
      break;

      case AT_BLOOD:
        sprintf( colorWord, "Dark Red" );
      break;

      case AT_DGREEN:
        sprintf( colorWord, "Dark Green" );
      break;

      case AT_ORANGE:
        sprintf( colorWord, "Brown" );
      break;

      case AT_DBLUE:
        sprintf( colorWord, "Dark Blue" );
      break;

      case AT_PURPLE:
        sprintf( colorWord, "Purple" );
      break;

      case AT_CYAN:
        sprintf( colorWord, "Cyan" );
      break;

      case AT_GREY:
        sprintf( colorWord, "Grey" );
      break;

      case AT_DGREY:
        sprintf( colorWord, "Dark Grey" );
      break;

      case AT_RED:
        sprintf( colorWord, "Red" );
      break;

      case AT_GREEN:
        sprintf( colorWord, "Green" );
      break;

      case AT_YELLOW:
        sprintf( colorWord, "Yellow" );
      break;

      case AT_BLUE:
        sprintf( colorWord, "Blue" );
      break;

      case AT_PINK:
        sprintf( colorWord, "Pink" );
      break;

      case AT_LBLUE:
        sprintf( colorWord, "Light Blue" );
      break;

      case AT_WHITE:
        sprintf( colorWord, "White" );
      break;

      default:
        sprintf( colorWord, "Yellow" );
      break;
    }
    sprintf( showBuf, "Valid color choices:\n\r"
                      "&W%-10s   &r%-10s   &g%-10s   &O%-10s\n\r"
                      "&b%-10s   &p%-10s   &c%-10s   &w%-10s\n\r"
                      "&z%-10s   &R%-10s   &G%-10s   &Y%-10s\n\r"
                      "&B%-10s   &P%-10s   &C%-10s&D\n\r"
                    , "white", "darkred", "darkgreen", "brown"
                    , "darkblue", "purple", "cyan", "grey", "darkgrey"
                    , "red", "green", "yellow", "blue", "pink"
                    , "lightblue" );
    send_to_char( showBuf, ch );
    if( ch->pcdata->auraColorPowerUp >= 0 )
    {
      sprintf( showBuf, "Your aura is currently set to: %s\n\r", colorWord );
      act( AT_GREY, showBuf, ch, NULL, NULL, TO_CHAR );
    }
    return;
  }

  if( ch->pcdata->auraColorPowerUp > 0 )
  {
    send_to_char( "You've already set your aura.\n\r", ch );
    return;
  }

  if( !str_prefix( arg, "set" ) )
  {
    if( argument[0] == '\0' )
    {
      send_to_char( "What color do you want?\n\r", ch );
      return;
    }

    if( !str_prefix( argument, "black" ) )
    {
      /*ch->pcdata->auraColorPowerUp = AT_BLACK;
      sprintf( colorWord, "%s", "Black" );*/
      ch_printf(ch,"You may not set your aura color to black.\n\r");
      return;
    }
    else
    if( !str_prefix( argument, "darkred" ) )
    {
      ch->pcdata->auraColorPowerUp = AT_BLOOD;
      sprintf( colorWord, "%s", "Dark Red" );
    }
    else
    if( !str_prefix( argument, "darkgreen" ) )
    {
      ch->pcdata->auraColorPowerUp = AT_DGREEN;
      sprintf( colorWord, "%s", "Dark Green" );
    }
    else
    if( !str_prefix( argument, "brown" ) )
    {
      ch->pcdata->auraColorPowerUp = AT_ORANGE;
      sprintf( colorWord, "%s", "Brown" );
    }
    else
    if( !str_prefix( argument, "darkblue" ) )
    {
      ch->pcdata->auraColorPowerUp = AT_DBLUE;
      sprintf( colorWord, "%s", "Dark Blue" );
    }
    else
    if( !str_prefix( argument, "purple" ) )
    {
      ch->pcdata->auraColorPowerUp = AT_PURPLE;
      sprintf( colorWord, "%s", "Purple" );
    }
    else
    if( !str_prefix( argument, "cyan" ) )
    {
      ch->pcdata->auraColorPowerUp = AT_CYAN;
      sprintf( colorWord, "%s", "Cyan" );
    }
    else
    if( !str_prefix( argument, "grey" ) )
    {
      ch->pcdata->auraColorPowerUp = AT_GREY;
      sprintf( colorWord, "%s", "Grey" );
    }
    else
    if( !str_prefix( argument, "darkgrey" ) )
    {
      ch->pcdata->auraColorPowerUp = AT_DGREY;
      sprintf( colorWord, "%s", "Dark Grey" );
    }
    else
    if( !str_prefix( argument, "red" ) )
    {
      ch->pcdata->auraColorPowerUp = AT_RED;
      sprintf( colorWord, "%s", "Red" );
    }
    else
    if( !str_prefix( argument, "green" ) )
    {
      ch->pcdata->auraColorPowerUp = AT_GREEN;
      sprintf( colorWord, "%s", "Green" );
    }
    else
    if( !str_prefix( argument, "yellow" ) )
    {
      ch->pcdata->auraColorPowerUp = AT_YELLOW;
      sprintf( colorWord, "%s", "Yellow" );
    }
    else
    if( !str_prefix( argument, "blue" ) )
    {
      ch->pcdata->auraColorPowerUp = AT_BLUE;
      sprintf( colorWord, "%s", "Blue" );
    }
    else
    if( !str_prefix( argument, "pink" ) )
    {
      ch->pcdata->auraColorPowerUp = AT_PINK;
      sprintf( colorWord, "%s", "Pink" );
    }
    else
    if( !str_prefix( argument, "lightblue" ) )
    {
      ch->pcdata->auraColorPowerUp = AT_LBLUE;
      sprintf( colorWord, "%s", "Light Blue" );
    }
    else
    if( !str_prefix( argument, "white" ) )
    {
      ch->pcdata->auraColorPowerUp = AT_WHITE;
      sprintf( colorWord, "%s", "White" );
    }
    else
    {
      send_to_char( "Invalid color.\n\r", ch );
      return;
    }

    sprintf( showBuf, "Powerup aura set to: %s\n\r", colorWord );
    act( ch->pcdata->auraColorPowerUp, showBuf, ch, NULL, NULL, TO_CHAR );
    return;
  }

  send_to_char( "Invalid choice.\n\r", ch );
  return;
}

/* Checks room to see if an Undertaker mob is present */
CHAR_DATA *find_undertaker( CHAR_DATA *ch )
{
  CHAR_DATA *undertaker = NULL;

  for ( undertaker = ch->in_room->first_person; undertaker; undertaker = undertaker->next_in_room )
    if ( IS_NPC( undertaker ) && xIS_SET( undertaker->act, ACT_UNDERTAKER ) )
      break;

  return undertaker;
}

void do_corpse( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj, *outer_obj;
    CHAR_DATA *mob;
    bool found = FALSE;
    int cost = 0;

    /* Avoids the potential for filling the room with hundreds of mob corpses */
    if( IS_NPC(ch) )
    {
	send_to_char( "Mobs cannot retreive corpses.\n\r", ch );
	return;
    }

    /* Search for an act_undertaker */
    if ( !( mob = find_undertaker( ch ) ) )
    {
        send_to_char( "There's no undertaker here!\n\r", ch );
        return;
    }

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
        act(AT_PLAIN,"$N says 'Ooo Yesss ... I can helpss you.'",ch,NULL,mob,TO_CHAR);
        send_to_char("  retrieve: Retrieves your corpse   100z / rank\n\r",ch);
        send_to_char(" Type corpse <type> for the service.\n\r",ch);
        return;
    }

    if (!str_cmp(arg,"retrieve"))
        cost  = 100 * ch->level;
    else
    {
        act(AT_PLAIN,"$N says ' Type 'corpse' for help on what I do.'",
            ch,NULL,mob,TO_CHAR);
        return;
    }

    if (cost > ch->gold )
    {
        act(AT_PLAIN,"$N says 'Pah! You do not have enough gold for my services!'",ch,NULL,mob,TO_CHAR);
        return;
    }

    strcpy( buf, "the corpse of " );
    strcat( buf, ch->name ); 			/* Bug fix here by Samson 12-21-00 See below */
    for ( obj = first_object; obj; obj = obj->next )
    {
        if ( !nifty_is_name( buf, obj->short_descr ) ) /* Fix here - Samson 1-26-01 */
              continue;

	  /* This will prevent NPC corpses from being retreived if the person has a mob's name */
	  if ( obj->item_type == ITEM_CORPSE_NPC )
		continue;

        found = TRUE;

        /* Could be carried by act_scavengers, or other idiots so ... */
        outer_obj = obj;
        while ( outer_obj->in_obj )
              outer_obj = outer_obj->in_obj;

        separate_obj( outer_obj );
        obj_from_room( outer_obj );
        obj_to_room( outer_obj, ch->in_room );

        ch->gold -= cost;
        act(AT_PLAIN,"$N creepily carts in your corpse.",ch,NULL,mob,TO_CHAR);
        act(AT_PLAIN,"$n creepily carts in the $T.",mob,NULL,buf,TO_ROOM);
    }

    /* Could've been extracted, so do this */
    if ( !found )
        act(AT_PLAIN,"$N says 'Sorry I can't find your corpse. There's nothing more I can do.'",ch,NULL,mob,TO_CHAR);

    return;
}

void do_sparcheck(CHAR_DATA *ch, char *argument)
{
	ch_printf(ch, "Spars done in the past 24 hours: %d.\n\r", ch->pcdata->sparcount);
	if( ch->pcdata->nextspartime > 0 )
	ch_printf(ch, "Next reset: %24.24s.\n\r", ctime( &ch->pcdata->nextspartime ) );
}

bool is_saiyan( CHAR_DATA *ch )
{
  if( !str_cmp(get_race(ch),"saiyan"))
  {
    return TRUE;
  }
  if( !str_cmp(get_race(ch),"saiyan-s"))
  {
    return TRUE;
  }
  if( !str_cmp(get_race(ch),"saiyan-n"))
  {
    return TRUE;
  }
  if( !str_cmp(get_race(ch),"saiyan-h"))
  {
    return TRUE;
  }
  if( !str_cmp(get_race(ch),"saiyan-hb"))
  {
    return TRUE;
  }
  return FALSE;
}
bool is_hb( CHAR_DATA *ch )
{
  if( !str_cmp(get_race(ch),"halfbreed"))
    {
      return TRUE;
    }
  if( !str_cmp(get_race(ch),"halfbreed-hb"))
    {
      return TRUE;
    }
  if( !str_cmp(get_race(ch),"halfbreed-s"))
    {
      return TRUE;
    }
  if( !str_cmp(get_race(ch),"halfbreed-n"))
    {
      return TRUE;
    }
  if( !str_cmp(get_race(ch),"halfbreed-h"))
    {
      return TRUE;
    }
  return FALSE;
}

bool is_namek( CHAR_DATA *ch )
{
  if( !str_cmp(get_race(ch),"namek"))
    {
      return TRUE;
    }
  if( !str_cmp(get_race(ch),"namek-n"))
    {
      return TRUE;
    }
  if( !str_cmp(get_race(ch),"namek-s"))
    {
      return TRUE;
    }
  if( !str_cmp(get_race(ch),"namek-hb"))
    {
      return TRUE;
    }
  if( !str_cmp(get_race(ch),"namek-h"))
    {
      return TRUE;
    }
  return FALSE;
}

bool is_genie( CHAR_DATA *ch )
{
  if( !str_cmp(get_race(ch),"genie"))
    {
      return TRUE;
    }
  return FALSE;
}

bool is_wizard( CHAR_DATA *ch )
{
  if( !str_cmp(get_race(ch),"wizard"))
    {
      return TRUE;
    }
  return FALSE;
}

bool is_human( CHAR_DATA *ch )
{
  if( !str_cmp(get_race(ch),"human"))
    {
      return TRUE;
    }
  if( !str_cmp(get_race(ch),"human-h"))
    {
      return TRUE;
    }
  if( !str_cmp(get_race(ch),"human-s"))
    {
      return TRUE;
    }
  if( !str_cmp(get_race(ch),"human-n"))
    {
      return TRUE;
    }
  if( !str_cmp(get_race(ch),"human-hb"))
    {
      return TRUE;
    }
  return FALSE;
}
bool is_icer( CHAR_DATA *ch )
{
  if( !str_cmp(get_race(ch),"icer"))
    {
      return TRUE;
    }
  return FALSE;
}
bool is_android( CHAR_DATA *ch )
{
  if( !str_cmp(get_race(ch),"android"))
    {
      return TRUE;
    }
  else if( !str_cmp(get_race(ch),"super-android"))
    {
      return TRUE;
    }
  else if( !str_cmp(get_race(ch),"android-h"))
    {
      return TRUE;
    }
  else if( !str_cmp(get_race(ch),"android-e"))
    {
      return TRUE;
    }
  else if( !str_cmp(get_race(ch),"android-fm"))
    {
      return TRUE;
    }
  return FALSE;
}
bool is_android_h( CHAR_DATA *ch )
{
  if( !str_cmp(get_race(ch),"android-h"))
    {
      return TRUE;
    }
  else if( !str_cmp(get_race(ch),"super-android"))
    {
      return TRUE;
    }
  return FALSE;
}
bool is_android_e( CHAR_DATA *ch )
{
  if( !str_cmp(get_race(ch),"android-e"))
    {
      return TRUE;
    }
  else if( !str_cmp(get_race(ch),"super-android"))
    {
      return TRUE;
    }
  return FALSE;
}
bool is_android_fm( CHAR_DATA *ch )
{
  if( !str_cmp(get_race(ch),"android-fm"))
    {
      return TRUE;
    }
  else if( !str_cmp(get_race(ch),"super-android"))
    {
      return TRUE;
    }
  return FALSE;
}
bool is_superandroid( CHAR_DATA *ch )
{
  if( !str_cmp(get_race(ch),"super-android"))
    {
      return TRUE;
    }
  return FALSE;
}
bool is_bio( CHAR_DATA *ch )
{
  if( !str_cmp(get_race(ch),"bio-android"))
    {
      return TRUE;
    }
  return FALSE;
}
bool is_kaio( CHAR_DATA *ch )
{
  if( !str_cmp(get_race(ch),"kaio"))
    {
      return TRUE;
    }
  return FALSE;
}

bool is_demon( CHAR_DATA *ch )
{
  if( !str_cmp(get_race(ch),"demon"))
    {
      return TRUE;
    }
  return FALSE;
}

bool is_transformed( CHAR_DATA *ch )
{
    if (xIS_SET((ch)->affected_by, AFF_KAIOKEN))
	return TRUE;
    if (xIS_SET((ch)->affected_by, AFF_SSJ))
        return TRUE;
    if (xIS_SET((ch)->affected_by, AFF_SSJ2))
        return TRUE;
    if (xIS_SET((ch)->affected_by, AFF_SSJ3))
        return TRUE;
    if (xIS_SET((ch)->affected_by, AFF_SSJ4))
        return TRUE;
    if (xIS_SET((ch)->affected_by, AFF_USSJ))
        return TRUE;
    if (xIS_SET((ch)->affected_by, AFF_USSJ2))
        return TRUE;
    if (xIS_SET((ch)->affected_by, AFF_ICER2))
        return TRUE;
    if (xIS_SET((ch)->affected_by, AFF_ICER3))
        return TRUE;
    if (xIS_SET((ch)->affected_by, AFF_ICER4))
        return TRUE;
    if (xIS_SET((ch)->affected_by, AFF_ICER5))
        return TRUE;
    if (xIS_SET((ch)->affected_by, AFF_SNAMEK))
        return TRUE;
    if (xIS_SET((ch)->affected_by, AFF_OOZARU))
        return TRUE;
    if (xIS_SET((ch)->affected_by, AFF_GOLDEN_OOZARU))
        return TRUE;
    if (xIS_SET((ch)->affected_by, AFF_HEART))
        return TRUE;
    if (xIS_SET((ch)->affected_by, AFF_HYPER))
        return TRUE;
    if (xIS_SET((ch)->affected_by, AFF_EXTREME))
        return TRUE;
    if (xIS_SET((ch)->affected_by, AFF_ELECTRICSHIELD))
        return TRUE;
    if (xIS_SET((ch)->affected_by, AFF_SEMIPERFECT))
        return TRUE;
    if (xIS_SET((ch)->affected_by, AFF_PERFECT))
        return TRUE;
    if (xIS_SET((ch)->affected_by, AFF_ULTRAPERFECT))
        return TRUE;
    if (xIS_SET((ch)->affected_by, AFF_GROWTH))
        return TRUE;
    if (xIS_SET((ch)->affected_by, AFF_GIANT))
        return TRUE;
    if (xIS_SET((ch)->affected_by, AFF_SPLIT_FORM))
        return TRUE;
    if (xIS_SET((ch)->affected_by, AFF_TRI_FORM))
        return TRUE;
    if (xIS_SET((ch)->affected_by, AFF_MULTI_FORM))
        return TRUE;
    if (xIS_SET((ch)->affected_by, AFF_MYSTIC))
        return TRUE;
    if (xIS_SET((ch)->affected_by, AFF_SUPERANDROID))
        return TRUE;
    if (xIS_SET((ch)->affected_by, AFF_MAKEOSTAR))
        return TRUE;
    if (xIS_SET((ch)->affected_by, AFF_EVILBOOST))
        return TRUE;
    if (xIS_SET((ch)->affected_by, AFF_EVILSURGE))
        return TRUE;
    if (xIS_SET((ch)->affected_by, AFF_EVILOVERLOAD))
        return TRUE;
    if (xIS_SET((ch)->affected_by, AFF_BIOJR))
        return TRUE;
    return FALSE;
}

bool is_leet( CHAR_DATA *ch )
{
    if( xIS_SET(ch->act, PLR_1337 ) )
      return TRUE;

    return FALSE;
}

bool is_split( CHAR_DATA *ch )
{
  if( xIS_SET(ch->affected_by, AFF_SPLIT_FORM) && IS_NPC(ch) )
    {
      return TRUE;
    }
  if( xIS_SET(ch->affected_by, AFF_TRI_FORM) && IS_NPC(ch) )
    {
      return TRUE;
    }
  if( xIS_SET(ch->affected_by, AFF_MULTI_FORM) && IS_NPC(ch) )
    {
      return TRUE;
    }
  if( xIS_SET(ch->affected_by, AFF_BIOJR) && IS_NPC(ch) )
    {
      return TRUE;
    }
  return FALSE;
}
bool is_saiyan_s( CHAR_DATA *ch )
{
  if( !str_cmp(get_race(ch),"saiyan-s"))
    {
      return TRUE;
    }
  return FALSE;
}
bool is_saiyan_n( CHAR_DATA *ch )
{
  if( !str_cmp(get_race(ch),"saiyan-n"))
    {
      return TRUE;
    }
  return FALSE;
}
bool is_saiyan_h( CHAR_DATA *ch )
{
  if( !str_cmp(get_race(ch),"saiyan-h"))
    {
      return TRUE;
    }
  return FALSE;
}
bool is_saiyan_hb( CHAR_DATA *ch )
{
  if( !str_cmp(get_race(ch),"saiyan-hb"))
    {
      return TRUE;
    }
  return FALSE;
}
bool is_human_h( CHAR_DATA *ch )
{
  if( !str_cmp(get_race(ch),"human-h"))
    {
      return TRUE;
    }
  return FALSE;
}
bool is_human_n( CHAR_DATA *ch )
{
  if( !str_cmp(get_race(ch),"human-n"))
    {
      return TRUE;
    }
  return FALSE;
}
bool is_human_hb( CHAR_DATA *ch )
{
  if( !str_cmp(get_race(ch),"human-hb"))
    {
      return TRUE;
    }
  return FALSE;
}
bool is_human_s( CHAR_DATA *ch )
{
  if( !str_cmp(get_race(ch),"human-s"))
    {
      return TRUE;
    }
  return FALSE;
}
bool is_namek_n( CHAR_DATA *ch )
{
  if( !str_cmp(get_race(ch),"namek-n"))
    {
      return TRUE;
    }
  return FALSE;
}
bool is_namek_h( CHAR_DATA *ch )
{
  if( !str_cmp(get_race(ch),"namek-h"))
    {
      return TRUE;
    }
  return FALSE;
}
bool is_namek_s( CHAR_DATA *ch )
{
  if( !str_cmp(get_race(ch),"namek-s"))
    {
      return TRUE;
    }
  return FALSE;
}
bool is_namek_hb( CHAR_DATA *ch )
{
  if( !str_cmp(get_race(ch),"namek-hb"))
    {
      return TRUE;
    }
  return FALSE;
}
bool is_hb_hb( CHAR_DATA *ch )
{
  if( !str_cmp(get_race(ch),"halfbreed-hb"))
    {
      return TRUE;
    }
  return FALSE;
}
bool is_hb_h( CHAR_DATA *ch )
{
  if( !str_cmp(get_race(ch),"halfbreed-h"))
    {
      return TRUE;
    }
  return FALSE;
}
bool is_hb_n( CHAR_DATA *ch )
{
  if( !str_cmp(get_race(ch),"halfbreed-n"))
    {
      return TRUE;
    }
  return FALSE;
}
bool is_hb_s( CHAR_DATA *ch )
{
  if( !str_cmp(get_race(ch),"halfbreed-s"))
    {
      return TRUE;
    }
  return FALSE;
}
bool kairanked( CHAR_DATA *ch )
{
  if( is_kaio(ch) && ch->kairank > 0 )
  {
    return TRUE;
  }
  return FALSE;
}
bool demonranked( CHAR_DATA *ch )
{
  if( is_demon(ch) && ch->demonrank > 0 )
  {
    return TRUE;
  }
  return FALSE;
}
void wss_scimitar( CHAR_DATA *ch, CHAR_DATA *victim, char *msg, int dam )
{
    if( !str_cmp(msg,"normal") )
    {

	act( AT_RED, "You begin charging a large amount of energy down through your arms and into your scimitars. They start glowing bright red as they soak up a high degree of evil power. You draw your arms back, then snap them forward, crossing your arms as you let go; sending the scimitars spinning through the air like sawblades at $N.", ch, NULL, victim, TO_CHAR );
	act( AT_RED, "The glowing scimitars slam into $N....and shatter! As the broken halves of the two blades fly away from $N, the energy they left behind erupts, enveloping $N in a gigantic explosion. &W[$t]", ch, num_punct(dam), victim, TO_CHAR );
	act( AT_RED, "$n begins charging a large amount of energy down through $s arms and into $s scimitars. They start glowing bright red as they soak up a high degree of evil power. $n draws $s arms back, then snaps them forward, crossing $s arms as $e lets go; sending the scimitars spinning through the air like sawblades at you.", ch, NULL, victim, TO_VICT );
	act( AT_RED, "The glowing scimitars slam into you....and shatter! As the broken halves of the two blades fly away from you, the energy they left behind erupts, enveloping you in a gigantic explosion. &W[$t]", ch, num_punct(dam), victim, TO_VICT );
	act( AT_RED, "$n begins charging a large amount of energy down through $s arms and into $s scimitars. They start glowing bright red as they soak up a high degree of evil power. $n draws $s arms back, then snaps them forward, crossing $s arms as $e lets go; sending the scimitars spinning through the air like sawblades at $N.", ch, NULL, victim, TO_NOTVICT );
	act( AT_RED, "The glowing scimitars slam into $N....and shatter! As the broken halves of the two blades fly away from $N, the energy they left behind erupts, enveloping $N in a gigantic explosion. &W[$t]", ch, num_punct(dam), victim, TO_NOTVICT );
    }
    if( !str_cmp(msg,"ikwork") )
    {
	act( AT_RED, "You begin charging a large amount of energy down through your arms and into your scimitars. They start glowing bright red as they soak up a high degree of evil power. You draw your arms back, then snap them forward, crossing your arms as you let go; sending the scimitars spinning through the air like sawblades at $N.", ch, NULL, victim, TO_CHAR );
	act( AT_DGREY, "The glowing scimitars slice right through $N's body, instantly killing $M in a spray of blood, guts and dismembered limbs. &W[$t]", ch, num_punct(dam), victim, TO_CHAR );
	act( AT_RED, "$n begins charging a large amount of energy down through $s arms and into $s scimitars. They start glowing bright red as they soak up a high degree of evil power. $n draws $s arms back, then snaps them forward, crossing $s arms as $e lets go; sending the scimitars spinning through the air like sawblades at you.", ch, NULL, victim, TO_VICT );
	act( AT_DGREY, "The glowing scimitars slice right through your body, instantly killing you in a spray of blood, guts and dismembered limbs. &W[$t]", ch, num_punct(dam), victim, TO_VICT );
	act( AT_RED, "$n begins charging a large amount of energy down through $s arms and into $s scimitars. They start glowing bright red as they soak up a high degree of evil power. $n draws $s arms back, then snaps them forward, crossing $s arms as $e lets go; sending the scimitars spinning through the air like sawblades at $N.", ch, NULL, victim, TO_NOTVICT );
	act( AT_DGREY, "The glowing scimitars slice right through $N's body, instantly killing $M in a spray of blood, guts and dismembered limbs. &W[$t]", ch, num_punct(dam), victim, TO_NOTVICT );
    }
    if( !str_cmp(msg,"ikfail") )
    {
	act( AT_RED, "You begin charging a large amount of energy down through your arms and into your scimitars. They start glowing bright red as they soak up a high degree of evil power. You draw your arms back, then snap them forward, crossing your arms as you let go; sending the scimitars spinning through the air like sawblades at $N.", ch, NULL, victim, TO_CHAR );
	act( AT_CYAN, "The glowing scimitars slice right through $N's body.....but it was just an after-image.", ch, NULL, victim, TO_CHAR );
	act( AT_RED, "$n begins charging a large amount of energy down through $s arms and into $s scimitars. They start glowing bright red as they soak up a high degree of evil power. $n draws $s arms back, then snaps them forward, crossing $s arms as $e lets go; sending the scimitars spinning through the air like sawblades at you.", ch, NULL, victim, TO_VICT );
	act( AT_CYAN, "At the last second you create an after-image and dodge far to the side, avoiding death.", ch, NULL, victim, TO_VICT );
	act( AT_RED, "$n begins charging a large amount of energy down through $s arms and into $s scimitars. They start glowing bright red as they soak up a high degree of evil power. $n draws $s arms back, then snaps them forward, crossing $s arms as $e lets go; sending the scimitars spinning through the air like sawblades at $N.", ch, NULL, victim, TO_NOTVICT );
	act( AT_CYAN, "The glowing scimitars slice right through $N's body.....but it was just an after-image.", ch, NULL, victim, TO_NOTVICT );
    }
}
void wss_sword( CHAR_DATA *ch, CHAR_DATA *victim, char *msg, int dam )
{
    if( !str_cmp(msg,"normal") )
    {
	act( AT_RED, "You begin charging a large amount of energy down through your arm and into your sword. It starts glowing bright red as it soaks up a high degree of evil power. You draw your arm back, then snap it forward and let go; sending the sword shooting straight through the air like a bullet at $N.", ch, NULL, victim, TO_CHAR );
	act( AT_RED, "The glowing sword slams into $N....and shatters! As the broken halves of the blade fly away from $N, the energy they left behind erupts, enveloping $N in a gigantic explosion. &W[$t]", ch, num_punct(dam), victim, TO_CHAR );
	act( AT_RED, "$n begins charging a large amount of energy down through $s arm and into $s sword. It starts glowing bright red as it soaks up a high degree of evil power. $n draws $s arm back, then snaps it forward and lets go; sending the sword shooting straight through the air like a bullet at you.", ch, NULL, victim, TO_VICT );
	act( AT_RED, "The glowing sword slams into you....and shatters! As the broken halves of the blade fly away from you, the energy they left behind erupts, enveloping you in a gigantic explosion. &W[$t]", ch, num_punct(dam), victim, TO_VICT );
	act( AT_RED, "$n begins charging a large amount of energy down through $s arm and into $s sword. It starts glowing bright red as it soaks up a high degree of evil power. $n draws $s arm back, then snaps it forward and lets go; sending the sword shooting straight through the air like a bullet at $N.", ch, NULL, victim, TO_NOTVICT );
	act( AT_RED, "The glowing sword slams into $N....and shatters! As the broken halves of the blade fly away from $N, the energy they left behind erupts, enveloping $N in a gigantic explosion. &W[$t]", ch, num_punct(dam), victim, TO_NOTVICT );
    }
    if( !str_cmp(msg,"ikwork") )
    {
	act( AT_RED, "You begin charging a large amount of energy down through your arm and into your sword. It starts glowing bright red as it soaks up a high degree of evil power. You draw your arm back, then snap it forward and let go; sending the sword shooting straight through the air like a bullet at $N.", ch, NULL, victim, TO_CHAR );
	act( AT_DGREY, "The glowing sword impales $N right in the chest, killing $M instantly. &W[$t]", ch, num_punct(dam), victim, TO_CHAR );
	act( AT_RED, "$n begins charging a large amount of energy down through $s arm and into $s sword. It starts glowing bright red as it soaks up a high degree of evil power. $n draws $s arm back, then snaps it forward and lets go; sending the sword shooting straight through the air like a bullet at you.", ch, NULL, victim, TO_VICT );
	act( AT_DGREY, "The glowing sword impales you right in the chest, killing you instantly. &W[$t]", ch, num_punct(dam), victim, TO_VICT );
	act( AT_RED, "$n begins charging a large amount of energy down through $s arm and into $s sword. It starts glowing bright red as it soaks up a high degree of evil power. $n draws $s arm back, then snaps it forward and lets go; sending the sword shooting straight through the air like a bullet at $N.", ch, NULL, victim, TO_NOTVICT );
	act( AT_DGREY, "The glowing sword impales $N right in the chest, killing $M instantly. &W[$t]", ch, num_punct(dam), victim, TO_NOTVICT );
    }
    if( !str_cmp(msg,"ikfail") )
    {
	act( AT_RED, "You begin charging a large amount of energy down through your arm and into your sword. It starts glowing bright red as it soaks up a high degree of evil power. You draw your arm back, then snap it forward and let go; sending the sword shooting straight through the air like a bullet at $N.", ch, NULL, victim, TO_CHAR );
	act( AT_CYAN, "$N dodges the glowing sword at the last second, the blade flying off into the distance before it explodes.", ch, NULL, victim, TO_CHAR );
	act( AT_RED, "$n begins charging a large amount of energy down through $s arm and into $s sword. It starts glowing bright red as it soaks up a high degree of evil power. $n draws $s arm back, then snaps it forward and lets go; sending the sword shooting straight through the air like a bullet at you.", ch, NULL, victim, TO_VICT );
	act( AT_CYAN, "You dodge the glowing sword at the last second, the blade flying off into the distance before it explodes.", ch, NULL, victim, TO_VICT );
	act( AT_RED, "$n begins charging a large amount of energy down through $s arm and into $s sword. It starts glowing bright red as it soaks up a high degree of evil power. $n draws $s arm back, then snaps it forward and lets go; sending the sword shooting straight through the air like a bullet at $N.", ch, NULL, victim, TO_NOTVICT );
	act( AT_CYAN, "$N dodges the glowing sword at the last second, the blade flying off into the distance before it explodes.", ch, NULL, victim, TO_NOTVICT );
    }
}
void wss_lance( CHAR_DATA *ch, CHAR_DATA *victim, char *msg, int dam )
{
    if( !str_cmp(msg,"normal") )
    {
	act( AT_RED, "You begin charging a large amount of energy down through your arm and into your lance. It starts glowing bright red as it soaks up a high degree of evil power. You draw your arm back, then snap it forward and let go; sending the lance shooting straight through the air like a bullet at $N.", ch, NULL, victim, TO_CHAR );
	act( AT_RED, "The glowing lance slams into $N....and shatters! As the broken halves of the polearm fly away from $N, the energy they left behind erupts, enveloping $N in a gigantic explosion. &W[$t]", ch, num_punct(dam), victim, TO_CHAR );
	act( AT_RED, "$n begins charging a large amount of energy down through $s arm and into $s lance. It starts glowing bright red as it soaks up a high degree of evil power. $n draws $s arm back, then snaps it forward and lets go; sending the lance shooting straight through the air like a bullet at you.", ch, NULL, victim, TO_VICT );
	act( AT_RED, "The glowing lance slams into you....and shatters! As the broken halves of the polearm fly away from you, the energy they left behind erupts, enveloping you in a gigantic explosion. &W[$t]", ch, num_punct(dam), victim, TO_VICT );
	act( AT_RED, "$n begins charging a large amount of energy down through $s arm and into $s lance. It starts glowing bright red as it soaks up a high degree of evil power. $n draws $s arm back, then snaps it forward and lets go; sending the lance shooting straight through the air like a bullet at $N.", ch, NULL, victim, TO_NOTVICT );
	act( AT_RED, "The glowing lance slams into $N....and shatters! As the broken halves of the polearm fly away from $N, the energy they left behind erupts, enveloping $N in a gigantic explosion. &W[$t]", ch, num_punct(dam), victim, TO_NOTVICT );
    }
    if( !str_cmp(msg,"ikwork") )
    {
	act( AT_RED, "You begin charging a large amount of energy down through your arm and into your lance. It starts glowing bright red as it soaks up a high degree of evil power. You draw your arm back, then snap it forward and let go; sending the lance shooting straight through the air like a bullet at $N.", ch, NULL, victim, TO_CHAR );
	act( AT_DGREY, "The glowing lance impales $N right in the chest, killing $M instantly. &W[$t]", ch, num_punct(dam), victim, TO_CHAR );
	act( AT_RED, "$n begins charging a large amount of energy down through $s arm and into $s lance. It starts glowing bright red as it soaks up a high degree of evil power. $n draws $s arm back, then snaps it forward and lets go; sending the lance shooting straight through the air like a bullet at you.", ch, NULL, victim, TO_VICT );
	act( AT_DGREY, "The glowing lance impales you right in the chest, killing you instantly. &W[$t]", ch, num_punct(dam), victim, TO_VICT );
	act( AT_RED, "$n begins charging a large amount of energy down through $s arm and into $s lance. It starts glowing bright red as it soaks up a high degree of evil power. $n draws $s arm back, then snaps it forward and lets go; sending the lance shooting straight through the air like a bullet at $N.", ch, NULL, victim, TO_NOTVICT );
	act( AT_DGREY, "The glowing lance impales $N right in the chest, killing $M instantly. &W[$t]", ch, num_punct(dam), victim, TO_NOTVICT );
    }
    if( !str_cmp(msg,"ikfail") )
    {
	act( AT_RED, "You begin charging a large amount of energy down through your arm and into your lance. It starts glowing bright red as it soaks up a high degree of evil power. You draw your arm back, then snap it forward and let go; sending the lance shooting straight through the air like a bullet at $N.", ch, NULL, victim, TO_CHAR );
	act( AT_CYAN, "$N dodges the glowing lance at the last second, the polearm flying off into the distance before it explodes.", ch, NULL, victim, TO_CHAR );
	act( AT_RED, "$n begins charging a large amount of energy down through $s arm and into $s lance. It starts glowing bright red as it soaks up a high degree of evil power. $n draws $s arm back, then snaps it forward and lets go; sending the lance shooting straight through the air like a bullet at you.", ch, NULL, victim, TO_VICT );
	act( AT_CYAN, "You dodge the glowing lance at the last second, the polearm flying off into the distance before it explodes.", ch, NULL, victim, TO_VICT );
	act( AT_RED, "$n begins charging a large amount of energy down through $s arm and into $s lance. It starts glowing bright red as it soaks up a high degree of evil power. $n draws $s arm back, then snaps it forward and lets go; sending the lance shooting straight through the air like a bullet at $N.", ch, NULL, victim, TO_NOTVICT );
	act( AT_CYAN, "$N dodges the glowing lance at the last second, the polearm flying off into the distance before it explodes.", ch, NULL, victim, TO_NOTVICT );
    }
}
void wss_maul( CHAR_DATA *ch, CHAR_DATA *victim, char *msg, int dam )
{
    if( !str_cmp(msg,"normal") )
    {
	act( AT_RED, "You begin charging a large amount of energy down through your arms and into your maul. It starts glowing bright red as it soaks up a high degree of evil power. You grip the base of the maul in both hands and begin spinning around. Once you are spinning so fast that you become a blur, you let go of the maul; sending it shooting towards $N like a bullet.", ch, NULL, victim, TO_CHAR );
	act( AT_RED, "The glowing maul slams into $N....and shatters! As the broken halves of the maul fly away from $N, the energy they left behind erupts, enveloping $N in a gigantic explosion. &W[$t]", ch, num_punct(dam), victim, TO_CHAR );
	act( AT_RED, "$n begins charging a large amount of energy down through $s arms and into $s maul. It starts glowing bright red as it soaks up a high degree of evil power. $n grips the base of the maul in both hands and begins spinning around. Once $e is spinning so fast that $e becomes a blur, $e lets go of the maul; sending it shooting towards you like a bullet.", ch, NULL, victim, TO_VICT );
	act( AT_RED, "The glowing maul slams into you....and shatters! As the broken halves of the maul fly away from you, the energy they left behind erupts, enveloping you in a gigantic explosion. &W[$t]", ch, num_punct(dam), victim, TO_VICT );
	act( AT_RED, "$n begins charging a large amount of energy down through $s arms and into $s maul. It starts glowing bright red as it soaks up a high degree of evil power. $n grips the base of the maul in both hands and begins spinning around. Once $e is spinning so fast that $e becomes a blur, $e lets go of the maul; sending it shooting towards $N like a bullet.", ch, NULL, victim, TO_NOTVICT );
	act( AT_RED, "The glowing maul slams into $N....and shatters! As the broken halves of the maul fly away from $N, the energy they left behind erupts, enveloping $N in a gigantic explosion. &W[$t]", ch, num_punct(dam), victim, TO_NOTVICT );
    }

    if( !str_cmp(msg,"ikwork") )
    {
	act( AT_RED, "You begin charging a large amount of energy down through your arms and into your maul. It starts glowing bright red as it soaks up a high degree of evil power. You grip the base of the maul in both hands and begin spinning around. Once you are spinning so fast that you become a blur, you let go of the maul; sending it shooting towards $N like a bullet.", ch, NULL, victim, TO_CHAR );
	act( AT_DGREY, "You vanish just as the maul comes within close range of $N, reappearing near it. You grab it right out of the air, and spinning with the momentum, in a single motion, you swing the maul around and slam its head right into the back of $N's head; causing $S head to explode in a spray of bone and flesh, killing $M instantly. &W[$t]", ch, num_punct(dam), victim, TO_CHAR );
	act( AT_RED, "$n begins charging a large amount of energy down through $s arms and into $s maul. It starts glowing bright red as it soaks up a high degree of evil power. $n grips the base of the maul in both hands and begins spinning around. Once $e is spinning so fast that $e becomes a blur, $e lets go of the maul; sending it shooting towards you like a bullet.", ch, NULL, victim, TO_VICT );
	act( AT_DGREY, "$n vanishes just as the maul comes within close range of you, reappearing near it. $*e grabs it right out of the air, and spinning with the momentum, in a single motion, $n swings the maul around and slam its head right into the back of your head; causing your head to explode in a spray of bone and flesh, killing you instantly. &W[$t]", ch, num_punct(dam), victim, TO_VICT );
	act( AT_RED, "$n begins charging a large amount of energy down through $s arms and into $s maul. It starts glowing bright red as it soaks up a high degree of evil power. $n grips the base of the maul in both hands and begins spinning around. Once $e is spinning so fast that $e becomes a blur, $e lets go of the maul; sending it shooting towards $N like a bullet.", ch, NULL, victim, TO_NOTVICT );
	act( AT_DGREY, "$n vanishes just as the maul comes within close range of $N, reappearing near it. $*e grabs it right out of the air, and spinning with the momentum, in a single motion, $n swings the maul around and slam its head right into the back of $N's head; causing $S head to explode in a spray of bone and flesh, killing $N instantly. &W[$t]", ch, num_punct(dam), victim, TO_NOTVICT );
    }
    if( !str_cmp(msg,"ikfail") )
    {
	act( AT_RED, "You begin charging a large amount of energy down through your arms and into your maul. It starts glowing bright red as it soaks up a high degree of evil power. You grip the base of the maul in both hands and begin spinning around. Once you are spinning so fast that you become a blur, you let go of the maul; sending it shooting towards $N like a bullet.", ch, NULL, victim, TO_CHAR );
	act( AT_CYAN, "$N dodges the glowing maul at the last second, the maul flying off into the distance before it explodes.", ch, NULL, victim, TO_CHAR );
	act( AT_RED, "$n begins charging a large amount of energy down through $s arms and into $s maul. It starts glowing bright red as it soaks up a high degree of evil power. $n grips the base of the maul in both hands and begins spinning around. Once $e is spinning so fast that $e becomes a blur, $e lets go of the maul; sending it shooting towards you like a bullet.", ch, NULL, victim, TO_VICT );
	act( AT_CYAN, "You dodge the glowing maul at the last second, the maul flying off into the distance before it explodes.", ch, NULL, victim, TO_VICT );
	act( AT_RED, "$n begins charging a large amount of energy down through $s arms and into $s maul. It starts glowing bright red as it soaks up a high degree of evil power. $n grips the base of the maul in both hands and begins spinning around. Once $e is spinning so fast that $e becomes a blur, $e lets go of the maul; sending it shooting towards $N like a bullet.", ch, NULL, victim, TO_NOTVICT );
	act( AT_CYAN, "$N dodges the glowing maul at the last second, the maul flying off into the distance before it explodes.", ch, NULL, victim, TO_NOTVICT );
    }
}
