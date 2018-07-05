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
 *                      Battle & death module                               *
 ****************************************************************************/

#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "mud.h"


extern char		lastplayercmd[MAX_INPUT_LENGTH];
extern CHAR_DATA *	gch_prev;

OBJ_DATA *used_weapon;   /* Used to figure out which weapon later */

/*
 * Local functions.
 */
void	new_dam_message	args( ( CHAR_DATA *ch, CHAR_DATA *victim, int dam,
			    int dt, OBJ_DATA *obj ) );
void	death_cry	args( ( CHAR_DATA *ch ) );
void	group_gain	args( ( CHAR_DATA *ch, CHAR_DATA *victim ) );
int		xp_compute	args( ( CHAR_DATA *gch, CHAR_DATA *victim ) );
int		align_compute	args( ( CHAR_DATA *gch, CHAR_DATA *victim ) );
ch_ret	one_hit		args( ( CHAR_DATA *ch, CHAR_DATA *victim, int dt ) );
int		obj_hitroll	args( ( OBJ_DATA *obj ) );
void    show_condition  args( ( CHAR_DATA *ch, CHAR_DATA *victim ) );
bool 	pkill_ok	args( ( CHAR_DATA *ch, CHAR_DATA *victim ) );

bool pkill_ok( CHAR_DATA *ch, CHAR_DATA *victim )
{
    bool clanbypass = FALSE;
    OBJ_DATA *o;

    if (IS_NPC(ch) || IS_NPC(victim))
	return TRUE;

    if (ch->exp <= 100000)
    {
	send_to_char( "You can not fight other players until you are a 'Skilled Fighter'.\n\r", ch );
	return FALSE;
    }
    if (victim->exp <= 100000)
    {
	send_to_char( "You can not fight other players until they are a 'Skilled Fighter'.\n\r", ch );
	return FALSE;
    }

    if( (o = carrying_dball(victim)) != NULL )
    {
	return TRUE;
    }

    if (ch->pcdata->clan && victim->pcdata->clan
	&& !xIS_SET(victim->act, PLR_PK1)
	&& !xIS_SET(victim->act, PLR_PK2) )
    {
	if( is_kaio(ch) && kairanked(victim)
	   && ch->kairank <=  victim->kairank )
	  clanbypass = TRUE;
	else if( is_demon(ch) && demonranked(victim) )
          clanbypass = TRUE;
	else if( kairanked(ch) && demonranked(victim) )
          clanbypass = TRUE;
        else if( demonranked(ch) && demonranked(victim) )
          clanbypass = TRUE;

	if( !clanbypass )
	{
		if (ch->pcdata->clan == victim->pcdata->clan )
		{
			send_to_char( "You can't kill another member of your clan!\n\r", ch );
			return FALSE;
		}
/**/		if (ch->pcdata->clan != victim->pcdata->clan)
		{
			switch (allianceStatus(ch->pcdata->clan, victim->pcdata->clan))
			{
				case ALLIANCE_FRIENDLY:
				case ALLIANCE_ALLIED:
					send_to_char( "Your clan is at peace with theirs!.\n\r", ch );
					return FALSE;
					break;
				case ALLIANCE_NEUTRAL:
					send_to_char( "Your clan is in a state of neutrality with theirs!\n\r", ch );
					return FALSE;
					break;
				case ALLIANCE_HOSTILE:
					if( xIS_SET(victim->act, PLR_WAR1) )
                                        {
                                          if ((float)ch->exp / victim->exp > 5 && !IS_HC(victim))
                                          {
                                            ch_printf(ch,"You are more than 5 times stronger.\n\r");
                                            return FALSE;
                                          }
                                          xSET_BIT(ch->act,PLR_WAR1);
                                          ch->pcdata->pk_timer = 60;
                                          return TRUE;
                                        }
					else if( xIS_SET(victim->act,PLR_WAR2) )
                                        {
                                          xSET_BIT(ch->act,PLR_WAR2);
                                          ch->pcdata->pk_timer = 60;
                                          return TRUE;
                                        }
					ch_printf(ch,"They have to have a war flag on when you're not at full war status with that clan!\n\r");
					return FALSE;
					break;
				case ALLIANCE_ATWAR:
					if ( !xIS_SET(ch->act, PLR_WAR2) )
					  xSET_BIT(ch->act,PLR_WAR2);
					else if ( !xIS_SET(ch->act, PLR_WAR1) )
                                          xSET_BIT(ch->act,PLR_WAR1);
					ch->pcdata->pk_timer = 60;
					return TRUE;
					break;
				default:
                                        break;
			}
		} /**/
        }
    }

	if (!IS_HC(ch))
	{
		ch->pcdata->pk_timer = 60;
	}
		ch->pcdata->gohometimer = 30;

	if (IS_HC(victim) && !IS_HC(ch))
	{
		xREMOVE_BIT(ch->act, PLR_PK1);
		xSET_BIT(ch->act, PLR_PK2);
		return TRUE;
	}


	if (xIS_SET(victim->act, PLR_BOUNTY) && victim->pcdata->bounty > 0
		&& !str_cmp( victim->name, ch->pcdata->hunting ))
		{
                if ( xIS_SET( victim->in_room->room_flags, ROOM_SAFE ) )
                {
                  return FALSE;
                }

		if ( xIS_SET(victim->act, PLR_PK1) && !IS_HC(ch))
		{
			xREMOVE_BIT(ch->act, PLR_PK1);
			xSET_BIT(ch->act, PLR_PK2);
		}
		else if ( !xIS_SET(victim->act, PLR_PK2) && !IS_HC(ch))
                {
			xSET_BIT(ch->act, PLR_PK2);
                }
                else if ( !xIS_SET(victim->act, PLR_PK1) && !xIS_SET(victim->act, PLR_PK2)
                      &&  !IS_HC( victim ) )
                {
                  return FALSE;
                }

		return TRUE;
		}

	if (xIS_SET(victim->act, PLR_PK1) || xIS_SET(victim->act, PLR_WAR1))
	{
	    if ((float)ch->exp / victim->exp > 5)
	    {
		send_to_char( "They have a yellow PK flag and you are more than 5 times stronger.\n\r", ch );

		if (!xIS_SET(ch->act, PLR_PK2) && !IS_HC(ch))
			xSET_BIT(ch->act, PLR_PK1);
		return FALSE;
		}
	}

	if (!is_atwar(ch, victim) && !IS_HC(ch))
	{
		if ( xIS_SET(victim->act, PLR_PK1) && !xIS_SET(ch->act, PLR_PK2))
			xSET_BIT(ch->act, PLR_PK1);
		else if (xIS_SET(victim->act, PLR_PK2))
		{
			if (xIS_SET(ch->act, PLR_PK1))
				xREMOVE_BIT(ch->act, PLR_PK1);
			xSET_BIT(ch->act, PLR_PK2);
		}
		else
		{
			if (xIS_SET(ch->act, PLR_PK1))
				xREMOVE_BIT(ch->act, PLR_PK2);
			else if (xIS_SET(ch->act, PLR_PK2))
				xREMOVE_BIT(ch->act, PLR_PK1);
			else
				xSET_BIT(ch->act, PLR_PK1);
		}
	}

	/*
	if (is_atwar(ch, victim))
	{
		if ( xIS_SET(victim->act, PLR_WAR1) && !xIS_SET(ch->act, PLR_WAR2))
			xSET_BIT(ch->act, PLR_WAR1);
		else if (xIS_SET(victim->act, PLR_WAR2))
		{
			if (xIS_SET(ch->act, PLR_WAR1))
				xREMOVE_BIT(ch->act, PLR_WAR1);
			xSET_BIT(ch->act, PLR_WAR2);
		}
		else
		{
			if (xIS_SET(ch->act, PLR_WAR1))
				xREMOVE_BIT(ch->act, PLR_WAR2);
			else if (xIS_SET(ch->act, PLR_WAR2))
				xREMOVE_BIT(ch->act, PLR_WAR1);
			else
				xSET_BIT(ch->act, PLR_WAR1);
		}
	}
	*/
    if( is_kaio(ch)&& kairanked(victim)
        && ch->kairank <= victim->kairank )
    {
	if ((float)ch->exp / victim->exp > 5 )
	{
	  ch_printf(ch,"You are 5 times stronger.\n\r");
	  return FALSE;
	}
	return TRUE;
    }
    if( is_demon(ch)&& demonranked(victim) )
    {
	if ((float)ch->exp / victim->exp > 5 )
        {
          ch_printf(ch,"You are 5 times stronger.\n\r");
          return FALSE;
        }
        return TRUE;
    }

    if( kairanked(ch) && demonranked(victim) )
    {
	if ((float)ch->exp / victim->exp > 5 )
        {
          ch_printf(ch,"You are 5 times stronger.\n\r");
          return FALSE;
        }
	return TRUE;
    }
    if( demonranked(ch) && kairanked(victim) )
    {
	if ((float)ch->exp / victim->exp > 5 )
        {
          ch_printf(ch,"You are 5 times stronger.\n\r");
          return FALSE;
        }
        return TRUE;
    }

    if (!xIS_SET(victim->act, PLR_PK1) && !xIS_SET(victim->act, PLR_PK2)
		&& !xIS_SET(victim->act, PLR_WAR1) && !xIS_SET(victim->act, PLR_WAR2)
		&& !IS_HC(victim))
    {
		send_to_char( "You can't kill someone without a PK flag.\n\r", ch );
		return FALSE;
    }

	return TRUE;
}




/*
 * Check to see if player's attacks are (still?) suppressed
 * #ifdef TRI
 */
bool is_attack_supressed( CHAR_DATA *ch )
{
  TIMER *timer;

  if (IS_NPC(ch))
    return FALSE;

  timer = get_timerptr( ch, TIMER_ASUPRESSED );

  if ( !timer )
	  timer = get_timerptr( ch, TIMER_DELAY );

  if ( !timer )
    return FALSE;

  /* perma-supression -- bard? (can be reset at end of fight, or spell, etc) */
  if ( timer->value == -1 )
    return TRUE;

  /* this is for timed supressions */
  if ( timer->count >= 1 )
    return TRUE;

  return FALSE;
}

/*
 * Check to see if weapon is poisoned.
 */
bool is_wielding_poisoned( CHAR_DATA *ch )
{
    OBJ_DATA *obj;

    if ( !used_weapon )
    	return FALSE;

    if ( (obj=get_eq_char(ch, WEAR_WIELD)) != NULL
    &&    used_weapon == obj
    &&    IS_OBJ_STAT(obj, ITEM_POISONED) )
	return TRUE;
    if ( (obj=get_eq_char(ch, WEAR_DUAL_WIELD)) != NULL
    &&    used_weapon == obj
    &&	  IS_OBJ_STAT(obj, ITEM_POISONED) )
    	return TRUE;

    return FALSE;
}

/*
 * hunting, hating and fearing code				-Thoric
 */
bool is_hunting( CHAR_DATA *ch, CHAR_DATA *victim )
{
    if ( !ch->hunting || ch->hunting->who != victim )
      return FALSE;

    return TRUE;
}

bool is_hating( CHAR_DATA *ch, CHAR_DATA *victim )
{
    if ( !ch->hating || ch->hating->who != victim )
      return FALSE;

    return TRUE;
}

bool is_fearing( CHAR_DATA *ch, CHAR_DATA *victim )
{
    if ( !ch->fearing || ch->fearing->who != victim )
      return FALSE;

    return TRUE;
}

void stop_hunting( CHAR_DATA *ch )
{
    if ( ch->hunting )
    {
	STRFREE( ch->hunting->name );
	DISPOSE( ch->hunting );
	ch->hunting = NULL;
    }
    return;
}

void stop_hating( CHAR_DATA *ch )
{
    if ( ch->hating )
    {
	STRFREE( ch->hating->name );
	DISPOSE( ch->hating );
	ch->hating = NULL;
    }
    return;
}

void stop_fearing( CHAR_DATA *ch )
{
    if ( ch->fearing )
    {
	STRFREE( ch->fearing->name );
	DISPOSE( ch->fearing );
	ch->fearing = NULL;
    }
    return;
}

void start_hunting( CHAR_DATA *ch, CHAR_DATA *victim )
{
    if ( ch->hunting )
      stop_hunting( ch );

    CREATE( ch->hunting, HHF_DATA, 1 );
    ch->hunting->name = QUICKLINK( victim->name );
    ch->hunting->who  = victim;
    return;
}

void start_hating( CHAR_DATA *ch, CHAR_DATA *victim )
{
    if ( ch->hating )
      stop_hating( ch );

    CREATE( ch->hating, HHF_DATA, 1 );
    ch->hating->name = QUICKLINK( victim->name );
    ch->hating->who  = victim;
    return;
}

void start_fearing( CHAR_DATA *ch, CHAR_DATA *victim )
{
    if ( ch->fearing )
      stop_fearing( ch );

    CREATE( ch->fearing, HHF_DATA, 1 );
    ch->fearing->name = QUICKLINK( victim->name );
    ch->fearing->who  = victim;
    return;
}


int max_fight( CHAR_DATA *ch )
{
    return 8;
}



/*
 * Control the fights going on.
 * Called periodically by update_handler.
 * Many hours spent fixing bugs in here by Thoric, as noted by residual
 * debugging checks.  If you never get any of these error messages again
 * in your logs... then you can comment out some of the checks without
 * worry.
 *
 * Note:  This function also handles some non-violence updates.
 */
void violence_update( void )
{
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *ch;
    CHAR_DATA *lst_ch;
    CHAR_DATA *victim;
    CHAR_DATA *rch, *rch_next;
    AFFECT_DATA *paf, *paf_next;
    TIMER	*timer, *timer_next;
    ch_ret	retcode;
    int		attacktype, cnt;
    SKILLTYPE	*skill;
    static int	pulse = 0;
    bool	repeat = FALSE;

    lst_ch = NULL;
    pulse = (pulse+1) % 100;

    for ( ch = last_char; ch; lst_ch = ch, ch = gch_prev )
    {
	set_cur_char( ch );

	if ( ch == first_char && ch->prev )
	{
	   bug( "ERROR: first_char->prev != NULL, fixing...", 0 );
	   ch->prev = NULL;
	}

	gch_prev	= ch->prev;

	if ( gch_prev && gch_prev->next != ch )
	{
	    sprintf( buf, "FATAL: violence_update: %s->prev->next doesn't point to ch.",
		ch->name );
	    bug( buf, 0 );
	    bug( "Short-cutting here", 0 );
	    ch->prev = NULL;
	    gch_prev = NULL;
	    do_shout( ch, "Goku says, 'Prepare for the worst!'" );
	}

	/*
	 * See if we got a pointer to someone who recently died...
	 * if so, either the pointer is bad... or it's a player who
	 * "died", and is back at the healer...
	 * Since he/she's in the char_list, it's likely to be the later...
	 * and should not already be in another fight already
	 */
	if ( char_died(ch) )
	    continue;

	/*
	 * See if we got a pointer to some bad looking data...
	 */
	if ( !ch->in_room || !ch->name )
	{
	    log_string( "violence_update: bad ch record!  (Shortcutting.)" );
	    sprintf( buf, "ch: %d  ch->in_room: %d  ch->prev: %d  ch->next: %d",
	    	(int) ch, (int) ch->in_room, (int) ch->prev, (int) ch->next );
	    log_string( buf );
	    log_string( lastplayercmd );
	    if ( lst_ch )
	      sprintf( buf, "lst_ch: %d  lst_ch->prev: %d  lst_ch->next: %d",
	      		(int) lst_ch, (int) lst_ch->prev, (int) lst_ch->next );
	    else
	      strcpy( buf, "lst_ch: NULL" );
	    log_string( buf );
	    gch_prev = NULL;
	    continue;
	}

	for ( timer = ch->first_timer; timer; timer = timer_next )
	{
	    timer_next = timer->next;
	    if ( --timer->count <= 0 )
	    {
		if ( timer->type == TIMER_ASUPRESSED )
		{
		    if ( timer->value == -1 )
		    {
			timer->count = 1000;
			continue;
		    }
		}
		if ( timer->type == TIMER_NUISANCE )
		{
		    DISPOSE( ch->pcdata->nuisance );
		}

		if ( timer->type == TIMER_DO_FUN )
		{
		    int tempsub;

		    tempsub = ch->substate;
		    ch->substate = timer->value;
		    (timer->do_fun)( ch, "" );
		    if ( char_died(ch) )
			break;
		    if (ch->substate != timer->value && ch->substate != SUB_NONE)
		    	repeat = TRUE;
		    ch->substate = tempsub;
		}
		if ( timer->type == TIMER_DELAY )
		{
		    if (ch->delay_vict)
		    	rage( ch, ch->delay_vict );
		    if ( char_died(ch) )
			break;
		}
		if (!repeat)
			extract_timer( ch, timer );
	    }
	}

	/* Goku's timer function */
	if (ch->timerDelay)
	{
		ch->timerDelay--;
		if (ch->timerDelay <= 0)
		{
			ch->timerDelay = 0;
			ch->timerType = 0;
			if (ch->timerDo_fun)
				(ch->timerDo_fun)( ch, "" );
			if (ch->timerType == 0)
				ch->timerDo_fun = NULL;
		}
	}

	if ( char_died(ch) )
	    continue;

	/*
	 * We need spells that have shorter durations than an hour.
	 * So a melee round sounds good to me... -Thoric
	 */
	for ( paf = ch->first_affect; paf; paf = paf_next )
	{
	    paf_next	= paf->next;
	    if ( paf->duration > 0 )
		paf->duration--;
	    else
	    if ( paf->duration < 0 )
		;
	    else
	    {
		if ( !paf_next
		||    paf_next->type != paf->type
		||    paf_next->duration > 0 )
		{
		    skill = get_skilltype(paf->type);
		    if ( paf->type > 0 && skill && skill->msg_off )
		    {
			set_char_color( AT_WEAROFF, ch );
			send_to_char( skill->msg_off, ch );
			send_to_char( "\n\r", ch );
		    }
		}
/*
		if (paf->type == gsn_possess)
	        {
		    ch->desc->character = ch->desc->original;
		    ch->desc->original  = NULL;
		    if (!ch->desc->character->desc)
			    bug( "violence_update: NULL ch->desc->character->desc: CH = %s", ch->name, 0 );
		    else
			    ch->desc->character->desc = ch->desc;
		    ch->desc->character->switched = NULL;
		    ch->desc            = NULL;
		}
*/
		affect_remove( ch, paf );
	    }
	}

	/* Kiaoken drain */
	if (xIS_SET((ch)->affected_by, AFF_KAIOKEN) && !IS_NPC(ch)
	   && ch->desc )
	{
		kaioken_drain(ch);
		heart_calc(ch, "");
	}

	if ( char_died(ch) )
	    continue;

	/* check for exits moving players around */
	if ( (retcode=pullcheck(ch, pulse)) == rCHAR_DIED || char_died(ch) )
	    continue;

	/* so people charging attacks stop fighting */
	if (ch->charge > 0)
		continue;

	/* Let the battle begin! */

	if ( ( victim = who_fighting( ch ) ) == NULL
	||   IS_AFFECTED( ch, AFF_PARALYSIS ) )
	    continue;

	sysdata.outBytesFlag = LOGBOUTCOMBAT;

	int foc = 0;
	foc = URANGE(0,((number_range(1, UMAX(1,get_curr_int(ch))) / 5)), get_curr_int(ch));
	//ch->focus += URANGE(0,((number_range(1, UMAX(1, get_curr_int(ch))) / 5)), get_curr_int(ch));

	if( !IS_NPC(ch) )
	{
	  int div = 0;
	  if( ch->pcdata->learned[gsn_dodge] > 0 &&
              !IS_SET( ch->pcdata->combatFlags, CMB_NO_DODGE) )
	    div += 5;
	  if( ch->pcdata->learned[gsn_block] > 0 &&
              !IS_SET( ch->pcdata->combatFlags, CMB_NO_BLOCK) )
            div += 5;
	  if( ch->pcdata->learned[gsn_dcd] > 0 &&
              !IS_SET( ch->pcdata->combatFlags, CMB_NO_DCD) )
            div += 40;

	  if( div > 0 )
	  {
	    foc = (foc - ((foc/100) * div));
	  }
	}

	if( foc < 0 )
	  foc = 0;

	ch->focus += foc;
	ch->focus = URANGE(0,ch->focus,get_curr_int(ch));

        retcode = rNONE;

	if ( xIS_SET(ch->in_room->room_flags, ROOM_SAFE ) )
	{
	   sprintf( buf, "violence_update: %s fighting %s in a SAFE room.",
	   	ch->name, victim->name );
	   log_string( buf );
	   stop_fighting( ch, TRUE );
	}
	else
	if ( IS_AWAKE(ch) && ch->in_room == victim->in_room )
	    retcode = multi_hit( ch, victim, TYPE_UNDEFINED );
	else
	    stop_fighting( ch, FALSE );

	if ( char_died(ch) )
	    continue;

	if ( retcode == rCHAR_DIED
	|| ( victim = who_fighting( ch ) ) == NULL )
	    continue;

	/*
	 *  Mob triggers
	 *  -- Added some victim death checks, because it IS possible.. -- Alty
	 */
	rprog_rfight_trigger( ch );
	if ( char_died(ch) || char_died(victim) )
	    continue;
	mprog_hitprcnt_trigger( ch, victim );
	if ( char_died(ch) || char_died(victim) )
	    continue;
	mprog_fight_trigger( ch, victim );
	if ( char_died(ch) || char_died(victim) )
	    continue;

	/*
	 * NPC special attack flags				-Thoric
	 */
	if ( IS_NPC(ch) )
	{
	    if ( !xIS_EMPTY(ch->attacks) )
	    {
		attacktype = -1;
		if ( 30 + (ch->level/4) >= number_percent( ) )
		{
		    cnt = 0;
		    for ( ;; )
		    {
			if ( cnt++ > 10 )
			{
			    attacktype = -1;
			    break;
			}
			attacktype = number_range( 7, MAX_ATTACK_TYPE-1 );
			if ( xIS_SET( ch->attacks, attacktype ) )
			    break;
		    }
		    switch( attacktype )
		    {
			case ATCK_BASH:
			    do_bash( ch, "" );
			    retcode = global_retcode;
			    break;
			case ATCK_STUN:
			    do_stun( ch, "" );
			    retcode = global_retcode;
			    break;
			case ATCK_GOUGE:
			    do_gouge( ch, "" );
			    retcode = global_retcode;
			    break;
			case ATCK_FEED:
			    do_gouge( ch, "" );
			    retcode = global_retcode;
			    break;
		    }
		    if ( attacktype != -1 && (retcode == rCHAR_DIED || char_died(ch)) )
			continue;
		}
	    }
	    /*
	     * NPC special defense flags				-Thoric
	     */
	    if ( !xIS_EMPTY(ch->defenses) )
	    {
		attacktype = -1;
		if ( 50 + (ch->level/4) > number_percent( ) )
		{
		    cnt = 0;
		    for ( ;; )
		    {
			if ( cnt++ > 10 )
			{
			    attacktype = -1;
			    break;
			}
			attacktype = number_range( 2, MAX_DEFENSE_TYPE-1 );
			if ( xIS_SET( ch->defenses, attacktype ) )
			    break;
		    }

		    switch( attacktype )
		    {
			case DFND_CURELIGHT:
			    /* A few quick checks in the cure ones so that a) less spam and
			       b) we don't have mobs looking stupider than normal by healing
			       themselves when they aren't even being hit (although that
			       doesn't happen TOO often */
			    if ( ch->hit < ch->max_hit )
			    {
				act( AT_MAGIC, "$n mutters a few incantations...and looks a little better.", ch, NULL, NULL, TO_ROOM );
				retcode = spell_smaug( skill_lookup( "cure light" ), ch->level, ch, ch );
			    }
			    break;
			case DFND_CURESERIOUS:
			    if ( ch->hit < ch->max_hit )
			    {
				act( AT_MAGIC, "$n mutters a few incantations...and looks a bit better.", ch, NULL, NULL, TO_ROOM );
				retcode = spell_smaug( skill_lookup( "cure serious" ), ch->level, ch, ch );
			    }
			    break;
			case DFND_CURECRITICAL:
			    if ( ch->hit < ch->max_hit )
			    {
				act( AT_MAGIC, "$n mutters a few incantations...and looks healthier.", ch, NULL, NULL, TO_ROOM );
				retcode = spell_smaug( skill_lookup( "cure critical" ), ch->level, ch, ch );
			    }
			    break;
			case DFND_HEAL:
			    if ( ch->hit < ch->max_hit )
			    {
				act( AT_MAGIC, "$n mutters a few incantations...and looks much healthier.", ch, NULL, NULL, TO_ROOM );
				retcode = spell_smaug( skill_lookup( "heal" ), ch->level, ch, ch );
			    }
			    break;
	               /* Thanks to guppy@wavecomputers.net for catching this */
			case DFND_SHOCKSHIELD:
			    if ( !IS_AFFECTED( ch, AFF_SHOCKSHIELD ) )
			    {
				act( AT_MAGIC, "$n utters a few incantations...", ch, NULL, NULL, TO_ROOM );
				retcode = spell_smaug( skill_lookup( "shockshield" ), ch->level, ch, ch );
			    }
			    else
				retcode = rNONE;
			    break;
                       case DFND_VENOMSHIELD:
                            if ( !IS_AFFECTED( ch, AFF_VENOMSHIELD ) )
                            {
                                act( AT_MAGIC, "$n utters a few incantations ...", ch, NULL, NULL, TO_ROOM );
                                retcode = spell_smaug( skill_lookup( "venomshield" ), ch->level, ch, ch );
                            }
                            else
                                retcode = rNONE;
                            break;
			case DFND_ACIDMIST:
			    if ( !IS_AFFECTED( ch, AFF_ACIDMIST ) )
			    {
				act( AT_MAGIC, "$n utters a few incantations ...", ch, NULL, NULL, TO_ROOM );
				retcode = spell_smaug( skill_lookup( "acidmist" ), ch->level, ch, ch );
			    }
			    else
				retcode = rNONE;
			    break;
			case DFND_FIRESHIELD:
			    if ( !IS_AFFECTED( ch, AFF_FIRESHIELD ) )
			    {
				act( AT_MAGIC, "$n utters a few incantations...", ch, NULL, NULL, TO_ROOM );
				retcode = spell_smaug( skill_lookup( "fireshield" ), ch->level, ch, ch );
			    }
			    else
				retcode = rNONE;
			    break;
			case DFND_ICESHIELD:
			    if ( !IS_AFFECTED( ch, AFF_ICESHIELD ) )
			    {
				act( AT_MAGIC, "$n utters a few incantations...", ch, NULL, NULL, TO_ROOM );
				retcode = spell_smaug( skill_lookup( "iceshield" ), ch->level, ch, ch );
			    }
			    else
				retcode = rNONE;
			    break;
			case DFND_TRUESIGHT:
			    if ( !IS_AFFECTED( ch, AFF_TRUESIGHT ) )
				retcode = spell_smaug( skill_lookup( "true" ), ch->level, ch, ch );
			    else
				retcode = rNONE;
			    break;
			case DFND_SANCTUARY:
			    if ( !IS_AFFECTED( ch, AFF_SANCTUARY ) )
			    {
				act( AT_MAGIC, "$n utters a few incantations...", ch, NULL, NULL, TO_ROOM );
				retcode = spell_smaug( skill_lookup( "sanctuary" ), ch->level, ch, ch );
			    }
			    else
				retcode = rNONE;
			    break;
		    }
		    if ( attacktype != -1 && (retcode == rCHAR_DIED || char_died(ch)) )
			continue;
		}
	    }
	}

	/*
	 * Fun for the whole family!
	 */
	for ( rch = ch->in_room->first_person; rch; rch = rch_next )
	{
	    rch_next = rch->next_in_room;

            /*
             *   Group Fighting Styles Support:
             *   If ch is tanking
             *   If rch is using a more aggressive style than ch
             *   Then rch is the new tank   -h
             */
            /* &&( is_same_group(ch, rch)      ) */

            if( ( !IS_NPC(ch) && !IS_NPC(rch) )
              &&( rch!=ch                     )
              &&( rch->fighting               )
              &&( who_fighting(rch->fighting->who) == ch    )
              &&( !xIS_SET( rch->fighting->who->act, ACT_AUTONOMOUS ) )
              &&( rch->style < ch->style      )
              )
            {
                 rch->fighting->who->fighting->who = rch;

            }

	    if ( IS_AWAKE(rch) && !rch->fighting )
	    {
		/*
		 * Split forms auto-assist others in their group.
		 */

		if ( !IS_NPC(ch) && is_splitformed(ch) )
		{
			if ( IS_NPC(rch)
				&& is_split(rch)
				&& is_same_group(ch, rch)
				&& !is_safe(rch, victim, TRUE) )
				multi_hit( rch, victim, TYPE_UNDEFINED );
			continue;
		}

		/*
		 * PC's auto-assist others in their group.
		 */
		if ( !IS_NPC(ch) || IS_AFFECTED(ch, AFF_CHARM) )
		{
		    if ( ( ( !IS_NPC(rch) && rch->desc )
		    ||        IS_AFFECTED(rch, AFF_CHARM) )
		    && is_same_group(ch, rch)
		    && !is_safe( rch, victim, TRUE) )
		    {
			multi_hit( rch, victim, TYPE_UNDEFINED );
			}
		    continue;
		}


		/*
		 * NPC's assist NPC's of same type or 12.5% chance regardless.
		 */
		if ( IS_NPC(rch) && !IS_AFFECTED(rch, AFF_CHARM)
		&&  !xIS_SET(rch->act, ACT_NOASSIST) )
		{
		    if ( char_died(ch) )
			break;
		    if ( rch->pIndexData == ch->pIndexData
		    ||   number_bits( 3 ) == 0 )
		    {
			CHAR_DATA *vch;
			CHAR_DATA *target;
			int number;

			target = NULL;
			number = 0;
                        for ( vch = ch->in_room->first_person; vch; vch = vch->next )
			{
			    if ( can_see( rch, vch )
			    &&   is_same_group( vch, victim )
			    &&   number_range( 0, number ) == 0 )
			    {
				if ( vch->mount && vch->mount == rch )
				  target = NULL;
				else
				{
				  target = vch;
				  number++;
				}
			    }
			}

			if ( target )
			    multi_hit( rch, target, TYPE_UNDEFINED );
		    }
		}
	    }
	}
    }
	sysdata.outBytesFlag = LOGBOUTNORM;

   return;
}



