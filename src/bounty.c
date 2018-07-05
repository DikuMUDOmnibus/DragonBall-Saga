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
 *                           Bounty Add-On                                  *
 ****************************************************************************/
/****************************************************************************
 * Bounty for ResortMUD 5.0 by Garinan                                      *
 *                                                                          *
 * A simple code which allows players to place a gold bounty on the head of *
 * a deadly player.                                                         *
 *                                                                          *
 * If you have any questions, comments, concerns or bugs to report which    *
 * are not resolved within the included readme.txt, please email me at:     *
 * garinan@hotmail.com, your support is greatly appreciated.                *
 *                                                                          *
 *                                                  - Garinan               *
 ****************************************************************************/
/****************************************************************************
 * Copyright (c) Andrew Brunelle 2000, All Rights Reserved                  *
 * This code, as well as this document is distributed to be used freely     *
 * provided that credit be given to myself, its author Andrew Brunelle      *
 * (Garinan Templar). If you wish to distribute this snippet via your own   *
 * website or any other means, please contact me first at the email address *
 * below, I will likely only ask that you add a link to UCMM in exchange,   *
 * thank you and good luck!                                                 *
 ****************************************************************************/
/****************************************************************************
 *********************************DISCLAIMER*********************************
 ****************************************************************************
 * By adding this code to your MUD you are doing so at your own risk.       *
 * Neither I (Andrew Brunelle), nor any organization with which i am        *
 * affiliated, will be held responsible for any damages to your MUD by      *
 * adding this code.                                                        *
 ****************************************************************************/

#include <sys/types.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include "mud.h"

CHAR_DATA *find_bounty( CHAR_DATA *ch )
{
  CHAR_DATA *bounty;

    for ( bounty = ch->in_room->first_person; bounty; bounty = bounty->next_in_room )
    {
    	if ( IS_NPC(bounty) && xIS_SET(bounty->act, ACT_BOUNTY) )
        	break;
    }

    return bounty;
}