/*
 * Do one group of attacks.
 */
ch_ret multi_hit( CHAR_DATA *ch, CHAR_DATA *victim, int dt )
{
    int     chance;
    int	    dual_bonus;
    ch_ret  retcode;

    if( !IS_NPC(ch) )
    {
        if( ch->fight_start == 0 )
          ch->fight_start = ch->exp;
    }
    if( !IS_NPC(victim) )
    {
	if( victim->fight_start == 0 )
	  victim->fight_start = victim->exp;
    }

    /* add timer to pkillers */
    if ( !IS_NPC(ch) && !IS_NPC(victim) )
    {
        if ( xIS_SET(ch->act, PLR_NICE) ) return rNONE;
        add_timer( ch,     TIMER_RECENTFIGHT, 11, NULL, 0 );
        add_timer( victim, TIMER_RECENTFIGHT, 11, NULL, 0 );
    }

    /*if ( is_attack_supressed( ch ) )
      return rNONE;*/

    if ( IS_NPC(ch) && xIS_SET(ch->act, ACT_NOATTACK) )
      return rNONE;

    if ( (retcode = one_hit( ch, victim, dt )) != rNONE )
      return retcode;

    if ( who_fighting( ch ) != victim || dt == gsn_backstab || dt == gsn_circle)
	return rNONE;

    /* Very high chance of hitting compared to chance of going berserk */
    /* 40% or higher is always hit.. don't learn anything here though. */
    /* -- Altrag */
    chance = IS_NPC(ch) ? 100 : (LEARNED(ch, gsn_berserk)*5/2);
    if ( IS_AFFECTED(ch, AFF_BERSERK) && number_percent() < chance )
      if ( (retcode = one_hit( ch, victim, dt )) != rNONE ||
            who_fighting( ch ) != victim )
        return retcode;

    if ( get_eq_char( ch, WEAR_DUAL_WIELD ) )
    {
      dual_bonus = IS_NPC(ch) ? (ch->level / 10) : (LEARNED(ch, gsn_dual_wield) / 10);
      chance = IS_NPC(ch) ? ch->level : LEARNED(ch, gsn_dual_wield);
      if ( number_percent( ) < chance )
      {
	learn_from_success( ch, gsn_dual_wield );
	retcode = one_hit( ch, victim, dt );
	if ( retcode != rNONE || who_fighting( ch ) != victim )
	    return retcode;
      }
      else
	learn_from_failure( ch, gsn_dual_wield );
    }
    else
      dual_bonus = 0;
/*
    if ( ch->move < 10 )
      dual_bonus = -20;
*/
    /*
     * NPC predetermined number of attacks			-Thoric
     */
    if ( IS_NPC(ch) && ch->numattacks > 0 )
    {
	for ( chance = 0; chance < ch->numattacks; chance++ )
	{
	   retcode = one_hit( ch, victim, dt );
	   if ( retcode != rNONE || who_fighting( ch ) != victim )
	     return retcode;
	}
	return retcode;
    }

    chance = IS_NPC(ch) ? ch->level
	   : (int) ((LEARNED(ch, gsn_tail_attack)+dual_bonus)/1.5);
    if ( number_percent( ) < chance )
    {
	learn_from_success( ch, gsn_tail_attack );
	retcode = one_hit( ch, victim, dt );
	if ( retcode != rNONE || who_fighting( ch ) != victim )
	    return retcode;
    }
    else
	learn_from_failure( ch, gsn_tail_attack );

    chance = IS_NPC(ch) ? ch->level
	   : (int) ((LEARNED(ch, gsn_second_attack)+dual_bonus)/1.5);
    if ( number_percent( ) < chance )
    {
	learn_from_success( ch, gsn_second_attack );
	retcode = one_hit( ch, victim, dt );
	if ( retcode != rNONE || who_fighting( ch ) != victim )
	    return retcode;
    }
    else
	learn_from_failure( ch, gsn_second_attack );

    chance = IS_NPC(ch) ? ch->level
	   : (int) ((LEARNED(ch, gsn_third_attack)+(dual_bonus*1.5))/2);
    if ( number_percent( ) < chance )
    {
	learn_from_success( ch, gsn_third_attack );
	retcode = one_hit( ch, victim, dt );
	if ( retcode != rNONE || who_fighting( ch ) != victim )
	    return retcode;
    }
    else
	learn_from_failure( ch, gsn_third_attack );

    chance = IS_NPC(ch) ? ch->level
	   : (int) ((LEARNED(ch, gsn_fourth_attack)+(dual_bonus*2))/3);
    if ( number_percent( ) < chance )
    {
	learn_from_success( ch, gsn_fourth_attack );
	retcode = one_hit( ch, victim, dt );
	if ( retcode != rNONE || who_fighting( ch ) != victim )
	    return retcode;
    }
    else
	learn_from_failure( ch, gsn_fourth_attack );

    chance = IS_NPC(ch) ? ch->level
	   : (int) ((LEARNED(ch, gsn_fifth_attack)+(dual_bonus*3))/4);
    if ( number_percent( ) < chance )
    {
	learn_from_success( ch, gsn_fifth_attack );
	retcode = one_hit( ch, victim, dt );
	if ( retcode != rNONE || who_fighting( ch ) != victim )
	    return retcode;
    }
    else
	learn_from_failure( ch, gsn_fifth_attack );

    retcode = rNONE;

    chance = IS_NPC(ch) ? (int) (ch->level / 2) : 0;
    if ( number_percent( ) < chance )
	retcode = one_hit( ch, victim, dt );
/*
    if ( retcode == rNONE )
    {
	int move;

	if ( !IS_AFFECTED(ch, AFF_FLYING)
	&&   !IS_AFFECTED(ch, AFF_FLOATING) )
	  move = encumbrance( ch, movement_loss[UMIN(SECT_MAX-1, ch->in_room->sector_type)] );
	else
	  move = encumbrance( ch, 1 );
	if ( ch->move )
	  ch->move = UMAX( 0, ch->move - move );
    }
*/
    return retcode;
}


/*
 * Weapon types, haus
 */
int weapon_prof_bonus_check( CHAR_DATA *ch, OBJ_DATA *wield, int *gsn_ptr )
{
    int bonus;

    bonus = 0;	*gsn_ptr = -1;
    if ( !IS_NPC(ch) && ch->level > 5 && wield )
    {
	switch(wield->value[3])
	{
	   default:		*gsn_ptr = -1;			break;
           case DAM_HIT:
	   case DAM_SUCTION:
	   case DAM_BITE:
	   case DAM_BLAST:	*gsn_ptr = gsn_pugilism;	break;
           case DAM_SLASH:
           case DAM_SLICE:	*gsn_ptr = gsn_long_blades;	break;
           case DAM_PIERCE:
           case DAM_STAB:	*gsn_ptr = gsn_short_blades;	break;
           case DAM_WHIP:	*gsn_ptr = gsn_flexible_arms;	break;
           case DAM_CLAW:	*gsn_ptr = gsn_talonous_arms;	break;
           case DAM_POUND:
           case DAM_CRUSH:	*gsn_ptr = gsn_bludgeons;	break;
	   case DAM_BOLT:
	   case DAM_ARROW:
	   case DAM_DART:
	   case DAM_STONE:
	   case DAM_PEA:	*gsn_ptr = gsn_missile_weapons;	break;

	}
	if ( *gsn_ptr != -1 )
	  bonus = (int) ((LEARNED(ch, *gsn_ptr) -50)/10);

       /* Reduce weapon bonuses for misaligned clannies.
       if ( IS_CLANNED(ch) )
       {
          bonus = bonus * 1000 /
          ( 1 + abs( ch->alignment - ch->pcdata->clan->alignment ) );
       }*/

	if ( IS_DEVOTED(ch) )
	   bonus -= ch->pcdata->favor / -400 ;

    }
    return bonus;
}

/*
 * Calculate the tohit bonus on the object and return RIS values.
 * -- Altrag
 */
int obj_hitroll( OBJ_DATA *obj )
{
	int tohit = 0;
	AFFECT_DATA *paf;

	for ( paf = obj->pIndexData->first_affect; paf; paf = paf->next )
		if ( paf->location == APPLY_HITROLL )
			tohit += paf->modifier;
	for ( paf = obj->first_affect; paf; paf = paf->next )
		if ( paf->location == APPLY_HITROLL )
			tohit += paf->modifier;
	return tohit;
}

/*
 * Offensive shield level modifier
 */
sh_int off_shld_lvl( CHAR_DATA *ch, CHAR_DATA *victim )
{
    sh_int lvl;

    if ( !IS_NPC(ch) )          /* players get much less effect */
    {
        lvl = UMAX( 1, (ch->level - 10) / 2 );
        if ( number_percent() + (victim->level - lvl) < 40 )
	{
	  if ( CAN_PKILL( ch ) && CAN_PKILL( victim ) )
	    return ch->level;
	  else
	    return lvl;
	}
        else
          return 0;
    }
    else
    {
	lvl = ch->level / 2;
	if ( number_percent() + (victim->level - lvl) < 70 )
	  return lvl;
	else
	  return 0;
    }
}

/*
 * Hit one guy once.
 */
ch_ret one_hit( CHAR_DATA *ch, CHAR_DATA *victim, int dt )
{
    OBJ_DATA *wield;
    int plusris;
    int dam;
    int diceroll;
    int attacktype, cnt;
    int	prof_bonus;
    int	prof_gsn = -1;
    ch_ret retcode = rNONE;
    static bool dual_flip = FALSE;
    double attmod = 0.000;
	int baseToHit = 0;

	attmod = get_attmod(ch, victim);

    /*
     * Can't beat a dead char!
     * Guard against weird room-leavings.
     */
    if ( victim->position == POS_DEAD || ch->in_room != victim->in_room )
	return rVICT_DIED;

    ch->melee = TRUE;

    used_weapon = NULL;
    /*
     * Figure out the weapon doing the damage			-Thoric
     * Dual wield support -- switch weapons each attack
     */
    if ( (wield = get_eq_char( ch, WEAR_DUAL_WIELD )) != NULL )
    {
	if ( dual_flip == FALSE )
	{
	    dual_flip = TRUE;
	    wield = get_eq_char( ch, WEAR_WIELD );
	}
	else
	    dual_flip = FALSE;
    }
    else
	wield = get_eq_char( ch, WEAR_WIELD );

    used_weapon = wield;

    if ( wield )
	prof_bonus = 0;
	/* Aparently this was screwing things way up - Karma */
	/*prof_bonus = weapon_prof_bonus_check( ch, wield, &prof_gsn );*/
    else
	prof_bonus = 0;

    if ( ch->fighting		/* make sure fight is already started */
    &&   dt == TYPE_UNDEFINED
    &&   IS_NPC(ch)
    &&  !xIS_EMPTY(ch->attacks) )
    {
	cnt = 0;
	for ( ;; )
	{
	    attacktype = number_range( 0, 6 );
	    if ( xIS_SET( ch->attacks, attacktype ) )
		break;
	    if ( cnt++ > 16 )
 	    {
		attacktype = -1;
		break;
	    }
	}
	if ( attacktype == ATCK_BACKSTAB )
	    attacktype = -1;
	if ( wield && number_percent( ) > 25 )
	    attacktype = -1;
	if ( !wield && number_percent( ) > 50 )
	    attacktype = -1;

	switch ( attacktype )
	{
	    default:
		break;
	    case ATCK_BITE:
		do_bite( ch, "" );
		retcode = global_retcode;
		break;
	    case ATCK_CLAWS:
		do_claw( ch, "" );
		retcode = global_retcode;
		break;
	    case ATCK_TAIL:
		do_tail( ch, "" );
		retcode = global_retcode;
		break;
	    case ATCK_STING:
		do_sting( ch, "" );
		retcode = global_retcode;
		break;
	    case ATCK_PUNCH:
		do_punch( ch, "" );
		retcode = global_retcode;
		break;
	    case ATCK_KICK:
		do_kick( ch, "" );
		retcode = global_retcode;
		break;
	    case ATCK_TRIP:
		attacktype = 0;
		break;
	}
	if ( attacktype >= 0 )
	    return retcode;
    }

    if ( dt == TYPE_UNDEFINED )
    {
	dt = TYPE_HIT;
	if ( wield && wield->item_type == ITEM_WEAPON )
	    dt += wield->value[3];
    }

    /*
     * Calculate percent to hit.
     */

	/* Base chance to hit.
	 * Change as needed
	 */
	baseToHit = 60;

	if (get_curr_dex(ch) > get_curr_dex(victim))
		baseToHit *= (double) (get_curr_dex(ch)/get_curr_dex(victim)) / 10 + 1;
	else
		baseToHit *= (double) get_curr_dex(ch)/get_curr_dex(victim);

	baseToHit = URANGE(1, baseToHit, 100);

	if (attmod > 1 && attmod < 10)
		baseToHit *= (double)attmod/10 + 1;
	else if (attmod >= 10)
		baseToHit = UMIN((attmod * 10), 1000000);
	else if (attmod < 1)
		baseToHit *= attmod;

    /* if you can't see what's coming... */
    if ( wield && !can_see_obj( victim, wield) )
		baseToHit -= 5;
    if ( !can_see( ch, victim ) )
		baseToHit -= 20;

/*
	if (!IS_NPC(ch))
		baseToHit -= ch->pcdata->condition[COND_DRUNK] / 4;
	if (!IS_NPC(victim))
		baseToHit -= ch->pcdata->condition[COND_DRUNK] / 8;
*/
	baseToHit = URANGE(1, baseToHit, 100);

    /*
     * The moment of excitement!
     */
	diceroll = number_range(1,100);
	if ((diceroll > 5 && diceroll > baseToHit) || diceroll >= 96)
    {
	/* Miss. */
	if ( prof_gsn != -1 )
	    learn_from_failure( ch, prof_gsn );
	damage( ch, victim, 0, dt );
	tail_chain( );
	return rNONE;
    }

    /*
     * Hit.
     * Calc damage.
     */

    if ( !wield )       /* bare hand dice formula fixed by Thoric */
    {
        if( is_android(ch) || is_superandroid(ch) )
		dam = number_range( ch->barenumdie, (ch->baresizedie) * (ch->barenumdie+1) );
        else
		dam = number_range( ch->barenumdie, ch->baresizedie * ch->barenumdie );
    }
    else
    {
		dam = number_range( wield->value[1], wield->value[2] );
    }


    /*
     * Calculate Damage Modifiers Based on strength and con
     */
	dam += get_damroll(ch);
	if (dt == TYPE_HIT)
		dam -= get_strDef(victim);
	dam -= get_conDef(victim);

	if (dam < 0)
		dam = 0;

    /*
     * Bonuses.
     */
    if ( prof_bonus )
		dam += prof_bonus / 4;

    /*
     * Max Damage Possable.
     */
    if ( dam * attmod > 999999999 )
		dam = 999999999;
	else
		dam *= attmod ;

    /*
     * Calculate Damage Modifiers from Victim's Fighting Style
     */
    if ( victim->position == POS_BERSERK )
       dam = 1.2 * dam;
    else if ( victim->position == POS_AGGRESSIVE )
       dam = 1.1 * dam;
    else if ( victim->position == POS_DEFENSIVE )
    {
       dam = .85 * dam;
       learn_from_success( victim, gsn_style_defensive );
    }
    else if ( victim->position == POS_EVASIVE )
    {
       dam = .8 * dam;
       learn_from_success( victim, gsn_style_evasive );
    }

    /*
     * Calculate Damage Modifiers from Attacker's Fighting Style
     */
    if ( ch->position == POS_BERSERK )
    {
       dam = 1.2 * dam;
       learn_from_success( ch, gsn_style_berserk );
    }
    else if ( ch->position == POS_AGGRESSIVE )
    {
       dam = 1.1 * dam;
       learn_from_success( ch, gsn_style_aggressive );
    }
    else if ( ch->position == POS_DEFENSIVE )
       dam = .85 * dam;
    else if ( ch->position == POS_EVASIVE )
       dam = .8 * dam;

    if ( !IS_NPC(ch) && ch->pcdata->learned[gsn_enhanced_damage] > 0 )
    {
	dam += (int) (dam * LEARNED(ch, gsn_enhanced_damage) / 120);
	learn_from_success( ch, gsn_enhanced_damage );
    }

    if ( !IS_AWAKE(victim) )
	dam *= 2;
    if ( dt == gsn_backstab )
	dam *= (2 + URANGE( 2, ch->level - (victim->level/4), 30 ) / 8);

    if ( dt == gsn_circle )
 	dam *= (2 + URANGE( 2, ch->level - (victim->level/4), 30 ) / 16);

	/*
	 * For NPC's, they can be set to automaticly take a % off dammage
	 */
	if (IS_NPC(victim))
	{
		if ((GET_AC(victim) > 0))
		{
			dam = (dam * (GET_AC(victim) * 0.01));
		}
		if (GET_AC(victim) <= 0)
		{
			dam = 0;
		}
	}

    if ( dam < 0 )
		dam = 0;

    if ( dam  > 999999999 )
		dam = 999999999;

    plusris = 0;

    if ( wield )
    {
	if ( IS_OBJ_STAT(wield, ITEM_MAGIC) )
 	    dam = ris_damage( victim, dam, RIS_MAGIC );
	else
	    dam = ris_damage( victim, dam, RIS_NONMAGIC );

	/*
	 * Handle PLUS1 - PLUS6 ris bits vs. weapon hitroll	-Thoric
	 */
	plusris = obj_hitroll( wield );
    }
    else
	dam = ris_damage( victim, dam, RIS_NONMAGIC );

    /* check for RIS_PLUSx 					-Thoric */
    if ( dam )
    {
	int x, res, imm, sus, mod;

	if ( plusris )
	   plusris = RIS_PLUS1 << UMIN(plusris, 7);

	/* initialize values to handle a zero plusris */
	imm = res = -1;  sus = 1;

	/* find high ris */
	for ( x = RIS_PLUS1; x <= RIS_PLUS6; x <<= 1 )
	{
	    if ( IS_SET( victim->immune, x ) )
		imm = x;
	    if ( IS_SET( victim->resistant, x ) )
		res = x;
	    if ( IS_SET( victim->susceptible, x ) )
		sus = x;
	}
	mod = 10;
	if ( imm >= plusris )
	    mod -= 10;
	if ( res >= plusris )
	    mod -= 2;
	if ( sus <= plusris )
	    mod += 2;

	/* check if immune */
	if ( mod <= 0 )
	    dam = -1;
	if ( mod != 10 )
	    dam = (dam * mod) / 10;
    }

    if ( prof_gsn != -1 )
    {
	if ( dam > 0 )
	    learn_from_success( ch, prof_gsn );
	else
	    learn_from_failure( ch, prof_gsn );
    }

    /* immune to damage */
    if ( dam == -1 )
    {
	if ( dt >= 0 && dt < top_sn )
	{
	    SKILLTYPE *skill = skill_table[dt];
	    bool found = FALSE;

	    if ( skill->imm_char && skill->imm_char[0] != '\0' )
	    {
		act( AT_HIT, skill->imm_char, ch, NULL, victim, TO_CHAR );
		found = TRUE;
	    }
	    if ( skill->imm_vict && skill->imm_vict[0] != '\0' )
	    {
		act( AT_HITME, skill->imm_vict, ch, NULL, victim, TO_VICT );
		found = TRUE;
	    }
	    if ( skill->imm_room && skill->imm_room[0] != '\0' )
	    {
		act( AT_ACTION, skill->imm_room, ch, NULL, victim, TO_NOTVICT );
		found = TRUE;
	    }
	    if ( found )
	      return rNONE;
	}
	dam = 0;
    }

    if ( (retcode = damage( ch, victim, dam, dt )) != rNONE )
	return retcode;
    if ( char_died(ch) )
	return rCHAR_DIED;
    if ( char_died(victim) )
	return rVICT_DIED;

    retcode = rNONE;
    if ( dam == 0 )
	return retcode;

    /*
     * Weapon spell support				-Thoric
     * Each successful hit casts a spell
     */
    if ( wield
    &&  !IS_SET(victim->immune, RIS_MAGIC)
    &&  !xIS_SET(victim->in_room->room_flags, ROOM_NO_MAGIC) )
    {
	AFFECT_DATA *aff;

	for ( aff = wield->pIndexData->first_affect; aff; aff = aff->next )
	   if ( aff->location == APPLY_WEAPONSPELL
	   &&   IS_VALID_SN(aff->modifier)
	   &&   skill_table[aff->modifier]->spell_fun )
		retcode = (*skill_table[aff->modifier]->spell_fun) ( aff->modifier, (wield->level+3)/3, ch, victim );
	if ( retcode != rNONE || char_died(ch) || char_died(victim) )
		return retcode;
	for ( aff = wield->first_affect; aff; aff = aff->next )
	   if ( aff->location == APPLY_WEAPONSPELL
	   &&   IS_VALID_SN(aff->modifier)
	   &&   skill_table[aff->modifier]->spell_fun )
		retcode = (*skill_table[aff->modifier]->spell_fun) ( aff->modifier, (wield->level+3)/3, ch, victim );
	if ( retcode != rNONE || char_died(ch) || char_died(victim) )
		return retcode;
    }

    /*
     * magic shields that retaliate				-Thoric
     */
    if ( IS_AFFECTED( victim, AFF_FIRESHIELD )
    &&  !IS_AFFECTED( ch, AFF_FIRESHIELD ) )
	retcode = spell_smaug( skill_lookup( "flare" ), off_shld_lvl(victim, ch), victim, ch );
    if ( retcode != rNONE || char_died(ch) || char_died(victim) )
      return retcode;

    if ( IS_AFFECTED( victim, AFF_ICESHIELD )
    &&  !IS_AFFECTED( ch, AFF_ICESHIELD ) )
         retcode = spell_smaug( skill_lookup( "iceshard" ), off_shld_lvl(victim, ch), victim, ch );
    if ( retcode != rNONE || char_died(ch) || char_died(victim) )
      return retcode;

    if ( IS_AFFECTED( victim, AFF_SHOCKSHIELD )
    &&  !IS_AFFECTED( ch, AFF_SHOCKSHIELD ) )
	retcode = spell_smaug( skill_lookup( "torrent" ), off_shld_lvl(victim, ch), victim, ch );
    if ( retcode != rNONE || char_died(ch) || char_died(victim) )
      return retcode;

    if ( IS_AFFECTED( victim, AFF_ACIDMIST )
    &&  !IS_AFFECTED( ch, AFF_ACIDMIST ) )
	retcode = spell_smaug( skill_lookup( "acidshot" ), off_shld_lvl(victim, ch), victim, ch );
    if ( retcode != rNONE || char_died(ch) || char_died(victim) )
	return retcode;

    if ( IS_AFFECTED( victim, AFF_VENOMSHIELD )
    &&  !IS_AFFECTED( ch, AFF_VENOMSHIELD ) )
        retcode = spell_smaug( skill_lookup( "venomshot" ), off_shld_lvl(victim, ch), victim, ch );
    if ( retcode != rNONE || char_died(ch) || char_died(victim) )
        return retcode;

    tail_chain( );
    return retcode;
}

/*
 * Hit one guy with a projectile.
 * Handles use of missile weapons (wield = missile weapon)
 * or thrown items/weapons
 */
ch_ret projectile_hit( CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *wield,
		       OBJ_DATA *projectile, sh_int dist )
{
    int victim_ac;
    int thac0;
    int thac0_00;
    int thac0_32;
    int plusris;
    int dam;
    int diceroll;
    int	prof_bonus;
    int	prof_gsn = -1;
    int proj_bonus;
    int dt;
    ch_ret retcode;

    if ( !projectile )
	return rNONE;

    if ( projectile->item_type == ITEM_PROJECTILE
    ||   projectile->item_type == ITEM_WEAPON )
    {
	dt = TYPE_HIT + projectile->value[3];
	proj_bonus = number_range(projectile->value[1], projectile->value[2]);
    }
    else
    {
	dt = TYPE_UNDEFINED;
	proj_bonus = number_range(1, URANGE(2, get_obj_weight(projectile), 100) );
    }

    /*
     * Can't beat a dead char!
     */
    if ( victim->position == POS_DEAD || char_died(victim) )
    {
	extract_obj(projectile);
	return rVICT_DIED;
    }

    if ( wield )
	prof_bonus = weapon_prof_bonus_check( ch, wield, &prof_gsn );
    else
	prof_bonus = 0;

    if ( dt == TYPE_UNDEFINED )
    {
	dt = TYPE_HIT;
	if ( wield && wield->item_type == ITEM_MISSILE_WEAPON )
	    dt += wield->value[3];
    }

    /*
     * Calculate to-hit-armor-class-0 versus armor.
     */
    if ( IS_NPC(ch) )
    {
	thac0_00 = ch->mobthac0;
	thac0_32 =  0;
    }
    else
    {
	thac0_00 = class_table[ch->class]->thac0_00;
	thac0_32 = class_table[ch->class]->thac0_32;
    }
    thac0     = interpolate( ch->level, thac0_00, thac0_32 ) - GET_HITROLL(ch) + (dist*2);
    victim_ac = UMAX( -19, (int) (GET_AC(victim) / 10) );

    /* if you can't see what's coming... */
    if ( !can_see_obj( victim, projectile) )
	victim_ac += 1;
    if ( !can_see( ch, victim ) )
	victim_ac -= 4;

    /* Weapon proficiency bonus */
    victim_ac += prof_bonus;

    /*
     * The moment of excitement!
     */
    while ( ( diceroll = number_bits( 5 ) ) >= 20 )
	;

    if ( diceroll == 0
    || ( diceroll != 19 && diceroll < thac0 - victim_ac ) )
    {
	/* Miss. */
	if ( prof_gsn != -1 )
	    learn_from_failure( ch, prof_gsn );

	/* Do something with the projectile */
	if ( number_percent() < 50 )
	    extract_obj(projectile);
	else
	{
	    if ( projectile->in_obj )
		obj_from_obj(projectile);
	    if ( projectile->carried_by )
		obj_from_char(projectile);
	    obj_to_room(projectile, victim->in_room);
	}
	damage( ch, victim, 0, dt );
	tail_chain( );
	return rNONE;
    }

    /*
     * Hit.
     * Calc damage.
     */

    if ( !wield )       /* dice formula fixed by Thoric */
	dam = proj_bonus;
    else
	dam = number_range(wield->value[1], wield->value[2]) + (proj_bonus / 10);

    /*
     * Bonuses.
     */
    dam += GET_DAMROLL(ch);

    if ( prof_bonus )
	dam += prof_bonus / 4;

    /*
     * Calculate Damage Modifiers from Victim's Fighting Style
     */
    if ( victim->position == POS_BERSERK )
       dam = 1.2 * dam;
    else if ( victim->position == POS_AGGRESSIVE )
       dam = 1.1 * dam;
    else if ( victim->position == POS_DEFENSIVE )
       dam = .85 * dam;
    else if ( victim->position == POS_EVASIVE )
       dam = .8 * dam;

    if ( !IS_NPC(ch) && ch->pcdata->learned[gsn_enhanced_damage] > 0 )
    {
	dam += (int) (dam * LEARNED(ch, gsn_enhanced_damage) / 120);
	learn_from_success( ch, gsn_enhanced_damage );
    }

    if ( !IS_AWAKE(victim) )
	dam *= 2;

    if ( dam <= 0 )
	dam = 1;

    plusris = 0;

    if ( IS_OBJ_STAT(projectile, ITEM_MAGIC) )
	dam = ris_damage( victim, dam, RIS_MAGIC );
    else
	dam = ris_damage( victim, dam, RIS_NONMAGIC );

    /*
     * Handle PLUS1 - PLUS6 ris bits vs. weapon hitroll	-Thoric
     */
    if ( wield )
	plusris = obj_hitroll( wield );

    /* check for RIS_PLUSx 					-Thoric */
    if ( dam )
    {
	int x, res, imm, sus, mod;

	if ( plusris )
	   plusris = RIS_PLUS1 << UMIN(plusris, 7);

	/* initialize values to handle a zero plusris */
	imm = res = -1;  sus = 1;

	/* find high ris */
	for ( x = RIS_PLUS1; x <= RIS_PLUS6; x <<= 1 )
	{
	   if ( IS_SET( victim->immune, x ) )
		imm = x;
	   if ( IS_SET( victim->resistant, x ) )
		res = x;
	   if ( IS_SET( victim->susceptible, x ) )
		sus = x;
	}
	mod = 10;
	if ( imm >= plusris )
	  mod -= 10;
	if ( res >= plusris )
	  mod -= 2;
	if ( sus <= plusris )
	  mod += 2;

	/* check if immune */
	if ( mod <= 0 )
	  dam = -1;
	if ( mod != 10 )
	  dam = (dam * mod) / 10;
    }

    if ( prof_gsn != -1 )
    {
      if ( dam > 0 )
        learn_from_success( ch, prof_gsn );
      else
        learn_from_failure( ch, prof_gsn );
    }

    /* immune to damage */
    if ( dam == -1 )
    {
	if ( dt >= 0 && dt < top_sn )
	{
	    SKILLTYPE *skill = skill_table[dt];
	    bool found = FALSE;

	    if ( skill->imm_char && skill->imm_char[0] != '\0' )
	    {
		act( AT_HIT, skill->imm_char, ch, NULL, victim, TO_CHAR );
		found = TRUE;
	    }
	    if ( skill->imm_vict && skill->imm_vict[0] != '\0' )
	    {
		act( AT_HITME, skill->imm_vict, ch, NULL, victim, TO_VICT );
		found = TRUE;
	    }
	    if ( skill->imm_room && skill->imm_room[0] != '\0' )
	    {
		act( AT_ACTION, skill->imm_room, ch, NULL, victim, TO_NOTVICT );
		found = TRUE;
	    }
	    if ( found )
	    {
		if ( number_percent() < 50 )
		    extract_obj(projectile);
		else
		{
		    if ( projectile->in_obj )
			obj_from_obj(projectile);
		    if ( projectile->carried_by )
			obj_from_char(projectile);
		    obj_to_room(projectile, victim->in_room);
		}
		return rNONE;
	    }
	}
	dam = 0;
    }
    if ( (retcode = damage( ch, victim, dam, dt )) != rNONE )
    {
	extract_obj(projectile);
	return retcode;
    }
    if ( char_died(ch) )
    {
	extract_obj(projectile);
	return rCHAR_DIED;
    }
    if ( char_died(victim) )
    {
	extract_obj(projectile);
	return rVICT_DIED;
    }

    retcode = rNONE;
    if ( dam == 0 )
    {
	if ( number_percent() < 50 )
	    extract_obj(projectile);
	else
	{
	    if ( projectile->in_obj )
		obj_from_obj(projectile);
	    if ( projectile->carried_by )
		obj_from_char(projectile);
	    obj_to_room(projectile, victim->in_room);
	}
	return retcode;
    }

/* weapon spells	-Thoric */
    if ( wield
    &&  !IS_SET(victim->immune, RIS_MAGIC)
    &&  !xIS_SET(victim->in_room->room_flags, ROOM_NO_MAGIC) )
    {
	AFFECT_DATA *aff;

	for ( aff = wield->pIndexData->first_affect; aff; aff = aff->next )
	   if ( aff->location == APPLY_WEAPONSPELL
	   &&   IS_VALID_SN(aff->modifier)
	   &&   skill_table[aff->modifier]->spell_fun )
		retcode = (*skill_table[aff->modifier]->spell_fun) ( aff->modifier, (wield->level+3)/3, ch, victim );
	if ( retcode != rNONE || char_died(ch) || char_died(victim) )
	{
	    extract_obj(projectile);
	    return retcode;
	}
	for ( aff = wield->first_affect; aff; aff = aff->next )
	   if ( aff->location == APPLY_WEAPONSPELL
	   &&   IS_VALID_SN(aff->modifier)
	   &&   skill_table[aff->modifier]->spell_fun )
		retcode = (*skill_table[aff->modifier]->spell_fun) ( aff->modifier, (wield->level+3)/3, ch, victim );
	if ( retcode != rNONE || char_died(ch) || char_died(victim) )
	{
	    extract_obj(projectile);
	    return retcode;
	}
    }

    extract_obj(projectile);

    tail_chain( );
    return retcode;
}

/*
 * Calculate damage based on resistances, immunities and suceptibilities
 *					-Thoric
 */
int ris_damage( CHAR_DATA *ch, int dam, int ris )
{
   sh_int modifier;

   modifier = 10;
   if ( IS_SET(ch->immune, ris )  && !IS_SET(ch->no_immune, ris) )
     modifier -= 10;
   if ( IS_SET(ch->resistant, ris ) && !IS_SET(ch->no_resistant, ris) )
     modifier -= 2;
   if ( IS_SET(ch->susceptible, ris ) && !IS_SET(ch->no_susceptible, ris) )
   {
     if ( IS_NPC( ch )
     &&   IS_SET( ch->immune, ris ) )
	modifier += 0;
     else
	modifier += 2;
   }
   if ( modifier <= 0 )
     return -1;
   if ( modifier == 10 )
     return dam;
   return (dam * modifier) / 10;
}


/*
 * Inflict damage from a hit.   This is one damn big function.
 */