void do_bounty( CHAR_DATA *ch, char *argument )
{
	DESCRIPTOR_DATA *d;
	CHAR_DATA *vch;
  CHAR_DATA *bounty;
  CHAR_DATA *victim = NULL;
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  char buf[MAX_STRING_LENGTH];
  int amount, bountymax;
  int count = 0;
  int cost = 0;

/* cost is the cost for a peaceful to become ineligable to be bountied *
 * bountymax is the maximum amount to be put on a player's head        *
 * alter below to suit your needs.                           - Garinan */

  bountymax = 2000000000;

    if( ch->exp < 100000 )
    {
	send_to_char( "You can't use the bounty command until you are 100,000 PL.\n\r", ch );
	return;
    }

    if ( ( bounty = find_bounty( ch ) ) == NULL )
    {

/* Why should players need to be at a bounty officer to check *
 * the value of their bounty? They shouldn't :P - Garinan     */

       if ( !str_cmp( argument, "check" ) )
       {
          if ( IS_NPC( ch ) )
             ch_printf( ch, "&YMobiles can not have bounties.\n\r" );
          else if ( ch->pcdata->bounty > 0 )
             ch_printf( ch, "&YYou have a bounty worth %s zeni on your head.\n\r", num_punct(ch->pcdata->bounty) );
          else
             ch_printf( ch, "&YYou have no bounty on your head.\n\r" );
          if ( IS_NPC( ch ) )
             ch_printf( ch, "&YMobiles can not collect bounties.\n\r" );
          else if ( ch->pcdata->bowed > 0 )
             ch_printf( ch, "&YYou are owed %s zeni in bounty earnings.\n\r", num_punct(ch->pcdata->bowed) );
          else
             ch_printf( ch, "&YYou have not claimed any bounties.\n\r" );
          return;
       }

     send_to_char( "There is no bounty officer here!\n\r", ch );
     return;
    }

    act( AT_ACTION, "$n and $N whisper quietly to each other.", ch, ch, bounty, TO_ROOM );
    act( AT_ACTION, "You whisper your request to $N.", ch, bounty, bounty, TO_CHAR );

    if ( IS_NPC( ch ) )
    {
       sprintf( buf, "We don't deal with your kind %s.",
       ch->short_descr );
       do_say( bounty, buf );
       return;
    }

    if ( argument[0] == '\0' )
    {
       sprintf( buf, "If you need help, see HELP BOUNTY." );
       do_say( bounty, buf );
       return;
    }

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( arg1 == '\0' )
    {
       sprintf( buf, "How much zeni do you want to place, and on who?" );
       do_say( bounty, buf );
       return;
    }

/* Bounty check, players can check if they have a bounty on them *
 * and if so, how much its value is.                             *
 *                                         - Garinan             */

     if ( !str_cmp( arg1, "check" ) )
     {

        if ( arg2[0] == '\0' )
        {
           if ( IS_NPC( ch ) )
              ch_printf( ch, "&YMobiles can not have bounties.\n\r" );
           else if ( ch->pcdata->bounty > 0 )
              ch_printf( ch, "&YYou have a bounty worth %s zeni on your head.\n\r", num_punct(ch->pcdata->bounty) );
           else
              ch_printf( ch, "&YYou have no bounty on your head.\n\r" );
           if ( IS_NPC( ch ) )
              ch_printf( ch, "&YMobiles can not collect bounties.\n\r" );
           else if ( ch->pcdata->bowed > 0 )
              ch_printf( ch, "&YYou are owed %s zeni in bounty earnings.\n\r", num_punct(ch->pcdata->bowed) );
           else
              ch_printf( ch, "&YYou have not collected any bounties.\n\r" );
           return;
        }

        if ( !( victim = get_char_world( ch, arg2 ) ) )
        {
           do_say( bounty, "I don't know anyone by that name." );
           return;
        }

        if ( IS_NPC( victim ) )
            do_say( bounty, "Mobiles cannot have bounties!" );
        else
        if ( ( victim = get_char_world( ch, arg2 ) ) )
        {

           if ( victim->pcdata->bounty > 0 )
           {
              sprintf( buf, "%s %s's bounty is worth %s zeni.",
              ch->name, victim->name, num_punct(victim->pcdata->bounty) );
              do_tell( bounty, buf );
              return;
           }

           sprintf( buf, "%s That player has no bounty.", ch->name );
           do_tell( bounty, buf );
           return;
        }
        return;

    }

    if ( !str_cmp( arg1, "buy" ) )
    {
        if ( ( bounty = find_bounty( ch ) ) == NULL )
        {
           ch_printf( ch, "There is no bounty officer here!\n\r" );
           return;
        }

        if ( ch->pcdata->bounty <= 0 || !xIS_SET(ch->act, PLR_BOUNTY) )
        {
           sprintf( buf, "%s You don't have a bounty on your head.", ch->name );
           do_tell( bounty, buf );
			if (xIS_SET(ch->act, PLR_BOUNTY))
			xREMOVE_BIT(ch->act, PLR_BOUNTY);
			if (ch->pcdata->bounty != 0)
			ch->pcdata->bounty = 0;
			if (ch->pcdata->bounty_by != str_dup( "" ))
			{
			DISPOSE(ch->pcdata->bounty_by);
			ch->pcdata->bounty_by = str_dup( "" );
			}
          return;
        }

		cost = ch->pcdata->bounty * 0.25;

        if ( ch->gold < cost )
        {
           sprintf( buf, "%s I'm sorry, you can't afford this.", ch->name );
           do_tell( bounty, buf );
           return;
        }

        ch->gold -= cost;
        sprintf( buf, "%s You have bought the bounty that was put on your head.", ch->name );
        do_tell( bounty, buf );
        ch->pcdata->bounty = 0;
        ch->pcdata->b_timeleft = 1440;
		xREMOVE_BIT(ch->act, PLR_BOUNTY);
		DISPOSE(ch->pcdata->bounty_by);
		ch->pcdata->bounty_by = str_dup( "" );
        return;
    }

    if ( !str_cmp( arg1, "who" ) )
    {

        if ( ( bounty = find_bounty( ch ) ) == NULL )
        {
           ch_printf( ch, "There is no bounty officer here!\n\r" );
           return;
        }

        if ( ch->pcdata->bounty <= 0 || !xIS_SET(ch->act, PLR_BOUNTY) )
        {
           sprintf( buf, "%s You don't have a bounty on your head.", ch->name );
           do_tell( bounty, buf );
			if (xIS_SET(ch->act, PLR_BOUNTY))
			xREMOVE_BIT(ch->act, PLR_BOUNTY);
			if (ch->pcdata->bounty != 0)
			ch->pcdata->bounty = 0;
			if (ch->pcdata->bounty_by != NULL)
			{
			DISPOSE(ch->pcdata->bounty_by);
			ch->pcdata->bounty_by = str_dup( "" );
			}
           return;
        }

		cost = ch->pcdata->bounty * 0.05;

        if ( ch->gold < cost )
        {
           sprintf( buf, "%s I'm sorry, you can't afford this.", ch->name );
           do_tell( bounty, buf );
           return;
        }

        ch->gold -= cost;
        sprintf( buf, "%s It was %s who put the bounty on your head.", ch->name, ch->pcdata->bounty_by );
        do_tell( bounty, buf );
        return;
    }

     if ( !str_cmp( arg1, "hunt" ) )
     {

        if ( ( bounty = find_bounty( ch ) ) == NULL )
        {
           ch_printf( ch, "There is no bounty officer here!\n\r" );
           return;
        }

        if (!IS_HC(ch) && !IS_IMMORTAL(ch))
        {
           sprintf( buf, "%s People like you don't have the skill or guts to hunt others.", ch->name );
           do_tell( bounty, buf );
           return;
        }
        if ( arg2[0] == '\0' )
        {

			pager_printf_color( ch, "The folowing people have a price on their head:\n\r" );
		for( d = first_descriptor; d; d= d->next )
		{
			vch = d->character;

			if (!vch)
			{
			continue;
			}

			if (xIS_SET(vch->act, PLR_BOUNTY))
			{
			pager_printf_color( ch, " &RWANTED: [&W%-16s&R]  REWARD:[&W%-10s&R]\n\r", vch->name, num_punct(vch->pcdata->bounty) );
			count++;
			}

		}

		if (count <= 0)
			pager_printf_color( ch, "&w No one had a price on their head right now.\n\r" );

	  return;
      }

        if ( !( victim = get_char_world( ch, arg2 ) ) )
        {
           do_say( bounty, "I don't know anyone by that name." );
           return;
        }

        if ( ( victim = get_char_world( ch, arg2 ) ) && !IS_NPC( victim ) )
        {
			cost = victim->pcdata->bounty * 0.25;

           if ( victim->pcdata->bounty > 0 && xIS_SET(victim->act, PLR_BOUNTY))
           {
	          if ( !str_cmp( argument, "pay" ) )
    	      {
    	      	if (ch->gold < cost)
	              {
	              	sprintf( buf, "%s You don't have enough zeni.",
	              	ch->name );
	              	do_tell( bounty, buf );
	              	return;
	              }
	             if ( !str_cmp( ch->name, victim->pcdata->bounty_by ) )
	              {
	              	sprintf( buf, "%s You can not clam your own bounty.",
	              	ch->name );
	              	do_tell( bounty, buf );
	              	return;
	              }
              sprintf( buf, "%s Very well, you may try to collect the bounty on %s's head.",
              ch->name, victim->name );
              do_tell( bounty, buf );
//              ch->gold -= (ch->pcdata->bounty * 0.25);
              ch->gold -= cost;
              ch->pcdata->hunting = strdup(victim->name);
              return;
         	  }
			sprintf( buf, "%s You will have to pay a service fee of %s to collect that bounty.",
			ch->name, num_punct(cost));
			do_tell( bounty, buf );
			return;
        	}
         }

	sprintf( buf, "%s That player has no bounty.", ch->name );
	do_tell( bounty, buf );
	return;
	}

    if ( !str_cmp( arg1, "collect" ) )
    {
       if ( ch->pcdata->bowed <= 0 )
       {
          sprintf( buf, "%s I owe you nothing %s!", ch->name, ch->name );
          do_tell( bounty, buf );
          return;
       }

       ch->gold += ch->pcdata->bowed;
       sprintf( buf, "&YYou collect %s zeni worth of bounties.\n\r", num_punct(ch->pcdata->bowed) );
       send_to_char( buf, ch );
       ch->pcdata->bowed = 0;
       sprintf( buf, "%s A pleasure doing business with you.", ch->name );
       do_tell( bounty, buf );
       act( AT_ACTION, "$N hands a small sack of zeni to $n.", ch, ch, bounty, TO_ROOM );
       return;
    }

    if ( !( victim = get_char_world( ch, arg2 ) ) )
    {
       do_say( bounty, "That player is not online." );
       return;
    }

    if ( IS_NPC( victim ) )
    {
       do_say( bounty, "A mobile? Kill it yourself!" );
       return;
    }

    amount = 0;
    if( !IS_NPC( victim ) && victim->pcdata->bounty > 0 )
        cost = victim->pcdata->bounty * .5;
    else
    if( !IS_HC( victim ) )
	cost = get_rank_number(victim) * get_rank_number(victim) * get_rank_number(victim) * 10000;
    else
	cost = get_true_rank(ch) * get_true_rank(ch) * get_true_rank(victim) * 1750;

	if (cost == 0)
        cost = 1;

    if ( !str_cmp( arg1, "all" ) )
    {
        amount = ch->gold;
    }

    if ( amount == 0 )
    {
        if (!is_number(arg1))
	    {
	       sprintf( buf, "%s You must pay me in zeni!", ch->name );
	       do_tell( bounty, buf );
	       return;
	    }

        amount = atoi( arg1 );
    }

    if ( amount > ch->gold )
    {
       sprintf( buf, "%s Check your pockets and try again, you don't have that much zeni!", ch->name );
       do_tell( bounty, buf );
       return;
    }

    if ( amount <= 0 )
    {
       sprintf( buf, "%s That amount is too frugal, try again!", ch->name );
       do_tell( bounty, buf );
       return;
    }

    if ( victim->level >= LEVEL_IMMORTAL && ch != victim )
    {
      sprintf( buf, "%s You may not place a bounty on the head of an administrator!", ch->name );
      do_tell( bounty, buf );
      return;
    }

    if ( victim->exp < 100000 && ch != victim )
    {
       sprintf( buf, "%s you can not place a bounty on a player under 100,000 power level.", ch->name );
       do_tell( bounty, buf );
       return;
    }

/*  this shouldn't be here  -Goku
    if( !xIS_SET( victim->act, PLR_PK1 )
     && !xIS_SET( victim->act, PLR_PK2 ) )
    {
      sprintf( buf, "%s You may not place a bounty on those who don't have pkill on.", ch->name );
      do_tell( bounty, buf );
      return;
    }
*/
    if ( ( amount + victim->pcdata->bounty ) > bountymax )
    {
        sprintf( buf, "%s That would exceed the maximum allowable bounty of %s zeni!", ch->name, num_punct(bountymax) );
        do_tell( bounty, buf );
        return;
    }


    if ( amount < cost )
    {
       sprintf( buf, "%s The minimum bounty you can place on %s is %s",
       ch->name, victim->name, num_punct(cost) );
       do_tell( bounty, buf );
       return;
    }

    if ( ch == victim )
    {
       sprintf( buf, "%s Place a bounty on yourself? Ever considered suicide?", ch->name );
       do_tell( bounty, buf );
       return;
    }

	if (victim->pcdata->b_timeleft > 0 && victim->pcdata->bounty <= 0)
    {
       sprintf( buf, "%s I don't like spam killing, you must wait %d more %s to put another bounty on %s.", ch->name,
       			victim->pcdata->b_timeleft >= 60 ? victim->pcdata->b_timeleft / 60 : victim->pcdata->b_timeleft,
       			victim->pcdata->b_timeleft >= 60 ? "hours" : "minutes", himher(victim, FALSE));
       do_tell( bounty, buf );
       return;
    }

    ch->gold -= amount;
    act( AT_ACTION, "You hand a small sack of zeni to $N.", ch, ch, bounty, TO_CHAR );
    act( AT_ACTION, "$n hands a small sack of zeni to $N.", ch, ch, bounty, TO_ROOM );
    if (victim->pcdata->bounty > 0)
    {
       sprintf( buf, "%s The bounty on %s has been increased by %s zeni!", ch->name, victim->name, num_punct(amount) );
       do_tell( bounty, buf );
       sprintf( buf, "&RThe bounty on %s has been increased by %s zeni!", victim->name, num_punct(amount) );
		do_info( bounty, buf );
       sprintf( buf, "&YThe bounty on your head has been increased by %s zeni!\n\r", num_punct(amount) );
       send_to_char( buf, victim );
       if (victim->pcdata->bounty < 0)
		victim->pcdata->bounty = amount;
       else
		victim->pcdata->bounty += amount;
       return;
    }
    victim->pcdata->bounty = amount;
	xSET_BIT(victim->act, PLR_BOUNTY);
	if (victim->pcdata->bounty_by)
	{
		DISPOSE(victim->pcdata->bounty_by);
		victim->pcdata->bounty_by = str_dup( "" );
	}
	victim->pcdata->bounty_by = str_dup( ch->name );
	act( AT_SOCIAL, "$n nods in agreement with you.", bounty, NULL, ch, TO_VICT );
	act( AT_SOCIAL, "$n nods in agreement to $N.", bounty, NULL, ch, TO_NOTVICT );
    sprintf( buf, "&RA bounty worth %s zeni has been placed on the head of %s!", num_punct(amount), victim->name );
	do_info( bounty, buf );
	pager_printf_color(victim, "&RA bounty worth %s zeni has been placed on your head!\n\r",num_punct(amount) );
}

void do_unbounty( CHAR_DATA *ch, char *argument )
{
  CHAR_DATA *victim;

    if ( !( victim = get_char_world( ch, argument ) ) )
    {
    ch_printf( ch, "&YNo such player online.\n\r" );
    return;
    }

    if ( victim->pcdata->bounty > 0 )
    {
    ch_printf( ch, "&pRemoving Bounty...\n\rDone.\n\r" );
    victim->pcdata->bounty  = 0;
	xREMOVE_BIT(victim->act, PLR_BOUNTY);
	DISPOSE(victim->pcdata->bounty_by);
	victim->pcdata->bounty_by = str_dup( "" );
    return;
    }

    ch_printf( ch, "&YThat player has no bounty.\n\r" );

	if (xIS_SET(victim->act, PLR_BOUNTY))
		xREMOVE_BIT(victim->act, PLR_BOUNTY);
	if (victim->pcdata->bounty != 0)
		victim->pcdata->bounty = 0;
	if (victim->pcdata->bounty_by != NULL)
	{
		DISPOSE(victim->pcdata->bounty_by);
		victim->pcdata->bounty_by = str_dup( "" );
	}
	return;
}