ch_ret damage( CHAR_DATA *ch, CHAR_DATA *victim, int dam, int dt )
{
    char buf[MAX_STRING_LENGTH];
    char buf1[MAX_STRING_LENGTH];
    char kbuf[MAX_STRING_LENGTH];
    char filename[256];
    bool npcvict;
    bool kaiopres = FALSE;
    bool demonpres = FALSE;
    bool loot;
/*    int  xp_gain = 0;
    int  xp_gain_post = 0;*/
    long double xp_gain = 0;
    long double xp_gain_post = 0;
    ch_ret retcode;
    sh_int dampmod;
    CHAR_DATA *gch /*, *lch */;
    int init_gold, new_gold, gold_diff;
    sh_int anopc = 0;  /* # of (non-pkill) pc in a (ch) */
    sh_int bnopc = 0;  /* # of (non-pkill) pc in b (victim) */
	float xp_mod;
	long double los = 0;
	bool preservation = FALSE;
    bool biopres = FALSE;
    bool warpres = FALSE;
    bool immortal = FALSE;
    ROOM_INDEX_DATA *pRoomIndex;
    double clothingPlMod = 0;

    retcode = rNONE;

    if ( !ch )
    {
	bug( "Damage: null ch!", 0 );
	return rERROR;
    }
    if ( !victim )
    {
	bug( "Damage: null victim!", 0 );
	return rVICT_DIED;
    }

    if( !IS_NPC(ch) && !IS_NPC(victim) )
      if (ch->pcdata->clan && victim->pcdata->clan )
	warpres = TRUE;

    if( !IS_NPC(victim) )
      if( IS_SET(victim->pcdata->flags, PCFLAG_IMMORTALITY) )
        immortal = TRUE;

    if ( victim->position == POS_DEAD )
	return rVICT_DIED;

	if( xIS_SET( victim->act, ACT_PROTOTYPE )
	&& !IS_NPC( ch )
	&& !IS_IMMORTAL( ch ) )
		return rNONE;

    npcvict = IS_NPC(victim);

    /*
     * Check damage types for RIS				-Thoric
     */
    if ( dam && dt != TYPE_UNDEFINED )
    {
	if ( IS_FIRE(dt) )
	  dam = ris_damage(victim, dam, RIS_FIRE);
	else
	if ( IS_COLD(dt) )
	  dam = ris_damage(victim, dam, RIS_COLD);
	else
	if ( IS_ACID(dt) )
	  dam = ris_damage(victim, dam, RIS_ACID);
	else
	if ( IS_ELECTRICITY(dt) )
	  dam = ris_damage(victim, dam, RIS_ELECTRICITY);
	else
	if ( IS_ENERGY(dt) )
	  dam = ris_damage(victim, dam, RIS_ENERGY);
	else
	if ( IS_DRAIN(dt) )
	  dam = ris_damage(victim, dam, RIS_DRAIN);
	else
	if ( dt == gsn_poison || IS_POISON(dt) )
	  dam = ris_damage(victim, dam, RIS_POISON);
	else
	if ( dt == (TYPE_HIT + DAM_POUND) || dt == (TYPE_HIT + DAM_CRUSH)
	||   dt == (TYPE_HIT + DAM_STONE) || dt == (TYPE_HIT + DAM_PEA) )
	  dam = ris_damage(victim, dam, RIS_BLUNT);
	else
	if ( dt == (TYPE_HIT + DAM_STAB) || dt == (TYPE_HIT + DAM_PIERCE)
	||   dt == (TYPE_HIT + DAM_BITE) || dt == (TYPE_HIT + DAM_BOLT)
	||   dt == (TYPE_HIT + DAM_DART) || dt == (TYPE_HIT + DAM_ARROW) )
	  dam = ris_damage(victim, dam, RIS_PIERCE);
	else
	if ( dt == (TYPE_HIT + DAM_SLICE) || dt == (TYPE_HIT + DAM_SLASH)
	||   dt == (TYPE_HIT + DAM_WHIP)  || dt == (TYPE_HIT + DAM_CLAW) )
	  dam = ris_damage(victim, dam, RIS_SLASH);

	if ( dam == -1 )
	{
	    if ( dt >= 0 && dt < top_sn )
	    {
		bool found = FALSE;
		SKILLTYPE *skill = skill_table[dt];

		if ( skill->imm_char && skill->imm_char[0] != '\0' )
		{
		   act( AT_HIT, skill->imm_char, ch, NULL, victim, TO_CHAR );
		   found = TRUE;
		}
		if ( skill->imm_vict && skill->imm_vict[0] != '\0' )
		{
		   act( AT_HITME, skill->imm_vict, ch, NULL, victim, TO_VICT );
		   found = TRUE;
		}
		if ( skill->imm_room && skill->imm_room[0] != '\0' )
		{
		   act( AT_ACTION, skill->imm_room, ch, NULL, victim, TO_NOTVICT );
		   found = TRUE;
		}
		if ( found )
		   return rNONE;
	    }
	    dam = 0;
	}
    }

    /*
     * Precautionary step mainly to prevent people in Hell from finding
     * a way out. --Shaddai
     */
    if ( xIS_SET(victim->in_room->room_flags, ROOM_SAFE) )
	dam = 0;

    if ( dam && npcvict && ch != victim )
    {
	if ( !xIS_SET( victim->act, ACT_SENTINEL ) )
 	{
	    if ( victim->hunting )
	    {
		if ( victim->hunting->who != ch )
		{
		    STRFREE( victim->hunting->name );
		    victim->hunting->name = QUICKLINK( ch->name );
		    victim->hunting->who  = ch;
		}
            }
/*
	    else
            if ( !xIS_SET(victim->act, ACT_PACIFIST)  )
		start_hunting( victim, ch );
*/
	}

	if ( victim->hating )
	{
   	    if ( victim->hating->who != ch )
 	    {
		STRFREE( victim->hating->name );
		victim->hating->name = QUICKLINK( ch->name );
		victim->hating->who  = ch;
	    }
	}
	else
	if ( !xIS_SET(victim->act, ACT_PACIFIST)  ) /* Gorog */
	    start_hating( victim, ch );
    }


    if (victim != ch)
    {
	/*
	 * Certain attacks are forbidden.
	 * Most other attacks are returned.
	 */
	if (is_safe(ch, victim, TRUE))
	    return rNONE;
	check_attacker(ch, victim);

	if (victim->position > POS_STUNNED)
	{
	    if (!victim->fighting && victim->in_room == ch->in_room)
		set_fighting(victim, ch);

	    /*
	       vwas: victim->position = POS_FIGHTING;
	     */
	    if ( IS_NPC(victim) && victim->fighting )
		victim->position = POS_FIGHTING;
	    else if (victim->fighting)
	    {
		switch (victim->style)
		{
		    case (STYLE_EVASIVE):
			victim->position = POS_EVASIVE;
			break;
		    case (STYLE_DEFENSIVE):
			victim->position = POS_DEFENSIVE;
			break;
		    case (STYLE_AGGRESSIVE):
			victim->position = POS_AGGRESSIVE;
			break;
		    case (STYLE_BERSERK):
			victim->position = POS_BERSERK;
			break;
		    default:
			victim->position = POS_FIGHTING;
		}

	    }

	}

	if (victim->position > POS_STUNNED)
	{
	    if (!ch->fighting && victim->in_room == ch->in_room)
		set_fighting(ch, victim);

	    /*
	     * If victim is charmed, ch might attack victim's master.
	     */
	    if (IS_NPC(ch)
		&& npcvict
		&& IS_AFFECTED(victim, AFF_CHARM)
		&& victim->master
		&& victim->master->in_room == ch->in_room
		&& number_bits(3) == 0)
	    {
		stop_fighting(ch, FALSE);
		retcode = multi_hit(ch, victim->master, TYPE_UNDEFINED);
		return retcode;
	    }
	}


	/*
	 * More charm stuff.
	 */
	if (victim->master == ch)
	    stop_follower(victim);

	/*
	 * Pkill stuff.  If a deadly attacks another deadly or is attacked by
	 * one, then ungroup any nondealies.  Disabled untill I can figure out
	 * the right way to do it.
	 */


	/*
	   count the # of non-pkill pc in a ( not including == ch )
	 */
	for (gch = ch->in_room->first_person; gch; gch = gch->next_in_room)
	    if (is_same_group(ch, gch) && !IS_NPC(gch)
		&& !IS_PKILL(gch) && (ch != gch))
		anopc++;

	/*
	   count the # of non-pkill pc in b ( not including == victim )
	 */
	for (gch = victim->in_room->first_person; gch; gch = gch->next_in_room)
	    if (is_same_group(victim, gch) && !IS_NPC(gch)
		&& !IS_PKILL(gch) && (victim != gch))
		bnopc++;


	/*
	   only consider disbanding if both groups have 1(+) non-pk pc,
	   or when one participant is pc, and the other group has 1(+)
	   pk pc's (in the case that participant is only pk pc in group)
	 */
	if ((bnopc > 0 && anopc > 0)
	    || (bnopc > 0 && !IS_NPC(ch))
	    || (anopc > 0 && !IS_NPC(victim)))
	{
	    /*
	       Disband from same group first
	     */
	    if (is_same_group(ch, victim))
	    {
		/*
		   Messages to char and master handled in stop_follower
		 */
		act(AT_ACTION, "$n disbands from $N's group.",
		    (ch->leader == victim) ? victim : ch, NULL,
		    (ch->leader == victim) ? victim->master : ch->master,
		    TO_NOTVICT);
		if (ch->leader == victim)
		    stop_follower(victim);
		else
		    stop_follower(ch);
	    }
	    /*
	       if leader isnt pkill, leave the group and disband ch
	     */
	    if (ch->leader != NULL && !IS_NPC(ch->leader) &&
		!IS_PKILL(ch->leader))
	    {
		act(AT_ACTION, "$n disbands from $N's group.", ch, NULL,
		    ch->master, TO_NOTVICT);
		stop_follower(ch);
	    }
	    else
	    {
		for (gch = ch->in_room->first_person; gch; gch = gch->next_in_room)
		    if (is_same_group(gch, ch) && !IS_NPC(gch) &&
			!IS_PKILL(gch) && gch != ch)
		    {
			act(AT_ACTION, "$n disbands from $N's group.", ch, NULL,
			    gch->master, TO_NOTVICT);
			stop_follower(gch);
		    }
	    }
	    /*
	       if leader isnt pkill, leave the group and disband victim
	     */
	    if (victim->leader != NULL && !IS_NPC(victim->leader) &&
		!IS_PKILL(victim->leader))
	    {
		act(AT_ACTION, "$n disbands from $N's group.", victim, NULL,
		    victim->master, TO_NOTVICT);
		stop_follower(victim);
	    }
	    else
	    {
		for (gch = victim->in_room->first_person; gch; gch = gch->next_in_room)
		    if (is_same_group(gch, victim) && !IS_NPC(gch) &&
			!IS_PKILL(gch) && gch != victim)
		    {
			act(AT_ACTION, "$n disbands from $N's group.", gch, NULL,
			    gch->master, TO_NOTVICT);
			stop_follower(gch);
		    }
	    }
	}

	/*
	 * Inviso attacks ... not.
	 */
	if (IS_AFFECTED(ch, AFF_INVISIBLE))
	{
	    affect_strip(ch, gsn_invis);
	    affect_strip(ch, gsn_mass_invis);
	    xREMOVE_BIT(ch->affected_by, AFF_INVISIBLE);
	    act(AT_MAGIC, "$n fades into existence.", ch, NULL, NULL, TO_ROOM);
	}

	/* Take away Hide */
	if (IS_AFFECTED(ch, AFF_HIDE))
	    xREMOVE_BIT(ch->affected_by, AFF_HIDE);
	/*
	 * Damage modifiers.
	 */
	if (IS_AFFECTED(victim, AFF_SANCTUARY))
	    dam /= 2;

	if (IS_AFFECTED(victim, AFF_PROTECT) && IS_EVIL(ch))
	    dam -= (int) (dam / 4);

	if (dam < 0)
	    dam = 0;

	/*
	 * Check for disarm, trip, parry, dodge and tumble.
	 */
	if (dt >= TYPE_HIT && ch->in_room == victim->in_room)
	{
	    if (IS_NPC(ch)
		&& xIS_SET(ch->defenses, DFND_DISARM)
		&& ch->level > 9
		&& number_percent() < ch->level / 3) /* Was 2 try this --Shaddai*/
		disarm(ch, victim);

	    if (IS_NPC(ch)
		&& xIS_SET(ch->attacks, ATCK_TRIP)
		&& ch->level > 5
		&& number_percent() < ch->level / 2)
		trip(ch, victim);

	    if (check_dodge(ch, victim))
		return rNONE;

	}

	/*
	 * Check control panel settings and modify damage
	 */
	if (IS_NPC(ch))
	{
	    if (npcvict)
		dampmod = sysdata.dam_mob_vs_mob;
	    else
		dampmod = sysdata.dam_mob_vs_plr;
	}
	else
	{
	    if (npcvict)
		dampmod = sysdata.dam_plr_vs_mob;
	    else
		dampmod = sysdata.dam_plr_vs_plr;
	}
	if (dampmod > 0 && dampmod != 100)
	    dam = (dam * dampmod) / 100;

    /*
     * Code to handle equipment getting damaged, and also support  -Thoric
     * bonuses/penalties for having or not having equipment where hit
     * Redone by Goku :)
     */

        /* Further redone to handle dodging/blocking of melee hits.
	   -Karma
        */

	dam = dam_armor_recalc(victim, dam);

	// placeholder
	// The actual dodge check is handled here; the message is done
	// in dam_message. -Karma

	if( !IS_NPC(victim) && ch->melee )
	{
	  int spd = 0;
	  int check = 0;

	  int att = ((get_attmod(victim,ch)/2) * 10);
          int revatt = ((get_attmod(ch,victim)/2) * 5);
          if( att > 100 )
            att = 100;
          if( att < 0 )
            att = 0;
          if( revatt > 100 )
            revatt = 100;
          if( revatt < 0 )
            revatt = 0;

	  // Checks to see if the ch is faster
	  if( get_curr_dex(ch) > get_curr_dex(victim) )
	  {
	    if( ((float)get_curr_dex(ch) / (float)get_curr_dex(victim)) >= 1.25 )
	      spd = -5;
	    if( ((float)get_curr_dex(ch) / (float)get_curr_dex(victim)) >= 1.5 )
              spd = -10;
	    if( ((float)get_curr_dex(ch) / (float)get_curr_dex(victim)) >= 2 )
              spd = -15;
	    if( ((float)get_curr_dex(ch) / (float)get_curr_dex(victim)) >= 3 )
              spd = -20;
	  }
	  else if( get_curr_dex(ch) < get_curr_dex(victim) )
          {
            if( ((float)get_curr_dex(victim) / (float)get_curr_dex(ch)) >= 1.25 )
              spd = 10;
            if( ((float)get_curr_dex(victim) / (float)get_curr_dex(ch)) >= 1.5 )
              spd = 20;
            if( ((float)get_curr_dex(victim) / (float)get_curr_dex(ch)) >= 2 )
              spd = 30;
	    if( ((float)get_curr_dex(victim) / (float)get_curr_dex(ch)) >= 3 )
              spd = 50;
          }
	  else
	    spd = 0;

	  check = ((20 + spd) + (att - revatt));

	  if( check < 1 )
	    check = 1;
	  if( check > 100 )
	    check = 100;

	  if( victim->pcdata->learned[gsn_dodge] > 0 &&
	      !IS_SET( victim->pcdata->combatFlags, CMB_NO_DODGE) )
	  {
	    if( can_use_skill(victim, number_percent(),gsn_dodge ) )
	    {
	      if( number_range(1,100) <= check )
	      {
	        dam = 0;
	        victim->dodge = TRUE;
		learn_from_success( victim, gsn_dodge );
	      }
	      else
		learn_from_failure( victim, gsn_dodge );
	    }
	  }
	  if( victim->pcdata->learned[gsn_block] > 0 && !victim->dodge
	      && !IS_SET( victim->pcdata->combatFlags, CMB_NO_BLOCK) )
	  {
	    if( can_use_skill(victim, number_percent(),gsn_block ) )
	    {
	      if( number_range(1,100) <= check )
              {
                dam = 0;
                victim->block = TRUE;
		learn_from_success( victim, gsn_block );
              }
	      else
		learn_from_failure( victim, gsn_block );
	    }
	  }
	  ch->melee = FALSE;
	}

	dam_message(ch, victim, dam, dt);
    }


    /*
     * Hurt the victim.
     * Inform the victim of his new state.
     */
	//placeholder
	if( dam < 0 )
	  dam = 0;
	if (dam > victim->max_hit)
	    victim->hit -= (victim->max_hit + 100);
	else
	    victim->hit -= dam;

	if( !is_android_h(victim) )
	  victim->mana -= URANGE(0,((double) dam/100 * 0.5 * victim->mana),victim->max_mana);

	heart_calc(victim, "");


	if ( !IS_NPC(victim) && !IS_IMMORTAL(victim))
	{
	if ( (victim->pcdata->learned[gsn_ssj] <= 0 && victim->exp >= 8000000)
		|| (victim->pcdata->learned[gsn_ssj] > 0 && victim->pcdata->learned[gsn_ssj2] <= 0 && victim->exp >= 50000000)
		|| (victim->pcdata->learned[gsn_ssj2] > 0 && victim->pcdata->learned[gsn_ssj3] <= 0 && victim->exp >= 500000000)
		|| (victim->pcdata->learned[gsn_ssj3] > 0 && victim->pcdata->learned[gsn_ssj4] <= 0 && victim->exp >= 2000000000)
		 )
	{
	if (IS_NPC(ch) || ( !IS_NPC(ch) && !xIS_SET(ch->act, PLR_SPAR) ))
	{
	if ( !IS_NPC(victim) && is_saiyan(victim) )
	{
		if (dam > 40)
			victim->rage += 4;
		else if (dam > 30)
			victim->rage += 3;
		else if (dam > 20)
			victim->rage += 2;
		else if (dam > 10)
			victim->rage += 1;
		else
			victim->rage += 1;
	}
	if ( !IS_NPC(victim) && is_hb(victim) )
	{
		if (dam > 40)
			victim->rage += 5;
		else if (dam > 30)
			victim->rage += 4;
		else if (dam > 20)
			victim->rage += 3;
		else if (dam > 10)
			victim->rage += 2;
		else
			victim->rage += 1;
	}
	if (victim->exp >= 8000000 && victim->pcdata->learned[gsn_ssj] <= 0)
		rage(victim, ch);
	else if (victim->exp >= 50000000
		&& victim->pcdata->learned[gsn_ssj] > 0
		&& victim->pcdata->learned[gsn_ssj2] <= 0 )
		rage2(victim, ch);
	else if (victim->exp >= 500000000
		&& victim->pcdata->learned[gsn_ssj2] > 0
		&& victim->pcdata->learned[gsn_ssj3] <= 0 )
		rage3(victim, ch);
	else if (victim->exp >= 2000000000
		&& victim->pcdata->learned[gsn_ssj3] > 0
		&& victim->pcdata->learned[gsn_ssj4] <= 0 )
		rage4(victim, ch);
	}
	}
	}
    /*
     * Get experience based on % of damage done			-Thoric
     *
     * nah, fuck that. we're basing it on power level   -Goku
     */
    if ( dam && ch != victim
    	&& !IS_NPC(ch) && ch->fighting && ch->fighting->xp &&
    	!IS_IMMORTAL(ch) && !IS_IMMORTAL(victim) && !is_split(victim))
    {

		/*
		 * muhahaha, i make pl go lower!  me am the greetist! -Goku 09.28.04
		 */

		// prevents a lot of overflow pl gains -Goku 09.28.04
		if (dam > victim->hit + 11)
			dam = victim->hit + 11;
		if (dam < 1)
			dam = 1;

		// lowered all mods by 0.005  it isn't much but it adds up
		// over time -Goku 09.28.04
		if (ch->race != 6)
		{
			if ( is_saiyan(ch) ) /* Saiyan */
				xp_mod = 0.655;
			else if ( is_namek(ch) ) /* Namek */
				xp_mod = 0.66;
			else if (ch->race == 19) /* Halfbreed-HB */
                                xp_mod = 0.685;
			else if ( is_hb(ch) ) /* Halfbreed */
				xp_mod = 0.675;
			else				/* Everyone Else */
				xp_mod = 0.665;
		}
		else
		{
			if (ch->pcdata->absorb_pl_mod == 0) /* Saiyan */
				xp_mod = 0.655;
			else if (ch->pcdata->absorb_pl_mod == 3) /* Namek */
				xp_mod = 0.66;
			else if (ch->pcdata->absorb_pl_mod == 2) /* Halfbreed */
				xp_mod = 0.675;
			else if (ch->pcdata->absorb_pl_mod == 6)
				xp_mod = 0.65;
			else  /* Everyone Else */
				xp_mod = 0.665;
		}

		if ( !IS_NPC(victim) )
			xp_gain = (long double) dam / 100 * pow(victim->pl, xp_mod);
		if ( IS_NPC(victim) )
			xp_gain = (long double) dam / 100 * pow(victim->exp, xp_mod);

		/* Sparing and deadly combat pl gain's */
		if ( !IS_NPC(ch) && !IS_NPC(victim)
			&& !xIS_SET(ch->act, PLR_SPAR) && !xIS_SET(victim->act, PLR_SPAR))
			{
				if (ch->race == 6)
					xp_mod = (float) xp_mod + 0.03;
				else
					xp_mod = (float) xp_mod + 0.01;
				xp_gain = (long double) dam / 100 * pow(victim->pl, xp_mod);
			}

		if ( !IS_NPC(ch) && !IS_NPC(victim)
			&& xIS_SET(ch->act, PLR_SPAR) && xIS_SET(victim->act, PLR_SPAR))
			{
//				xp_mod = (float) xp_mod - 0.01;
				xp_mod = (float) xp_mod + 0.00;
				xp_gain = (long double) dam / 100 * pow(victim->pl, xp_mod);
			}
/* PL Gains cut if player is stronger than opontants */

		if ( !IS_NPC(victim)) {
			if ((ch->pl / victim->pl) < 3)
			    xp_gain = xp_gain;
			else if ((ch->pl / victim->pl) < 4)
			    xp_gain *= 0.7;
			else if ((ch->pl / victim->pl) < 5)
			    xp_gain *= 0.6;
			else if ((ch->pl / victim->pl) < 6)
			    xp_gain *= 0.5;
			else if ((ch->pl / victim->pl) < 7)
			    xp_gain *= 0.4;
			else if ((ch->pl / victim->pl) < 8)
			    xp_gain *= 0.3;
			else if ((ch->pl / victim->pl) < 9)
			    xp_gain *= 0.2;
			else if ((ch->pl / victim->pl) < 10)
	    		xp_gain *= 0.1;
			else
				xp_gain = 0;
		}
		if ( IS_NPC(victim)) {
			if ((ch->pl / victim->exp) < 3)
			    xp_gain = xp_gain;
			else if ((ch->pl / victim->exp) < 4)
			    xp_gain *= 0.75;
			else if ((ch->pl / victim->exp) < 5)
			    xp_gain *= 0.6;
			else if ((ch->pl / victim->exp) < 6)
			    xp_gain *= 0.5;
			else if ((ch->pl / victim->exp) < 7)
			    xp_gain *= 0.4;
			else if ((ch->pl / victim->exp) < 8)
			    xp_gain *= 0.3;
			else if ((ch->pl / victim->exp) < 9)
			    xp_gain *= 0.2;
			else if ((ch->pl / victim->exp) < 10)
	    		xp_gain *= 0.1;
			else
				xp_gain = 0;
		}

/* PL Gains cut if player is weaker than opontant */

		if (ch->exp != ch->pl && ch->exp < ch->pl)
		{
			int pl_exp = 0;

			pl_exp = (ch->pl / ch->exp);
			xp_gain = xp_gain -( (long double) pl_exp * 0.025 * xp_gain );
		}

/* A little help to get newbies started */
		if (ch->pl < 2500)
			xp_gain += 1;
		if (ch->pl < 5000)
			xp_gain += 1;

		if (xp_gain < 0)
			xp_gain = 0;

		if (xIS_SET(ch->in_room->room_flags, ROOM_TIME_CHAMBER) )
//			&& number_range(1, 100) < 35)
		{
                        switch( number_range( 1, 4 ) )
                        {
                          case 1:
                            xp_gain *= 1;
                          break;

                          case 2:
                            xp_gain *= 1.5;
                          break;

                          case 3:
                            xp_gain *= 2;
                          break;

                          case 4:
                            xp_gain *= 3;
                          break;
                        }
		}

		if ( ch->race == 6 && !IS_NPC(ch) && !IS_NPC(victim)
			&& xIS_SET(ch->act, PLR_SPAR) && xIS_SET(victim->act, PLR_SPAR))
			{
			xp_gain = (long double) xp_gain * 0.75;
			}
		if (ch->race == 6 && !xIS_SET(ch->act, PLR_SPAR) && !xIS_SET(victim->act, PLR_SPAR))
		{
			ch->pcdata->absorb_pl += floor( xp_gain ) / 2;
			xp_gain /= 2;
		}

		if ((clothingPlMod = weightedClothingPlMod(ch)) > 0)
			xp_gain += xp_gain * clothingPlMod;

		if (xIS_SET(ch->act, PLR_3XPL_GAIN))
		{
			xREMOVE_BIT(ch->act, PLR_3XPL_GAIN);
			xp_gain *= 3;
		}
		else if (xIS_SET(ch->act, PLR_2XPL_GAIN))
                {
                        xREMOVE_BIT(ch->act, PLR_2XPL_GAIN);
                        xp_gain *= 2;
                }
		if( ch->mod == 0 )
		  ch->mod = 1;
		xp_gain *= ch->mod;

		xp_gain_post = floor( xp_gain * (long double)( race_table[ch->race]->exp_multiplier/100.0) );

		int a = 0;

		if (is_android(ch) || is_superandroid(ch) )
		{
		if (xp_gain_post != 1)
		{
		  //a = number_range(1,2);
		  a = 1;
		  if( !is_leet(ch) )
			sprintf( buf1, "Your tl increases by %s points.", num_punct_ld(xp_gain_post) );
		  else
		    sprintf( buf1, "Omgawd u git %s tee el. hawt!", num_punct_ld(xp_gain_post) );
		  act( AT_HIT, buf1, ch, NULL, victim, TO_CHAR );
		}
		else
		{
			sprintf( buf1, "Your tl increases by %s point.", num_punct_ld(xp_gain_post) );
	    	act( AT_HIT, buf1, ch, NULL, victim, TO_CHAR );
		}
		}
		else
		{
		if (xp_gain_post != 1)
		{
		   if( !is_leet(ch) )
		     sprintf( buf1, "Your pl increases by %s points.", num_punct_ld(xp_gain_post) );

		   else
		     sprintf( buf1, "Omgawd u git %s pee el. hawt!",num_punct_ld(xp_gain_post) );
		    act( AT_HIT, buf1, ch, NULL, victim, TO_CHAR );
		}
		else
		{
			sprintf( buf1, "Your pl increases by %s point.", num_punct_ld(xp_gain_post) );
	    	act( AT_HIT, buf1, ch, NULL, victim, TO_CHAR );
		}
		}

		gain_exp( ch, xp_gain );
   }


    if ( !IS_NPC(victim)
    &&   victim->level >= LEVEL_IMMORTAL
    &&   victim->hit < 1 )
    {
		victim->hit = 1;
		stop_fighting( victim, TRUE );
	}

    /* Make sure newbies dont die */

    if (!IS_NPC(victim) && NOT_AUTHED(victim) && victim->hit < 1)
	victim->hit = 1;

    if ( dam > 0 && dt > TYPE_HIT
    &&  !IS_AFFECTED( victim, AFF_POISON )
    &&   is_wielding_poisoned( ch )
    &&  !IS_SET( victim->immune, RIS_POISON )
    &&  !saves_poison_death( ch->level, victim ) )
    {
	AFFECT_DATA af;

	af.type      = gsn_poison;
	af.duration  = 20;
	af.location  = APPLY_STR;
	af.modifier  = -2;
	af.bitvector = meb(AFF_POISON);
	affect_join( victim, &af );
	victim->mental_state = URANGE( 20, victim->mental_state +
		(IS_PKILL(victim) ? 1 : 2), 100 );    }

    if ( !npcvict
    &&   get_trust(victim) >= LEVEL_IMMORTAL
    &&	 get_trust(ch)	   >= LEVEL_IMMORTAL
    &&   victim->hit < 1 )
    {
		victim->hit = 1;
		stop_fighting( victim, TRUE );
	}

    if ( !IS_NPC(ch) && !IS_NPC(victim) && xIS_SET(ch->act, PLR_SPAR)
		&& victim->hit <= 0 )
    {
	victim->hit = -1;
	if ( victim->fighting
	&&   victim->fighting->who->hunting
	&&   victim->fighting->who->hunting->who == victim )
	   stop_hunting( victim->fighting->who );

	if ( victim->fighting
	&&   victim->fighting->who->hating
	&&   victim->fighting->who->hating->who == victim )
	   stop_hating( victim->fighting->who );

	  stop_fighting( victim, TRUE );

	  if ( ch->pcdata->clan )
	  {
	    if ( victim->exp < 10000 )           /* 10k */
	      ch->pcdata->clan->spar_wins[0]++;
	    else if ( victim->exp < 100000 )     /* 100k */
	      ch->pcdata->clan->spar_wins[1]++;
	    else if ( victim->exp < 1000000 )    /* 1m */
	      ch->pcdata->clan->spar_wins[2]++;
	    else if ( victim->exp < 10000000 )   /* 10m */
	      ch->pcdata->clan->spar_wins[3]++;
	    else if ( victim->exp < 100000000 )  /* 100m */
	      ch->pcdata->clan->spar_wins[4]++;
	    else if ( victim->exp < 1000000000 ) /* 1b */
	      ch->pcdata->clan->spar_wins[5]++;
	    else                                 /* +1b */
	      ch->pcdata->clan->spar_wins[6]++;
	  }
	  if ( victim->pcdata->clan )
	  {
	    if ( ch->exp < 10000 )           /* 10k */
	      victim->pcdata->clan->spar_loss[0]++;
	    else if ( ch->exp < 100000 )     /* 100k */
	      victim->pcdata->clan->spar_loss[1]++;
	    else if ( ch->exp < 1000000 )    /* 1m */
	      victim->pcdata->clan->spar_loss[2]++;
	    else if ( ch->exp < 10000000 )   /* 10m */
	      victim->pcdata->clan->spar_loss[3]++;
	    else if ( ch->exp < 100000000 )  /* 100m */
	      victim->pcdata->clan->spar_loss[4]++;
	    else if ( ch->exp < 1000000000 ) /* 1b */
	      victim->pcdata->clan->spar_loss[5]++;
	    else                                 /* +1b */
	      victim->pcdata->clan->spar_loss[6]++;
	  }
	ch->pcdata->spar_wins += 1;
	victim->pcdata->spar_loss += 1;
	ch->pcdata->sparcount += 1;
	victim->pcdata->sparcount += 1;
    if( ch->pcdata->nextspartime <= 0 )
    {
        struct tm *tms;

        tms = localtime(&current_time);
        tms->tm_mday += 1;
        ch->pcdata->nextspartime = mktime(tms);
    }
    if( victim->pcdata->nextspartime <= 0 )
    {
        struct tm *tms;

        tms = localtime(&current_time);
        tms->tm_mday += 1;
        victim->pcdata->nextspartime = mktime(tms);
    }
	adjust_hiscore( "sparwins", ch, ch->pcdata->spar_wins );
	adjust_hiscore( "sparloss", victim, victim->pcdata->spar_loss );

	act( AT_WHITE, "You stop fighting and spare $N's life.", ch, NULL, victim, TO_CHAR );
	act( AT_WHITE, "$n stops fighting and spares your life.", ch, NULL, victim, TO_VICT );
	act( AT_WHITE, "$n stops fighting and spares $N's life.", ch, NULL, victim, TO_NOTVICT );
	long double cspar = 0;
	long double vspar = 0;
	cspar = ch->exp - ch->spar_start;
	vspar = victim->exp - victim->spar_start;
	ch_printf(ch,"&cTotal pl gained this spar: &C%s\n\r",
			num_punct_ld(cspar) );
	ch_printf(victim,"&cTotal pl gained this spar: &C%s\n\r",
                        num_punct_ld(vspar) );


	if (!IS_NPC(ch))
	{
		switch (number_range(1, 4))
		{
		default:
			break;
		case 1:
			ch->pcdata->tStr -= number_range(0,3);
			ch->pcdata->tStr = URANGE(0, ch->pcdata->tStr, 99);
			break;
		case 2:
			ch->pcdata->tSpd -= number_range(0,3);
			ch->pcdata->tSpd = URANGE(0, ch->pcdata->tSpd, 99);
			break;
		case 3:
			ch->pcdata->tInt -= number_range(0,3);
			ch->pcdata->tInt = URANGE(0, ch->pcdata->tInt, 99);
			break;
		case 4:
			ch->pcdata->tCon -= number_range(0,3);
			ch->pcdata->tCon = URANGE(0, ch->pcdata->tCon, 99);
			break;
	    }
	}

	if (!IS_NPC(victim))
	{
		switch (number_range(1, 4))
		{
		default:
			break;
		case 1:
			victim->pcdata->tStr -= number_range(0,3);
			victim->pcdata->tStr = URANGE(0, victim->pcdata->tStr, 99);
			break;
		case 2:
			victim->pcdata->tSpd -= number_range(0,3);
			victim->pcdata->tSpd = URANGE(0, victim->pcdata->tSpd, 99);
			break;
		case 3:
			victim->pcdata->tInt -= number_range(0,3);
			victim->pcdata->tInt = URANGE(0, victim->pcdata->tInt, 99);
			break;
		case 4:
			victim->pcdata->tCon -= number_range(0,3);
			victim->pcdata->tCon = URANGE(0, victim->pcdata->tCon, 99);
			break;
	    }
	}

    }



    update_pos( victim );

    if (ch->race == 6 && victim->position <= POS_STUNNED && victim->hit < 1
    	 && !xIS_SET(ch->act, PLR_SPAR))
    	bio_absorb(ch, victim);
    else
    switch( victim->position )
    {
    case POS_MORTAL:
		act( AT_DYING, "$n is mortally wounded, and will die soon, if not aided.", victim, NULL, NULL, TO_ROOM );
		act( AT_DANGER, "You are mortally wounded, and will die soon, if not aided.", victim, NULL, NULL, TO_CHAR );
	break;

    case POS_INCAP:
	act( AT_DYING, "$n is incapacitated and will slowly die, if not aided.",
	    victim, NULL, NULL, TO_ROOM );
	act( AT_DANGER, "You are incapacitated and will slowly die, if not aided.",
	victim, NULL, NULL, TO_CHAR );
	break;

    case POS_STUNNED:
        if ( !IS_AFFECTED( victim, AFF_PARALYSIS ) )
        {
	  act( AT_ACTION, "$n is stunned, but will probably recover.",
	    victim, NULL, NULL, TO_ROOM );
	  act( AT_HURT, "You are stunned, but will probably recover.",
	    victim, NULL, NULL, TO_CHAR );
	}
	break;

    case POS_DEAD:
	if ( dt >= 0 && dt < top_sn )
	{
	    SKILLTYPE *skill = skill_table[dt];

	    if ( skill->die_char && skill->die_char[0] != '\0' )
	      act( AT_DEAD, skill->die_char, ch, NULL, victim, TO_CHAR );
	    if ( skill->die_vict && skill->die_vict[0] != '\0' )
	      act( AT_DEAD, skill->die_vict, ch, NULL, victim, TO_VICT );
	    if ( skill->die_room && skill->die_room[0] != '\0' )
	      act( AT_DEAD, skill->die_room, ch, NULL, victim, TO_NOTVICT );
	}
	act( AT_DEAD, "$n is DEAD!!", victim, 0, 0, TO_ROOM );
	act( AT_DEAD, "You have been KILLED!!\n\r", victim, 0, 0, TO_CHAR );
	break;

    default:
	/*
	 * Victim mentalstate affected, not attacker -- oops ;)
	 * Thanks to gfinello@mail.karmanet.it for finding this bug
	 */
	if ( dam > victim->max_hit / 4 )
	{
	   act( AT_HURT, "That really did HURT!", victim, 0, 0, TO_CHAR );
	   if ( number_bits(3) == 0 )
		worsen_mental_state( victim, 1 );
	}
	if ( victim->hit < victim->max_hit / 4 )

	{
	if (is_android(victim))
	   act( AT_DANGER, "You wish that your wounds would stop leaking oil so much!", victim, 0, 0, TO_CHAR );
	else
	   act( AT_DANGER, "You wish that your wounds would stop BLEEDING so much!", victim, 0, 0, TO_CHAR );
	   if ( number_bits(2) == 0 )
		worsen_mental_state( victim, 1 );
	}
	break;
    }

    /*
     * Sleep spells and extremely wounded folks.
     */
    if  ((ch)->position == POS_SLEEPING &&
    	 !IS_AFFECTED( victim, AFF_PARALYSIS ) )
    {
	if ( victim->fighting
	&&   victim->fighting->who->hunting
	&&   victim->fighting->who->hunting->who == victim )
	   stop_hunting( victim->fighting->who );

	if ( victim->fighting
	&&   victim->fighting->who->hating
	&&   victim->fighting->who->hating->who == victim )
	   stop_hating( victim->fighting->who );

	if (!npcvict && IS_NPC(ch))
	  stop_fighting( victim, TRUE );
	else
	  stop_fighting( victim, FALSE );
    }

    /*
     * Payoff for killing things.
     */
    if ( victim->position == POS_DEAD )
    {
      group_gain( ch, victim );

      /*
       * Stuff for handling loss of kai and demon ranks. -Karma
       */
      if( !IS_NPC(ch) && !IS_NPC(victim) )
      {
	if( is_kaio(ch) && kairanked(victim) )
        {
          if( ch->kairank < victim->kairank )
          {
            int lower = 0;
            int higher = 0;
            sprintf(kbuf,"%s has stolen %s's kaioshin title %s",
                  ch->name, victim->name, get_kai(victim) );
            to_channel( kbuf, CHANNEL_MONITOR, "Monitor", LEVEL_IMMORTAL );
            sprintf(kbuf,"%s has stolen %s's kaioshin title of %s",
                  ch->name, victim->name, get_kai(victim) );
            do_info(ch,kbuf);
            lower = ch->kairank;
            higher = victim->kairank;
	    STRFREE( kaioshin[higher-1] );
	    kaioshin[higher-1] = STRALLOC( ch->name );
            ch->kairank = higher;
            victim->kairank = lower;
            if( victim->kairank == 0 )
            {
              sprintf(kbuf,"%s is no longer a kaioshin", victim->name );
              do_info(ch,kbuf);
	      if( xIS_SET(victim->affected_by, AFF_SANCTUARY) )
		xREMOVE_BIT(victim->affected_by, AFF_SANCTUARY);
            }
            else
            {
              sprintf(kbuf,"%s has dropped to kaioshin title %s",
                victim->name, get_kai(victim) );
	      STRFREE( kaioshin[lower-1] );
	      kaioshin[lower-1] = STRALLOC( victim->name );

              do_info(ch,kbuf);
            }
	    save_sysdata(sysdata);
	    save_char_obj(ch);
	    save_char_obj(victim);
          }
	kaiopres = TRUE;
        }

	if( is_demon(ch) && demonranked(victim) )
        {
	  if( ch->demonrank < victim->demonrank )
	  {
            int lower = 0;
            int higher = 0;
	    bool cg = FALSE, vg = FALSE; // greater demon
	    bool cw = FALSE, vw = FALSE; // demon warlord
	    bool vk = FALSE; // demon king
	    int c = 0;
	    int d = 0;
	    int e = 0;
	    if( demonranked(ch) )
	    {
	      if( ch->demonrank == 1 )
	      {
	        for( c = 0; c < 6; c++ )
	        {
		  if( !str_cmp(greaterdemon[c],ch->name) )
		  {
		    cg = TRUE;
		    d = c;
		    break;
		  }
	        }
	      }
	      else if( ch->demonrank == 2 )
              {
                for( c = 0; c < 3; c++ )
                {
                  if( !str_cmp(demonwarlord[c],ch->name) )
                  {
                    cw = TRUE;
		    d = c;
                    break;
                  }
                }
              }
	    }
	    if( demonranked(victim) )
            {
              if( victim->demonrank == 1 )
              {
                for( c = 0; c < 6; c++ )
                {
                  if( !str_cmp(greaterdemon[c],victim->name) )
                  {
                    vg = TRUE;
		    e = c;
                    break;
                  }
                }
              }
              else if( victim->demonrank == 2 )
              {
                for( c = 0; c < 3; c++ )
                {
                  if( !str_cmp(demonwarlord[c],victim->name) )
                  {
                    vw = TRUE;
		    e = c;
                    break;
                  }
                }
              }
              else if( victim->demonrank == 3 )
              {
                if( !str_cmp(demonking,victim->name) )
                {
                  vk = TRUE;
                }
              }
            }
            sprintf(kbuf,"%s has stolen %s's demon title %s",
                  ch->name, victim->name, get_demon(victim) );
            to_channel( kbuf, CHANNEL_MONITOR, "Monitor", LEVEL_IMMORTAL );
            sprintf(kbuf,"%s has stolen %s's demon title of %s",
                  ch->name, victim->name, get_demon(victim) );
            do_info(ch,kbuf);
            lower = ch->demonrank;
            higher = victim->demonrank;
	    if( vg )
	    {
		STRFREE( greaterdemon[e] );
		greaterdemon[e] = STRALLOC( ch->name );
	    }
	    else if( vw )
            {
                STRFREE( demonwarlord[e] );
                demonwarlord[e] = STRALLOC( ch->name );
            }
	    else if( vk )
            {
                STRFREE( demonking );
                demonking = STRALLOC( ch->name );
		if( xIS_SET(victim->affected_by, AFF_SANCTUARY) )
                  xREMOVE_BIT(victim->affected_by, AFF_SANCTUARY);
            }
            ch->demonrank = higher;
            victim->demonrank = lower;
            if( victim->demonrank == 0 )
            {
              sprintf(kbuf,"%s is no longer a demon", victim->name );
              do_info(ch,kbuf);
            }
            else
            {
              sprintf(kbuf,"%s has dropped to demon title %s",
                victim->name, get_demon(victim) );
	      if( cg )
              {
                STRFREE( greaterdemon[d] );
                greaterdemon[d] = STRALLOC( victim->name );
              }
              else if( cw )
              {
                STRFREE( demonwarlord[d] );
                demonwarlord[d] = STRALLOC( victim->name );
              }
              do_info(ch,kbuf);
            }
	    save_sysdata(sysdata);
	    save_char_obj(ch);
            save_char_obj(victim);
	  }
	  demonpres = TRUE;
        }
      }

	if (!IS_NPC(ch) && (is_bio(ch) || is_android(ch) || is_superandroid(ch) ) && ch->pcdata->learned[gsn_self_destruct] > 0)
	{
		if ( ch->exp > ch->pl)
		{
			if ( IS_NPC(victim))
			{
				if ( victim->exp >= ch->exp)
					ch->pcdata->sd_charge++;
			}
			else
			{
				if (victim->pl >= ch->exp)
					ch->pcdata->sd_charge++;
			}
		}
		else
		{
			if ( IS_NPC(victim))
			{
				if ( victim->exp >= ch->pl)
					ch->pcdata->sd_charge++;
			}
			else
			{
				if ( victim->pl >= ch->pl)
					ch->pcdata->sd_charge++;
			}
		}
	}

	if (!IS_NPC(ch))
	{
		switch (number_range(1, 4))
		{
		default:
			break;
		case 1:
			ch->pcdata->tStr -= number_range(0,3);
			ch->pcdata->tStr = URANGE(0, ch->pcdata->tStr, 99);
			break;
		case 2:
			ch->pcdata->tSpd -= number_range(0,3);
			ch->pcdata->tSpd = URANGE(0, ch->pcdata->tSpd, 99);
			break;
		case 3:
			ch->pcdata->tInt -= number_range(0,3);
			ch->pcdata->tInt = URANGE(0, ch->pcdata->tInt, 99);
			break;
		case 4:
			ch->pcdata->tCon -= number_range(0,3);
			ch->pcdata->tCon = URANGE(0, ch->pcdata->tCon, 99);
			break;
	    }
	}

	if (!IS_NPC(victim))
	{
		switch (number_range(1, 4))
		{
		default:
			break;
		case 1:
			victim->pcdata->tStr -= number_range(0,3);
			victim->pcdata->tStr = URANGE(0, victim->pcdata->tStr, 99);
			break;
		case 2:
			victim->pcdata->tSpd -= number_range(0,3);
			victim->pcdata->tSpd = URANGE(0, victim->pcdata->tSpd, 99);
			break;
		case 3:
			victim->pcdata->tInt -= number_range(0,3);
			victim->pcdata->tInt = URANGE(0, victim->pcdata->tInt, 99);
			break;
		case 4:
			victim->pcdata->tCon -= number_range(0,3);
			victim->pcdata->tCon = URANGE(0, victim->pcdata->tCon, 99);
			break;
	    }
	}

	if ( !npcvict )
	{
       /* Bounty stuff begins - Garinan */

            if ( !IS_NPC( ch ) && !IS_NPC( victim ))
            {
            	victim->pcdata->pk_timer = 0;
            if  (!str_cmp( victim->name, ch->pcdata->hunting )
            	&& xIS_SET(victim->act, PLR_BOUNTY)
            	&& victim->pcdata->bounty > 0 )
            {
                ch->pcdata->bowed += victim->pcdata->bounty;
                ch->pcdata->bkills++;
                    adjust_hiscore("bounty", ch, ch->pcdata->bkills);
                sprintf( log_buf, "You have claimed %d zeni from the head of %s!\n\r", victim->pcdata->bounty, victim->name );
                send_to_char( log_buf, ch );
                send_to_char( "You may collect your earnings at any bounty officer.&R\n\r", ch );
				xREMOVE_BIT(victim->act, PLR_BOUNTY);
				victim->pcdata->bounty = 0;
				victim->pcdata->b_timeleft = 1440;
				DISPOSE(ch->pcdata->hunting);
				ch->pcdata->hunting = str_dup( "" );
                sprintf( log_buf, "%s has claimed the bounty from the head of %s!", ch->name, victim->name );
                do_info( ch, log_buf );
            }
            else if  (!str_cmp( ch->name, victim->pcdata->hunting )
            	&& xIS_SET(ch->act, PLR_BOUNTY)
            	&& ch->pcdata->bounty > 0 )
            {
            	pager_printf_color( victim, "You have lost the right to take the bounty on %s's head", ch->name );
				DISPOSE(victim->pcdata->hunting);
				victim->pcdata->hunting = str_dup( "" );
			}
			}

       /* Bounty stuff ends - Garinan */
	    sprintf( log_buf, "%s (%d) killed by %s at %d",
		victim->name,
		victim->level,
		(IS_NPC(ch) ? ch->short_descr : ch->name),
		victim->in_room->vnum );
	    log_string( log_buf );
	    to_channel( log_buf, CHANNEL_MONITOR, "Monitor", LEVEL_IMMORTAL );
	    if( !is_leet(ch) )
		sprintf( buf, "%s has been killed by %s", victim->name, (IS_NPC(ch) ? ch->short_descr : ch->name));
	    else
		sprintf( buf, "%s set %s up teh bomb. OMG %s IZ SUCH A LOOSER",
		(IS_NPC(ch) ? ch->short_descr : ch->name), victim->name,
				victim->name );
	    do_info(ch, buf);

            if (!IS_NPC( ch ) && ch->pcdata->clan
	    &&   ch->pcdata->clan->clan_type != CLAN_ORDER
            &&   ch->pcdata->clan->clan_type != CLAN_GUILD
            &&   victim != ch )
            {
                sprintf( filename, "%s%s.record", CLAN_DIR, ch->pcdata->clan->short_name );
                sprintf( log_buf, "&P(%2d) %-12s &wvs &P(%2d) %s &P%s ... &w%s",
		  ch->level,
                  ch->name,
		  victim->level,
		  !CAN_PKILL( victim ) ? "&W<Peaceful>" :
		    victim->pcdata->clan ? victim->pcdata->clan->badge :
		      "&P(&WRonin&P)",
		  victim->name,
                  ch->in_room->area->name );
		if ( victim->pcdata->clan &&
		     victim->pcdata->clan->name == ch->pcdata->clan->name)
		;
		else
		  append_to_file( filename, log_buf );
            }

/* newer death penality 10.5% pl loss   - Warren */

		if (!IS_IMMORTAL(ch) && !IS_IMMORTAL(victim))
		{
		  if ( can_use_skill(victim, number_percent(), gsn_preservation ) )
		  {
			learn_from_success( victim, gsn_preservation );
			preservation = TRUE;
		  }
		  else
		    learn_from_failure( victim, gsn_preservation );

		  if ( can_use_skill(victim, number_percent(),gsn_regeneration ) )
                  {
                        learn_from_success( victim, gsn_regeneration );
                        biopres = TRUE;
                  }
                  else
                    learn_from_failure( victim, gsn_regeneration );

//		if( IS_AFFECTED(victim, AFF_IMMORTAL) )
//		if( xIS_SET(victim->affected_by, AFF_IMMORTAL) )
		if( IS_SET(victim->pcdata->flags, PCFLAG_IMMORTALITY) )
		  immortal = TRUE;

		if (!IS_NPC(ch) && !IS_NPC(victim))
		{
/*		  if (ch->pcdata->clan && victim->pcdata->clan
		    && allianceStatus(ch->pcdata->clan, victim->pcdata->clan) == ALLIANCE_ATWAR)
		  {
		    if (preservation || biopres )
		      los = (long double)victim->exp * 0.0025;
		    else
		      los = (long double)victim->exp * 0.005;
		  }
		  else
		  {
*/

		    if (preservation || biopres )
		    {
		      if( warpres || immortal )
			los = (long double)victim->exp * 0.01;
		      else if( (kairanked(ch) && demonranked(victim)) ||
			  (demonranked(ch) && kairanked(victim)) ||
			  kaiopres || demonpres )
		        los = (long double)victim->exp * 0.015;
		      else
		        los = (long double)victim->exp * 0.02;
		    }
		    else
		    {
		      if( warpres || immortal )
                        los = (long double)victim->exp * 0.01;
		      else if( (kairanked(ch) && demonranked(victim)) ||
                          (demonranked(ch) && kairanked(victim)) ||
			  kaiopres || demonpres )
                        los = (long double)victim->exp * 0.025;
		      else
		        los = (long double)victim->exp * 0.03;
		    }
//		  }
		}
		else
		{
		  if( is_split(ch) )
		  {
			if ( immortal )
				los = (long double)victim->exp * 0.01;
			else if (preservation || biopres )
				los = (long double)victim->exp * 0.02;
			else
				los = (long double)victim->exp * 0.03;
		  }
		  else
		  {
		    if ( immortal )
		      los = (long double)victim->exp * 0.01;
		    else if (preservation || biopres )
		      los = (long double)victim->exp * 0.03;
		    else
		      los = (long double)victim->exp * 0.085;
		  }
		}
		gain_exp( victim, 0 - los);
		}

		if (IS_NPC(ch) && !IS_NPC(victim))
		{
			if (is_hunting(ch, victim))
				stop_hunting( ch );

			if (is_hating(ch, victim))
				stop_hating( ch );
		}
	}
	else
	if ( !IS_NPC(ch) )		/* keep track of mob vnum killed */
	    add_kill( ch, victim );

	check_killer( ch, victim );
	if ( !IS_NPC( ch ) && ch->pcdata->clan )
	   update_member( ch );
         if ( !IS_NPC( victim ) && victim->pcdata->clan )
            update_member( victim );

	if ( ch->in_room == victim->in_room )
	    loot = legal_loot( ch, victim );
	else
	    loot = FALSE;

	if ( !IS_NPC(victim) && !IS_IMMORTAL(victim))
	{
	if ( (victim->pcdata->learned[gsn_ssj] <= 0 && victim->exp >= 8000000)
		|| (victim->pcdata->learned[gsn_ssj] > 0 && victim->pcdata->learned[gsn_ssj2] <= 0 && victim->exp >= 50000000)
		|| (victim->pcdata->learned[gsn_ssj2] > 0 && victim->pcdata->learned[gsn_ssj3] <= 0 && victim->exp >= 500000000)
		|| (victim->pcdata->learned[gsn_ssj3] > 0 && victim->pcdata->learned[gsn_ssj4] <= 0 && victim->exp >= 2000000000)
		 )
	{
	if ( !IS_NPC(victim) && is_saiyan(victim))
		victim->rage += 10;
	if ( !IS_NPC(victim) && is_hb(victim))
		victim->rage += 15;
	}
	}

	if (!IS_NPC(victim) && (preservation || biopres || immortal ))
	{
    	stop_fighting( victim, TRUE );
    	make_corpse( victim, ch );
    	victim->mana = victim->max_mana;
    	victim->hit = 100;
    for ( ; ; )
    {
      pRoomIndex = get_room_index( number_range( victim->in_room->area->low_r_vnum, victim->in_room->area->hi_r_vnum ) );
      if ( pRoomIndex )
      {
        if ( !xIS_SET(pRoomIndex->room_flags, ROOM_PRIVATE)
          && !xIS_SET(pRoomIndex->room_flags, ROOM_SOLITARY)
          && !xIS_SET(pRoomIndex->room_flags, ROOM_NO_ASTRAL)
          && !xIS_SET(pRoomIndex->room_flags, ROOM_PROTOTYPE)
	  && has_exits(pRoomIndex) )
        {
          break;
        }
      }
    }
    if ( victim->fighting )
	stop_fighting( victim, TRUE );
    if( preservation )
    {
      act( AT_RED, "With $n's final ounce of energy, $n coughs up an egg and launches it in to the air!", victim, NULL, NULL, TO_ROOM );
      send_to_char("&RWith your final ounce of energy, you cough up an egg and launch it in to the air!", victim );
      char_from_room( victim );
      char_to_room( victim, pRoomIndex );
      victim->position = POS_STANDING;
      act( AT_MAGIC, "An egg falls from the sky creating a small crater as it hits the ground.", victim, NULL, NULL, TO_ROOM );
      act( AT_MAGIC, "The egg begins to crack open as $n pops out!", victim, NULL, NULL, TO_ROOM );
      act( AT_MAGIC, "Your egg lands with a THUD!  You break free of your egg and emerge reborn!", victim, NULL, NULL, TO_CHAR );
    }
    else if( biopres )
    {
      act( AT_DGREEN, "A single cell of $n survives. Within moments, $n begins to regenerate $s body. A shapeless mass of flesh sprouts new legs...new arms...new wings...and a new head. Suddenly, $n is made whole again.", victim, NULL, NULL, TO_ROOM );
      ch_printf(victim,"&gA single one of your cells survives. Within moments, you begin to regenerate your body. Your shapeless mass of flesh sprouts new legs...new arms...new wings...and a new head. Suddenly, you are made whole again.");
      victim->position = POS_STANDING;
    }
    else if( immortal )
    {
      act( AT_WHITE, "As $n lays dead on the floor, $s blood suddenly flows back into $s wounds. $n's wounds heal back up on their own, as $s body glows with a faint, supernatural light. $*s eyes suddenly snap back open as $e is alive again.", victim, NULL, NULL, TO_ROOM );
      ch_printf(victim,"&WAs you lay dead on the ground, your powers of immortality suddenly\n\r"
		"kick in. Your blood flows back into your wounds, and they heal up on\n\r"
		"their own. You begin to feel your body again as you come back to\n\r"
		"life. You open your eyes and look out at the land of living, rather\n\r"
		"than the land of the dead.\n\r");
      victim->position = POS_STANDING;
    }
    do_look( victim, "auto" );

    	if ( IS_SET( sysdata.save_flags, SV_DEATH ) )
			save_char_obj( victim );
	}
	else
	{
		set_cur_char(victim);
		raw_kill( ch, victim );
		victim = NULL;
	}
	if ( !IS_NPC(ch) && loot )
	{
	   /* Autogold by Scryn 8/12 */
	    if ( xIS_SET(ch->act, PLR_AUTOGOLD) )
	    {
		init_gold = ch->gold;
		do_get( ch, "zeni corpse" );
		new_gold = ch->gold;
		gold_diff = (new_gold - init_gold);
		/*if (gold_diff > 0)
                {
                  sprintf(buf1,"%d",gold_diff);
		  do_split( ch, buf1 );
		}*/
	    }
	    if ( xIS_SET(ch->act, PLR_AUTOLOOT)
	    &&   victim != ch )  /* prevent nasty obj problems -- Blodkai */
		do_get( ch, "all corpse" );
	    else
		do_look( ch, "in corpse" );

	    if ( xIS_SET(ch->act, PLR_AUTOSAC) )
		do_sacrifice( ch, "corpse" );
	}

	long double cfight = 0;
	cfight = ch->exp - ch->fight_start;
	if( cfight == ch->exp )
	  cfight = 0;
	if( is_android(ch) || is_superandroid(ch) )
	  ch_printf(ch,"&cTotal tl gained this fight: &C%s\n\r",
                        num_punct_ld(cfight) );
	else
	  ch_printf(ch,"&cTotal pl gained this fight: &C%s\n\r",
                        num_punct_ld(cfight) );

        ch->fight_start = 0;
	//victim->fight_start = 0;

	if ( IS_SET( sysdata.save_flags, SV_KILL ) )
	   save_char_obj( ch );
	return rVICT_DIED;
    }

    if ( victim == ch )
	return rNONE;

    /*
     * Take care of link dead people.
    if ( !npcvict && !victim->desc
    && !IS_SET( victim->pcdata->flags, PCFLAG_NORECALL ) )
    {
	if ( number_range( 0, victim->wait ) == 0)
	{
	    do_recall( victim, "" );
	    return rNONE;
	}
    }
     */

    /*
     * Wimp out?
     */
    if ( npcvict && dam > 0 )
    {
	if ( ( xIS_SET(victim->act, ACT_WIMPY) && number_bits( 1 ) == 0
	&&   victim->hit < victim->max_hit / 2 )
	||   ( IS_AFFECTED(victim, AFF_CHARM) && victim->master
	&&     victim->master->in_room != victim->in_room ) )
	{
	    start_fearing( victim, ch );
	    stop_hunting( victim );
	    do_flee( victim, "" );
	}
    }

    if ( !npcvict
    &&   victim->hit > 0
    &&   victim->hit <= victim->wimpy
    &&   victim->wait == 0 )
	do_flee( victim, "" );
    else
    if ( !npcvict && xIS_SET( victim->act, PLR_FLEE ) )
	do_flee( victim, "" );

    tail_chain( );
    return rNONE;
}



/*
 * Changed is_safe to have the show_messg boolian.  This is so if you don't
 * want to show why you can't kill someone you can't turn it off.  This is
 * useful for things like area attacks.  --Shaddai
 */
bool is_safe( CHAR_DATA *ch, CHAR_DATA *victim, bool show_messg )
{
    if ( char_died(victim) || char_died(ch) )
    	return TRUE;

    /* Thx Josh! */
    if ( who_fighting( ch ) == ch )
	return FALSE;

    if ( !victim ) /*Gonna find this is_safe crash bug -Blod*/
    {
        bug( "Is_safe: %s opponent does not exist!", ch->name );
        return TRUE;
    }
    if ( !victim->in_room )
    {
	bug( "Is_safe: %s has no physical location!", victim->name );
	return TRUE;
    }

    if ( xIS_SET( victim->in_room->room_flags, ROOM_SAFE ) )
    {
        if ( show_messg ) {
	set_char_color( AT_MAGIC, ch );
	send_to_char( "A magical force prevents you from attacking.\n\r", ch );
	}
	return TRUE;
    }

	if (!IS_NPC(victim) && !IS_NPC(ch) )
	{
	if (xIS_SET(victim->act, PLR_BOUNTY) && victim->pcdata->bounty > 0
		&& !str_cmp( victim->name, ch->pcdata->hunting ))
		{
                  if( !xIS_SET( victim->act, PLR_PK1 )
                   && !xIS_SET( victim->act, PLR_PK2 )
                   && !IS_HC( victim ) )
                  {
                    return TRUE;
                  }
                  else
                  {
		    return FALSE;
                  }
		}
	}

    if(IS_PACIFIST(ch)) /* Fireblade */
    {
        if ( show_messg ) {
    	set_char_color(AT_MAGIC, ch);
    	ch_printf(ch, "You are a pacifist and will not fight.\n\r");
	}
	return TRUE;
    }

    if ( IS_PACIFIST(victim) ) /* Gorog */
    {
        char buf[MAX_STRING_LENGTH];
        if ( show_messg ) {
        sprintf(buf, "%s is a pacifist and will not fight.\n\r",
                capitalize(victim->short_descr));
        set_char_color( AT_MAGIC, ch );
        send_to_char( buf, ch);
	}
        return TRUE;
    }

    if( !IS_NPC( ch ) && !IS_NPC( victim )
    &&   ch != victim
    &&   !in_arena(ch) )
    {
        if ( show_messg ) {
        set_char_color( AT_IMMORT, ch );
        send_to_char( "The gods have forbidden player killing in this area.\n\r", ch );
	}
        return TRUE;
    }

	if ( xIS_SET(ch->in_room->room_flags, ROOM_ARENA)
		 && (!xIS_SET(ch->act, PLR_SPAR) || !xIS_SET(victim->act, PLR_SPAR)) )
	{
        if ( show_messg )
        	send_to_char( "You must SPAR someone in an arena.\n\r", ch );
        return TRUE;
    }

/*  bad goku!  clan leaders shouldn't be pk able all the time -Goku 10.06.03
	if (!IS_NPC(victim) && victim->pcdata->clan)
	{
		CLAN_DATA *clan;

		clan = victim->pcdata->clan;

		if (is_leader(victim))
	    	return FALSE;
	}
*/
    if ( IS_NPC(ch) || IS_NPC(victim) )
	return FALSE;
/*
 *    if ( get_age( ch ) < 18 || ch->level < 5 )
 *    {
 *        if ( show_messg ) {
 *	set_char_color( AT_WHITE, ch );
 *	send_to_char( "You are not yet ready, needing age or experience, if not both. \n\r", ch );
 *	}
 *	return TRUE;
 *    }
 *
 *    if ( get_age( victim ) < 18 || victim->level < 5 )
 *    {
 *        if ( show_messg ) {
 *	set_char_color( AT_WHITE, ch );
 *	send_to_char( "They are yet too young to die.\n\r", ch );
 *	}
 *	return TRUE;
 *    }
 *
 *    if ( ch->level - victim->level > 5
 *    ||   victim->level - ch->level > 5 )
 *    {
 *        if ( show_messg ) {
 *	set_char_color( AT_IMMORT, ch );
 *	send_to_char( "The gods do not allow murder when there is such a difference in level.\n\r", ch );
 *	}
 *	return TRUE;
 *    }
 */
	if (!xIS_SET(ch->act, PLR_SPAR) && !xIS_SET(victim->act, PLR_SPAR))
	{
    if ( get_timer(victim, TIMER_PKILLED) > 0 )
    {
        if ( show_messg ) {
	set_char_color( AT_GREEN, ch );
        send_to_char( "That character has died within the last 30 minutes.\n\r", ch);
	}
        return TRUE;
    }

    if ( get_timer(ch, TIMER_PKILLED) > 0 )
    {
        if ( show_messg ) {
	set_char_color( AT_GREEN, ch );
        send_to_char( "You have been killed within the last 30 minutes.\n\r", ch );
	}
        return TRUE;
    }
	}

    return FALSE;
}

/*
 * just verify that a corpse looting is legal
 */
bool legal_loot( CHAR_DATA *ch, CHAR_DATA *victim )
{
    /* anyone can loot mobs */
    if ( IS_NPC(victim) )
      return TRUE;
    /* non-charmed mobs can loot anything */
    if ( IS_NPC(ch) && !ch->master )
      return TRUE;
    /* members of different clans can loot too! -Thoric */
    if ( !IS_NPC(ch) && !IS_NPC(victim)
    &&    IS_SET( ch->pcdata->flags, PCFLAG_DEADLY )
    &&    IS_SET( victim->pcdata->flags, PCFLAG_DEADLY ) )
	return TRUE;
    return FALSE;
}

/*
 * See if an attack justifies a KILLER flag.
 */
void check_killer( CHAR_DATA *ch, CHAR_DATA *victim )
{
    bool split = FALSE;

    if( xIS_SET( (ch)->affected_by, AFF_SPLIT_FORM ) )
      split = TRUE;

    /*
     * NPC's are fair game.
     */
    if ( IS_NPC(victim) )
    {
	if ( !IS_NPC( ch ) )
	{
	  int level_ratio;
	  level_ratio = URANGE( 1, ch->level / victim->level, 50);
	  if ( ch->pcdata->clan )
	    ch->pcdata->clan->mkills++;
	  ch->pcdata->mkills++;
	  ch->in_room->area->mkills++;
	if (ch->race == 6)
		update_absorb(ch, victim);
	adjust_hiscore( "mkills", ch, ch->pcdata->mkills );
	  if ( ch->pcdata->deity )
	  {
	    if ( victim->race == ch->pcdata->deity->npcrace )
	      adjust_favor( ch, 3, level_ratio );
	    else
	      if ( victim->race == ch->pcdata->deity->npcfoe )
		adjust_favor( ch, 17, level_ratio );
	      else
                adjust_favor( ch, 2, level_ratio );
	  }
	}
	return;
    }



    /*
     * If you kill yourself nothing happens.
     */

    if ( ch == victim  )
      return;

    /*
     * Any character in the arena is ok to kill.
     * Added pdeath and pkills here
    if ( in_arena( ch ) )
    {
    	if ( !IS_NPC(ch) && !IS_NPC(victim) )
	{
	  ch->pcdata->pkills++;
	  victim->pcdata->pdeaths++;
	  adjust_hiscore( "pkill", ch, ch->pcdata->pkills );
	}
	else if ( (IS_NPC(ch) && is_split(ch))
	           && !IS_NPC(victim) )
        {
	  if( !ch->master )
	  {
	    log("%s just killed the split form %s without the owner online",
	        victim->name, ch->short_descr );
	  }
          ch->master->pcdata->pkills++;
          victim->pcdata->pdeaths++;
          adjust_hiscore( "pkill", ch, ch->master->pcdata->pkills );
        }

      return;
    }
     */

    /*
     * So are killers and thieves.
     */
    if ( xIS_SET(victim->act, PLR_KILLER)
    ||   xIS_SET(victim->act, PLR_THIEF) )
    {
	if ( !IS_NPC( ch ) )
	{
	  if ( ch->pcdata->clan )
	  {
	    if ( victim->exp < 10000 )           /* 10k */
	      ch->pcdata->clan->pkills[0]++;
	    else if ( victim->exp < 100000 )     /* 100k */
	      ch->pcdata->clan->pkills[1]++;
	    else if ( victim->exp < 1000000 )    /* 1m */
	      ch->pcdata->clan->pkills[2]++;
	    else if ( victim->exp < 10000000 )   /* 10m */
	      ch->pcdata->clan->pkills[3]++;
	    else if ( victim->exp < 100000000 )  /* 100m */
	      ch->pcdata->clan->pkills[4]++;
	    else if ( victim->exp < 1000000000 ) /* 1b */
	      ch->pcdata->clan->pkills[5]++;
	    else                                 /* +1b */
	      ch->pcdata->clan->pkills[6]++;
	  }
	  ch->pcdata->pkills++;
	  ch->in_room->area->pkills++;
	if (ch->race == 6)
		update_absorb(ch, victim);
	  adjust_hiscore( "pkill", ch, ch->pcdata->pkills ); /* cronel hiscore */
	}
	return;
    }

    /* clan checks					-Thoric */
    if ( !IS_NPC(ch) && !IS_NPC(victim)
    &&    IS_SET( ch->pcdata->flags, PCFLAG_DEADLY )
    &&    IS_SET( victim->pcdata->flags, PCFLAG_DEADLY ) )
    {
      /* not of same clan? Go ahead and kill!!! */
      if ( !ch->pcdata->clan
      ||   !victim->pcdata->clan
      ||   ( ch->pcdata->clan->clan_type != CLAN_NOKILL
      &&   victim->pcdata->clan->clan_type != CLAN_NOKILL
      &&   ch->pcdata->clan != victim->pcdata->clan ) )
      {
	if ( ch->pcdata->clan )
	{
	    if ( victim->exp < 10000 )           /* 10k */
	      ch->pcdata->clan->pkills[0]++;
	    else if ( victim->exp < 100000 )     /* 100k */
	      ch->pcdata->clan->pkills[1]++;
	    else if ( victim->exp < 1000000 )    /* 1m */
	      ch->pcdata->clan->pkills[2]++;
	    else if ( victim->exp < 10000000 )   /* 10m */
	      ch->pcdata->clan->pkills[3]++;
	    else if ( victim->exp < 100000000 )  /* 100m */
	      ch->pcdata->clan->pkills[4]++;
	    else if ( victim->exp < 1000000000 ) /* 1b */
	      ch->pcdata->clan->pkills[5]++;
	    else                                 /* +1b */
	      ch->pcdata->clan->pkills[6]++;
	}
	  ch->pcdata->pkills++;
	if (ch->race == 6)
		update_absorb(ch, victim);

	  adjust_hiscore( "pkill", ch, ch->pcdata->pkills ); /* cronel hiscore */
	  ch->hit = ch->max_hit;
	  ch->mana = ch->max_mana;
	  ch->move = ch->max_move;
	update_pos(victim);
	if ( victim != ch )
	{
	  act( AT_MAGIC, "Bolts of blue energy rise from the corpse, seeping into $n.", ch, victim->name, NULL, TO_ROOM );
	  act( AT_MAGIC, "Bolts of blue energy rise from the corpse, seeping into you.", ch, victim->name, NULL, TO_CHAR );
	}
	if ( victim->pcdata->clan )
	{
	    if ( victim->exp < 10000 )           /* 10k */
              victim->pcdata->clan->pdeaths[0]++;
	    else if ( victim->exp < 100000 )     /* 100k */
              victim->pcdata->clan->pdeaths[1]++;
	    else if ( victim->exp < 1000000 )    /* 1m */
              victim->pcdata->clan->pdeaths[2]++;
	    else if ( victim->exp < 10000000 )   /* 10m */
              victim->pcdata->clan->pdeaths[3]++;
	    else if ( victim->exp < 100000000 )  /* 100m */
              victim->pcdata->clan->pdeaths[4]++;
	    else if ( victim->exp < 1000000000 ) /* 1b */
              victim->pcdata->clan->pdeaths[5]++;
	    else                                 /* +1b */
              victim->pcdata->clan->pdeaths[6]++;
	}
	victim->pcdata->pdeaths++;
	adjust_hiscore( "deaths", victim, (victim->pcdata->pdeaths + victim->pcdata->mdeaths) );
	adjust_favor( victim, 11, 1 );
	adjust_favor( ch, 2, 1 );
	if( victim->pcdata->pk_timer > 0 )
	  victim->pcdata->pk_timer = 0;
	if( !IS_HC(victim) )
	  add_timer( victim, TIMER_PKILLED, 690, NULL, 0 );
	else
	  add_timer( victim, TIMER_PKILLED, 345, NULL, 0 );
	WAIT_STATE( victim, 3 * PULSE_VIOLENCE );
	/* xSET_BIT(victim->act, PLR_PK); */
	return;
      }
    }
    else if( IS_NPC(ch) && !IS_NPC(victim) && is_split(ch) )
    {
	if( ch->master )
	  ch->master->pcdata->pkills++;
	victim->pcdata->pdeaths++;
	adjust_hiscore( "deaths", victim, (victim->pcdata->pdeaths + victim->pcdata->mdeaths) );
	if( !IS_HC(victim) )
          add_timer( victim, TIMER_PKILLED, 690, NULL, 0 );
	else
	  add_timer( victim, TIMER_PKILLED, 345, NULL, 0 );
	if( victim->pcdata->pk_timer > 0 )
          victim->pcdata->pk_timer = 0;
        WAIT_STATE( victim, 3 * PULSE_VIOLENCE );
        return;
    }

    /*
     * Charm-o-rama.
     */
    if ( IS_AFFECTED(ch, AFF_CHARM) )
    {
	if ( !ch->master )
	{
	    char buf[MAX_STRING_LENGTH];

	    sprintf( buf, "Check_killer: %s bad AFF_CHARM",
		IS_NPC(ch) ? ch->short_descr : ch->name );
	    bug( buf, 0 );
	    affect_strip( ch, gsn_charm_person );
	    xREMOVE_BIT( ch->affected_by, AFF_CHARM );
	    return;
	}

	/* stop_follower( ch ); */
	if ( ch->master )
	  check_killer( ch->master, victim );
	return;
    }

    /*
     * NPC's are cool of course (as long as not charmed).
     * Hitting yourself is cool too (bleeding).
     * So is being immortal (Alander's idea).
     * And current killers stay as they are.
     */



    if ( IS_NPC(ch) )
    {
	    if ( !IS_NPC(victim) )
	    {
              int level_ratio;
	      if( !IS_HC(victim) )
	        add_timer( victim, TIMER_PKILLED, 690, NULL, 0 );
	      else
		add_timer( victim, TIMER_PKILLED, 345, NULL, 0 );
    	      if ( victim->pcdata->clan )
	      {
		/*if( split )
		{
		  victim->pcdata->clan->pdeaths++;
		  ch->master->pcdata->clan->pkills++;
		}
		else*/
        	  victim->pcdata->clan->mdeaths++;
	      }
	      if( split )
	      {
                victim->pcdata->pdeaths++;
		ch->master->pcdata->pkills++;
	        victim->in_room->area->pdeaths++;
	      }
	      else
	      {
		victim->pcdata->mdeaths++;
		victim->in_room->area->mdeaths++;
	      }
	      adjust_hiscore( "deaths", victim, (victim->pcdata->pdeaths + victim->pcdata->mdeaths) );
	      level_ratio = URANGE( 1, ch->level / victim->level, 50 );
	      if ( victim->pcdata->deity )
	      {
			if ( ch->race == victim->pcdata->deity->npcrace )
	    			adjust_favor( victim, 12, level_ratio );
	  		else
	    		if ( ch->race == victim->pcdata->deity->npcfoe )
				adjust_favor( victim, 15, level_ratio );
	    		else
	        		adjust_favor( victim, 11, level_ratio );
			}
		}
	return;
    }


    if ( !IS_NPC(ch) )
    {
      if ( ch->pcdata->clan )
        ch->pcdata->clan->illegal_pk++;
      ch->pcdata->illegal_pk++;
      ch->in_room->area->illegal_pk++;
    }
    if ( !IS_NPC(victim) )
    {
      if ( victim->pcdata->clan )
      {
 	    if ( victim->exp < 10000 )           /* 10k */
				victim->pcdata->clan->pdeaths[0]++;
	    else if ( victim->exp < 100000 )     /* 100k */
              victim->pcdata->clan->pdeaths[1]++;
	    else if ( victim->exp < 1000000 )    /* 1m */
              victim->pcdata->clan->pdeaths[2]++;
	    else if ( victim->exp < 10000000 )   /* 10m */
              victim->pcdata->clan->pdeaths[3]++;
	    else if ( victim->exp < 100000000 )  /* 100m */
              victim->pcdata->clan->pdeaths[4]++;
	    else if ( victim->exp < 1000000000 ) /* 1b */
              victim->pcdata->clan->pdeaths[5]++;
	    else                                 /* +1b */
              victim->pcdata->clan->pdeaths[6]++;
      }
	adjust_hiscore( "deaths", victim, (victim->pcdata->pdeaths + victim->pcdata->mdeaths) );
      victim->pcdata->pdeaths++;
      victim->in_room->area->pdeaths++;
    }

    if ( xIS_SET(ch->act, PLR_KILLER) )
      return;

    set_char_color( AT_WHITE, ch );
    send_to_char( "A strange feeling grows deep inside you, and a tingle goes up your spine...\n\r", ch );
    set_char_color( AT_IMMORT, ch );
    send_to_char( "A deep voice booms inside your head, 'Thou shall now be known as a deadly murderer!!!'\n\r", ch );
    set_char_color( AT_WHITE, ch );
    send_to_char( "You feel as if your soul has been revealed for all to see.\n\r", ch );
//    xSET_BIT(ch->act, PLR_KILLER);
    if ( xIS_SET( ch->act, PLR_ATTACKER) )
      xREMOVE_BIT(ch->act, PLR_ATTACKER);
    save_char_obj( ch );
    return;
}

/*
 * See if an attack justifies a ATTACKER flag.
 */
void check_attacker( CHAR_DATA *ch, CHAR_DATA *victim )
{

return;
/*
 * Made some changes to this function Apr 6/96 to reduce the prolifiration
 * of attacker flags in the realms. -Narn
 */
    /*
     * NPC's are fair game.
     * So are killers and thieves.
     */
    if ( IS_NPC(victim)
    ||  xIS_SET(victim->act, PLR_KILLER)
    ||  xIS_SET(victim->act, PLR_THIEF) )
	return;

    /* deadly char check */
    if ( !IS_NPC(ch) && !IS_NPC(victim)
         && CAN_PKILL( ch ) && CAN_PKILL( victim ) )
	return;

/* Pkiller versus pkiller will no longer ever make an attacker flag
    { if ( !(ch->pcdata->clan && victim->pcdata->clan
      && ch->pcdata->clan == victim->pcdata->clan ) )  return; }
*/

    /*
     * Charm-o-rama.
     */
    if ( IS_AFFECTED(ch, AFF_CHARM) )
    {
	if ( !ch->master )
	{
	    char buf[MAX_STRING_LENGTH];

	    sprintf( buf, "Check_attacker: %s bad AFF_CHARM",
		IS_NPC(ch) ? ch->short_descr : ch->name );
	    bug( buf, 0 );
	    affect_strip( ch, gsn_charm_person );
	    xREMOVE_BIT( ch->affected_by, AFF_CHARM );
	    return;
	}

        /* Won't have charmed mobs fighting give the master an attacker
           flag.  The killer flag stays in, and I'll put something in
           do_murder. -Narn */
	/* xSET_BIT(ch->master->act, PLR_ATTACKER);*/
	/* stop_follower( ch ); */
	return;
    }

    /*
     * NPC's are cool of course (as long as not charmed).
     * Hitting yourself is cool too (bleeding).
     * So is being immortal (Alander's idea).
     * And current killers stay as they are.
     */
    if ( IS_NPC(ch)
    ||   ch == victim
    ||   ch->level >= LEVEL_IMMORTAL
    ||   xIS_SET(ch->act, PLR_ATTACKER)
    ||   xIS_SET(ch->act, PLR_KILLER) )
	return;

    xSET_BIT(ch->act, PLR_ATTACKER);
    save_char_obj( ch );
    return;
}


/*
 * Set position of a victim.
 */
void update_pos( CHAR_DATA *victim )
{
    if ( !victim )
    {
      bug( "update_pos: null victim", 0 );
      return;
    }

    if ( victim->hit > 0 )
    {
	if ( victim->position <= POS_STUNNED )
	  victim->position = POS_STANDING;
	if ( IS_AFFECTED( victim, AFF_PARALYSIS ) )
	  victim->position = POS_STUNNED;
	return;
    }

    if ( IS_NPC(victim) || victim->hit <= -11 )
    {
	if ( victim->mount )
	{
	  act( AT_ACTION, "$n falls from $N.",
		victim, NULL, victim->mount, TO_ROOM );
	  xREMOVE_BIT( victim->mount->act, ACT_MOUNTED );
	  victim->mount = NULL;
	}
	victim->position = POS_DEAD;
	return;
    }

	 if ( victim->hit <= -6 ) victim->position = POS_MORTAL;
    else if ( victim->hit <= -3 ) victim->position = POS_INCAP;
    else                          victim->position = POS_STUNNED;

    if ( victim->position > POS_STUNNED
    &&   IS_AFFECTED( victim, AFF_PARALYSIS ) )
      victim->position = POS_STUNNED;

    if ( victim->mount )
    {
	act( AT_ACTION, "$n falls unconscious from $N.",
		victim, NULL, victim->mount, TO_ROOM );
	xREMOVE_BIT( victim->mount->act, ACT_MOUNTED );
	victim->mount = NULL;
    }
    return;
}


/*
 * Start fights.
 */
void set_fighting( CHAR_DATA *ch, CHAR_DATA *victim )
{
    FIGHT_DATA *fight;

    if ( ch->fighting )
    {
	char buf[MAX_STRING_LENGTH];

	sprintf( buf, "Set_fighting: %s -> %s (already fighting %s)",
		ch->name, victim->name, ch->fighting->who->name );
	bug( buf, 0 );
	return;
    }

    if ( IS_AFFECTED(ch, AFF_SLEEP) )
	affect_strip( ch, gsn_sleep );

    /* Limit attackers -Thoric */
    if ( victim->num_fighting > max_fight(victim) )
    {
	send_to_char( "There are too many people fighting for you to join in.\n\r", ch );
	return;
    }

    CREATE( fight, FIGHT_DATA, 1 );
    fight->who	 = victim;
    fight->xp	 = (int) xp_compute( ch, victim ) * 0.85;
    fight->align = align_compute( ch, victim );
    if ( !IS_NPC(ch) && IS_NPC(victim) )
      fight->timeskilled = times_killed(ch, victim);
    ch->num_fighting = 1;
    ch->fighting = fight;
    /* ch->position = POS_FIGHTING; */
	if ( IS_NPC(ch) )
		ch->position = POS_FIGHTING;
	else
	switch(ch->style)
	{
		case(STYLE_EVASIVE):
			ch->position = POS_EVASIVE;
			break;
		case(STYLE_DEFENSIVE):
			ch->position = POS_DEFENSIVE;
			break;
		case(STYLE_AGGRESSIVE):
			ch->position = POS_AGGRESSIVE;
			break;
		case(STYLE_BERSERK):
			ch->position = POS_BERSERK;
			break;
		default: ch->position = POS_FIGHTING;
	}
    victim->num_fighting++;
    if ( victim->switched && IS_AFFECTED(victim->switched, AFF_POSSESS) )
    {
	send_to_char( "You are disturbed!\n\r", victim->switched );
	do_return( victim->switched, "" );
    }
    return;
}


CHAR_DATA *who_fighting( CHAR_DATA *ch )
{
    if ( !ch )
    {
	bug( "who_fighting: null ch", 0 );
	return NULL;
    }
    if ( !ch->fighting )
      return NULL;
    return ch->fighting->who;
}

void free_fight( CHAR_DATA *ch )
{
   if ( !ch )
   {
	bug( "Free_fight: null ch!", 0 );
	return;
   }
   if ( ch->fighting )
   {
     if ( !char_died(ch->fighting->who) )
       --ch->fighting->who->num_fighting;
     DISPOSE( ch->fighting );
   }
   ch->fighting = NULL;
   /* Get rid of charged attacks hitting after the fight stops */
   ch->substate = SUB_NONE;
   ch->skillGsn = -1;
   /* Bug with mobs retaining prefocus -Karma */
   ch->focus = 0;
   ch->charge = 0;
   ch->timerDelay = 0;
   ch->timerType = 0;
   ch->timerDo_fun = NULL;
   /* end added code */
   if ( ch->mount )
     ch->position = POS_MOUNTED;
   else
     ch->position = POS_STANDING;
   /* Berserk wears off after combat. -- Altrag */
   if ( IS_AFFECTED(ch, AFF_BERSERK) )
   {
     affect_strip(ch, gsn_berserk);
     set_char_color(AT_WEAROFF, ch);
     send_to_char(skill_table[gsn_berserk]->msg_off, ch);
     send_to_char("\n\r", ch);
   }
   return;
}


/*
 * Stop fights.
 */
void stop_fighting( CHAR_DATA *ch, bool fBoth )
{
    CHAR_DATA *fch;

    free_fight( ch );
    update_pos( ch );

    if ( !fBoth )   /* major short cut here by Thoric */
      return;

    for ( fch = first_char; fch; fch = fch->next )
    {
	if ( who_fighting( fch ) == ch )
	{
	    free_fight( fch );
	    update_pos( fch );
	}
    }
    return;
}

/* Vnums for the various bodyparts */
int part_vnums[] =
{	12,	/* Head */
	14,	/* arms */
	15,	/* legs */
	13,	/* heart */
	44,	/* brains */
	16,	/* guts */
	45,	/* hands */
	46,	/* feet */
	47,	/* fingers */
	48,	/* ear */
	49,	/* eye */
	50,	/* long_tongue */
	51,	/* eyestalks */
	52,	/* tentacles */
	53,	/* fins */
	54,	/* wings */
	55,	/* tail */
	56,	/* scales */
	59,	/* claws */
	87,	/* fangs */
	58,	/* horns */
	57,	/* tusks */
	55,	/* tailattack */
	85,	/* sharpscales */
	84,	/* beak */
	86,	/* haunches */
	83,	/* hooves */
	82,	/* paws */
	81,	/* forelegs */
	80,	/* feathers */
	0,	/* r1 */
	0	/* r2 */
};

/* Messages for flinging off the various bodyparts */
char* part_messages[] =
{
	"$n's severed head plops from its neck.",
	"$n's arm is sliced from $s dead body.",
	"$n's leg is sliced from $s dead body.",
	"$n's heart is torn from $s chest.",
	"$n's brains spill grotesquely from $s head.",
	"$n's guts spill grotesquely from $s torso.",
	"$n's hand is sliced from $s dead body.",
	"$n's foot is sliced from $s dead body.",
	"A finger is sliced from $n's dead body.",
	"$n's ear is sliced from $s dead body.",
	"$n's eye is gouged from its socket.",
	"$n's tongue is torn from $s mouth.",
	"An eyestalk is sliced from $n's dead body.",
	"A tentacle is severed from $n's dead body.",
	"A fin is sliced from $n's dead body.",
	"A wing is severed from $n's dead body.",
	"$n's tail is sliced from $s dead body.",
	"A scale falls from the body of $n.",
	"A claw is torn from $n's dead body.",
	"$n's fangs are torn from $s mouth.",
	"A horn is wrenched from the body of $n.",
	"$n's tusk is torn from $s dead body.",
	"$n's tail is sliced from $s dead body.",
	"A ridged scale falls from the body of $n.",
	"$n's beak is sliced from $s dead body.",
	"$n's haunches are sliced from $s dead body.",
	"A hoof is sliced from $n's dead body.",
	"A paw is sliced from $n's dead body.",
	"$n's foreleg is sliced from $s dead body.",
	"Some feathers fall from $n's dead body.",
	"r1 message.",
	"r2 message."
};

/*
 * Improved Death_cry contributed by Diavolo.
 * Additional improvement by Thoric (and removal of turds... sheesh!)
 * Support for additional bodyparts by Fireblade
 */
void death_cry( CHAR_DATA *ch )
{
    ROOM_INDEX_DATA *was_in_room;
    char *msg;
    EXIT_DATA *pexit;
    int vnum, shift, index, i;

    if ( !ch )
    {
      bug( "DEATH_CRY: null ch!", 0 );
      return;
    }

    vnum = 0;
    msg = NULL;

    switch ( number_range(0, 5) )
    {
    default: msg  = "You hear $n's death cry.";				break;
    case  0:
      msg = "$n screams furiously as $e falls to the ground in a heap!"; break;
    case  1:
      msg  = "$n hits the ground ... DEAD.";			        break;
    case  2:
      msg = "$n catches $s guts in $s hands as they pour through $s fatal"
            " wound!";                                                  break;
    case  3: msg  = "$n splatters blood on your armor.";		break;
    case  4: msg  = "$n gasps $s last breath and blood spurts out of $s "
                    "mouth and ears.";                                  break;
    case  5:
    	shift = number_range(0, 31);
    	index = 1 << shift;

       	for(i = 0;i < 32 && ch->xflags;i++)
    	{
    		if(HAS_BODYPART(ch, index))
    		{
    			msg = part_messages[shift];
    			vnum = part_vnums[shift];
    			break;
    		}
    		else
    		{
    			shift = number_range(0, 31);
    			index = 1 << shift;
    		}
    	}

    	if(!msg)
    		msg = "You hear $n's death cry.";
    	break;
    }

    act( AT_CARNAGE, msg, ch, NULL, NULL, TO_ROOM );

    if ( vnum )
    {
	char buf[MAX_STRING_LENGTH];
	OBJ_DATA *obj;
	char *name;

	if(!get_obj_index(vnum))
	{
		bug("death_cry: invalid vnum", 0);
		return;
	}

	name		= IS_NPC(ch) ? ch->short_descr : ch->name;
	obj		= create_object( get_obj_index( vnum ), 0 );
	obj->timer	= number_range( 4, 7 );
	if ( IS_AFFECTED( ch, AFF_POISON ) )
	  obj->value[3] = 10;

	sprintf( buf, obj->short_descr, name );
	STRFREE( obj->short_descr );
	obj->short_descr = STRALLOC( buf );

	sprintf( buf, obj->description, name );
	STRFREE( obj->description );
	obj->description = STRALLOC( buf );

	obj = obj_to_room( obj, ch->in_room );
    }

    if (number_range(1,4) == 1)
    {
    if ( IS_NPC(ch) )
	msg = "You hear something's death cry.";
    else
	msg = "You hear someone's death cry.";

    was_in_room = ch->in_room;
    for ( pexit = was_in_room->first_exit; pexit; pexit = pexit->next )
    {
	if ( pexit->to_room
	&&   pexit->to_room != was_in_room )
	{
	    ch->in_room = pexit->to_room;
	    act( AT_CARNAGE, msg, ch, NULL, NULL, TO_ROOM );
	}
    }
    ch->in_room = was_in_room;
	}

    return;
}


void raw_kill( CHAR_DATA *ch, CHAR_DATA *victim )
{
	AREA_DATA *karea;
	int lostZeni;

    if ( !victim )
    {
      bug( "raw_kill: null victim!", 0 );
      return;
    }
/* backup in case hp goes below 1 */
    if (NOT_AUTHED(victim))
    {
      bug( "raw_kill: killing unauthed", 0 );
      return;
    }

	/* so we know which economy to drop zeni into -Goku 09.28.04 */
    karea = victim->in_room->area;

    stop_fighting( victim, TRUE );

    /* Take care of morphed characters */
    if(victim->morph)
    {
      do_unmorph_char( victim );
      raw_kill(ch, victim);
      return;
    }

    mprog_death_trigger( ch, victim );
    if ( char_died(victim) )
      return;
 /* death_cry( victim ); */

    rprog_death_trigger( ch, victim );
    if ( char_died(victim) )
      return;

    make_corpse( victim, ch );
    if ( victim->in_room->sector_type == SECT_OCEANFLOOR
    ||   victim->in_room->sector_type == SECT_UNDERWATER
    ||   victim->in_room->sector_type == SECT_WATER_SWIM
    ||   victim->in_room->sector_type == SECT_WATER_NOSWIM )
      act( AT_BLOOD, "$n's blood slowly clouds the surrounding water.", victim, NULL, NULL, TO_ROOM );
    else if ( victim->in_room->sector_type == SECT_AIR )
      act( AT_BLOOD, "$n's blood sprays wildly through the air.", victim, NULL, NULL, TO_ROOM );
    else if ( victim->in_room->sector_type == SECT_SPACE )
      act( AT_BLOOD, "$n's blood forms into floating spheres.", victim, NULL, NULL, TO_ROOM );
//    else
//      make_blood( victim );

    if ( IS_NPC(victim) )
    {
	victim->pIndexData->killed++;
	extract_char( victim, TRUE );
	victim = NULL;
	return;
    }

/*
	if (!IS_NPC(ch) && !IS_NPC(victim) && victim->pcdata->clan
		&& !IS_IMMORTAL(ch) && !IS_IMMORTAL(victim) )
	{
		CLAN_DATA *clan;

		clan = victim->pcdata->clan;

		if (ch->pcdata->clan == clan)
		{
			if (!str_cmp( victim->name, clan->leader) && clan->alignment != 1)
			{
				STRFREE( clan->leader );
				clan->leader = STRALLOC( ch->name );
				do_clantalk(ch, "&RI have killed our leader, now bow down to me.&W");
				sprintf( log_buf, "[INSIDE]%s has become the leader of the %s, by combat.", ch->name, ch->pcdata->clan->name );
				log_string_plus( log_buf, LOG_NORMAL, ch->level );
				victim->pcdata->clan = NULL;
				STRFREE(victim->pcdata->clan_name);
				victim->pcdata->clan_name = STRALLOC( "" );
		    	--clan->members;
		    }
		}
	}
*/

    if (is_splitformed(victim))
    {
        CHAR_DATA *och;
        CHAR_DATA *och_next;
        for ( och = first_char; och; och = och_next )
        {
             och_next = och->next;

             if (!IS_NPC(och))
                 continue;

             if( (xIS_SET(och->affected_by, AFF_SPLIT_FORM)
                 || xIS_SET(och->affected_by, AFF_TRI_FORM)
                 || xIS_SET(och->affected_by, AFF_MULTI_FORM)
		 || xIS_SET(och->affected_by, AFF_BIOJR))
                 && och->master == victim)
             {
                 extract_char( och, TRUE );
             }
        }
        xREMOVE_BIT( (victim)->affected_by, AFF_MULTI_FORM );
        xREMOVE_BIT( (victim)->affected_by, AFF_TRI_FORM );
        xREMOVE_BIT( (victim)->affected_by, AFF_SPLIT_FORM );
	xREMOVE_BIT( (victim)->affected_by, AFF_BIOJR );
    }

    set_char_color( AT_DIEMSG, victim );
    if ( is_android(victim) || is_superandroid(victim) )
    {
	if( !is_leet(victim) )
	  do_help(victim, "_ANDROID_DIEMSG_");
	else
	  do_help(victim, "_1337DIEMSG_");
    }
    else
    {
      /*if ( victim->pcdata->mdeaths + victim->pcdata->pdeaths < 3 )
      do_help( victim, "new_death" );
    else*/

      if( !is_leet(victim) )
        do_help( victim, "_DIEMSG_" );
      else
	do_help(victim, "_1337DIEMSG_");
    }

	if (victim->race == 6)
		evolveCheck(victim, ch, TRUE);

    new_extract_char( victim, FALSE, TRUE );
    if ( !victim )
    {
      bug( "oops! raw_kill: extract_char destroyed pc char", 0 );
      return;
    }

    while ( victim->first_affect )
	affect_remove( victim, victim->first_affect );
    victim->affected_by	= race_table[victim->race]->affected;
/*
    victim->resistant   = 0;
    victim->susceptible = 0;
    victim->immune      = 0;
    victim->carry_weight= 0;
    victim->armor	= 100;
    victim->armor	+= race_table[victim->race]->ac_plus;
    victim->attacks	= race_table[victim->race]->attacks;
    victim->defenses	= race_table[victim->race]->defenses;
    victim->mod_str	= 0;
    victim->mod_dex	= 0;
    victim->mod_int	= 0;
    victim->mod_con	= 0;
    victim->mod_lck = 0;
    victim->damroll	= 0;
    victim->hitroll	= 0;
    victim->mental_state = -10;
    victim->alignment	= URANGE( -1000, victim->alignment, 1000 );
*/
/*  victim->alignment		= race_table[victim->race]->alignment;
-- switched lines just for now to prevent mortals from building up
days of bellyaching about their angelic or satanic humans becoming
neutral when they die given the difficulting of changing align */

    victim->pl = victim->exp;
/*
    victim->saving_poison_death = race_table[victim->race]->saving_poison_death;
    victim->saving_wand 	= race_table[victim->race]->saving_wand;
    victim->saving_para_petri 	= race_table[victim->race]->saving_para_petri;
    victim->saving_breath 	= race_table[victim->race]->saving_breath;
    victim->saving_spell_staff 	= race_table[victim->race]->saving_spell_staff;
*/
    victim->position	= POS_RESTING;
    victim->hit		= 1;
	victim->mana	= 1;
	victim->focus	= 0;
	heart_calc(victim, "");
	victim->powerup = 0;
	victim->pcdata->tStr = 0;
	victim->pcdata->tSpd = 0;
	victim->pcdata->tInt = 0;
	victim->pcdata->tCon = 0;

	if (!IS_NPC(victim) && victim->race == 6)
	{
		victim->pcdata->absorb_sn = -1;
		victim->pcdata->absorb_learn = 0;
	}


    /* Shut down some of those naked spammer killers - Blodkai */

    /*
     * Pardon crimes...						-Thoric
     */
    if ( xIS_SET( victim->act, PLR_KILLER) )
    {
      xREMOVE_BIT( victim->act, PLR_KILLER);
      send_to_char("The gods have pardoned you for your murderous acts.\n\r",victim);
    }
    if ( xIS_SET( victim->act, PLR_THIEF) )
    {
      xREMOVE_BIT( victim->act, PLR_THIEF);
      send_to_char("The gods have pardoned you for your thievery.\n\r",victim);
    }
    victim->pcdata->condition[COND_FULL]   = 12;
    victim->pcdata->condition[COND_THIRST] = 12;

    lostZeni = victim->gold * 0.07;
    victim->gold -= lostZeni;
    boost_economy( karea, lostZeni );
    pager_printf(victim, "&YYou have lost %d zeni!\n\r&D",lostZeni);

	if ( !is_android(victim) && !is_superandroid(victim)
	     && !xIS_SET( victim->affected_by, AFF_DEAD ))
		 xSET_BIT( victim->affected_by, AFF_DEAD );

    if ( IS_SET( sysdata.save_flags, SV_DEATH ) )
	save_char_obj( victim );
    return;
}



void group_gain( CHAR_DATA *ch, CHAR_DATA *victim )
{
    CHAR_DATA *gch;
    CHAR_DATA *lch;
    int xp;
    int members;

    /*
     * Monsters don't get kill xp's or alignment changes.
     * Dying of mortal wounds or poison doesn't give xp to anyone!
     */
    if ( IS_NPC(ch) || victim == ch )
	return;

    members = 0;
    for ( gch = ch->in_room->first_person; gch; gch = gch->next_in_room )
    {
	if ( is_same_group( gch, ch ) )
	    members++;
    }

    if ( members == 0 )
    {
	bug( "Group_gain: members.", members );
	members = 1;
    }

    lch = ch->leader ? ch->leader : ch;

    for ( gch = ch->in_room->first_person; gch; gch = gch->next_in_room )
    {
	OBJ_DATA *obj;
	OBJ_DATA *obj_next;

	if ( !is_same_group( gch, ch ) )
	    continue;

	if ( gch->pl / lch->pl >  10 )
	{
	    send_to_char( "You are too high for this group.\n\r", gch );
	    continue;
	}

	if ( gch->pl / lch->pl < 0.1 )
	{
	    send_to_char( "You are too low for this group.\n\r", gch );
	    continue;
	}

	xp = (int) (xp_compute( gch, victim ));
	if ( !gch->fighting )
	  xp /= 2;
	gch->alignment = align_compute( gch, victim );
	clan_auto_kick(gch);
//	sprintf( buf, "You receive %d experience points.\n\r", xp );
//	send_to_char( buf, gch );
//	gain_exp( gch, xp );

	for ( obj = ch->first_carrying; obj; obj = obj_next )
	{
	    obj_next = obj->next_content;
	    if ( obj->wear_loc == WEAR_NONE )
		continue;

	    if ( (( IS_OBJ_STAT(obj, ITEM_ANTI_EVIL)    && IS_EVIL(ch)    )
	    ||   ( IS_OBJ_STAT(obj, ITEM_ANTI_GOOD)    && IS_GOOD(ch)    )
	    ||   ( IS_OBJ_STAT(obj, ITEM_ANTI_NEUTRAL) && IS_NEUTRAL(ch) ))
	    &&   !IS_HC(ch) )
	    {
		act( AT_MAGIC, "You are zapped by $p.", ch, obj, NULL, TO_CHAR );
		act( AT_MAGIC, "$n is zapped by $p.",   ch, obj, NULL, TO_ROOM );

		obj_from_char( obj );
		obj_to_char( obj, ch );
		/*obj = obj_to_room( obj, ch->in_room );*/
		oprog_zap_trigger(ch, obj);  /* mudprogs */
		if ( char_died(ch) )
		  return;
	    }
	}
    }

    return;
}


int align_compute( CHAR_DATA *gch, CHAR_DATA *victim )
{
    int newalign;

    /* slowed movement in good & evil ranges by a factor of 5, h */
    /* Added divalign to keep neutral chars shifting faster -- Blodkai */
    /* This is obviously gonna take a lot more thought */

	if (!IS_NPC(victim) && !IS_NPC(gch))
	{
		/*if (IS_GOOD(gch) && IS_GOOD(victim) )
			newalign = 0 - victim->alignment;
		else*/
			newalign = gch->alignment;

		if( is_kaio(gch) && newalign < 0 )
                  newalign = 0;
                if( is_demon(gch) && newalign > 0 )
                  newalign = 0;
		if( is_bio(gch) && newalign > 0 )
                  newalign = 0;

	    return newalign;
	}
	else
	{
		newalign = gch->alignment - victim->alignment * 0.02;

		if (newalign > 1000)
			newalign = 1000;
		if (newalign < -1000)
			newalign = -1000;

		if( is_kaio(gch) && newalign < 0 )
		  newalign = 0;
		if( is_demon(gch) && newalign > 0 )
                  newalign = 0;
		if( is_bio(gch) && newalign > 0 )
                  newalign = 0;

	    return newalign;
	}
}


/*
 * Calculate how much XP gch should gain for killing victim
 * Lots of redesigning for new exp system by Thoric
 *
 * More editing by Warren to remove levels
 */
int xp_compute( CHAR_DATA *gch, CHAR_DATA *victim )
{
//	int align;
	int xp = 0;
//	int xp_ratio;
//	int gchlev = gch->level;

	if ( !IS_NPC(victim) ) {
		if ((gch->pl / victim->pl) <= 5)
		    xp = (victim->pl * (number_range( 4, 6 ) * 0.01));
		else if ((gch->pl / victim->pl) <= 8)
		    xp = (victim->pl * (number_range( 3, 5 ) * 0.01));
		else if ((gch->pl / victim->pl) < 10)
	    	xp = (victim->pl * (number_range( 2, 4 ) * 0.01));
		else
			xp = 0;
	}
	if ( IS_NPC(victim) ) {
		if ((gch->pl / victim->exp) <= 5)
		    xp = (victim->exp * (number_range( 4, 6 ) * 0.01));
		else if ((gch->pl / victim->exp) <= 8)
		    xp = (victim->exp * (number_range( 3, 5 ) * 0.01));
		else if ((gch->pl / victim->exp) < 10)
	    	xp = (victim->exp * (number_range( 2, 4 ) * 0.01));
		else
			xp = 0;
	}
/*
    xp	  = (get_exp_worth( victim )
    	  *  URANGE( 0, (victim->level - gchlev) + 10, 13 )) / 10;
    align = gch->alignment - victim->alignment;
*/
    /* bonus for attacking opposite alignment */
/*
    if ( align >  990 || align < -990 )
	xp = (xp*5) >> 2;
    else
*/
    /* penalty for good attacking same alignment */
/*
    if ( gch->alignment > 300 && align < 250 )
	xp = (xp*3) >> 2;

    xp = number_range( (xp*3) >> 2, (xp*5) >> 2 );
*/
    /* get 1/4 exp for players					-Thoric */
/*
    if ( !IS_NPC( victim ) )
      xp /= 4;
    else
*/
    /* reduce exp for killing the same mob repeatedly		-Thoric */
/*
    if ( !IS_NPC( gch ) )
    {
	int times = times_killed( gch, victim );

	if ( times >= 20 )
	   xp = 0;
	else
	if ( times )
	{
	   xp = (xp * (20-times)) / 20;
	   if ( times > 15 )
	     xp /= 3;
	   else
	   if ( times > 10 )
	     xp >>= 1;
	}
    }
*/
    /*
     * semi-intelligent experienced player vs. novice player xp gain
     * "bell curve"ish xp mod by Thoric
     * based on time played vs. level
     */
/*
    if ( !IS_NPC( gch ) && gchlev > 5 )
    {
	xp_ratio = (int) gch->played / gchlev;
	if ( xp_ratio > 20000 )
	    xp = (xp*5) >> 2;
	else
	if ( xp_ratio < 16000 )
	    xp = (xp*3) >> 2;
	else
	if ( xp_ratio < 10000 )
	    xp >>= 1;
	else
	if ( xp_ratio < 5000 )
	    xp >>= 2;
	else
	if ( xp_ratio < 3500 )
	    xp >>= 3;
	else
	if ( xp_ratio < 2000 )
	    xp >>= 4;
    }
*/
    /*
     * Level based experience gain cap.  Cannot get more experience for
     * a kill than the amount for your current experience level   -Thoric
     */

    return xp;
}


/*
 * Revamped by Thoric to be more realistic
 * Added code to produce different messages based on weapon type - FB
 * Added better bug message so you can track down the bad dt's -Shaddai
 */
void new_dam_message( CHAR_DATA *ch, CHAR_DATA *victim, int dam, int dt, OBJ_DATA *obj )
{
    char buf1[256], buf2[256], buf3[256];
    char bugbuf[MAX_STRING_LENGTH];
    const char *vs;
    const char *vp;
    const char *attack;
    char punct;
    struct skill_type *skill = NULL;
    bool gcflag = FALSE;
    bool gvflag = FALSE;
    int d_index, w_index;
    ROOM_INDEX_DATA *was_in_room;

    if ( ch->in_room != victim->in_room )
    {
	was_in_room = ch->in_room;
	char_from_room(ch);
	char_to_room(ch, victim->in_room);
    }
    else
	was_in_room = NULL;

    /* Get the weapon index */

    if ( dt > 0 && dt < top_sn )
    {
    	w_index = 0;
    }
    else
    if ( dt >= TYPE_HIT && dt < TYPE_HIT + sizeof(attack_table)/sizeof(attack_table[0]) )
    {
   	w_index = dt - TYPE_HIT;
    }
    else
    {
	sprintf(bugbuf, "Dam_message: bad dt %d from %s in %d.",
		dt, ch->name, ch->in_room->vnum );
	bug( bugbuf, 0);
   	dt = TYPE_HIT;
   	w_index = 0;
    }

    /* get the damage index  */
    if(dam == 0)
    	d_index = 0;
	else if(dam < 2)
    	d_index = 1;
	else if(dam < 3)
    	d_index = 2;
	else if(dam < 4)
    	d_index = 3;
	else if(dam < 5)
    	d_index = 4;
	else if(dam < 6)
    	d_index = 5;
	else if(dam < 7)
    	d_index = 6;
	else if(dam < 8)
    	d_index = 7;
	else if(dam < 10)
    	d_index = 8;
	else if(dam < 12)
    	d_index = 9;
	else if(dam < 14)
    	d_index = 10;
	else if(dam < 18)
    	d_index = 11;
	else if(dam < 24)
    	d_index = 12;
	else if(dam < 30)
    	d_index = 13;
	else if(dam < 40)
    	d_index = 14;
	else if(dam < 50)
    	d_index = 15;
	else if(dam < 60)
    	d_index = 16;
	else if(dam < 70)
    	d_index = 17;
	else if(dam < 80)
    	d_index = 18;
	else if(dam < 90)
    	d_index = 19;
	else if(dam < 100)
    	d_index = 20;
	else if(dam < 300)
    	d_index = 21;
	else if(dam < 1000)
    	d_index = 22;
	else if(dam < 10000)
    	d_index = 23;
	else if(dam < 100000)
    	d_index = 24;
	else if(dam < 1000000)
    	d_index = 25;
	else if(dam < 10000000)
    	d_index = 26;
	else
    	d_index = 27;

    /* Lookup the damage message */
    if( is_leet(ch) )
    {
	vs = s_1337_messages[d_index];
	vp = p_message_table[w_index][d_index];
    }
    else
    {
      vs = s_message_table[w_index][d_index];
      vp = p_message_table[w_index][d_index];
    }

    punct   = (dam <= 30) ? '.' : '!';

    if ( dam == 0 && (!IS_NPC(ch) &&
       (IS_SET(ch->pcdata->flags, PCFLAG_GAG)))) gcflag = TRUE;

    if ( dam == 0 && (!IS_NPC(victim) &&
       (IS_SET(victim->pcdata->flags, PCFLAG_GAG)))) gvflag = TRUE;

    if ( dt >=0 && dt < top_sn )
	skill = skill_table[dt];

    if( victim->dodge )
    {
	if( IS_NPC(ch) )
	{
	  sprintf( buf1, "&B%s dodges %s's attack.", victim->name, ch->short_descr );
          sprintf( buf2, "&B%s dodges your attack.", victim->name);
          sprintf( buf3, "&BYou dodge %s's attack.", ch->short_descr );
	}
	else
	{
	  sprintf( buf1, "&B%s dodges %s's attack.", victim->name, ch->name );
          sprintf( buf2, "&B%s dodges your attack.", victim->name);
          sprintf( buf3, "&BYou dodge %s's attack.", ch->name );
	}
	victim->dodge = FALSE;
    }
    else if( victim->block )
    {
	if( IS_NPC(ch) )
	{
          sprintf( buf1, "&B%s blocks %s's attack.", victim->name, ch->short_descr );
          sprintf( buf2, "&B%s blocks your attack.", victim->name);
          sprintf( buf3, "&BYou block %s's attack.", ch->short_descr );
	}
	else
	{
	  sprintf( buf1, "&B%s blocks %s's attack.", victim->name, ch->name );
          sprintf( buf2, "&B%s blocks your attack.", victim->name);
          sprintf( buf3, "&BYou block %s's attack.", ch->name );
	}
        victim->block = FALSE;
    }
    else if( victim->ki_dodge )
    {
        if( IS_NPC(ch) )
        {
          sprintf( buf1, "&B%s flickers and vanishes, dodging %s's attack with super-speed.", victim->name, ch->short_descr );
          sprintf( buf2, "&B%s flickers and vanishes, dodging your attack with super-speed.", victim->name);
          sprintf( buf3, "&BYou flicker and vanish, dodging %s's attack with super-speed.", ch->short_descr );
        }
        else
        {
	  sprintf( buf1, "&B%s flickers and vanishes, dodging %s's attack with super-speed.", victim->name, ch->name );
          sprintf( buf2, "&B%s flickers and vanishes, dodging your attack with super-speed.", victim->name);
          sprintf( buf3, "&BYou flicker and vanish, dodging %s's attack with super-speed.", ch->name );
        }
        victim->ki_dodge = FALSE;
    }
    else if( victim->ki_deflect )
    {
        if( IS_NPC(ch) )
        {
          sprintf( buf1, "&B%s deflects %s's attack.", victim->name, ch->short_descr );
          sprintf( buf2, "&B%s deflects your attack.", victim->name);
          sprintf( buf3, "&BYou deflect %s's attack.", ch->short_descr );
        }
        else
        {
          sprintf( buf1, "&B%s deflects %s's attack.", victim->name, ch->name );
          sprintf( buf2, "&B%s deflects your attack.", victim->name);
          sprintf( buf3, "&BYou deflect %s's attack.", ch->name );
        }
        victim->ki_deflect = FALSE;
    }
    else if( victim->ki_cancel )
    {
        if( IS_NPC(ch) )
        {
          sprintf( buf1, "&B%s cancels out %s's attack.", victim->name, ch->short_descr );
          sprintf( buf2, "&B%s cancels out your attack.", victim->name);
          sprintf( buf3, "&BYou cancel out %s's attack.", ch->short_descr );
        }
        else
        {
          sprintf( buf1, "&B%s cancels out %s's attack.", victim->name, ch->name );
          sprintf( buf2, "&B%s cancels out your attack.", victim->name);
          sprintf( buf3, "&BYou cancel out %s's attack.", ch->name );
        }
        victim->ki_cancel = FALSE;
    }
    else if ( dt == TYPE_HIT )
    {
	if ( dam > 0 && !is_leet(ch) )
	{
		sprintf( buf1, "$n %s $N%c  [%s]",  vp, punct, num_punct(dam) );
		sprintf( buf2, "You %s $N%c  [%s]", vs, punct, num_punct(dam) );
		sprintf( buf3, "$n %s you%c  [%s]", vp, punct, num_punct(dam) );
	}
	if ( dam > 0 && is_leet(ch) )
        {
	  sprintf( buf1, "$n %s $N%c  [%s]",  vp, punct, num_punct(dam) );
          sprintf( buf2, "%s  [%s]", vs, num_punct(dam) );
          sprintf( buf3, "$n %s you%c  [%s]", vp, punct, num_punct(dam) );
	}
	if ( dam <= 0 && !is_leet(ch) )
	{
		switch (number_range(1,15))
		{
		default:
		sprintf( buf1, "$n hits $N. $N just laughs at $s weak power.  [%s]",  num_punct(dam) );
		sprintf( buf2, "You hit $N. $N just laughs at your weak power. [%s]", num_punct(dam) );
		sprintf( buf3, "$n hits you. You laugh at $s weak power. [%s]", num_punct(dam) );
		break;
		case 1:
		sprintf( buf1, "$n hits $N. $N just laughs at $s weak power.  [%s]",  num_punct(dam) );
		sprintf( buf2, "You hit $N. $N just laughs at your weak power. [%s]", num_punct(dam) );
		sprintf( buf3, "$n hits you. You laugh at $s weak power. [%s]", num_punct(dam) );
		break;
		case 2:
		sprintf( buf1, "$n %s $N%c  [%s]",  vp, punct, num_punct(dam) );
		sprintf( buf2, "You %s $N%c  [%s]", vs, punct, num_punct(dam) );
		sprintf( buf3, "$n %s you%c  [%s]", vp, punct, num_punct(dam) );
		break;
		case 3:
		sprintf( buf1, "$n's hit is absorbed by the ki energy surrounding $N.  [%s]",  num_punct(dam) );
		sprintf( buf2, "Your hit is absorbed by the ki energy surrounding $N.  [%s]",  num_punct(dam) );
		sprintf( buf3, "$n's hit is absorbed by the ki energy surrounding you. [%s]", num_punct(dam) );
		break;
		case 4:
		sprintf( buf1, "$N dodges $n's hit with incredible speed.  [%s]",  num_punct(dam) );
		sprintf( buf2, "$N dodges your hit with incredible speed.  [%s]",  num_punct(dam) );
		sprintf( buf3, "You dodge $n's hit with incredible speed. [%s]", num_punct(dam) );
		break;
		case 5:
		sprintf( buf1, "$N crosses $S arms and blocks $n's hit.  [%s]",  num_punct(dam) );
		sprintf( buf2, "$N crosses $S arms and blocks your hit.  [%s]",  num_punct(dam) );
		sprintf( buf3, "You cross your arms and block $n's hit. [%s]", num_punct(dam) );
		break;
		case 6:
		sprintf( buf1, "$n aims too low and misses $N.  [%s]",  num_punct(dam) );
		sprintf( buf2, "You aim too low and miss $N.  [%s]",  num_punct(dam) );
		sprintf( buf3, "$n aims too low and misses you. [%s]", num_punct(dam) );
		break;
		case 7:
		sprintf( buf1, "$n aims too high and misses $N.  [%s]",  num_punct(dam) );
		sprintf( buf2, "You aim too high and miss $N.  [%s]",  num_punct(dam) );
		sprintf( buf3, "$n aims too high and misses you. [%s]", num_punct(dam) );
		break;
		case 8:
		sprintf( buf1, "$n lands $s hit but $N just grins at $m.  [%s]",  num_punct(dam) );
		sprintf( buf2, "You land your hit but $N just grins at you.  [%s]",  num_punct(dam) );
		sprintf( buf3, "$n lands $s hit but you just grin at $m. [%s]", num_punct(dam) );
		break;
		case 9:
		sprintf( buf1, "$n loses concentration for a moment and misses $N.  [%s]",  num_punct(dam) );
		sprintf( buf2, "You lose concentration for a moment and miss $N.  [%s]",  num_punct(dam) );
		sprintf( buf3, "$n loses concentration for a moment and misses you. [%s]", num_punct(dam) );
		break;
		case 10:
		sprintf( buf1, "$n overextends $mself and misses $N.  [%s]",  num_punct(dam) );
		sprintf( buf2, "You overextend yourself and miss $N.  [%s]",  num_punct(dam) );
		sprintf( buf3, "$n overextends $mself and misses you. [%s]", num_punct(dam) );
		break;
		case 11:
		sprintf( buf1, "$n hits $N but $E seems to shrug off the damage.  [%s]",  num_punct(dam) );
		sprintf( buf2, "You hit $N but $E seems to shrug off the damage.  [%s]",  num_punct(dam) );
		sprintf( buf3, "$n hits you, but you soak the damage. [%s]", num_punct(dam) );
		break;
		case 12:
		sprintf( buf1, "$N chuckles as $n goes off-balance by missing $M.  [%s]",  num_punct(dam) );
		sprintf( buf2, "You get laughed at by $N for your off-balance miss.  [%s]",  num_punct(dam) );
		sprintf( buf3, "You chuckle as $n goes off-balance by missing you. [%s]", num_punct(dam) );
		break;
		case 13:
		sprintf( buf1, "$N ducks under $n's attack.  [%s]",  num_punct(dam) );
		sprintf( buf2, "$N ducks under your attack.  [%s]",  num_punct(dam) );
		sprintf( buf3, "You duck under $n's attack. [%s]", num_punct(dam) );
		break;
		case 14:
		sprintf( buf1, "$N jumps over $n's attack.  [%s]",  num_punct(dam) );
		sprintf( buf2, "$N jumps over your attack.  [%s]",  num_punct(dam) );
		sprintf( buf3, "You jump over $n's attack. [%s]", num_punct(dam) );
		break;
		case 15:
		sprintf( buf1, "$n trips on something and misses $N.  [%s]",  num_punct(dam) );
		sprintf( buf2, "You trip on something and miss $N.  [%s]",  num_punct(dam) );
		sprintf( buf3, "$n trips on something and misses you. [%s]", num_punct(dam) );
		break;
		}
	}
	else if ( dam <= 0 && is_leet(ch) )
	{
	  sprintf( buf1, "$n %s $N%c  [%s]",  vp, punct,num_punct(dam) );
	  sprintf( buf2, "U mised $N. omgawd u sux. ih8u. H8!!! [%s]",num_punct(dam) );
          sprintf( buf3, "$n %s you%c  [%s]", vp, punct,num_punct(dam) );
	}
    }
    else
    if ( dt > TYPE_HIT && is_wielding_poisoned( ch ) )
    {
	if ( dt < TYPE_HIT + sizeof(attack_table)/sizeof(attack_table[0]) )
	    attack	= attack_table[dt - TYPE_HIT];
	else
	{
         sprintf(bugbuf, "Dam_message: bad dt %d from %s in %d.",
                dt, ch->name, ch->in_room->vnum );
         bug( bugbuf, 0);
	    dt  = TYPE_HIT;
	    attack  = attack_table[0];
        }

	sprintf( buf1, "$n's poisoned %s %s $N%c", attack, vp, punct );
	sprintf( buf2, "Your poisoned %s %s $N%c", attack, vp, punct );
	sprintf( buf3, "$n's poisoned %s %s you%c", attack, vp, punct );
    }
    else
    {
	if ( skill )
	{
	    attack	= skill->noun_damage;
	    if ( dam == 0 )
	    {
		bool found = FALSE;

		if ( skill->miss_char && skill->miss_char[0] != '\0' )
		{
		   act( AT_HIT, skill->miss_char, ch, NULL, victim, TO_CHAR );
		   found = TRUE;
		}
		if ( skill->miss_vict && skill->miss_vict[0] != '\0' )
		{
		   act( AT_HITME, skill->miss_vict, ch, NULL, victim, TO_VICT );
		   found = TRUE;
		}
		if ( skill->miss_room && skill->miss_room[0] != '\0' )
		{
		   if (strcmp( skill->miss_room,"supress" ) )
			act( AT_ACTION, skill->miss_room, ch, NULL, victim, TO_NOTVICT );
		   found = TRUE;
		}
		if ( found )	/* miss message already sent */
		{
		   if ( was_in_room )
		   {
			char_from_room(ch);
			char_to_room(ch, was_in_room);
		   }
		   return;
		}
	    }
	    else
	    {
		if ( skill->hit_char && skill->hit_char[0] != '\0' )
		  act( AT_HIT, skill->hit_char, ch, NULL, victim, TO_CHAR );
		if ( skill->hit_vict && skill->hit_vict[0] != '\0' )
		  act( AT_HITME, skill->hit_vict, ch, NULL, victim, TO_VICT );
		if ( skill->hit_room && skill->hit_room[0] != '\0' )
		  act( AT_ACTION, skill->hit_room, ch, NULL, victim, TO_NOTVICT );
	    }
	}
	else if ( dt >= TYPE_HIT
	&& dt < TYPE_HIT + sizeof(attack_table)/sizeof(attack_table[0]) )
	{
	    if ( obj )
		attack = obj->short_descr;
	    else
		attack = attack_table[dt - TYPE_HIT];
	}
	else
	{
            sprintf(bugbuf, "Dam_message: bad dt %d from %s in %d.",
                dt, ch->name, ch->in_room->vnum );
            bug( bugbuf, 0);
	    dt  = TYPE_HIT;
	    attack  = attack_table[0];
	}

	if( !is_leet(ch) )
	{
	  sprintf( buf1, "$n's %s %s $N%c  [%s]",  attack, vp, punct, num_punct(dam) );
	  sprintf( buf2, "Your %s %s $N%c  [%s]",  attack, vp, punct, num_punct(dam) );
	  sprintf( buf3, "$n's %s %s you%c  [%s]", attack, vp, punct, num_punct(dam) );
	}
	else
	{
	  sprintf( buf1, "$n's %s %s $N%c  [%s]",  attack, vp, punct,num_punct(dam) );
          sprintf( buf2, "%s [%s]", vs, num_punct(dam) );
          sprintf( buf3, "$n's %s %s you%c  [%s]", attack, vp, punct, num_punct(dam) );
	}
    }


    act( AT_ACTION, buf1, ch, NULL, victim, TO_NOTVICT );
    if (!gcflag)  act( AT_HIT, buf2, ch, NULL, victim, TO_CHAR );
    if (!gvflag)  act( AT_HITME, buf3, ch, NULL, victim, TO_VICT );

   if ( was_in_room )
   {
	char_from_room(ch);
	char_to_room(ch, was_in_room);
   }
    return;
}

#ifndef dam_message
void dam_message( CHAR_DATA *ch, CHAR_DATA *victim, int dam, int dt )
{
    new_dam_message(ch, victim, dam, dt);
}
#endif

void do_stopspar( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *victim;

	if ( (victim = who_fighting(ch)) == NULL )
    {
	send_to_char( "You arn't fighting anyone.\n\r", ch );
	return;
    }
	if (IS_NPC(victim))
    {
	send_to_char( "You think you are sparing?!?.\n\r", ch );
	return;
    }

	if (!IS_NPC(ch) && !IS_NPC(victim)
		&& !xIS_SET(ch->act, PLR_SPAR) && !xIS_SET(victim->act, PLR_SPAR) )
    {
	send_to_char( "You think you are sparing?!?.\n\r", ch );
	return;
    }

	stop_fighting( ch, TRUE );
	act( AT_GREEN, "You decide to stop sparring with $N.", ch, NULL, victim, TO_CHAR );
	act( AT_GREEN, "$n decides to stop sparring with you.", ch, NULL, victim, TO_VICT );
	act( AT_GREEN, "$n decides to stop sparring with $N.", ch, NULL, victim, TO_NOTVICT );
	ch->pcdata->sparcount += 1;
	victim->pcdata->sparcount += 1;

    if( ch->pcdata->nextspartime <= 0 )
    {
        struct tm *tms;

        tms = localtime(&current_time);
        tms->tm_mday += 1;
        ch->pcdata->nextspartime = mktime(tms);
    }
    if( victim->pcdata->nextspartime <= 0 )
    {
        struct tm *tms;

        tms = localtime(&current_time);
        tms->tm_mday += 1;
        victim->pcdata->nextspartime = mktime(tms);
    }

    long double cspar = 0;
    long double vspar = 0;
    cspar = ch->exp - ch->spar_start;
    vspar = victim->exp - victim->spar_start;
    ch_printf(ch,"&cTotal pl gained this spar: &C%s\n\r",
                 num_punct_ld(cspar) );
    ch_printf(victim,"&cTotal pl gained this spar: &C%s\n\r",
                 num_punct_ld(vspar) );

    ch->spar_start = 0;
    victim->spar_start = 0;

    return;
}

void do_spar( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Spar whom?\n\r", ch );
	return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL )
    {
	send_to_char( "They aren't here.\n\r", ch );
	return;
    }

    if ( !IS_NPC(victim) && !victim->desc )
    {
	send_to_char("They are link dead, it wouldn't be right.", ch);
	return;
    }

	if ( IS_NPC(victim) )
    {
	send_to_char("You can only spar other players.", ch);
	return;
    }

    if ( victim == ch )
    {
	send_to_char( "You can't spar your self.\n\r", ch );
	return;
    }

	if (!IS_NPC(ch) && !IS_NPC(victim) && ch -> pcdata -> sparcount >= 100)
	{
		send_to_char("You are too tired from sparring to spar further today.\n\r",ch);
		return;
	}

	if (!IS_NPC(ch) && !IS_NPC(victim) && victim -> pcdata -> sparcount >= 100)
	{
		send_to_char("Your partner is too tired from sparring to spar further.\n\r",ch);
		return;
	}

    if (!IS_IMMORTAL(ch) && !IS_NPC(ch) && !IS_NPC(victim))
    {
    	if (!xIS_SET(ch->act, PLR_QUESTING) && xIS_SET(victim->act, PLR_QUESTING))
    	{
    		send_to_char( "You can't spar a player involved in a role playing event.\n\r", ch );
    		return;
    	}
    	if (xIS_SET(ch->act, PLR_QUESTING) && !xIS_SET(victim->act, PLR_QUESTING))
    	{
    		send_to_char( "You can't spar a player not involved in a role playing event.\n\r", ch );
    		return;
    	}
    }
	if (!IS_NPC(ch) && !IS_NPC(victim) && victim->hit < 86)
    {
	send_to_char( "They need to rest a bit before sparring some more.\n\r", ch );
	return;
    }
	if (!IS_NPC(ch) && !IS_NPC(victim) && ch->hit < 86)
    {
	send_to_char( "You need to rest a bit before sparring some more.\n\r", ch );
	return;
    }

	if ( !xIS_SET(ch->act, PLR_SPAR) )
		xSET_BIT(ch->act, PLR_SPAR);
	if ( !xIS_SET(victim->act, PLR_SPAR) )
		xSET_BIT(victim->act, PLR_SPAR);

    if ( is_safe( ch, victim, TRUE ) )
	return;

    if( ch->position == POS_RESTING
     || ch->position == POS_SLEEPING )
    {
      send_to_char( "How do you propose to do that in your current state?\n\r", ch );
      return;
    }

    if ( ch->position == POS_FIGHTING
       || ch->position ==  POS_EVASIVE
       || ch->position ==  POS_DEFENSIVE
       || ch->position ==  POS_AGGRESSIVE
       || ch->position ==  POS_BERSERK
       )
    {
	send_to_char( "You do the best you can!\n\r", ch );
	return;
    }

	if ( !xIS_SET(ch->in_room->room_flags, ROOM_ARENA))
	{
       	send_to_char( "You must be in an arena to spar someone.\n\r", ch );
        return;
    }
	if (!IS_NPC(ch) && !IS_NPC(victim) && ch->exp <= 5000)
    {
	send_to_char( "You can not fight other players while you are in training.\n\r", ch );
	return;
    }
	if (!IS_NPC(ch) && !IS_NPC(victim) && victim->exp <= 5000)
    {
	send_to_char( "You can not fight other players while they are in training.\n\r", ch );
	return;
    }
	if (who_fighting(victim) != NULL)
    {
	send_to_char( "It would not be honorable to interfere with some one else's battle.\n\r", ch );
	return;
    }
	if (!IS_NPC(ch) && !IS_NPC(victim) && victim->hit < 2)
    {
	send_to_char( "They are too hurt to fight anymore.\n\r", ch );
	return;
    }
	if (!IS_NPC(ch) && !IS_NPC(victim) && ch->hit < 2)
    {
	send_to_char( "You are too hurt to fight anymore.\n\r", ch );
	return;
    }
	if ( !IS_NPC(ch) && !IS_NPC(victim) && !IS_HC(ch) && !IS_HC(victim)
		&& xIS_SET(ch->act, PLR_SPAR) && xIS_SET(victim->act, PLR_SPAR)
		&& ( (IS_GOOD(ch) && !IS_GOOD(victim))
			|| (IS_EVIL(ch) && !IS_EVIL(victim))
			|| (IS_NEUTRAL(ch) && !IS_NEUTRAL(victim))
			) )
    {
	send_to_char( "You would not spar someone who is aligned that way.\n\r", ch );
	return;
    }

    if( !victim->pcdata->HBTCPartner )
    {
      send_to_char( "They are not accepting sparring partners at this time.\n\r", ch );
      return;
    }

    if( strcasecmp( ch->name, victim->pcdata->HBTCPartner ) )
    {
      send_to_char( "They do not want to spar with you.\n\r", ch );
      return;
    }
/*
		sprintf( log_buf, "PLAYER COMBAT: %s[%s] vs. %s[%s].",
			ch->name, !xIS_SET(ch->act, PLR_SPAR) ? "DEADLY" : "SPARING",
			victim->name, !xIS_SET(victim->act, PLR_SPAR) ? "DEADLY" : "SPARING");
		log_string_plus( log_buf, LOG_NORMAL, ch->level );
*/
		WAIT_STATE( ch, 1 * PULSE_VIOLENCE );
		if ( !xIS_SET(ch->act, PLR_SPAR) )
		{
			if( !is_leet(ch) )
			  sprintf( buf, "Help!  I am being attacked by %s!", IS_NPC( ch ) ? ch->short_descr : ch->name );
			else
			  sprintf( buf, "Omigawd!  %s atakin me!! STAWP!!11shift-one1", IS_NPC( ch ) ? ch->short_descr : ch->name );
			if ( IS_PKILL(victim) )
			{
				do_wartalk( victim, buf );
			}
			else
			{
				do_yell( victim, buf );
			}
		}
		check_illegal_pk( ch, victim );

    ch->spar_start = ch->exp;
    victim->spar_start = victim->exp;
    ch->delay = 0;
    check_attacker( ch, victim );
    multi_hit( ch, victim, TYPE_UNDEFINED );
    return;


}

void do_kill( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    OBJ_DATA *o;

	argument = one_argument( argument, arg );
//    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Kill whom?\n\r", ch );
        WAIT_STATE( ch, 1 * PULSE_VIOLENCE / 2 );
	return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL )
    {
	send_to_char( "They aren't here.\n\r", ch );
        WAIT_STATE( ch, 1 * PULSE_VIOLENCE / 2 );
		return;
    }

    if ( !IS_NPC(victim) && !victim->desc )
    {
	send_to_char("They are link dead, it wouldn't be right.", ch);
        WAIT_STATE( ch, 1 * PULSE_VIOLENCE / 2 );
		return;
    }
    if( IS_NPC(victim) && is_split(victim) )
    {
	ch_printf(ch, "You are not allowed to kill splitforms.\n\r");
	return;
    }
    if ( IS_NPC(victim) && victim->morph)
    {
	send_to_char("This creature appears strange to you.  Look upon it more closely before attempting to kill it.", ch);
        WAIT_STATE( ch, 1 * PULSE_VIOLENCE / 2 );
		return;
    }

    if ((float)victim->exp / ch->exp > 5 && !IS_NPC(victim) )
    {
          ch_printf(ch,"They are more than 5 times stronger than you.\n\r");
          return;
    }

    if( IS_NPC( victim ) && is_splitformed( victim ) && victim->master
     && victim->master == ch )
    {
        send_to_char( "You can't kill your own Split Form.\n\r", ch );
        WAIT_STATE( ch, 1 * PULSE_VIOLENCE / 2 );
        return;
    }

    if ( IS_NPC(ch) && is_splitformed(ch) && !IS_NPC(victim) )
    {
		send_to_char("You can't do that.", ch);
        WAIT_STATE( ch, 1 * PULSE_VIOLENCE / 2 );
		return;
    }

    if (!IS_NPC(ch) && IS_NPC(victim) && is_splitformed(victim))
    {
		pager_printf_color(ch, "Why don't you try attacking the real %s.", victim->short_descr);
        WAIT_STATE( ch, 1 * PULSE_VIOLENCE / 2 );
		return;
    }

    if( IS_NPC( victim ) && victim->master != NULL && IS_IMMORTAL( victim->master ) && !IS_IMMORTAL( ch ) )
    {
	send_to_char( "Cheatz.\n\r", ch );
	return;
    }

    if( IS_GOOD(ch) && !IS_EVIL(victim) && !IS_NPC(victim) &&
	!IS_HC(ch) && (o = carrying_dball(victim)) == NULL )
    {
	if( ch->kairank >= victim->kairank && !is_kaio(victim) )
	{
	  ch_printf(ch,"Its not in your nature to kill others that are not evil.\n\r");
	  return;
	}
    }

    if( victim->position <= POS_STUNNED )
    {
	ch_printf(ch,"They have one foot in the grave already. That would be pointless.\n\r");
	return;
    }

   /*
    *
    else
    {
	if ( IS_AFFECTED(victim, AFF_CHARM) && victim->master != NULL )
	{
	    send_to_char( "You must MURDER a charmed creature.\n\r", ch );
	    return;
	}
    }
    *
    */

    if ( victim == ch )
    {
	send_to_char( "You hit yourself.  Ouch!\n\r", ch );
/* Removed to avoid abuse and "self-inflicted SSJ*/
/*	multi_hit( ch, ch, TYPE_UNDEFINED );*/
        WAIT_STATE( ch, 1 * PULSE_VIOLENCE / 2 );
		return;
    }


    if ( is_safe( ch, victim, TRUE ) )
    {
        WAIT_STATE( ch, 1 * PULSE_VIOLENCE / 2 );
	return;
    }

    if (!IS_IMMORTAL(ch) && !IS_NPC(ch) && !IS_NPC(victim))
    {
    	if (!xIS_SET(ch->act, PLR_QUESTING) && xIS_SET(victim->act, PLR_QUESTING))
    	{
    		send_to_char( "You can't attack a player involved in a role playing event.\n\r", ch );
    		return;
    	}
    	if (xIS_SET(ch->act, PLR_QUESTING) && !xIS_SET(victim->act, PLR_QUESTING))
    	{
    		send_to_char( "You can't attack a player not involved in a role playing event.\n\r", ch );
    		return;
    	}
    }

    if ( xIS_SET(ch->in_room->room_flags, ROOM_ARENA))
    {
       	send_to_char( "You can only spar while in an arena.\n\r", ch );
        WAIT_STATE( ch, 1 * PULSE_VIOLENCE / 2 );
        return;
    }

	if ( xIS_SET(ch->act, PLR_SPAR)  )
        {
	  xREMOVE_BIT(ch->act, PLR_SPAR);
        }
	if ( xIS_SET(victim->act, PLR_SPAR)  )
        {
	  xREMOVE_BIT(victim->act, PLR_SPAR);
        }


    if ( IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim )
    {
    act( AT_PLAIN, "$N is your beloved master.", ch, NULL, victim, TO_CHAR );
        WAIT_STATE( ch, 1 * PULSE_VIOLENCE / 2 );
	return;
    }

    if( ch->position == POS_RESTING
     || ch->position == POS_SLEEPING )
    {
      send_to_char( "How do you propose to do that in your current state?\n\r", ch );
      return;
    }

    if ( ch->position == POS_FIGHTING
       || ch->position ==  POS_EVASIVE
       || ch->position ==  POS_DEFENSIVE
       || ch->position ==  POS_AGGRESSIVE
       || ch->position ==  POS_BERSERK
       )
    {
	send_to_char( "You do the best you can!\n\r", ch );
	return;
    }

	if (who_fighting(victim) != NULL)
    {
	send_to_char( "It would not be honorable to interfere with some one else's battle.\n\r", ch );
	return;
    }

	if (!IS_NPC(ch) && !IS_NPC(victim) && strcmp(argument, "now") )
    {
	send_to_char( "You must type 'kill <person> now' if you want to kill a player.\n\r", ch );
	return;
    }

	if (!IS_NPC(ch) && !IS_NPC(victim))
		if (!pkill_ok(ch, victim))
			return;

    if (!IS_NPC (victim))
//	{
//		send_to_char("PKing is currently disabled.\n\r",ch);
//		return;
//	}

	{
		sprintf( log_buf, "PLAYER COMBAT: %s[%s] vs. %s[%s].",
			ch->name, !xIS_SET(ch->act, PLR_SPAR) ? "DEADLY" : "SPARING",
			victim->name, !xIS_SET(victim->act, PLR_SPAR) ? "DEADLY" : "SPARING" );
		log_string_plus( log_buf, LOG_NORMAL, ch->level );
		WAIT_STATE( ch, 1 * PULSE_VIOLENCE );
		if ( !xIS_SET(ch->act, PLR_SPAR) )
		{
			if( !is_leet(ch) )
			  sprintf( buf, "Help!  I am being attacked by %s!", IS_NPC( ch ) ? ch->short_descr : ch->name );
			else
			  sprintf( buf, "Omigawd!  %s atakin me!! STAWP!!11shift-one1", IS_NPC( ch ) ? ch->short_descr : ch->name );
			if ( IS_PKILL(victim) )
			{
				do_wartalk( victim, buf );
			}
			else
			{
				do_yell( victim, buf );
			}
                        ch->focus     = 0;
                        victim->focus = 0;
		}
		check_illegal_pk( ch, victim );
	}

    else
    	WAIT_STATE( ch, 1 * PULSE_VIOLENCE );

	if (!IS_NPC(ch) && ch->race == 6)
	{
		find_absorb_data(ch, victim);
		ch->pcdata->absorb_pl = 0;
	}
	if (!IS_NPC(victim) && victim->race == 6)
	{
		find_absorb_data(victim, ch);
		victim->pcdata->absorb_pl = 0;
	}

    /* Moved to here so that mobs no longer retain prefocus. -Karma */
    ch->focus     = 0;
    victim->focus = 0;

    ch->delay = 0;
    ch->fight_start = 0;
    ch->fight_start = ch->exp;
    victim->fight_start = 0;
    victim->fight_start = victim->exp;
    check_attacker( ch, victim );
    multi_hit( ch, victim, TYPE_UNDEFINED );
    return;


}



void do_murde( CHAR_DATA *ch, char *argument )
{
    send_to_char( "If you want to MURDER, spell it out.\n\r", ch );
    return;
}



void do_murder( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Murder whom?\n\r", ch );
	return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL )
    {
	send_to_char( "They aren't here.\n\r", ch );
	return;
    }

    if ( victim == ch )
    {
	send_to_char( "Suicide is a mortal sin.\n\r", ch );
	return;
    }

	if ( !IS_NPC(victim) )
	{
	send_to_char( "Use KILL to kill another player.\n\r", ch );
	return;
    }

	if ( IS_NPC(victim) )
	{
	send_to_char( "Use KILL to kill another player.\n\r", ch );
	return;
    }

    if ( is_safe( ch, victim, TRUE ) )
	return;

    if ( IS_AFFECTED(ch, AFF_CHARM) )
    {
      if ( ch->master == victim )
      {
        act( AT_PLAIN, "$N is your beloved master.", ch, NULL, victim, TO_CHAR );
	return;
      }
      else
      {
        if ( ch->master )
          xSET_BIT(ch->master->act, PLR_ATTACKER);
      }
    }

    if ( ch->position == POS_FIGHTING
       || ch->position ==  POS_EVASIVE
       || ch->position ==  POS_DEFENSIVE
       || ch->position ==  POS_AGGRESSIVE
       || ch->position ==  POS_BERSERK
       )
    {
	send_to_char( "You do the best you can!\n\r", ch );
	return;
    }

    if ( !IS_NPC( victim ) && xIS_SET(ch->act, PLR_NICE ) )
    {
      send_to_char( "You feel too nice to do that!\n\r", ch );
      return;
    }
/*
    if ( !IS_NPC( victim ) && xIS_SET(victim->act, PLR_PK ) )
*/

    if (!IS_NPC (victim))
       {
       sprintf( log_buf, "%s: murder %s.", ch->name, victim->name );
       log_string_plus( log_buf, LOG_NORMAL, ch->level );
       }

    WAIT_STATE( ch, 1 * PULSE_VIOLENCE );
    if( !is_leet(ch) )
      sprintf( buf, "Help!  I am being attacked by %s!",
                   IS_NPC( ch ) ? ch->short_descr : ch->name );
    else
      sprintf( buf, "Omigawd!  %s atakin me!! STAWP!!11shift-one1", IS_NPC( ch ) ? ch->short_descr : ch->name );

    if ( IS_PKILL(victim) )
	do_wartalk( victim, buf );
    else
	do_yell( victim, buf );
    check_illegal_pk( ch, victim );
    check_attacker( ch, victim );
    multi_hit( ch, victim, TYPE_UNDEFINED );
    return;
}

/*
 * Check to see if the player is in an "Arena".
 */
bool in_arena( CHAR_DATA *ch )
{
    if ( xIS_SET(ch->in_room->room_flags, ROOM_ARENA) )
	return TRUE;
    if ( IS_SET(ch->in_room->area->flags, AFLAG_FREEKILL) )
	return TRUE;
    if ( ch->in_room->vnum >= 29 && ch->in_room->vnum <= 43 )
	return TRUE;
    if ( !str_cmp(ch->in_room->area->filename, "arena.are") )
	return TRUE;

    return FALSE;
}

bool check_illegal_pk( CHAR_DATA *ch, CHAR_DATA *victim )
{
  char buf[MAX_STRING_LENGTH];
  char buf2[MAX_STRING_LENGTH];


  if ( !IS_NPC(victim) && !IS_NPC(ch) )
  {
	if ( ( !IS_SET(victim->pcdata->flags, PCFLAG_DEADLY)
	|| !IS_SET(ch->pcdata->flags, PCFLAG_DEADLY) )
	&& !in_arena(ch)
	&& ch != victim )
	{
	    if ( IS_NPC(ch) )
		sprintf(buf, " (%s)", ch->name);
	    if ( IS_NPC(victim) )
		sprintf(buf2, " (%s)", victim->name);

	    sprintf( log_buf, "&p%s on %s%s in &W***&rILLEGAL PKILL&W*** &pattempt at %d",
		(lastplayercmd),
		(IS_NPC(victim) ? victim->short_descr : victim->name),
		(IS_NPC(victim) ? buf2 : ""),
		victim->in_room->vnum );
	    last_pkroom = victim->in_room->vnum;
	    log_string(log_buf);
	    to_channel( log_buf, CHANNEL_MONITOR, "Monitor", LEVEL_IMMORTAL );
	    return TRUE;
	}
    }
    return FALSE;
}


void do_flee( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    ROOM_INDEX_DATA *was_in;
    ROOM_INDEX_DATA *now_in;
    int attempt, los;
    sh_int door;
    EXIT_DATA *pexit;
	CHAR_DATA *wf;

    if ( !who_fighting(ch) )
    {
	if ( ch->position == POS_FIGHTING
	||   ch->position == POS_EVASIVE
        ||   ch->position == POS_DEFENSIVE
        ||   ch->position == POS_AGGRESSIVE
        ||   ch->position == POS_BERSERK )
	{
	  if ( ch->mount )
	    ch->position = POS_MOUNTED;
	  else
	    ch->position = POS_STANDING;
	}
	send_to_char( "You aren't fighting anyone.\n\r", ch );
	return;
    }
    if ( IS_AFFECTED( ch, AFF_BERSERK ) ) {
        send_to_char( "Flee while berserking?  You aren't thinking very clearly...\n\r", ch);
        return;
    }
/*
    if ( ch->move <= 0 ) {
	send_to_char( "You're too exhausted to flee from combat!\n\r", ch );
	return;
    }
*/
    /* No fleeing while more aggressive than standard or hurt. - Haus */
    if ( !IS_NPC( ch ) && ch->position < POS_FIGHTING ) {
	send_to_char( "You can't flee in an aggressive stance...\n\r", ch );
	return;
    }
    if ( IS_NPC( ch ) && ch->position <= POS_SLEEPING )
	return;
	if (!IS_NPC(ch) && !IS_NPC(who_fighting(ch))
		&& xIS_SET(ch->act, PLR_SPAR) && xIS_SET(who_fighting(ch)->act, PLR_SPAR))
	{
		send_to_char( "Use 'STOPSPAR' to stop fighting.\n\r", ch );
		return;
    }

    was_in = ch->in_room;

    /* Decided to make fleeing harder to accomplish when in a pk fight.
     * -Karma
     */
	wf = who_fighting( ch );
    bool nochance = FALSE;
    if( !IS_NPC(ch) && !IS_NPC(wf) )
    {
	if( number_range(1,2) == 2 ) // 50% chance
	  nochance = TRUE;
    }
    else
    {
	if( number_range(1,5) < 2 ) // 80% chance
	  nochance = TRUE;
    }

    if( !nochance )
    {
      for ( attempt = 0; attempt < 8; attempt++ )
      {
	door = number_door( );
	if (( pexit = get_exit( was_in, door ) ) == NULL
	||    !pexit->to_room
	||   IS_SET( pexit->exit_info, EX_NOFLEE )
	|| ( IS_SET( pexit->exit_info, EX_CLOSED )
	&&  !IS_AFFECTED( ch, AFF_PASS_DOOR ) )
	|| ( IS_NPC( ch )
	&&   xIS_SET( pexit->to_room->room_flags, ROOM_NO_MOB ) ) )
	    continue;
        affect_strip ( ch, gsn_sneak );
        xREMOVE_BIT  ( ch->affected_by, AFF_SNEAK );
	if ( ch->mount && ch->mount->fighting )
	    stop_fighting( ch->mount, TRUE );
	move_char( ch, pexit, 0 );
	if ( ( now_in = ch->in_room ) == was_in )
	    continue;
	ch->in_room = was_in;
	act( AT_FLEE, "$n flees head over heels!", ch, NULL, NULL, TO_ROOM );
	ch->in_room = now_in;
	act( AT_FLEE, "$n glances around for signs of pursuit.", ch, NULL, NULL, TO_ROOM );
        if( !IS_NPC( ch ) && is_bio(ch) )
        {
          /* Clear out any chance to absorb something */
	  ch->pcdata->absorb_sn = 0;
	  ch->pcdata->absorb_learn = 0;
        }
	if ( !IS_NPC( ch ) )
	{
	    act( AT_FLEE, "You flee head over heels from combat!", ch, NULL, NULL, TO_CHAR );
	    if ( !IS_NPC(ch) && !IS_NPC(wf) )
	    {
		if( ch->pcdata->clan && wf->pcdata->clan )
		{
/*		    if( allianceStatus( ch->pcdata->clan, wf->pcdata->clan ) == ALLIANCE_ATWAR )
		    {
			los = ch->exp * 0.001;
		    }
		    else
		    { */
			los = ch->exp * 0.005;
//		    }
		}
		else
		{
		    los = ch->exp * 0.005;
		}
	    }
	    else
		los = ch->exp * 0.005;
/*
             && ch->pcdata && wf->pcdata
             && ch->pcdata->clan && wf->pcdata->clan
             && allianceStatus(ch->pcdata->clan, wf->pcdata->clan) == ALLIANCE_ATWAR)
	    	{
				los = ch->exp * 0.001;
			}
		else
	    	{
				los = ch->exp * 0.005;
			}
*/
	    if ( !IS_NPC(wf) )
	    {
	      if( is_android(wf) || is_superandroid(wf) )
		sprintf( buf, "Shit, you've lost %d tech level!", los );
	      else
	       sprintf( buf, "Shit, you've lost %d power level!", los );
 	      act( AT_FLEE, buf, ch, NULL, NULL, TO_CHAR );
	      gain_exp( ch, 0 - los );
	    }

	    if ( wf && ch->pcdata->deity )
	    {
	      int level_ratio = URANGE( 1, wf->level / ch->level, 50 );

	      if ( wf && wf->race == ch->pcdata->deity->npcrace )
		adjust_favor( ch, 1, level_ratio );
   	      else
		if ( wf && wf->race == ch->pcdata->deity->npcfoe )
		  adjust_favor( ch, 16, level_ratio );
		else
		  adjust_favor( ch, 0, level_ratio );
	    }
	}
	stop_fighting( ch, TRUE );
	return;
      }
    }
    act( AT_FLEE, "You attempt to flee from combat but can't escape!", ch, NULL, NULL, TO_CHAR );
    return;
}


void do_sla( CHAR_DATA *ch, char *argument )
{
    send_to_char( "If you want to SLAY, spell it out.\n\r", ch );
    return;
}


void do_slay( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *victim;
    char arg[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];

    argument = one_argument( argument, arg );
    one_argument( argument, arg2 );
    if ( arg[0] == '\0' )
    {
	send_to_char( "Syntax: [Char] [Type]\n\r", ch );
	send_to_char( "Types: Skin, Slit, Immolate, Demon, Shatter, 9mm, Deheart, Pounce, Cookie, Fslay.\n\r", ch);
	return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL )
    {
	send_to_char( "They aren't here.\n\r", ch );
	return;
    }

    if ( ch == victim )
    {
	send_to_char( "Suicide is a mortal sin.\n\r", ch );
	return;
    }

    if ( !IS_NPC(victim) && ch->level < sysdata.level_mset_player )
    {
	send_to_char( "You can't do that to players.\n\r", ch );
	return;
    }

    if ( !str_cmp( arg2, "immolate" ) )
    {
      act( AT_FIRE, "Your fireball turns $N into a blazing inferno.",  ch, NULL, victim, TO_CHAR    );
      act( AT_FIRE, "$n releases a searing fireball in your direction.", ch, NULL, victim, TO_VICT    );
      act( AT_FIRE, "$n points at $N, who bursts into a flaming inferno.",  ch, NULL, victim, TO_NOTVICT );
    }

    else if ( !str_cmp( arg2, "skin" ) )
    {
    act( AT_BLOOD, "You rip the flesh from $N and send his soul to the fiery depths of hell.", ch, NULL, victim, TO_CHAR );
    act( AT_BLOOD, "Your flesh has been torn from your bones and your bodyless soul now watches your bones incenerate in the fires of hell.", ch, NULL, victim,TO_VICT );
    act( AT_BLOOD, "$n rips the flesh off of $N, releasing his soul into the fiery depths of hell.", ch, NULL, victim, TO_NOTVICT );
	}

	else if ( !str_cmp( arg2, "9mm" ) )
	{
	act( AT_IMMORT, "You pull out your 9mm and bust a cap in $N's ass.", ch, NULL, victim, TO_CHAR );
	act( AT_IMMORT, "$n pulls out $s 9mm and busts a cap in your ass.", ch, NULL, victim, TO_VICT );
	act( AT_IMMORT, "$n pulls out $s 9mm and busts a cap in $N's ass.", ch, NULL, victim, TO_NOTVICT );
	}

	else if ( !str_cmp( arg2, "deheart" ) )
	{
	act( AT_BLOOD, "You rip through $N's chest and pull out $M beating heart in your hand.", ch, NULL, victim, TO_CHAR );
	act( AT_BLOOD, "You feel a sharp pain as $n rips into your chest and pulls our your beating heart in $M hand.", ch, NULL, victim, TO_VICT );
	act( AT_BLOOD, "Specks of blood hit your face as $n rips through $N's chest pulling out $M's beating heart.", ch, NULL, victim, TO_NOTVICT );
	}

    else if ( !str_cmp( arg2, "shatter" ) )
    {
      act( AT_LBLUE, "You freeze $N with a glance and shatter the frozen corpse into tiny shards.",  ch, NULL, victim, TO_CHAR    );
      act( AT_LBLUE, "$n freezes you with a glance and shatters your frozen body into tiny shards.", ch, NULL, victim, TO_VICT    );
      act( AT_LBLUE, "$n freezes $N with a glance and shatters the frozen body into tiny shards.",  ch, NULL, victim, TO_NOTVICT );
    }

    else if ( !str_cmp( arg2, "demon" ) )
    {
      act( AT_IMMORT, "You gesture, and a slavering demon appears.  With a horrible grin, the",  ch, NULL, victim, TO_CHAR );
      act( AT_IMMORT, "foul creature turns on $N, who screams in panic before being eaten alive.",  ch, NULL, victim, TO_CHAR );
      act( AT_IMMORT, "$n gestures, and a slavering demon appears.  The foul creature turns on",  ch, NULL, victim, TO_VICT );
      act( AT_IMMORT, "you with a horrible grin.   You scream in panic before being eaten alive.",  ch, NULL, victim, TO_VICT );
      act( AT_IMMORT, "$n gestures, and a slavering demon appears.  With a horrible grin, the",  ch, NULL, victim, TO_NOTVICT );
      act( AT_IMMORT, "foul creature turns on $N, who screams in panic before being eaten alive.",  ch, NULL, victim, TO_NOTVICT );
    }

    else if ( !str_cmp( arg2, "pounce" ) )
    {
      act( AT_BLOOD, "Leaping upon $N with bared fangs, you tear open $S throat and toss the corpse to the ground...",  ch, NULL, victim, TO_CHAR );
      act( AT_BLOOD, "In a heartbeat, $n rips $s fangs through your throat!  Your blood sprays and pours to the ground as your life ends...", ch, NULL, victim, TO_VICT );
      act( AT_BLOOD, "Leaping suddenly, $n sinks $s fangs into $N's throat.  As blood sprays and gushes to the ground, $n tosses $N's dying body away.",  ch, NULL, victim, TO_NOTVICT );
    }

    else if ( !str_cmp( arg2, "slit" ) )
    {
      act( AT_BLOOD, "You calmly slit $N's throat.", ch, NULL, victim, TO_CHAR );
      act( AT_BLOOD, "$n reaches out with a clawed finger and calmly slits your throat.", ch, NULL, victim, TO_VICT );
      act( AT_BLOOD, "$n calmly slits $N's throat.", ch, NULL, victim, TO_NOTVICT );
    }

    else if ( !str_cmp( arg2, "dog" ) )
    {
      act( AT_BLOOD, "You order your dogs to rip $N to shreds.", ch, NULL, victim, TO_CHAR );
      act( AT_BLOOD, "$n orders $s dogs to rip you apart.", ch, NULL, victim, TO_VICT );
      act( AT_BLOOD, "$n orders $s dogs to rip $N to shreds.", ch, NULL, victim, TO_NOTVICT );
    }

    else if ( !str_cmp( arg2, "fslay" ))
    {
      act( AT_IMMORT, "You point at $N and fall down laughing.", ch, NULL, victim, TO_CHAR );
      act( AT_IMMORT, "$n points at you and falls down laughing. How embaressing!.", ch, NULL, victim, TO_VICT );
      act( AT_IMMORT, "$n points at $N and falls down laughing.", ch, NULL, victim, TO_NOTVICT );
      return;
    }
    else if ( !str_cmp( arg2, "cookie" ) )
    {
      act( AT_BLOOD, "You point a finger at $N and $e turns into a cookie!", ch, NULL, victim, TO_CHAR );
      act( AT_BLOOD, "$n points $s finger at you and you turn into a cookie!", ch, NULL, victim, TO_VICT );
      act( AT_BLOOD, "$n points $s finger at $N and $e turns into a cookie!", ch, NULL, victim, TO_NOTVICT );
    }

    else
    {
      act( AT_IMMORT, "You slay $N in cold blood!",  ch, NULL, victim, TO_CHAR    );
      act( AT_IMMORT, "$n slays you in cold blood!", ch, NULL, victim, TO_VICT    );
      act( AT_IMMORT, "$n slays $N in cold blood!",  ch, NULL, victim, TO_NOTVICT );
    }

    set_cur_char(victim);
    raw_kill( ch, victim );
    return;
}

void do_fslay( CHAR_DATA *ch, char *argument )
{
    {
    CHAR_DATA *victim;
    char arg[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    /*char buf[MAX_STRING_LENGTH];*/

    argument = one_argument( argument, arg );
    one_argument( argument, arg2 );
    if ( arg[0] == '\0' )
    {
	send_to_char( "Syntax: [Char] [Type]\n\r", ch );
	send_to_char( "Types: Skin, Slit, Immolate, Demon, Shatter, 9mm, Deheart, Pounce, Cookie, Fslay, Lightning.\n\r", ch);
	return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL )
    {
	send_to_char( "They aren't here.\n\r", ch );
	return;
    }

    if ( ch == victim )
    {
	send_to_char( "Suicide is a mortal sin.\n\r", ch );
	return;
    }

    if ( !IS_NPC(victim) && get_trust( victim ) >= get_trust( ch ) )
    {
	send_to_char( "You failed.\n\r", ch );
	return;
    }

    if ( !str_cmp( arg2, "immolate" ) )
    {
      act( AT_FIRE, "Your fireball turns $N into a blazing inferno.",  ch, NULL, victim, TO_CHAR    );
      act( AT_FIRE, "$n releases a searing fireball in your direction.", ch, NULL, victim, TO_VICT    );
      act( AT_FIRE, "$n points at $N, who bursts into a flaming inferno.",  ch, NULL, victim, TO_NOTVICT );
    }

    if ( !str_cmp( arg2, "lightning" ) )
    {
      act( AT_RED, "You throw a lightning bolt at $N, reducing them to cinders.", ch, NULL, victim, TO_CHAR    );
      act( AT_RED, "A holy lightning bolt from the skies above reduces you to blasphemous cinders.", ch, NULL, victim, TO_VICT    );
      act( AT_RED, "$N is reduced to blasphemous cinders by a lightning bolt.",  ch, NULL, victim, TO_NOTVICT );
    }

    else if ( !str_cmp( arg2, "skin" ) )
    {
    act( AT_BLOOD, "You rip the flesh from $N and send his soul to the fiery depths of hell.", ch, NULL, victim, TO_CHAR );
    act( AT_BLOOD, "Your flesh has been torn from your bones and your bodyless soul now watches your bones incenerate in the fires of hell.", ch, NULL, victim,TO_VICT );
    act( AT_BLOOD, "$n rips the flesh off of $N, releasing his soul into the fiery depths of hell.", ch, NULL, victim, TO_NOTVICT );
	}

	else if ( !str_cmp( arg2, "9mm" ) )
	{
	act( AT_IMMORT, "You pull out your 9mm and bust a cap in $N's ass.", ch, NULL, victim, TO_CHAR );
	act( AT_IMMORT, "$n pulls out $s 9mm and busts a cap in your ass.", ch, NULL, victim, TO_VICT );
	act( AT_IMMORT, "$n pulls out $s 9mm and busts a cap in $N's ass.", ch, NULL, victim, TO_NOTVICT );
	}

	else if ( !str_cmp( arg2, "deheart" ) )
	{
	act( AT_BLOOD, "You rip through $N's chest and pull out $M beating heart in your hand.", ch, NULL, victim, TO_CHAR );
	act( AT_BLOOD, "You feel a sharp pain as $n rips into your chest and pulls our your beating heart in $M hand.", ch, NULL, victim, TO_VICT );
	act( AT_BLOOD, "Specks of blood hit your face as $n rips through $N's chest pulling out $M's beating heart.", ch, NULL, victim, TO_NOTVICT );
	}

    else if ( !str_cmp( arg2, "shatter" ) )
    {
      act( AT_LBLUE, "You freeze $N with a glance and shatter the frozen corpse into tiny shards.",  ch, NULL, victim, TO_CHAR    );
      act( AT_LBLUE, "$n freezes you with a glance and shatters your frozen body into tiny shards.", ch, NULL, victim, TO_VICT    );
      act( AT_LBLUE, "$n freezes $N with a glance and shatters the frozen body into tiny shards.",  ch, NULL, victim, TO_NOTVICT );
    }

    else if ( !str_cmp( arg2, "demon" ) )
    {
      act( AT_IMMORT, "You gesture, and a slavering demon appears.  With a horrible grin, the",  ch, NULL, victim, TO_CHAR );
      act( AT_IMMORT, "foul creature turns on $N, who screams in panic before being eaten alive.",  ch, NULL, victim, TO_CHAR );
      act( AT_IMMORT, "$n gestures, and a slavering demon appears.  The foul creature turns on",  ch, NULL, victim, TO_VICT );
      act( AT_IMMORT, "you with a horrible grin.   You scream in panic before being eaten alive.",  ch, NULL, victim, TO_VICT );
      act( AT_IMMORT, "$n gestures, and a slavering demon appears.  With a horrible grin, the",  ch, NULL, victim, TO_NOTVICT );
      act( AT_IMMORT, "foul creature turns on $N, who screams in panic before being eaten alive.",  ch, NULL, victim, TO_NOTVICT );
    }

    else if ( !str_cmp( arg2, "pounce" ) )
    {
      act( AT_BLOOD, "Leaping upon $N with bared fangs, you tear open $S throat and toss the corpse to the ground...",  ch, NULL, victim, TO_CHAR );
      act( AT_BLOOD, "In a heartbeat, $n rips $s fangs through your throat!  Your blood sprays and pours to the ground as your life ends...", ch, NULL, victim, TO_VICT );
      act( AT_BLOOD, "Leaping suddenly, $n sinks $s fangs into $N's throat.  As blood sprays and gushes to the ground, $n tosses $N's dying body away.",  ch, NULL, victim, TO_NOTVICT );
    }

    else if ( !str_cmp( arg2, "slit" ) )
    {
      act( AT_BLOOD, "You calmly slit $N's throat.", ch, NULL, victim, TO_CHAR );
      act( AT_BLOOD, "$n reaches out with a clawed finger and calmly slits your throat.", ch, NULL, victim, TO_VICT );
      act( AT_BLOOD, "$n calmly slits $N's throat.", ch, NULL, victim, TO_NOTVICT );
    }

    else if ( !str_cmp( arg2, "dog" ) )
    {
      act( AT_BLOOD, "You order your dogs to rip $N to shreds.", ch, NULL, victim, TO_CHAR );
      act( AT_BLOOD, "$n orders $s dogs to rip you apart.", ch, NULL, victim, TO_VICT );
      act( AT_BLOOD, "$n orders $s dogs to rip $N to shreds.", ch, NULL, victim, TO_NOTVICT );
    }

    else if ( !str_cmp( arg2, "fslay" ))
    {
      act( AT_IMMORT, "You point at $N and fall down laughing.", ch, NULL, victim, TO_CHAR );
      act( AT_IMMORT, "$n points at you and falls down laughing. How embaressing!.", ch, NULL, victim, TO_VICT );
      act( AT_IMMORT, "$n points at $N and falls down laughing.", ch, NULL, victim, TO_NOTVICT );
      return;
    }
    else if ( !str_cmp( arg2, "cookie" ) )
    {
      act( AT_BLOOD, "You point a finger at $N and $e turns into a cookie!", ch, NULL, victim, TO_CHAR );
      act( AT_BLOOD, "$n points $s finger at you and you turn into a cookie!", ch, NULL, victim, TO_VICT );
      act( AT_BLOOD, "$n points $s finger at $N and $e turns into a cookie!", ch, NULL, victim, TO_NOTVICT );
    }

    else
    {
      act( AT_IMMORT, "You slay $N in cold blood!",  ch, NULL, victim, TO_CHAR    );
      act( AT_IMMORT, "$n slays you in cold blood!", ch, NULL, victim, TO_VICT    );
      act( AT_IMMORT, "$n slays $N in cold blood!",  ch, NULL, victim, TO_NOTVICT );
    }

    /*victim->hit 	= victim->max_hit - victim->max_hit + 1;
    victim->mana	= victim->mana;
    victim->move	= victim->move;
    sprintf( buf, "%s 1500", victim->name );
    do_transfer(ch, buf);
    update_pos(victim);*/
    return;
	}

}
