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
*			     Informational module			    *
****************************************************************************/


#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <math.h>
#include "mud.h"
#include "sha256.h"

#ifdef MCCP
#define TELOPT_COMPRESS 85
#define TELOPT_COMPRESS2 86
#endif

/* Had to add unknowns because someone added new positions and didn't
 * update them.  Just a band-aid till I have time to fix it right.
 * This was found thanks to mud@mini.axcomp.com pointing it out :)
 * --Shaddai
 */


char *	const	where_name	[] =
    {
        "&w<used as light>      ",
        "&w<worn on finger>     ",
        "&w<worn on finger>     ",
        "&w<worn around neck>   ",
        "&w<worn around neck>   ",
        "&w<worn on body>       ",
        "&w<worn on head>       ",
        "&w<worn on legs>       ",
        "&w<worn on feet>       ",
        "&w<worn on hands>      ",
        "&w<worn on arms>       ",
        "&w<worn as shield>     ",
        "&w<worn about body>    ",
        "&w<worn about waist>   ",
        "&w<worn around wrist>  ",
        "&w<worn around wrist>  ",
        "&w<wielded>            ",
        "&w<held>               ",
        "&w<dual wielded>       ",
        "&w<worn on ears>       ",
        "&w<worn on eyes>       ",
        "&w<missile wielded>    ",
        "&w<worn on back>       ",
        "&w<worn over face>     ",
        "&w<worn around ankle>  ",
        "&w<worn around ankle>  ",
        "&w<access panel>       ",
        "&w<BUG Inform Goku>  ",
        "&w<BUG Inform Goku>  "
//        "&w<BUG Inform Goku>  "
    };

char *  const   fusion_flags [] =
{
"stasis","dance","potara","namek","superandroid"
};


/*
 * Local functions.
 */
void	show_char_to_char_0	args( ( CHAR_DATA *victim, CHAR_DATA *ch ) );
void	show_char_to_char_1	args( ( CHAR_DATA *victim, CHAR_DATA *ch ) );
void	show_char_to_char	args( ( CHAR_DATA *list, CHAR_DATA *ch ) );
bool	check_blind	args( ( CHAR_DATA *ch ) );
void show_condition args( ( CHAR_DATA *ch, CHAR_DATA *victim ) );
void	show_ships_to_char	args( ( SHIP_DATA *ship, CHAR_DATA *ch ) );
void show_compass_dir	args( (CHAR_DATA *ch) );

char *	const	compass_dir	[]	=
    {
        "&YN", "&YE", "&YS", "&YW", "&Yu", "&Yd",
        "&YNE", "&YNW", "&YSE", "&YSW",
        "|", "-", "|", "-", "-", "-",
        " -", "- ", " -", "- ",
        "&P#", "&P#", "&P#", "&P#", "&P#", "&P#", "&P#", "&P#", "&P#", "&P#"
    };

void show_compass_dir(CHAR_DATA *ch)
{
	OBJ_DATA *scouter;
	EXIT_DATA *pexit;
	int cDir[11];
	int i;

	if ( !is_android(ch) && (scouter = has_scouter(ch)) == NULL )
	{
		send_to_char( "You need a scouter to do that.\n\r", ch );
		return ;
	}

	for (i = 0; i < 10;i++)
		cDir[i] = i + 10;

	if (has_scouter(ch) || is_android(ch))
	{
		for ( pexit = ch->in_room->first_exit; pexit; pexit = pexit->next )
		{
			if (IS_SET(pexit->exit_info, EX_WINDOW)
			    || IS_SET(pexit->exit_info, EX_HIDDEN))
				cDir[pexit->vdir] = pexit->vdir + 10;
			else if (IS_SET(pexit->exit_info, EX_CLOSED)
			         && IS_SET(pexit->exit_info, EX_ISDOOR))
				cDir[pexit->vdir] = pexit->vdir + 20;
			else
				cDir[pexit->vdir] = pexit->vdir;
		}

		pager_printf_color(ch, "&G%2s&G     %s&G     %-2s&G\n\r",
		                   compass_dir[cDir[7]], compass_dir[cDir[0]], compass_dir[cDir[6]]);
		pager_printf_color(ch, "&G%s&G <-%s&G-(+)-%s&G-> %s&G&G\n\r",
		                   compass_dir[cDir[3]], compass_dir[cDir[4]], compass_dir[cDir[5]], compass_dir[cDir[1]]);
		pager_printf_color(ch, "&G%2s&G     %s&G     %-2s&G\n\r",
		                   compass_dir[cDir[9]], compass_dir[cDir[2]], compass_dir[cDir[8]]);

		/*		pager_printf_color(ch, "^g&G%2s^g&G     %s^g&G     %-2s^g&G\n\r",
					compass_dir[cDir[7]], compass_dir[cDir[0]], compass_dir[cDir[6]]);
				pager_printf_color(ch, "^g&G%s^g&G <-%s^g&G-(+)-%s^g&G-> %s^g&G^g&G\n\r",
					compass_dir[cDir[3]], compass_dir[cDir[4]], compass_dir[cDir[5]], compass_dir[cDir[1]]);
				pager_printf_color(ch, "^g&G%2s^g&G     %s^g&G     %-2s^g&G\n\r",
					compass_dir[cDir[9]], compass_dir[cDir[2]], compass_dir[cDir[8]]);
		*/
	}
	return ;
}

void show_ships_to_char( SHIP_DATA *ship, CHAR_DATA *ch )
{
	SHIP_DATA *rship;
	SHIP_DATA *nship = NULL;

	for ( rship = ship; rship; rship = nship )
	{
		pager_printf_color( ch , "&C%-35s     ", rship->name );
		if ( ( nship = rship->next_in_room ) != NULL )
		{
			pager_printf_color( ch , "%-35s", nship->name );
			nship = nship->next_in_room;
		}
		pager_printf_color( ch, "\n\r&w");
	}

	return ;
}

char *format_obj_to_char( OBJ_DATA *obj, CHAR_DATA *ch, bool fShort )
{
	static char buf[MAX_STRING_LENGTH];
	bool glowsee = FALSE;

	/* can see glowing invis items in the dark */
	if ( IS_OBJ_STAT(obj, ITEM_GLOW) && IS_OBJ_STAT(obj, ITEM_INVIS)
	     && !IS_AFFECTED(ch, AFF_TRUESIGHT) && !IS_AFFECTED(ch, AFF_DETECT_INVIS) )
		glowsee = TRUE;

	buf[0] = '\0';
	if ( IS_OBJ_STAT(obj, ITEM_INVIS) )
		strcat( buf, "(Invis) " );
	if ( (IS_AFFECTED(ch, AFF_DETECT_EVIL))
	     && IS_OBJ_STAT(obj, ITEM_EVIL) )
		strcat( buf, "(Red Aura) " );


	if ( IS_AFFECTED(ch, AFF_DETECT_MAGIC)
	     && IS_OBJ_STAT(obj, ITEM_MAGIC) )
		strcat( buf, "(Magical) " );
	if ( !glowsee && IS_OBJ_STAT(obj, ITEM_GLOW) )
		strcat( buf, "(Glowing) " );
	if ( IS_OBJ_STAT(obj, ITEM_HUM) )
		strcat( buf, "(Humming) " );
	if ( IS_OBJ_STAT(obj, ITEM_HIDDEN)	)
		strcat( buf, "(Hidden) "	);
	if ( IS_OBJ_STAT(obj, ITEM_BURIED)	)
		strcat( buf, "(Buried) "	);
	if ( IS_IMMORTAL(ch)
	     && IS_OBJ_STAT(obj, ITEM_PROTOTYPE) )
		strcat( buf, "(PROTO) "	);
	if ( IS_AFFECTED(ch, AFF_DETECTTRAPS)
	     && is_trapped(obj) )
		strcat( buf, "(Trap) " );

	if ( fShort )
	{
		if ( glowsee && !IS_IMMORTAL(ch) )
			strcat( buf, "the faint glow of something" );
		else
			if ( obj->short_descr )
				strcat( buf, obj->short_descr );
	}
	else
	{
		if ( glowsee )
			strcat( buf, "You see the faint glow of something nearby." );
		if ( obj->description )
			strcat( buf, obj->description );
	}

	return buf;
}


/*
 * Some increasingly freaky hallucinated objects		-Thoric
 * (Hats off to Albert Hoffman's "problem child")
 */
char *hallucinated_object( int ms, bool fShort )
{
	int sms = URANGE( 1, (ms + 10) / 5, 20 );

	if ( fShort )
		switch ( number_range( 6 - URANGE(1, sms / 2, 5), sms ) )
		{
			case 1:
				return "a sword";
			case 2:
				return "a stick";
			case 3:
				return "something shiny";
			case 4:
				return "something";
			case 5:
				return "something interesting";
			case 6:
				return "something colorful";
			case 7:
				return "something that looks cool";
			case 8:
				return "a nifty thing";
			case 9:
				return "a cloak of flowing colors";
			case 10:
				return "a mystical flaming sword";
			case 11:
				return "a swarm of insects";
			case 12:
				return "a deathbane";
			case 13:
				return "a figment of your imagination";
			case 14:
				return "your gravestone";
			case 15:
				return "the long lost boots of Ranger Thoric";
			case 16:
				return "a glowing tome of arcane knowledge";
			case 17:
				return "a long sought secret";
			case 18:
				return "the meaning of it all";
			case 19:
				return "the answer";
			case 20:
				return "the key to life, the universe and everything";
		}
	switch ( number_range( 6 -URANGE(1, sms / 2, 5), sms ) )
	{
		case 1:
			return "A nice looking sword catches your eye.";
		case 2:
			return "The ground is covered in small sticks.";
		case 3:
			return "Something shiny catches your eye.";
		case 4:
			return "Something catches your attention.";
		case 5:
			return "Something interesting catches your eye.";
		case 6:
			return "Something colorful flows by.";
		case 7:
			return "Something that looks cool calls out to you.";
		case 8:
			return "A nifty thing of great importance stands here.";
		case 9:
			return "A cloak of flowing colors asks you to wear it.";
		case 10:
			return "A mystical flaming sword awaits your grasp.";
		case 11:
			return "A swarm of insects buzzes in your face!";
		case 12:
			return "The extremely rare Deathbane lies at your feet.";
		case 13:
			return "A figment of your imagination is at your command.";
		case 14:
			return "You notice a gravestone here... upon closer examination, it reads your name.";
		case 15:
			return "The long lost boots of Ranger Thoric lie off to the side.";
		case 16:
			return "A glowing tome of arcane knowledge hovers in the air before you.";
		case 17:
			return "A long sought secret of all mankind is now clear to you.";
		case 18:
			return "The meaning of it all, so simple, so clear... of course!";
		case 19:
			return "The answer.  One.  It's always been One.";
		case 20:
			return "The key to life, the universe and everything awaits your hand.";
	}
	return "Whoa!!!";
}


/* This is the punct snippet from Desden el Chaman Tibetano - Nov 1998
   Email: jlalbatros@mx2.redestb.es
*/
char *num_punct(int foo)
{
	int index, index_new, rest;
	char buf[16];
	static char buf_new[16];

	sprintf(buf, "%d", foo);
	rest = strlen(buf) % 3;

	for (index = index_new = 0;index < strlen(buf);index++, index_new++)
	{
		if (index != 0 && (index - rest) % 3 == 0 )
		{
			buf_new[index_new] = ',';
			index_new++;
			buf_new[index_new] = buf[index];
		}
		else
			buf_new[index_new] = buf[index];
	}
	buf_new[index_new] = '\0';
	return buf_new;
}

char *num_punct_d(double foo)
{


	int index, index_new, rest;
	char buf[20];
	static char buf_new[20];

	sprintf(buf, "%.0f", foo);
	rest = strlen(buf) % 3;
	for (index = index_new = 0;index < strlen(buf);index++, index_new++)
	{
		if (index != 0 && (index - rest) % 3 == 0 )
		{
			buf_new[index_new] = ',';
			index_new++;
			buf_new[index_new] = buf[index];
		}
		else
			buf_new[index_new] = buf[index];
	}
	buf_new[index_new] = '\0';
	return buf_new;
}

char *num_punct_ld(long double foo)
{


	int index, index_new, rest;
	char buf[20];
	static char buf_new[20];

	sprintf(buf, "%.0Lf", foo);
	rest = strlen(buf) % 3;
	for (index = index_new = 0;index < strlen(buf);index++, index_new++)
	{
		if (index != 0 && (index - rest) % 3 == 0 )
		{
			buf_new[index_new] = ',';
			index_new++;
			buf_new[index_new] = buf[index];
		}
		else
			buf_new[index_new] = buf[index];
	}
	buf_new[index_new] = '\0';
	return buf_new;
}

/*
 * Show a list to a character.
 * Can coalesce duplicated items.
 */
void show_list_to_char( OBJ_DATA *list, CHAR_DATA *ch, bool fShort, bool fShowNothing )
{
	char **prgpstrShow;
	int *prgnShow;
	int *pitShow;
	char *pstrShow;
	OBJ_DATA *obj;
	int nShow;
	int iShow;
	int count, offcount, tmp, ms, cnt;
	bool fCombine;

	if ( !ch->desc )
		return ;

	/*
	 * if there's no list... then don't do all this crap!  -Thoric
	 */
	if ( !list )
	{
		if ( fShowNothing )
		{
			if ( IS_NPC(ch) || xIS_SET(ch->act, PLR_COMBINE) )
				send_to_char( "     ", ch );
			set_char_color( AT_OBJECT, ch );
			send_to_char( "Nothing.\n\r", ch );
		}
		return ;
	}
	/*
	 * Alloc space for output lines.
	 */
	count = 0;
	for ( obj = list; obj; obj = obj->next_content )
		count++;

	ms = (ch->mental_state ? ch->mental_state : 1)
	     * (IS_NPC(ch) ? 1 : (ch->pcdata->condition[COND_DRUNK] ? (ch->pcdata->condition[COND_DRUNK] / 12) : 1));

	/*
	 * If not mentally stable...
	 */
	if ( abs(ms) > 40 )
	{
		offcount = URANGE( -(count), (count * ms) / 100, count * 2 );
		if ( offcount < 0 )
			offcount += number_range(0, abs(offcount));
		else
			if ( offcount > 0 )
				offcount -= number_range(0, offcount);
	}
	else
		offcount = 0;

	if ( count + offcount <= 0 )
	{
		if ( fShowNothing )
		{
			if ( IS_NPC(ch) || xIS_SET(ch->act, PLR_COMBINE) )
				send_to_char( "     ", ch );
			set_char_color( AT_OBJECT, ch );
			send_to_char( "Nothing.\n\r", ch );
		}
		return ;
	}

	CREATE( prgpstrShow, char*, count + ((offcount > 0) ? offcount : 0) );
	CREATE( prgnShow, int, count + ((offcount > 0) ? offcount : 0) );
	CREATE( pitShow, int, count + ((offcount > 0) ? offcount : 0) );
	nShow	= 0;
	tmp	= (offcount > 0) ? offcount : 0;
	cnt	= 0;

	/*
	 * Format the list of objects.
	 */
	for ( obj = list; obj; obj = obj->next_content )
	{
		if ( offcount < 0 && ++cnt > (count + offcount) )
			break;
		if ( tmp > 0 && number_bits(1) == 0 )
		{
			prgpstrShow [nShow] = str_dup( hallucinated_object(ms, fShort) );
			prgnShow	[nShow] = 1;
			pitShow	[nShow] = number_range( ITEM_LIGHT, ITEM_BOOK );
			nShow++;
			--tmp;
		}
		if ( obj->wear_loc == WEAR_NONE
		     && can_see_obj( ch, obj )
		     && (obj->item_type != ITEM_TRAP || IS_AFFECTED(ch, AFF_DETECTTRAPS) ) )
		{
			pstrShow = format_obj_to_char( obj, ch, fShort );
			fCombine = FALSE;

			if ( IS_NPC(ch) || xIS_SET(ch->act, PLR_COMBINE) )
			{
				/*
				 * Look for duplicates, case sensitive.
				 * Matches tend to be near end so run loop backwords.
				 */
				for ( iShow = nShow - 1; iShow >= 0; iShow-- )
				{
					if ( !strcmp( prgpstrShow[iShow], pstrShow ) )
					{
						prgnShow[iShow] += obj->count;
						fCombine = TRUE;
						break;
					}
				}
			}

			pitShow[nShow] = obj->item_type;
			/*
			 * Couldn't combine, or didn't want to.
			 */
			if ( !fCombine )
			{
				prgpstrShow [nShow] = str_dup( pstrShow );
				prgnShow [nShow] = obj->count;
				nShow++;
			}
		}
	}
	if ( tmp > 0 )
	{
		int x;
		for ( x = 0; x < tmp; x++ )
		{
			prgpstrShow [nShow] = str_dup( hallucinated_object(ms, fShort) );
			prgnShow	[nShow] = 1;
			pitShow	[nShow] = number_range( ITEM_LIGHT, ITEM_BOOK );
			nShow++;
		}
	}

	/*
	 * Output the formatted list.		-Color support by Thoric
	 */
	for ( iShow = 0; iShow < nShow; iShow++ )
	{
		switch (pitShow[iShow])
		{
			default:
				set_char_color( AT_OBJECT, ch );
				break;
			case ITEM_BLOOD:
				set_char_color( AT_BLOOD, ch );
				break;
			case ITEM_MONEY:
			case ITEM_TREASURE:
				set_char_color( AT_YELLOW, ch );
				break;
			case ITEM_COOK:
			case ITEM_FOOD:
				set_char_color( AT_HUNGRY, ch );
				break;
			case ITEM_DRINK_CON:
			case ITEM_FOUNTAIN:
				set_char_color( AT_THIRSTY, ch );
				break;
			case ITEM_FIRE:
				set_char_color( AT_FIRE, ch );
				break;
			case ITEM_SCROLL:
			case ITEM_WAND:
			case ITEM_STAFF:
				set_char_color( AT_MAGIC, ch );
				break;
		}
		if ( fShowNothing )
			send_to_char( "     ", ch );
		send_to_char( prgpstrShow[iShow], ch );
		/*	if ( IS_NPC(ch) || xIS_SET(ch->act, PLR_COMBINE) ) */
		{
			if ( prgnShow[iShow] != 1 )
				ch_printf( ch, " (%d)", prgnShow[iShow] );
		}

		send_to_char( "\n\r", ch );
		DISPOSE( prgpstrShow[iShow] );
	}

	if ( fShowNothing && nShow == 0 )
	{
		if ( IS_NPC(ch) || xIS_SET(ch->act, PLR_COMBINE) )
			send_to_char( "     ", ch );
		set_char_color( AT_OBJECT, ch );
		send_to_char( "Nothing.\n\r", ch );
	}

	/*
	 * Clean up.
	 */
	DISPOSE( prgpstrShow );
	DISPOSE( prgnShow	);
	DISPOSE( pitShow	);
	return ;
}


/*
 * Show fancy descriptions for certain spell affects		-Thoric
 */
void show_visible_affects_to_char( CHAR_DATA *victim, CHAR_DATA *ch )
{
	char buf[MAX_STRING_LENGTH];
	char name[MAX_STRING_LENGTH];

	if ( IS_NPC( victim ) )
		strcpy( name, victim->short_descr );
	else
		strcpy( name, victim->name);
	name[0] = toupper(name[0]);

	sh_int z = get_aura(victim);

	if ( IS_AFFECTED(victim, AFF_DEAD) )
	{
	    pager_printf_color(ch, "  &P*");
	    if( !is_leet(ch) )
	      act( AT_WHITE, "$N has a halo above $S head.&D",ch, NULL, victim, TO_CHAR );
	    else
	      act( AT_WHITE, "$N iz ded. sux2be$m.&D",ch, NULL, victim, TO_CHAR );
	}
	if ( IS_AFFECTED(victim, AFF_OOZARU) )
	{
	    pager_printf_color(ch,"  &P*");
	    if( !is_leet(ch) )
	      act( AT_YELLOW, "$N is a huge Oozaru.&D", ch, NULL, victim, TO_CHAR );
	    else
	      act( AT_YELLOW, "$N iz a big hairy monkeh. oh em gee&D", ch, NULL, victim, TO_CHAR );
	}
	if ( IS_AFFECTED(victim, AFF_GOLDEN_OOZARU) )
        {
            pager_printf_color(ch,"  &P*");
            if( !is_leet(ch) )
	    {
              act( AT_YELLOW, "$N is a huge Oozaru.&D", ch, NULL,victim, TO_CHAR );
	      pager_printf_color(ch,"  &P*");
	      act( AT_YELLOW, "$N has bright golden fur and solid blue eyes.&D", ch, NULL,victim, TO_CHAR );
	    }
            else
              act( AT_YELLOW, "$N iz a big hairy gold monkeh. oh em gee&D", ch, NULL, victim, TO_CHAR );
        }
	/*if ( IS_AFFECTED(victim, AFF_MYSTIC) )
	{
	    pager_printf_color(ch,"  &P*");
	    act( AT_WHITE, "$N looks normal.", ch, NULL, victim, TO_CHAR );
	}*/
	if ( IS_AFFECTED(victim, AFF_SUPERANDROID) )
	{
	    pager_printf_color(ch,"  &P*");
	    act( AT_DGREY, "$N's metal body is bulging with raw power.", ch, NULL, victim, TO_CHAR );
	    pager_printf_color(ch,"  &P*");
	    act( AT_RED, "$N has dark red energy crackling around $S form.",ch, NULL, victim, TO_CHAR );
	    /*act( AT_RED, "$N is engulfed in a blazing aura of red.", ch, NULL, victim, TO_CHAR );*/
	}
	if ( IS_AFFECTED(victim, AFF_KAIOKEN) )
	{
	    pager_printf_color(ch, "  &P*");
	    if( !is_leet(ch) )
	      act( AT_RED, "$N is engulfed within a blaze of mystical flame.", ch, NULL, victim, TO_CHAR );
	    else
	      act( AT_RED, "$N iz red hawt. LOLZ.", ch, NULL, victim, TO_CHAR );
	}
	if ( IS_AFFECTED(victim, AFF_SSJ) && !IS_AFFECTED(victim, AFF_SSJ2)
	     && !IS_AFFECTED(victim, AFF_SSJ3) && !IS_AFFECTED(victim, AFF_SSJ4) )
	{
		pager_printf_color(ch, "  &P*");
		if( !is_leet(ch) )
		  act( AT_YELLOW, "$N is surrounded by a fiery yellow aura.", ch, NULL, victim, TO_CHAR );
		else
		  act( AT_YELLOW, "$N iz hawt.", ch, NULL, victim, TO_CHAR );
	}

	if( IS_AFFECTED( victim, AFF_KID_TRANS ) )
	{
		pager_printf_color( ch, "  &P*" );
		act( AT_PINK, "$N radiates an intense energy, far more than $S size indicates.", ch, NULL, victim, TO_CHAR );
	}

	if ( ( IS_AFFECTED(victim, AFF_SSJ) && IS_AFFECTED(victim, AFF_SSJ2)
	     && !IS_AFFECTED(victim, AFF_SSJ3) && !IS_AFFECTED(victim, AFF_SSJ4) ) )
	{
		pager_printf_color(ch, "  &P*");
//		act( AT_YELLOW, "$N has electricity arcing around $S body.", ch, NULL, victim, TO_CHAR );
		if( !is_leet(ch) )
		  act( AT_YELLOW, "$N has electricity arcing through $S yellow aura.", ch, NULL, victim, TO_CHAR );
		else
		  act( AT_YELLOW, "$N iz hawt * 2.", ch, NULL, victim, TO_CHAR );
//		pager_printf_color(ch, "  &P*");
//		act( AT_BLUE, "$N is surrounded by random bolts of electricity.", ch, NULL, victim, TO_CHAR );
	}
	if ( ( IS_AFFECTED(victim, AFF_SSJ) && IS_AFFECTED(victim, AFF_SSJ2)
	     && IS_AFFECTED(victim, AFF_SSJ3) && !IS_AFFECTED(victim, AFF_SSJ4) )
	     || IS_AFFECTED(victim, AFF_KID_TRANS ) )
	{
		if( !IS_AFFECTED(victim, AFF_KID_TRANS ) )
		{
			pager_printf_color(ch, "  &P*");
			if( !is_leet(ch) )
			  act( AT_YELLOW, "$N has a huge golden aura and extremely long, yellow hair.", ch, NULL, victim, TO_CHAR );
			else
			  act( AT_YELLOW, "$N iz hawt * 3. Omigawd.", ch, NULL, victim, TO_CHAR );
		}
		pager_printf_color(ch, "  &P*");
		act( AT_BLUE, "$N is surrounded by random bolts of electricity.", ch, NULL, victim, TO_CHAR );
	}
	if ( IS_AFFECTED(victim, AFF_SSJ) && IS_AFFECTED(victim, AFF_SSJ2)
	     && IS_AFFECTED(victim, AFF_SSJ3) && IS_AFFECTED(victim, AFF_SSJ4) )
	{
		pager_printf_color(ch, "  &P*");
//		act( AT_RED, "$N is covered in thick red fur, with a fiery red aura.&P", ch, NULL, victim, TO_CHAR );
		if( !is_leet(ch) )
		  act( AT_RED, "$N's body is covered in thick, red fur.&P", ch, NULL, victim, TO_CHAR );
		else
		  act( AT_RED, "$N iz red hawt * 4. PWNT!", ch, NULL, victim, TO_CHAR );
		pager_printf_color(ch, "  &P*");
		act( AT_RED, "$N is surrounded by a menacing aura of fiery red.&P", ch, NULL, victim, TO_CHAR );
	}
	if ( IS_AFFECTED(victim, AFF_SSJ) && IS_AFFECTED(victim, AFF_USSJ)
	     && !IS_AFFECTED(victim, AFF_USSJ2) )
	{
		pager_printf_color(ch, "  &P*");
		act( AT_YELLOW, "$N's muscles are bulging with energy.&P", ch, NULL, victim, TO_CHAR );
	}
	if ( IS_AFFECTED(victim, AFF_SSJ) && IS_AFFECTED(victim, AFF_USSJ)
	     && IS_AFFECTED(victim, AFF_USSJ2) )
	{
		pager_printf_color(ch, "  &P*");
		act( AT_YELLOW, "$N's muscles are grossly oversized.&P", ch, NULL, victim, TO_CHAR );
	}
	if ( IS_AFFECTED( victim, AFF_ELECTRICSHIELD ) )
	{
		pager_printf_color(ch, "  &P*");
		if( !is_leet(ch) )
		  act( AT_DGREEN, "$N is surrounded by a crackling field of pure electricity.&P", ch, NULL, victim, TO_CHAR );
		else
		  act( AT_DGREEN, "$N is surrounded by a crackling field of cereal. LOLZ. Seriel, get it? Snap crackle pop. are oh eff el&P", ch, NULL, victim, TO_CHAR );
	}
	if ( IS_AFFECTED(victim, AFF_SNAMEK) )
	{
	  if( !is_leet(ch) )
	  {
		pager_printf_color(ch, "  &P*&G%s is covered in bright green flames.&P\n\r", name);
		pager_printf_color(ch, "  &P*" );
		act( AT_GREEN, "$N has a calm, calculating look on $S face.&P", ch, NULL, victim, TO_CHAR );
	  }
	  else
	  {
                pager_printf_color(ch, "  &P*" );
                act( AT_GREEN, "$N iz a namuk. omg lawl. $E is WEEK.&P", ch, NULL, victim, TO_CHAR );
	  }
	}
        if ( IS_AFFECTED(victim, AFF_GROWTH ) && !IS_AFFECTED(victim, AFF_GIANT) )
        {
		if( !is_leet(ch) )
		  pager_printf_color(ch, "  &P*&G%s has grown to an enormous size.&P\n\r", name);
		else
		  pager_printf_color(ch, "  &P*&G%s has grown to an enormous size...IN THE PANTS! lolz&P\n\r", name);
        }
        if ( IS_AFFECTED(victim, AFF_GIANT ) )
        {
		if( !is_leet(ch) )
		  pager_printf_color(ch, "  &P*&G%s is a monstrously huge Namek.&P\n\r", name);
		else
		  pager_printf_color(ch, "  &P*&G%s is a monstrously huge Namek...IN THE PANTS! lolz&P\n\r", name);
        }
	if ( IS_AFFECTED(victim, AFF_ICER2) && !IS_AFFECTED(victim, AFF_ICER3)
	     && !IS_AFFECTED(victim, AFF_ICER4) && !IS_AFFECTED(victim, AFF_ICER5) )
	{
//		pager_printf_color(ch, "  &P*&P%s is covered in bright purple flames.&P\n\r", name);
		if( !is_leet(ch) )
		  pager_printf_color(ch, "  &P*&P%s's large, menacing body is covered in purple flames.&P\n\r", name);
		else
		  pager_printf_color(ch, "  &P*&P%s rawks.&P\n\r", name);
	}
	if ( !IS_AFFECTED(victim, AFF_ICER2) && IS_AFFECTED(victim, AFF_ICER3)
	     && !IS_AFFECTED(victim, AFF_ICER4) && !IS_AFFECTED(victim, AFF_ICER5) )
	{
//		pager_printf_color(ch, "  &P*&P%s is covered in bright purple flames.&P\n\r", name);
		if( !is_leet(ch) )
		  pager_printf_color(ch, "  &P*&P%s is a hideous monster covered in bright, purple flames.&P\n\r", name);
		else
		  pager_printf_color(ch, "  &P*&P%s RAWKS d00d.&P\n\r", name);
	}
	if ( !IS_AFFECTED(victim, AFF_ICER2) && !IS_AFFECTED(victim, AFF_ICER3)
	     && IS_AFFECTED(victim, AFF_ICER4) && !IS_AFFECTED(victim, AFF_ICER5) )
	{
//		pager_printf_color(ch, "  &P*&P%s is covered in dark, icy purple flames.&P\n\r", name);
		if( !is_leet(ch) )
		{
		pager_printf_color(ch, "  &P*" );
		act( AT_PURPLE, "$N has glass-like skin that shines and reflects the energy around $M.&P",
                     ch, NULL, victim, TO_CHAR );
		pager_printf_color(ch, "  &P*&P%s is covered in dark, icy purple flames.&P\n\r", name);
		}
		else
		  pager_printf_color(ch, "  &P*&p%s rawks...IN THE PANTS!...hay! stawp lookin at my pants!&P\n\r", name);
	}
	if ( !IS_AFFECTED(victim, AFF_ICER2) && !IS_AFFECTED(victim, AFF_ICER3)
	     && !IS_AFFECTED(victim, AFF_ICER4) && IS_AFFECTED(victim, AFF_ICER5) )
	{
//		pager_printf_color(ch, "  &P*&P%s is covered in dark, icy purple flames.&P\n\r", name);
		if( !is_leet(ch) )
		{
		pager_printf_color(ch, "  &P*" );
		act( AT_PURPLE, "$N has armored plating and cruel spikes protecting $S giant, "
                                "muscle-bound form.&P", ch, NULL, victim, TO_CHAR );
		pager_printf_color(ch, "  &P*&P%s is covered in dark, icy purple flames.&P\n\r", name);
		}
		else
		{
		  pager_printf_color(ch, "  &P*&p%s > All.&P\n\r", name);
		}
	}
	if ( IS_AFFECTED(victim, AFF_HYPER) )
	{
		pager_printf_color(ch, "  &P*");
		if( !is_leet(ch) )
		  act( z, "$N is covered in bright energy flames.", ch, NULL, victim, TO_CHAR );
		else
		  act( z, "$N iz hypur...ON SUGAR. lolz $N is doing kartwheles. wut a spaz.", ch, NULL, victim, TO_CHAR );
	}
	if (IS_AFFECTED(victim, AFF_EXTREME))
	{
		/*
		pager_printf_color(ch, "  &P*&W%s has a corona of white lightning.&P\n", name);
                pager_printf_color(ch, "  &P*&W%s is bathed in an aura of enlightenment.&P\n\r", name);
		*/
		if( !is_leet(ch) )
		{
		pager_printf_color(ch, "  &P*");
		act( z, "$N has a corona of energy lightning.", ch, NULL, victim, TO_CHAR );
		pager_printf_color(ch, "  &P*");
		act( z, "$N is bathed in an aura of enlightenment.", ch, NULL, victim, TO_CHAR );
		}
		else
		{
		  pager_printf_color(ch, "  &P*");
                  act( z, "$N is X-TREME. dewd he is treme + X. dat's lik...mor than treme. lolz. ffs. math sux.", ch, NULL, victim, TO_CHAR );
		}
	}
	if ( IS_AFFECTED(victim, AFF_SEMIPERFECT) )
	{
		pager_printf_color(ch, "  &P*", name);
		act( AT_PURPLE, "$N's aura pulses with black and purple energy.&P", ch, NULL, victim, TO_CHAR );
	}
	if ( IS_AFFECTED(victim, AFF_PERFECT) )
	{
		pager_printf_color(ch, "  &P*", name);
		act( AT_PURPLE, "$N is surrounded by a chilling, crackling purple and black aura.&P", ch, NULL, victim, TO_CHAR );
                if( !IS_NPC( victim ) && victim->race == race_lookup( "bio-android" ) )
                {
                  if( victim->pcdata->absorb_pl_mod == race_lookup( "saiyan" )
                   || victim->pcdata->absorb_pl_mod == race_lookup( "halfbreed" ) )
                  {
		    pager_printf_color(ch, "  &P*");
		    act( AT_YELLOW, "$N is surrounded by a fiery yellow aura.", ch, NULL, victim, TO_CHAR );
                  }
                  else
                  if( victim->pcdata->absorb_pl_mod == race_lookup( "namek" ) )
                  {
		    pager_printf_color(ch, "  &P*&G%s is covered in bright green flames.&P\n\r", name);
                  }
                  else
                  if( victim->pcdata->absorb_pl_mod == race_lookup( "human" ) )
                  {
		    pager_printf_color(ch, "  &P*");
		    act( z, "$N is covered in bright energy flames.", ch, NULL, victim, TO_CHAR );
                  }
                  else
                  if( victim->pcdata->absorb_pl_mod == race_lookup( "icer" ) )
                  {
		    pager_printf_color(ch, "  &P*&P%s is covered in bright purple flames.&P\n\r", name);
                  }
                }
	}
	if ( IS_AFFECTED(victim, AFF_ULTRAPERFECT) )
	{
		pager_printf_color(ch, "  &P*", name);
		act( AT_PURPLE, "$N is coated in a shifting purple aura of foreboding power.&P"
                              , ch, NULL, victim, TO_CHAR );
		pager_printf_color(ch, "  &P*", name);
		act( AT_DGREY, "$N has black energy crackling around $S form.&P"
                              , ch, NULL, victim, TO_CHAR );
                if( !IS_NPC( victim ) && victim->race == race_lookup( "bio-android" ) )
                {
                  if( victim->pcdata->absorb_pl_mod == race_lookup( "saiyan" )
                   || victim->pcdata->absorb_pl_mod == race_lookup( "halfbreed" ) )
                  {
		    pager_printf_color(ch, "  &P*");
		    act( AT_YELLOW, "$N has a huge golden aura.", ch, NULL, victim, TO_CHAR );
                  }
                  else
                  if( victim->pcdata->absorb_pl_mod == race_lookup( "namek" ) )
                  {
		    pager_printf_color(ch, "  &P*&G%s is covered in bright green flames.&P\n\r", name);
                  }
                  else
                  if( victim->pcdata->absorb_pl_mod == race_lookup( "human" ) )
                  {
		    pager_printf_color(ch, "  &P*&R%s is engulfed within a blaze of mystical flame.&P\n\r", name);
                  }
                  else
                  if( victim->pcdata->absorb_pl_mod == race_lookup( "icer" ) )
                  {
		    pager_printf_color(ch, "  &P*&P%s is covered in dark, icy purple flames.&P\n\r", name);
                  }
                }
	}

	if( IS_AFFECTED( victim, AFF_SUPER_TRANS ) )
	{
		pager_printf_color( ch, "  &P*%s is covered in rippling pink muscles.\n\r", name );
		pager_printf_color( ch, "  &P*&p%s is surrounded by an aura of violet energy.\n\r", name );
	}

	if( IS_AFFECTED( victim, AFF_EVIL_TRANS ) )
	{
		pager_printf_color( ch, "  &P*%s is a tall, lean, pink mockery of a humanoid.\n\r", name );
	}

	if( IS_AFFECTED( victim, AFF_EVILBOOST ) )
	{
	    pager_printf_color(ch,"  &P*");
            act( AT_RED, "$N's muscles are bulging with evil power.", ch, NULL, victim, TO_CHAR );
	}

	if( IS_AFFECTED( victim, AFF_EVILSURGE ) )
        {
            pager_printf_color(ch,"  &P*");
            act( AT_RED, "$N's large body is surging with evil power.", ch, NULL, victim, TO_CHAR );
        }
	if( IS_AFFECTED( victim, AFF_EVILOVERLOAD ) )
        {
            pager_printf_color(ch,"  &P*");
            act( AT_RED, "$N's gigantic body is bulging with incredibly evil power.", ch, NULL, victim, TO_CHAR );
        }
	if( IS_AFFECTED( victim, AFF_MAKEOSTAR ) )
        {
            pager_printf_color(ch,"  &P*");
            act( AT_BLOOD, "$N is filled with horribly evil power from the Makeo Star.", ch, NULL, victim, TO_CHAR );
        }
	if ( IS_AFFECTED(victim, AFF_SANCTUARY) )
	{
		set_char_color( AT_WHITE, ch );
		if ( IS_GOOD(victim) )
			ch_printf( ch, "  &P*&W%s glows with an aura of divine radiance.\n\r", name );
		else if ( IS_EVIL(victim) )
				ch_printf( ch, "  &P*&z%s shimmers beneath an aura of dark energy.\n\r", name );
		else
			ch_printf( ch, "  &P*&w%s is shrouded in flowing shadow and light.\n\r", name );
	}
	if ( IS_AFFECTED(victim, AFF_FIRESHIELD) )
	{
		set_char_color( AT_FIRE, ch );
		ch_printf( ch, "  &P*&R%s is engulfed within a blaze of mystical flame.\n\r", name );
	}
	if ( IS_AFFECTED(victim, AFF_SHOCKSHIELD) )
	{
		set_char_color( AT_BLUE, ch );
		ch_printf( ch, "  &P*&B%s is surrounded by cascading torrents of energy.\n\r", name );
	}
	if ( IS_AFFECTED(victim, AFF_ACIDMIST) )
	{
		set_char_color( AT_GREEN, ch );
		ch_printf( ch, "  &P*&G%s is visible through a cloud of churning mist.\n\r", name );
	}
	/*Scryn 8/13*/
	if ( IS_AFFECTED(victim, AFF_ICESHIELD) )
	{
		set_char_color( AT_LBLUE, ch );
		ch_printf( ch, "%  &P*&Cs is ensphered by shards of glistening ice.\n\r", name );
	}
	if ( IS_AFFECTED(victim, AFF_CHARM) )
	{
		set_char_color( AT_MAGIC, ch );
		ch_printf( ch, "%s wanders in a dazed, zombie-like state.\n\r", name );
	}
	if ( !IS_NPC(victim) && !victim->desc
	     && victim->switched && IS_AFFECTED(victim->switched, AFF_POSSESS) )
	{
		set_char_color( AT_MAGIC, ch );
		strcpy( buf, PERS( victim, ch ) );
		strcat( buf, " appears to be in a deep trance...\n\r" );
	}
	if( wearing_sentient_chip(victim) )
	{
	    pager_printf_color(ch,"  &P*");
            act( AT_GREY, "$N's skin has become a sleek, shiny, mirror-like metal.", ch, NULL, victim, TO_CHAR );
	}
	if ( IS_AFFECTED(victim, AFF_FLYING) )
        {
                pager_printf_color(ch, "  &P*&B%s is hovering here in mid air.&P\n\r", name);
        }
}

void show_char_to_char_0( CHAR_DATA *victim, CHAR_DATA *ch )
{
	char buf[MAX_STRING_LENGTH];
	char buf1[MAX_STRING_LENGTH];

	buf[0] = '\0';

	set_char_color( AT_PERSON, ch );
	if ( !IS_NPC(victim) && !victim->desc )
	{
		if ( !victim->switched )
			send_to_char_color( "[&g(Link Dead)&c] ", ch );
		else if ( !IS_AFFECTED(victim, AFF_POSSESS) )
			strcat( buf, "(&gSwitched&c) " );
	}
	if ( IS_NPC(victim) && IS_AFFECTED(victim, AFF_POSSESS) && IS_IMMORTAL(ch)
	     && victim->desc )
	{
		sprintf( buf1, "(%s)", victim->desc->original->name );
		strcat( buf, buf1 );
	}
	if ( !IS_NPC(victim) )
	{
		if (xIS_SET(victim->affected_by, AFF_TAG))
			strcat( buf, "[&WIT&c] ");
		if ( xIS_SET(victim->act, PLR_AFK) )
			strcat( buf, "[&rAFK&c] ");
	}
	if ( (!IS_NPC(victim) && xIS_SET(victim->act, PLR_WIZINVIS))
	     || (IS_NPC(victim) && xIS_SET(victim->act, ACT_MOBINVIS)) )
	{
		if (!IS_NPC(victim))
			sprintf( buf1, "(Invis %d) ", victim->pcdata->wizinvis );
		else
			sprintf( buf1, "(Mobinvis %d) ", victim->mobinvis);
		strcat( buf, buf1 );
	}

	if ( !IS_NPC( victim) )
	{
		if ( IS_IMMORTAL( victim ) && victim->level > 50 )
			send_to_char_color( "&c(&WAdmin&c) ", ch );
		if ( victim->pcdata->clan
		     && IS_SET( victim->pcdata->flags, PCFLAG_DEADLY )
		     && victim->pcdata->clan->badge
		     && ( victim->pcdata->clan->clan_type != CLAN_ORDER
		     && victim->pcdata->clan->clan_type != CLAN_GUILD ) )
			{
				ch_printf_color( ch, "%s ", victim->pcdata->clan->badge );
			}
		else if (xIS_SET(victim->act, PLR_OUTCAST))
			send_to_char_color( "&c(&zOutcast&c) ", ch );
		else if ( CAN_PKILL( victim ) && victim->level < 51 )
			send_to_char_color( "&c(&wRonin&c) ", ch );
	}

	set_char_color( AT_PERSON, ch );

	if ( IS_AFFECTED(victim, AFF_INVISIBLE) )
		strcat( buf, "(Invis) " );
	if ( IS_AFFECTED(victim, AFF_HIDE) )
		strcat( buf, "(Hide) " );
	if ( IS_AFFECTED(victim, AFF_PASS_DOOR) )
		strcat( buf, "(Translucent) ");
	if ( IS_AFFECTED(victim, AFF_FAERIE_FIRE) )
		strcat( buf, "(Pink Aura) " );
	if ( IS_EVIL(victim)
	     && ( IS_AFFECTED(ch, AFF_DETECT_EVIL)) )
		strcat( buf, "(Red Aura) " );


	if ( IS_AFFECTED(victim, AFF_BERSERK) )
		strcat( buf, "(Wild-eyed) " );
	if ( !IS_NPC(victim) && xIS_SET(victim->act, PLR_ATTACKER ) )
		strcat( buf, "(ATTACKER) " );
	if ( !IS_NPC(victim) && xIS_SET(victim->act, PLR_KILLER ) )
		strcat( buf, "(KILLER) " );
	if ( !IS_NPC(victim) && xIS_SET(victim->act, PLR_THIEF ) )
		strcat( buf, "(THIEF) " );
	if ( !IS_NPC(victim) && xIS_SET(victim->act, PLR_LITTERBUG ) )
		strcat( buf, "(LITTERBUG) " );
	if ( IS_NPC(victim) && IS_IMMORTAL(ch)
	     && xIS_SET(victim->act, ACT_PROTOTYPE) )
		strcat( buf, "(PROTO) " );
	if ( IS_NPC(victim) && ch->mount && ch->mount == victim
	     && ch->in_room == ch->mount->in_room )
		strcat( buf, "(Mount) " );
	if ( victim->desc && victim->desc->connected == CON_EDITING )
		strcat( buf, "(Writing) " );
	if ( victim->morph != NULL )
		strcat (buf, "(Morphed) ");
	if ( !IS_NPC(victim) && xIS_SET(victim->act, PLR_BOUNTY)
	     && !str_cmp( victim->name, ch->pcdata->hunting ) )
		strcat( buf, "(&RBOUNTY&C) ");

	if( victim->kairank > 0 )
	{
	  if( victim->kairank == 1 )
	  {
	    strcat( buf, "South Kaioshin " );
	  }
	  else if( victim->kairank == 2 )
          {
            strcat( buf, "East Kaioshin " );
          }
	  else if( victim->kairank == 3 )
          {
            strcat( buf, "West Kaioshin " );
          }
	  else if( victim->kairank == 4 )
          {
            strcat( buf, "North Kaioshin " );
          }
	  else if( victim->kairank == 5 )
          {
            strcat( buf, "Grand Kaioshin " );
          }
	  else if( victim->kairank == 6 )
          {
            strcat( buf, "Supreme Kaioshin " );
          }
	}
	if( victim->demonrank > 0 )
	{
	  if( victim->demonrank == 1 )
          {
            strcat( buf, "Greater Demon " );
          }
	  if( victim->demonrank == 2 )
          {
            strcat( buf, "Demon Warlord " );
          }
	  if( victim->demonrank == 3 )
          {
            strcat( buf, "Demon King " );
          }
	}

	set_char_color( AT_PERSON, ch );
	if ((victim->position == victim->defposition && victim->long_descr[0] != '\0')
	    || ( victim->morph && victim->morph->morph &&
	         victim->morph->morph->defpos == victim->position ) )
	{
		if ( victim->morph != NULL )
		{
			if ( !IS_IMMORTAL(ch) )
			{
				if ( victim->morph->morph != NULL)
					strcat ( buf, victim->morph->morph->long_desc );
				else
					strcat ( buf, victim->long_descr );
			}
			else
			{
				strcat (buf, PERS(victim, ch) );
				if (!IS_NPC(victim))
				{
					if (victim->pcdata->last_name)
						strcat (buf, victim->pcdata->last_name);
				}
				if (!ch->desc->psuppress_cmdspam)
				{
					if ( !IS_NPC(victim)
						&& !xIS_SET(ch->act, PLR_BRIEF))
						strcat( buf, victim->pcdata->title );
				}
				strcat( buf, "&D.\n\r" );
			}
		}
		else
			strcat (buf, victim->long_descr);
		send_to_char( buf, ch );
		show_visible_affects_to_char( victim, ch );
		return ;
	}
	else
	{
		if ( victim->morph != NULL && victim->morph->morph != NULL &&
		     !IS_IMMORTAL( ch ) )
			strcat( buf, MORPHPERS( victim, ch ) );
		else
			strcat( buf, PERS( victim, ch ) );
	}

	if (!ch || !ch->desc)
		return;

	if (!ch->desc->psuppress_cmdspam)
	{
		if ( !IS_NPC(victim)
			&& !xIS_SET(ch->act, PLR_BRIEF))
			strcat( buf, victim->pcdata->title );
	}
	strcat( buf, "&D");
	switch ( victim->position )
	{
		case POS_DEAD:
			strcat( buf, "&c is DEAD!!" );
			break;
		case POS_MORTAL:
			strcat( buf, "&c is mortally wounded." );
			break;
		case POS_INCAP:
			strcat( buf, "&c is incapacitated." );
			break;
		case POS_STUNNED:
			strcat( buf, "&c is lying here stunned." );
			break;
		case POS_SLEEPING:
			if (ch->position == POS_SITTING
			    || ch->position == POS_RESTING )
				strcat( buf, "&c is sleeping nearby." );
			else
				strcat( buf, "&c is deep in slumber here." );
			break;
		case POS_RESTING:
			if (ch->position == POS_RESTING)
				strcat ( buf, "&c is sprawled out alongside you." );
			else
				if (ch->position == POS_MOUNTED)
					strcat ( buf, "&c is sprawled out at the foot of your mount." );
				else
					strcat (buf, "&c is sprawled out here." );
			break;
		case POS_SITTING:
			if (ch->position == POS_SITTING)
				strcat( buf, "&c sits here with you." );
			else
				if (ch->position == POS_RESTING)
					strcat( buf, "&c sits nearby as you lie around." );
				else
					strcat( buf, "&c sits upright here." );
			break;
		case POS_STANDING:
			if ( IS_IMMORTAL(victim) )
				strcat( buf, "&c is here before you." );
			else
				if ( ( victim->in_room->sector_type == SECT_UNDERWATER )
				     && !IS_AFFECTED(victim, AFF_AQUA_BREATH) && !IS_NPC(victim)
				     && !is_icer(victim)
				     && !is_android(victim)
				     && !is_bio(victim))
					strcat( buf, "&c is drowning here." );
				else
					if ( victim->in_room->sector_type == SECT_UNDERWATER )
						strcat( buf, "&c is here in the water." );
					else
						if ( ( victim->in_room->sector_type == SECT_OCEANFLOOR )
						     && !IS_AFFECTED(victim, AFF_AQUA_BREATH) && !IS_NPC(victim) )
							strcat( buf, "&c is drowning here." );
						else
							if ( victim->in_room->sector_type == SECT_OCEANFLOOR )
								strcat( buf, "&c is standing here in the water." );
							else
								if ( IS_AFFECTED(victim, AFF_FLOATING)
								     || IS_AFFECTED(victim, AFF_FLYING) )
									strcat( buf, "&c is hovering here." );
								else
									strcat( buf, "&c is standing here." );
			break;
		case POS_SHOVE:
			strcat( buf, "&c is being shoved around." );
			break;
		case POS_DRAG:
			strcat( buf, "&c is being dragged around." );
			break;
		case POS_MOUNTED:
			strcat( buf, "&c is here, upon " );
			if ( !victim->mount )
				strcat( buf, "thin air???" );
			else
				if ( victim->mount == ch )
					strcat( buf, "your back." );
				else
					if ( victim->in_room == victim->mount->in_room )
					{
						strcat( buf, PERS( victim->mount, ch ) );
						strcat( buf, "." );
					}
					else
						strcat( buf, "&csomeone who left??" );
			break;
		case POS_FIGHTING:
		case POS_EVASIVE:
		case POS_DEFENSIVE:
		case POS_AGGRESSIVE:
		case POS_BERSERK:
			strcat( buf, "&c is here, fighting " );
			if ( !victim->fighting )
			{
				strcat( buf, "thin air???" );

				/* some bug somewhere.... kinda hackey fix -h */
				if (! victim->mount)
					victim->position = POS_STANDING;
				else
					victim->position = POS_MOUNTED;
			}
			else if ( who_fighting( victim ) == ch )
				strcat( buf, "YOU!" );
			else if ( victim->in_room == victim->fighting->who->in_room )
			{
				strcat( buf, PERS( victim->fighting->who, ch ) );
				strcat( buf, "." );
			}
			else
				strcat( buf, "someone who left??" );
			break;
	}

	strcat( buf, "\n\r" );
	buf[0] = UPPER(buf[0]);
	send_to_char( buf, ch );
	show_visible_affects_to_char( victim, ch );
	return ;
}



void show_char_to_char_1( CHAR_DATA *victim, CHAR_DATA *ch )
{
	OBJ_DATA *obj;
	int iWear;
	bool found;
	int descNo = -1;

	if ( can_see( victim, ch ) && !xIS_SET( ch->act, PLR_WIZINVIS ) )
	{
		MOBtrigger = TRUE;
		act( AT_ACTION, "$n looks at you.", ch, NULL, victim, TO_VICT );
		if ( victim != ch )
			act( AT_ACTION, "$n looks at $N.", ch, NULL, victim, TO_NOTVICT );
		else
			act( AT_ACTION, "$n looks at $mself.", ch, NULL, victim, TO_NOTVICT );
		MOBtrigger = FALSE;
	}

	if ( !IS_NPC( victim ) )
	{
		if ( victim->description[0] != '\0' )
			descNo = 0;

		if ( ( xIS_SET( (victim)->affected_by, AFF_SSJ )
		       || xIS_SET( (victim)->affected_by, AFF_HYPER )
		       || xIS_SET( (victim)->affected_by, AFF_ICER2 )
		       || xIS_SET( (victim)->affected_by, AFF_SNAMEK )
		       || xIS_SET( (victim)->affected_by, AFF_SEMIPERFECT ) )
		     && victim->pcdata->description1 != '\0' )
			descNo = 1;

		if ( ( xIS_SET( (victim)->affected_by, AFF_SSJ2 )
			   || xIS_SET( (victim)->affected_by, AFF_EXTREME)
		       || xIS_SET( (victim)->affected_by, AFF_ICER3 )
		       || xIS_SET( (victim)->affected_by, AFF_PERFECT ) )
		     && victim->pcdata->description2 != '\0' )
			descNo = 2;

		if ( ( xIS_SET( (victim)->affected_by, AFF_SSJ3 )
		       || xIS_SET( (victim)->affected_by, AFF_ICER4 )
		       || xIS_SET( (victim)->affected_by, AFF_ULTRAPERFECT ) )
		     && victim->pcdata->description3 != '\0' )
			descNo = 3;

		if ( ( xIS_SET( (victim)->affected_by, AFF_SSJ4 )
		       || xIS_SET( (victim)->affected_by, AFF_ICER5 ) )
		     && victim->pcdata->description4 != '\0' )
			descNo = 4;
	}
	else
		if ( IS_NPC( victim ) )
		{
			if ( victim->description[0] != '\0' )
				descNo = 0;
		}

	ch_printf(ch,"\n\r");
	if ( descNo > -1 )
	{
		if ( victim->morph != NULL && victim->morph->morph != NULL)
			send_to_char ( victim->morph->morph->description , ch );
		else
		{
			if ( descNo == 0 )
				send_to_char (victim->description, ch);
			else
				if ( descNo == 1 )
					send_to_char( victim->pcdata->description1, ch );
				else
					if ( descNo == 2 )
						send_to_char( victim->pcdata->description2, ch );
					else
						if ( descNo == 3 )
							send_to_char( victim->pcdata->description3, ch );
						else
							if ( descNo == 4 )
								send_to_char( victim->pcdata->description4, ch );
							else
								if ( descNo == 5 )
									send_to_char( victim->pcdata->description5, ch );
		}
	}
	else
	{
		if ( victim->morph != NULL && victim->morph->morph != NULL)
			send_to_char ( victim->morph->morph->description , ch );
		else if ( IS_NPC( victim ) )
			act( AT_PLAIN, "You see nothing special about $M.", ch, NULL, victim, TO_CHAR );
		else if ( ch != victim )
			act( AT_PLAIN, "$E isn't much to look at...", ch, NULL, victim, TO_CHAR );
		else
			act( AT_PLAIN, "You're not much to look at...", ch, NULL, NULL, TO_CHAR );
	}

	if (!IS_NPC(victim))
	{
		if (victim->race == 5)
			pager_printf_color(ch, "\n\r%s looks pretty %s, with a %s and %s colored skin.\n\r",
			                   victim->name, get_build(victim), get_complexion(victim), get_secondary_color(victim));
		else if (victim->race == 3 || ch->race == 6)
			pager_printf_color(ch, "\n\r%s looks pretty %s, with a %s colored skin.\n\r",
			                   victim->name, get_build(victim), get_complexion(victim));
		else
			pager_printf_color(ch, "\n\r%s looks pretty %s, with a %s colored complexion.\n\r",
			                   victim->name, get_build(victim), get_complexion(victim));
	}
	show_race_line( ch, victim );
	if (!IS_NPC(victim))
	{
		if (victim->pcdata->haircolor == 24)
			pager_printf_color(ch, "%s has no hair and %s eyes.\n\r",
			                   heshe(victim, TRUE), get_eyes(victim));
		else
			pager_printf_color(ch, "%s has %s colored hair and %s eyes.\n\r",
			                   heshe(victim, TRUE), get_haircolor(victim), get_eyes(victim));
	}
	if (!IS_NPC(victim))
	{
		if (victim->pcdata->tail > 0 &&
		   (is_saiyan(victim) || is_icer(victim) ||
		    is_bio(victim) || is_hb(victim)) )
			pager_printf_color(ch, "%s has a tail.\n\r\n\r", heshe(victim, TRUE));
		else
			pager_printf_color(ch, "\n\r");
	}

	show_condition( ch, victim );

	show_visible_affects_to_char(victim, ch);
	send_to_pager_color("&D&w", ch);

	found = FALSE;
	for ( iWear = 0; iWear < MAX_WEAR; iWear++ )
	{
		if ( ( obj = get_eq_char( victim, iWear ) ) != NULL
		     && can_see_obj( ch, obj ) )
		{
			if ( !found )
			{
				send_to_char( "\n\r", ch );
				if ( victim != ch )
					act( AT_PLAIN, "$N is using:", ch, NULL, victim, TO_CHAR );
				else
					act( AT_PLAIN, "You are using:", ch, NULL, NULL, TO_CHAR );
				found = TRUE;
			}
			if ( (!IS_NPC(victim)) && (victim->race > 0) && (victim->race < MAX_PC_RACE))
				send_to_char(race_table[victim->race]->where_name[iWear], ch);
			else
				send_to_char( where_name[iWear], ch );
			send_to_char( format_obj_to_char( obj, ch, TRUE ), ch );
			send_to_char( "&D\n\r", ch );
		}
	}

	/*
	 * Crash fix here by Thoric
	 */
	if ( IS_NPC(ch) || victim == ch )
		return ;

	if ( IS_IMMORTAL( ch ) )
	{
		if ( IS_NPC( victim ) )
		{
			ch_printf( ch, "\n\rMobile #%d '%s' ",
			           victim->pIndexData->vnum,
			           victim->name );
			ch_printf( ch, "\n\rYou peek at %s inventory:\n\r",
			           victim->sex == 1 ? "his" : victim->sex == 2 ? "her" : "its" );
			show_list_to_char( victim->first_carrying, ch, TRUE, TRUE );
		}
		else
		{
			ch_printf( ch, "\n\r%s ", victim->name );
			ch_printf( ch, "is a level %d %s %s.\n\r",
			           victim->level,
			           IS_NPC(victim) ? victim->race < MAX_NPC_RACE && victim->race >= 0 ?
		           npc_race[victim->race] : "unknown" : victim->race < MAX_PC_RACE &&
			           race_table[victim->race]->race_name &&
			           race_table[victim->race]->race_name[0] != '\0' ?
			           race_table[victim->race]->race_name : "unknown",
			           IS_NPC(victim) ? victim->class < MAX_NPC_CLASS && victim->class >= 0 ?
		           npc_class[victim->class] : "unknown" : victim->class < MAX_PC_CLASS &&
			           class_table[victim->class]->who_name &&
			           class_table[victim->class]->who_name[0] != '\0' ?
			           class_table[victim->class]->who_name : "unknown");
			if (get_trust(ch) >= 56)
			{
				ch_printf( ch, "\n\rYou peek at %s inventory:\n\r",
				           victim->sex == 1 ? "his" : victim->sex == 2 ? "her" : "its" );
				show_list_to_char( victim->first_carrying, ch, TRUE, TRUE );
			}
		}

		/* Fix so it shows what is in class table
			victim->race<MAX_NPC_RACE&&victim->race>=0?npc_race[victim->race] : "unknown",
			victim->class<MAX_NPC_CLASS&&victim->class>=0?npc_class[victim->class] : "unknown" );
		*/
	}

	/*
	    if ( number_percent( ) < LEARNED(ch, gsn_peek) )
	    {
		ch_printf( ch, "\n\rYou peek at %s inventory:\n\r",
		  victim->sex == 1 ? "his" : victim->sex == 2 ? "her" : "its" );
		show_list_to_char( victim->first_carrying, ch, TRUE, TRUE );
		learn_from_success( ch, gsn_peek );
	    }
	    else
	      if ( ch->pcdata->learned[gsn_peek] > 0 )
	        learn_from_failure( ch, gsn_peek );
	*/

	return ;
}


void show_char_to_char( CHAR_DATA *list, CHAR_DATA *ch )
{
	CHAR_DATA *rch;

	for ( rch = list; rch; rch = rch->next_in_room )
	{
		if ( rch == ch )
			continue;

		if ( can_see( ch, rch ) )
		{
			show_char_to_char_0( rch, ch );
		}
		else if ( room_is_dark( ch->in_room )
		          && IS_AFFECTED(rch, AFF_INFRARED )
		          && !(!IS_NPC(rch) && IS_IMMORTAL(rch)) )
		{
			set_char_color( AT_BLOOD, ch );
			send_to_char( "The red form of a living creature is here.\n\r", ch );
		}
	}

	return ;
}



bool check_blind( CHAR_DATA *ch )
{
	if ( !IS_NPC(ch) && xIS_SET(ch->act, PLR_HOLYLIGHT) )
		return TRUE;

	if ( IS_AFFECTED(ch, AFF_TRUESIGHT) )
		return TRUE;

	if ( IS_AFFECTED(ch, AFF_BLIND) )
	{
		send_to_char( "You can't see a thing!\n\r", ch );
		return FALSE;
	}

	return TRUE;
}

/*
 * Returns classical DIKU door direction based on text in arg	-Thoric
 */
int get_door( char *arg )
{
	int door;

	if ( !str_cmp( arg, "n" ) || !str_cmp( arg, "north"	) )
		door = 0;
	else if ( !str_cmp( arg, "e" ) || !str_cmp( arg, "east"	) )
		door = 1;
	else if ( !str_cmp( arg, "s" ) || !str_cmp( arg, "south"	) )
		door = 2;
	else if ( !str_cmp( arg, "w" ) || !str_cmp( arg, "west"	) )
		door = 3;
	else if ( !str_cmp( arg, "u" ) || !str_cmp( arg, "up"	) )
		door = 4;
	else if ( !str_cmp( arg, "d" ) || !str_cmp( arg, "down"	) )
		door = 5;
	else if ( !str_cmp( arg, "ne" ) || !str_cmp( arg, "northeast" ) )
		door = 6;
	else if ( !str_cmp( arg, "nw" ) || !str_cmp( arg, "northwest" ) )
		door = 7;
	else if ( !str_cmp( arg, "se" ) || !str_cmp( arg, "southeast" ) )
		door = 8;
	else if ( !str_cmp( arg, "sw" ) || !str_cmp( arg, "southwest" ) )
		door = 9;
	else
		door = -1;
	return door;
}

void do_look( CHAR_DATA *ch, char *argument )
{
	char arg [MAX_INPUT_LENGTH];
	char arg1 [MAX_INPUT_LENGTH];
	char arg2 [MAX_INPUT_LENGTH];
	char arg3 [MAX_INPUT_LENGTH];
	char *kwd; /* cronel hiscore */
	EXIT_DATA *pexit;
	CHAR_DATA *victim;
	OBJ_DATA *obj;
	ROOM_INDEX_DATA *original;
	char *pdesc;
	bool doexaprog;
	sh_int door;
	int number, cnt;

	if ( !ch->desc )
		return ;

	if ( ch->position < POS_SLEEPING )
	{
		send_to_char( "You can't see anything but stars!\n\r", ch );
		return ;
	}

	if ( ch->position == POS_SLEEPING )
	{
		send_to_char( "You can't see anything, you're sleeping!\n\r", ch );
		return ;
	}

	if ( !check_blind( ch ) )
		return ;

	if ( !IS_NPC(ch)
	     && !xIS_SET(ch->act, PLR_HOLYLIGHT)
	     && !IS_AFFECTED(ch, AFF_TRUESIGHT)
	     && room_is_dark( ch->in_room )
	     && !is_android(ch) && !is_superandroid(ch)
	     && !is_demon(ch) )
	{
		set_char_color( AT_DGREY, ch );
		send_to_char( "It is pitch black ... \n\r", ch );
		show_char_to_char( ch->in_room->first_person, ch );
		return ;
	}

	argument = one_argument( argument, arg1 );
	argument = one_argument( argument, arg2 );
	argument = one_argument( argument, arg3 );

	doexaprog = str_cmp( "noprog", arg2 ) && str_cmp( "noprog", arg3 );

	if ( arg1[0] == '\0' || !str_cmp( arg1, "auto" ) )
	{

		SHIP_DATA * ship;

		sysdata.outBytesFlag = LOGBOUTMOVEMENT;

		switch ( ch->inter_page )    /* rmenu */
		{
			case ROOM_PAGE_A :
				do_rmenu(ch, "a");
				break;
			case ROOM_PAGE_B :
				do_rmenu(ch, "b");
				break;
			case ROOM_PAGE_C :
				do_rmenu(ch, "c");
				break;
		}
		/* 'look' or 'look auto' */
		set_char_color( AT_RMNAME, ch );
		send_to_char( "\n\r", ch );
		send_to_char( ch->in_room->name, ch );
		send_to_char( "\n\r", ch );
		set_char_color( AT_RMDESC, ch );

		if (!ch->desc->psuppress_cmdspam)
		{
			if ( arg1[0] == '\0'
				|| ( !IS_NPC(ch) && !xIS_SET(ch->act, PLR_BRIEF)))
				send_to_char( ch->in_room->description, ch );
		}

		if ( !IS_NPC(ch) && xIS_SET(ch->act, PLR_AUTOMAP) )   /* maps */
		{
			if (ch->in_room->map != NULL)
			{
				do_lookmap(ch, NULL);
			}
		}

		if ( !IS_NPC(ch) && xIS_SET(ch->act, PLR_AUTOEXIT) )
			do_exits( ch, "auto" );

		if ((has_scouter(ch) || is_android(ch))
		    && xIS_SET(ch->act, PLR_AUTO_COMPASS))
			show_compass_dir(ch);

		show_ships_to_char( ch->in_room->first_ship, ch );
		show_list_to_char( ch->in_room->first_content, ch, FALSE, FALSE );
		show_char_to_char( ch->in_room->first_person, ch );

		if ( str_cmp( arg1, "auto" ) )
			if ( (ship = ship_from_cockpit(ch->in_room->vnum)) != NULL )
			{
				set_char_color( AT_WHITE, ch );
				pager_printf_color( ch , "\n\rThrough the transparisteel windows you see:\n\r" );

				if (ship->starsystem)
				{
					MISSILE_DATA *missile;
					SHIP_DATA *target;

					set_char_color( AT_GREEN, ch );
					if ( ship->starsystem->star1 && str_cmp(ship->starsystem->star1, "") )
						pager_printf_color(ch, "&Y%s\n\r" ,
						                   ship->starsystem->star1);
					if ( ship->starsystem->star2 && str_cmp(ship->starsystem->star2, "") )
						pager_printf_color(ch, "&Y%s\n\r" ,
						                   ship->starsystem->star2 );
					if ( ship->starsystem->planet1 && str_cmp(ship->starsystem->planet1, "") )
						pager_printf_color(ch, "&G%s\n\r" ,
						                   ship->starsystem->planet1 );
					if ( ship->starsystem->planet2 && str_cmp(ship->starsystem->planet2, "") )
						pager_printf_color(ch, "&G%s\n\r" ,
						                   ship->starsystem->planet2 );
					if ( ship->starsystem->planet3 && str_cmp(ship->starsystem->planet3, "") )
						pager_printf_color(ch, "&G%s\n\r" ,
						                   ship->starsystem->planet3 );
					for ( target = ship->starsystem->first_ship; target; target = target->next_in_starsystem )
					{
						if ( target != ship )
							pager_printf_color(ch, "&C%s\n\r",
							                   target->name);
					}
					for ( missile = ship->starsystem->first_missile; missile; missile = missile->next_in_starsystem )
					{
						pager_printf_color(ch, "&R%s\n\r",
						                   missile->missiletype == CONCUSSION_MISSILE ? "A Concusion Missile" :
						                   ( missile->missiletype == PROTON_TORPEDO ? "A Torpedo" :
						                     ( missile->missiletype == HEAVY_ROCKET ? "A Heavy Rocket" : "A Heavy Bomb" ) ) );
					}
				pager_printf_color(ch, "&D");
				}
				else if ( ship->location == ship->lastdoc )
				{
					ROOM_INDEX_DATA *to_room;

					if ( (to_room = get_room_index( ship->location ) ) != NULL )
					{
						pager_printf_color( ch, "\n\r" );
						original = ch->in_room;
						char_from_room( ch );
						char_to_room( ch, to_room );
						do_glance( ch, "" );
						char_from_room( ch );
						char_to_room( ch, original );
					}
				}


			}
		sysdata.outBytesFlag = LOGBOUTNORM;
		return ;
	}

	if ( !str_cmp( arg1, "compass" ) )
	{
		show_compass_dir(ch);
		return ;
	}
	if ( !str_cmp( arg1, "under" ) )
	{
		int count;

		/* 'look under' */
		if ( arg2[0] == '\0' )
		{
			send_to_char( "Look beneath what?\n\r", ch );
			return ;
		}

		if ( ( obj = get_obj_here( ch, arg2 ) ) == NULL )
		{
			send_to_char( "You do not see that here.\n\r", ch );
			return ;
		}
		if ( !CAN_WEAR( obj, ITEM_TAKE ) && ch->level < sysdata.level_getobjnotake )
		{
			send_to_char( "You can't seem to get a grip on it.\n\r", ch );
			return ;
		}
		if ( ch->carry_weight + obj->weight > can_carry_w( ch ) )
		{
			send_to_char( "It's too heavy for you to look under.\n\r", ch );
			return ;
		}
		count = obj->count;
		obj->count = 1;
		act( AT_PLAIN, "You lift $p and look beneath it:", ch, obj, NULL, TO_CHAR );
		act( AT_PLAIN, "$n lifts $p and looks beneath it:", ch, obj, NULL, TO_ROOM );
		obj->count = count;
		if ( IS_OBJ_STAT( obj, ITEM_COVERING ) )
			show_list_to_char( obj->first_content, ch, TRUE, TRUE );
		else
			send_to_char( "Nothing.\n\r", ch );
		if ( doexaprog )
			oprog_examine_trigger( ch, obj );
		return ;
	}

	if ( !str_cmp( arg1, "i" ) || !str_cmp( arg1, "in" ) )
	{
		int count;

		/* 'look in' */
		if ( arg2[0] == '\0' )
		{
			send_to_char( "Look in what?\n\r", ch );
			return ;
		}

		if ( ( obj = get_obj_here( ch, arg2 ) ) == NULL )
		{
			send_to_char( "You do not see that here.\n\r", ch );
			return ;
		}

		switch ( obj->item_type )
		{
			default:
				send_to_char( "That is not a container.\n\r", ch );
				break;

			case ITEM_DRINK_CON:
				if ( obj->value[1] <= 0 )
				{
					send_to_char( "It is empty.\n\r", ch );
					if ( doexaprog )
						oprog_examine_trigger( ch, obj );
					break;
				}

				ch_printf( ch, "It's %s full of a %s liquid.\n\r",
				           obj->value[1] < obj->value[0] / 4
				           ? "less than" :
				           obj->value[1] < 3 * obj->value[0] / 4
				           ? "about" : "more than",
				           liq_table[obj->value[2]].liq_color
				         );

				if ( doexaprog )
					oprog_examine_trigger( ch, obj );
				break;

			case ITEM_PORTAL:
				for ( pexit = ch->in_room->first_exit; pexit; pexit = pexit->next )
				{
					if ( pexit->vdir == DIR_PORTAL
					     && IS_SET(pexit->exit_info, EX_PORTAL) )
					{
						if ( room_is_private( pexit->to_room )
						     && get_trust(ch) < sysdata.level_override_private )
						{
							set_char_color( AT_WHITE, ch );
							send_to_char( "That room is private buster!\n\r", ch );
							return ;
						}
						original = ch->in_room;
						char_from_room( ch );
						char_to_room( ch, pexit->to_room );
						do_look( ch, "auto" );
						char_from_room( ch );
						char_to_room( ch, original );
						return ;
					}
				}
				send_to_char( "You see swirling chaos...\n\r", ch );
				break;
			case ITEM_CONTAINER:
			case ITEM_QUIVER:
			case ITEM_CORPSE_NPC:
			case ITEM_CORPSE_PC:
				if ( IS_SET(obj->value[1], CONT_CLOSED) )
				{
					send_to_char( "It is closed.\n\r", ch );
					break;
				}

			case ITEM_KEYRING:
				count = obj->count;
				obj->count = 1;
				if ( obj->item_type == ITEM_CONTAINER )
					act( AT_PLAIN, "$p contains:", ch, obj, NULL, TO_CHAR );
				else
					act( AT_PLAIN, "$p holds:", ch, obj, NULL, TO_CHAR );
				obj->count = count;
				show_list_to_char( obj->first_content, ch, TRUE, TRUE );
				if ( doexaprog )
					oprog_examine_trigger( ch, obj );
				break;
		}
		return ;
	}

	if ( (pdesc = get_extra_descr(arg1, ch->in_room->first_extradesc)) != NULL )
	{
		send_to_char_color( pdesc, ch );
		return ;
	}

	door = get_door(arg1);
	if ( (pexit = find_door(ch, arg1, TRUE)) != NULL )
	{
		if ( IS_SET(pexit->exit_info, EX_CLOSED)
		     && !IS_SET(pexit->exit_info, EX_WINDOW) )
		{
			if ( (IS_SET(pexit->exit_info, EX_SECRET)
			      || IS_SET(pexit->exit_info, EX_DIG)) && door != -1 )
				send_to_char( "Nothing special there.\n\r", ch );
			else
				act( AT_PLAIN, "The $d is closed.", ch, NULL, pexit->keyword, TO_CHAR );
			return ;
		}
		if ( IS_SET( pexit->exit_info, EX_BASHED ) )
			act(AT_RED, "The $d has been bashed from its hinges!", ch, NULL, pexit->keyword, TO_CHAR);

		if ( pexit->description && pexit->description[0] != '\0' )
			send_to_char( pexit->description, ch );
		else
			send_to_char( "Nothing special there.\n\r", ch );

		/*
		 * Ability to look into the next room			-Thoric
		 */
		if ( pexit->to_room
		     && ( IS_AFFECTED( ch, AFF_SCRYING )
		          || IS_SET( pexit->exit_info, EX_xLOOK )
		          || get_trust(ch) >= LEVEL_IMMORTAL ) )
		{
			if ( !IS_SET( pexit->exit_info, EX_xLOOK )
			     && get_trust( ch ) < LEVEL_IMMORTAL )
			{
				set_char_color( AT_MAGIC, ch );
				send_to_char( "You attempt to scry...\n\r", ch );
				/*
				 * Change by Narn, Sept 96 to allow characters who don't have the
				 * scry spell to benefit from objects that are affected by scry.
				 */
				if (!IS_NPC(ch) )
				{
					int percent = LEARNED(ch, skill_lookup("scry") );
					if ( !percent )
					{
						percent = 55;	/* 95 was too good -Thoric */
					}

					if ( number_percent( ) > percent )
					{
						send_to_char( "You fail.\n\r", ch );
						return ;
					}
				}
			}
			if ( room_is_private( pexit->to_room )
			     && get_trust(ch) < sysdata.level_override_private )
			{
				set_char_color( AT_WHITE, ch );
				send_to_char( "That room is private buster!\n\r", ch );
				return ;
			}
			original = ch->in_room;
			char_from_room( ch );
			char_to_room( ch, pexit->to_room );
			do_look( ch, "auto" );
			char_from_room( ch );
			char_to_room( ch, original );
		}
		return ;
	}
	else
		if ( door != -1 )
		{
			send_to_char( "Nothing special there.\n\r", ch );
			return ;
		}

	if ( (victim = get_char_room(ch, arg1)) != NULL )
	{
		show_char_to_char_1( victim, ch );
		return ;
	}


	/* finally fixed the annoying look 2.obj desc bug	-Thoric */
	number = number_argument( arg1, arg );
	for ( cnt = 0, obj = ch->last_carrying; obj; obj = obj->prev_content )
	{

		/* cronel hiscore */
		kwd = is_hiscore_obj( obj );
		if ( kwd )
		{
			show_hiscore( kwd, ch );
			return ;
		}

		if ( can_see_obj( ch, obj ) )
		{
			if ( (pdesc = get_extra_descr(arg, obj->first_extradesc)) != NULL )
			{
				if ( (cnt += obj->count) < number )
					continue;
				send_to_char_color( pdesc, ch );
				if ( doexaprog )
					oprog_examine_trigger( ch, obj );
				return ;
			}

			if ( (pdesc = get_extra_descr(arg, obj->pIndexData->first_extradesc)) != NULL )
			{
				if ( (cnt += obj->count) < number )
					continue;
				send_to_char_color( pdesc, ch );
				if ( doexaprog )
					oprog_examine_trigger( ch, obj );
				return ;
			}
			if ( nifty_is_name_prefix( arg, obj->name ) )
			{
				if ( (cnt += obj->count) < number )
					continue;
				pdesc = get_extra_descr( obj->name, obj->pIndexData->first_extradesc );
				if ( !pdesc )
					pdesc = get_extra_descr( obj->name, obj->first_extradesc );
				if ( !pdesc )
					send_to_char_color( "You see nothing special.\r\n", ch );
				else
					send_to_char_color( pdesc, ch );
				if ( doexaprog )
					oprog_examine_trigger( ch, obj );
				return ;
			}
		}
	}

	for ( obj = ch->in_room->last_content; obj; obj = obj->prev_content )
	{

		/* cronel hiscore */
		kwd = is_hiscore_obj( obj );
		if ( kwd )
		{
			show_hiscore( kwd, ch );
			return ;
		}

		if ( can_see_obj( ch, obj ) )
		{
			if ( (pdesc = get_extra_descr(arg, obj->first_extradesc)) != NULL )
			{
				if ( (cnt += obj->count) < number )
					continue;
				send_to_char_color( pdesc, ch );
				if ( doexaprog )
					oprog_examine_trigger( ch, obj );
				return ;
			}

			if ( (pdesc = get_extra_descr(arg, obj->pIndexData->first_extradesc)) != NULL )
			{
				if ( (cnt += obj->count) < number )
					continue;
				send_to_char_color( pdesc, ch );
				if ( doexaprog )
					oprog_examine_trigger( ch, obj );
				return ;
			}
			if ( nifty_is_name_prefix( arg, obj->name ) )
			{
				if ( (cnt += obj->count) < number )
					continue;
				pdesc = get_extra_descr( obj->name, obj->pIndexData->first_extradesc );
				if ( !pdesc )
					pdesc = get_extra_descr( obj->name, obj->first_extradesc );
				if ( !pdesc )
					send_to_char( "You see nothing special.\r\n", ch );
				else
					send_to_char_color( pdesc, ch );
				if ( doexaprog )
					oprog_examine_trigger( ch, obj );
				return ;
			}
		}
	}

	send_to_char( "You do not see that here.\n\r", ch );
	return ;
}


void show_race_line( CHAR_DATA *ch, CHAR_DATA *victim )
{
	char buf[MAX_STRING_LENGTH];
	int feet, inches;


	if ( !IS_NPC(victim) && (victim != ch) )
	{
		feet = victim->height / 12;
		inches = victim->height % 12;
		sprintf( buf, "%s is %d'%d\" and weighs %d pounds.\n\r", PERS(victim, ch), feet, inches, victim->weight );
		send_to_char( buf, ch);
		return ;
	}
	if ( !IS_NPC(victim) && (victim == ch) )
	{
		feet = victim->height / 12;
		inches = victim->height % 12;
		sprintf( buf, "You are %d'%d\" and weigh %d pounds.\n\r", feet, inches, victim->weight );
		send_to_char( buf, ch);
		return ;
	}

}


void show_condition( CHAR_DATA *ch, CHAR_DATA *victim )
{
	char buf[MAX_STRING_LENGTH];
	int percent;

	if ( victim->max_hit > 0 )
		percent = ( 100 * victim->hit ) / victim->max_hit;
	else
		percent = -1;


	if (is_android(victim))
	{
		if ( victim != ch )
		{
			strcpy( buf, PERS(victim, ch) );
			if ( percent >= 100 )
				strcat( buf, " is in perfect health.\n\r" );
			else if ( percent >= 90 )
				strcat( buf, " is slightly scratched.\n\r" );
			else if ( percent >= 80 )
				strcat( buf, " has a few dents.\n\r" );
			else if ( percent >= 70 )
				strcat( buf, " has some cuts.\n\r" );
			else if ( percent >= 60 )
				strcat( buf, " has several wounds.\n\r" );
			else if ( percent >= 50 )
				strcat( buf, " has many nasty wounds.\n\r" );
			else if ( percent >= 40 )
				strcat( buf, " is leaking oil freely.\n\r" );
			else if ( percent >= 30 )
				strcat( buf, " is covered in oil.\n\r" );
			else if ( percent >= 20 )
				strcat( buf, " has sparks flying every where.\n\r" );
			else if ( percent >= 10 )
				strcat( buf, " is almost dead.\n\r" );
			else
				strcat( buf, " is DYING.\n\r" );
		}
		else
		{
			strcpy( buf, "You" );
			if ( percent >= 100 )
				strcat( buf, " are in perfect health.\n\r" );
			else if ( percent >= 90 )
				strcat( buf, " are slightly scratched.\n\r");
			else if ( percent >= 80 )
				strcat( buf, " have a few dents.\n\r" );
			else if ( percent >= 70 )
				strcat( buf, " have some cuts.\n\r" );
			else if ( percent >= 60 )
				strcat( buf, " have several wounds.\n\r" );
			else if ( percent >= 50 )
				strcat( buf, " have many nasty wounds.\n\r");
			else if ( percent >= 40 )
				strcat( buf, " are leaking oil freely.\n\r" );
			else if ( percent >= 30 )
				strcat( buf, " are covered in oil.\n\r" );
			else if ( percent >= 20 )
				strcat( buf, " have sparks flying every where.\n\r" );
			else if ( percent >= 10 )
				strcat( buf, " are almost dead.\n\r" );
			else
				strcat( buf, " are DYING.\n\r" );
		}
	}
	else
	{
		if ( victim != ch )
		{
			strcpy( buf, PERS(victim, ch) );
			if ( percent >= 100 )
				strcat( buf, " is in perfect health.\n\r" );
			else if ( percent >= 90 )
				strcat( buf, " is slightly scratched.\n\r" );
			else if ( percent >= 80 )
				strcat( buf, " has a few bruises.\n\r" );
			else if ( percent >= 70 )
				strcat( buf, " has some cuts.\n\r" );
			else if ( percent >= 60 )
				strcat( buf, " has several wounds.\n\r" );
			else if ( percent >= 50 )
				strcat( buf, " has many nasty wounds.\n\r" );
			else if ( percent >= 40 )
				strcat( buf, " is bleeding freely.\n\r" );
			else if ( percent >= 30 )
				strcat( buf, " is covered in blood.\n\r" );
			else if ( percent >= 20 )
				strcat( buf, " is leaking guts.\n\r" );
			else if ( percent >= 10 )
				strcat( buf, " is almost dead.\n\r" );
			else
				strcat( buf, " is DYING.\n\r" );
		}
		else
		{
			strcpy( buf, "You" );
			if ( percent >= 100 )
				strcat( buf, " are in perfect health.\n\r" );
			else if ( percent >= 90 )
				strcat( buf, " are slightly scratched.\n\r");
			else if ( percent >= 80 )
				strcat( buf, " have a few bruises.\n\r" );
			else if ( percent >= 70 )
				strcat( buf, " have some cuts.\n\r" );
			else if ( percent >= 60 )
				strcat( buf, " have several wounds.\n\r" );
			else if ( percent >= 50 )
				strcat( buf, " have many nasty wounds.\n\r");
			else if ( percent >= 40 )
				strcat( buf, " are bleeding freely.\n\r" );
			else if ( percent >= 30 )
				strcat( buf, " are covered in blood.\n\r" );
			else if ( percent >= 20 )
				strcat( buf, " are leaking guts.\n\r" );
			else if ( percent >= 10 )
				strcat( buf, " are almost dead.\n\r" );
			else
				strcat( buf, " are DYING.\n\r" );
		}
	}

	buf[0] = UPPER(buf[0]);
	send_to_char( buf, ch );
	return ;
}

/* A much simpler version of look, this function will show you only
the condition of a mob or pc, or if used without an argument, the
same you would see if you enter the room and have config +brief.
-- Narn, winter '96
*/
void do_glance( CHAR_DATA *ch, char *argument )
{
	char arg1 [MAX_INPUT_LENGTH];
	CHAR_DATA *victim;
	bool brief;

	if ( !ch->desc )
		return ;

	if ( ch->position < POS_SLEEPING )
	{
		send_to_char( "You can't see anything but stars!\n\r", ch );
		return ;
	}

	if ( ch->position == POS_SLEEPING )
	{
		send_to_char( "You can't see anything, you're sleeping!\n\r", ch );
		return ;
	}

	if ( !check_blind( ch ) )
		return ;

	set_char_color( AT_ACTION, ch );
	argument = one_argument( argument, arg1 );

	if ( arg1[0] == '\0' )
	{
		if ( xIS_SET(ch->act, PLR_BRIEF) )
			brief = TRUE;
		else
			brief = FALSE;
		xSET_BIT( ch->act, PLR_BRIEF );
		do_look( ch, "auto" );
		if ( !brief )
			xREMOVE_BIT(ch->act, PLR_BRIEF);
		return ;
	}

	if ( ( victim = get_char_room( ch, arg1 ) ) == NULL )
	{
		send_to_char( "They're not here.\n\r", ch );
		return ;
	}
	else
	{
		if ( can_see( victim, ch ) )
		{
			act( AT_ACTION, "$n glances at you.", ch, NULL, victim, TO_VICT );
			act( AT_ACTION, "$n glances at $N.", ch, NULL, victim, TO_NOTVICT );
		}
		if ( IS_IMMORTAL( ch ) && victim != ch )
		{
			if ( IS_NPC( victim ) )
				ch_printf( ch, "Mobile #%d '%s' ",
				           victim->pIndexData->vnum, victim->name );
			else
				ch_printf( ch, "%s ", victim->name );
			ch_printf( ch, "is a level %d %s %s.\n\r",
			           victim->level,
			           IS_NPC(victim) ? victim->race < MAX_NPC_RACE && victim->race >= 0 ?
		           npc_race[victim->race] : "unknown" : victim->race < MAX_PC_RACE &&
			           race_table[victim->race]->race_name &&
			           race_table[victim->race]->race_name[0] != '\0' ?
			           race_table[victim->race]->race_name : "unknown",
			           IS_NPC(victim) ? victim->class < MAX_NPC_CLASS && victim->class >= 0 ?
		           npc_class[victim->class] : "unknown" : victim->class < MAX_PC_CLASS &&
			           class_table[victim->class]->who_name &&
			           class_table[victim->class]->who_name[0] != '\0' ?
			           class_table[victim->class]->who_name : "unknown");
			/* New Change
				    victim->race<MAX_NPC_RACE&&victim->race>=0?npc_race[victim->race] : "unknown",
				    victim->class<MAX_NPC_CLASS&&victim->class>=0?npc_class[victim->class] : "unknown" );
			*/
		}
		show_condition( ch, victim );

		return ;
	}

	return ;
}


void do_examine( CHAR_DATA *ch, char *argument )
{
	char buf[MAX_STRING_LENGTH];
	char arg[MAX_INPUT_LENGTH];
	OBJ_DATA *obj;
	BOARD_DATA *board;
	sh_int dam;

	if ( !argument )
	{
		bug( "do_examine: null argument.", 0);
		return ;
	}

	if ( !ch )
	{
		bug( "do_examine: null ch.", 0);
		return ;
	}

	one_argument( argument, arg );

	if ( arg[0] == '\0' )
	{
		send_to_char( "Examine what?\n\r", ch );
		return ;
	}

	sprintf( buf, "%s noprog", arg );
	do_look( ch, buf );

	/*
	 * Support for looking at boards, checking equipment conditions,
	 * and support for trigger positions by Thoric
	 */
	if ( (obj = get_obj_here(ch, arg)) != NULL )
	{
		if ( (board = get_board(obj)) != NULL )
		{
			if ( board->num_posts )
				ch_printf( ch, "There are about %d notes posted here.  Type 'note list' to list them.\n\r", board->num_posts );
			else
				send_to_char( "There aren't any notes posted here.\n\r", ch );
		}

		switch ( obj->item_type )
		{
			default:
				break;

			case ITEM_ARMOR:
				pager_printf_color( ch, "As you look more closely, you notice that it's armor rating is ");
				pager_printf_color(ch, "%d/%d", obj->value[4], obj->value[5]);
				pager_printf_color( ch, "\n\r" );
				break;

			case ITEM_WEAPON:
				dam = INIT_WEAPON_CONDITION - obj->value[0];
				strcpy( buf, "As you look more closely, you notice that it is ");
				if (dam == 0)
					strcat( buf, "in superb condition.");
				else if (dam == 1)
					strcat( buf, "in excellent condition.");
				else if (dam == 2)
					strcat( buf, "in very good condition.");
				else if (dam == 3)
					strcat( buf, "in good shape.");
				else if (dam == 4)
					strcat( buf, "showing a bit of wear.");
				else if (dam == 5)
					strcat( buf, "a little run down.");
				else if (dam == 6)
					strcat( buf, "in need of repair.");
				else if (dam == 7)
					strcat( buf, "in great need of repair.");
				else if (dam == 8)
					strcat( buf, "in dire need of repair.");
				else if (dam == 9)
					strcat( buf, "very badly worn.");
				else if (dam == 10)
					strcat( buf, "practically worthless.");
				else if (dam == 11)
					strcat( buf, "almost broken.");
				else if (dam == 12)
					strcat( buf, "broken.");
				strcat( buf, "\n\r" );
				send_to_char( buf, ch );
				break;

			case ITEM_COOK:
				strcpy( buf, "As you examine it carefully you notice that it " );
				dam = obj->value[2];
				if (dam >= 3)
					strcat( buf, "is burned to a crisp.");
				else if (dam == 2)
					strcat( buf, "is a little over cooked.");
				else if (dam == 1)
					strcat( buf, "is perfectly roasted.");
				else
					strcat( buf, "is raw.");
				strcat( buf, "\n\r" );
				send_to_char( buf, ch );
			case ITEM_FOOD:
				if ( obj->timer > 0 && obj->value[1] > 0 )
					dam = (obj->timer * 10) / obj->value[1];
				else
					dam = 10;
				if ( obj->item_type == ITEM_FOOD )
					strcpy( buf, "As you examine it carefully you notice that it " );
				else
					strcpy( buf, "Also it " );
				if (dam >= 10)
					strcat( buf, "is fresh.");
				else if (dam == 9)
					strcat( buf, "is nearly fresh.");
				else if (dam == 8)
					strcat( buf, "is perfectly fine.");
				else if (dam == 7)
					strcat( buf, "looks good.");
				else if (dam == 6)
					strcat( buf, "looks ok.");
				else if (dam == 5)
					strcat( buf, "is a little stale.");
				else if (dam == 4)
					strcat( buf, "is a bit stale.");
				else if (dam == 3)
					strcat( buf, "smells slightly off.");
				else if (dam == 2)
					strcat( buf, "smells quite rank.");
				else if (dam == 1)
					strcat( buf, "smells revolting!");
				else if (dam <= 0)
					strcat( buf, "is crawling with maggots!");
				strcat( buf, "\n\r" );
				send_to_char( buf, ch );
				break;


			case ITEM_SWITCH:
			case ITEM_LEVER:
			case ITEM_PULLCHAIN:
				if ( IS_SET( obj->value[0], TRIG_UP ) )
					send_to_char( "You notice that it is in the up position.\n\r", ch );
				else
					send_to_char( "You notice that it is in the down position.\n\r", ch );
				break;
			case ITEM_BUTTON:
				if ( IS_SET( obj->value[0], TRIG_UP ) )
					send_to_char( "You notice that it is depressed.\n\r", ch );
				else
					send_to_char( "You notice that it is not depressed.\n\r", ch );
				break;

				/* Not needed due to check in do_look already
					case ITEM_PORTAL:
					    sprintf( buf, "in %s noprog", arg );
					    do_look( ch, buf );
					    break;
				*/

			case ITEM_CORPSE_PC:
			case ITEM_CORPSE_NPC:
				{
					sh_int timerfrac = obj->timer;
					if ( obj->item_type == ITEM_CORPSE_PC )
						timerfrac = (int)obj->timer / 8 + 1;

					switch (timerfrac)
					{
						default:
							send_to_char( "This corpse has recently been slain.\n\r", ch );
							break;
						case 4:
							send_to_char( "This corpse was slain a little while ago.\n\r", ch );
							break;
						case 3:
							send_to_char( "A foul smell rises from the corpse, and it is covered in flies.\n\r", ch );
							break;
						case 2:
							send_to_char( "A writhing mass of maggots and decay, you can barely go near this corpse.\n\r", ch );
							break;
						case 1:
						case 0:
							send_to_char( "Little more than bones, there isn't much left of this corpse.\n\r", ch );
							break;
					}
				}
			case ITEM_CONTAINER:
				if ( IS_OBJ_STAT( obj, ITEM_COVERING ) )
					break;
			case ITEM_DRINK_CON:
			case ITEM_QUIVER:
				send_to_char( "When you look inside, you see:\n\r", ch );
			case ITEM_KEYRING:
				sprintf( buf, "in %s noprog", arg );
				do_look( ch, buf );
				break;
		}
		if ( IS_OBJ_STAT( obj, ITEM_COVERING ) )
		{
			sprintf( buf, "under %s noprog", arg );
			do_look( ch, buf );
		}
		oprog_examine_trigger( ch, obj );
		if ( char_died(ch) || obj_extracted(obj) )
			return ;

		check_for_trap( ch, obj, TRAP_EXAMINE );
	}
	return ;
}


void do_exits( CHAR_DATA *ch, char *argument )
{
	char buf[MAX_STRING_LENGTH];
	EXIT_DATA *pexit;
	bool found;
	bool fAuto;

	set_char_color( AT_EXITS, ch );
	buf[0] = '\0';
	fAuto = !str_cmp( argument, "auto" );

	if ( !check_blind(ch) )
		return ;

	strcpy( buf, fAuto ? "Exits:" : "Obvious exits:\n\r" );

	found = FALSE;
	for ( pexit = ch->in_room->first_exit; pexit; pexit = pexit->next )
	{
		if ( pexit->to_room
		     && !IS_SET(pexit->exit_info, EX_CLOSED)
		     && (!IS_SET(pexit->exit_info, EX_WINDOW)
		         || IS_SET(pexit->exit_info, EX_ISDOOR))
		     && !IS_SET(pexit->exit_info, EX_HIDDEN) )
		{
			found = TRUE;
			if ( fAuto )
			{
				strcat( buf, " " );
				strcat( buf, dir_name[pexit->vdir] );
			}
			else
			{
				sprintf( buf + strlen(buf), "%-5s - %s\n\r",
				         capitalize( dir_name[pexit->vdir] ),
				         room_is_dark( pexit->to_room )
				         ? "Too dark to tell"
				         : pexit->to_room->name
				       );
			}
		}
	}

	if ( !found )
		strcat( buf, fAuto ? " none.\n\r" : "None.\n\r" );
	else
		if ( fAuto )
			strcat( buf, ".\n\r" );
	send_to_char( buf, ch );
	return ;
}

char *	const	day_name	[] =
    {
        "the Moon", "the Bull", "Deception", "Thunder", "Freedom",
        "the Great Gods", "the Sun"
    };

char *	const	month_name	[] =
    {
        "Winter", "the Winter Wolf", "the Frost Giant", "the Old Forces",
        "the Grand Struggle", "the Spring", "Nature", "Futility", "the Dragon",
        "the Sun", "the Heat", "the Battle", "the Dark Shades", "the Shadows",
        "the Long Shadows", "the Ancient Darkness", "the Great Evil"
    };

void do_time( CHAR_DATA *ch, char *argument )
{
	extern char str_boot_time[];
	extern char reboot_time[];
	char *suf;
	char *moon = "\r";
	int day;

	day = time_info.day + 1;

	if ( day > 4 && day < 20 )
		suf = "th";
	else if ( day % 10 == 1 )
		suf = "st";
	else if ( day % 10 == 2 )
		suf = "nd";
	else if ( day % 10 == 3 )
		suf = "rd";
	else
		suf = "th";

	if ( time_info.hour > 6 && time_info.hour < 20 )
		moon = "&YThe sun is up.";
	else
	{
		if ( time_info.day == 13 )
			moon = "&BIt's a new moon tonight.";
		else if ( time_info.day == 29 )
			moon = "&WIt's a full moon tonight.";
		else if ( (time_info.day <= 9 && time_info.day >= 3)
		          || (time_info.day >= 17 && time_info.day <= 26) )
			moon = "&WA quarter moon is up.";
		else if ( time_info.day > 9 && time_info.day < 17 )
		{
		    if( time_info.day == 14 )
			moon = "&RThe Makeo Star shines down with evil, blood red light.";
		    else
			moon = "&WOnly a sliver of the moon is visable.";
		}
		else if ( time_info.day < 3 || time_info.day > 26 )
			moon = "&WThe moon is almost full.";
		else
			bug( "act_info.c: do_time - unknown moon phase.", 0 );
	}

	set_char_color( AT_YELLOW, ch );
	pager_printf_color( ch,
	                    "&YToday is the Day of %s, in the year &O%s &YA.D.\n\r"
	                    "&YIt is &O%d o'clock %s&Y, during the %d%s day in the Month of %s.\n\r"
	                    "%s&Y\n\r"
	                    " - The mud started up at:    &R%s&Y\r"
	                    " - The system time (M.S.T.): &R%s&Y\r"
	                    " - Next Reboot is set for:   &R%s&Y\r",
	                    day_name[day % 7], num_punct(time_info.year),
	                    (time_info.hour % 12 == 0) ? 12 : time_info.hour % 12,
	                    time_info.hour >= 12 ? "pm" : "am",
	                    day, suf, month_name[time_info.month],
	                    moon,
	                    str_boot_time,
	                    (char *) ctime( &current_time ),
	                    reboot_time
	                  );


	/*	"&YIt is &C%d o'clock %s&Y, Day of %s, %d%s the Month of %s.\n\r"
	        "%s&Y\n\r"
	        "The mud started up at:    &R%s&Y\r"
	        "The system time (M.S.T.): &R%s&Y\r"
	        "Next Reboot is set for:   &R%s&Y\r",

		(time_info.hour % 12 == 0) ? 12 : time_info.hour % 12,
		time_info.hour >= 12 ? "pm" : "am",
		day_name[day % 7],
		day, suf,
		month_name[time_info.month],
		moon,
		str_boot_time,
		(char *) ctime( &current_time ),
		reboot_time
		);
	*/

	return;
}

/*
void do_weather( CHAR_DATA *ch, char *argument )
{
    static char * const sky_look[4] =
    {
	"cloudless",
	"cloudy",
	"rainy",
	"lit by flashes of lightning"
    };

    if ( !IS_OUTSIDE(ch) )
    {
	send_to_char( "You can't see the sky from here.\n\r", ch );
	return;
    }

    set_char_color( AT_BLUE, ch );
    ch_printf( ch, "The sky is %s and %s.\n\r",
	sky_look[weather_info.sky],
	weather_info.change >= 0
	? "a warm southerly breeze blows"
	: "a cold northern gust blows"
	);
    return;
}
*/

/*
 * Produce a description of the weather based on area weather using
 * the following sentence format:
 *		<combo-phrase> and <single-phrase>.
 * Where the combo-phrase describes either the precipitation and
 * temperature or the wind and temperature. The single-phrase
 * describes either the wind or precipitation depending upon the
 * combo-phrase.
 * Last Modified: July 31, 1997
 * Fireblade - Under Construction
 */
void do_weather(CHAR_DATA *ch, char *argument)
{
	char *combo, *single;
	char buf[MAX_INPUT_LENGTH];
	int temp, precip, wind;

	if ( !IS_OUTSIDE(ch) )
	{
		ch_printf(ch, "You can't see the sky from here.\n\r");
		return ;
	}

	temp = (ch->in_room->area->weather->temp + 3 * weath_unit - 1) /
	       weath_unit;
	precip = (ch->in_room->area->weather->precip + 3 * weath_unit - 1) /
	         weath_unit;
	wind = (ch->in_room->area->weather->wind + 3 * weath_unit - 1) /
	       weath_unit;

	if ( precip >= 3 )
	{
		combo = preciptemp_msg[precip][temp];
		single = wind_msg[wind];
	}
	else
	{
		combo = windtemp_msg[wind][temp];
		single = precip_msg[precip];
	}

	sprintf(buf, "%s and %s.\n\r", combo, single);

	set_char_color(AT_BLUE, ch);

	ch_printf(ch, buf);
}

/*
 * Moved into a separate function so it can be used for other things
 * ie: online help editing				-Thoric
 */
HELP_DATA *get_help( CHAR_DATA *ch, char *argument )
{
	char argall[MAX_INPUT_LENGTH];
	char argone[MAX_INPUT_LENGTH];
	char argnew[MAX_INPUT_LENGTH];
	HELP_DATA *pHelp;
	int lev;

	if ( argument[0] == '\0' )
		argument = "summary";

	if ( isdigit(argument[0]) )
	{
		lev = number_argument( argument, argnew );
		argument = argnew;
	}
	else
		lev = -2;
	/*
	 * Tricky argument handling so 'help a b' doesn't match a.
	 */
	argall[0] = '\0';
	while ( argument[0] != '\0' )
	{
		argument = one_argument( argument, argone );
		if ( argall[0] != '\0' )
			strcat( argall, " " );
		strcat( argall, argone );
	}

	for ( pHelp = first_help; pHelp; pHelp = pHelp->next )
	{
		if ( pHelp->level > get_trust( ch ) )
			continue;
		if ( lev != -2 && pHelp->level != lev )
			continue;

		if ( is_name( argall, pHelp->keyword ) )
			return pHelp;
	}

	return NULL;
}

/*
 * LAWS command
 */
void do_laws( CHAR_DATA *ch, char *argument )
{
	char buf[1024];

	if ( argument == NULL)
		do_help( ch, "laws" );
	else
	{
		sprintf( buf, "law %s", argument );
		do_help( ch, buf );
	}
}

/*
 * Now this is cleaner
 */
void do_help( CHAR_DATA *ch, char *argument )
{
	HELP_DATA *pHelp;
	char nohelp[MAX_STRING_LENGTH];

	strcpy(nohelp, argument); /* For Finding "needed" helpfiles */

	if ( (pHelp = get_help( ch, argument )) == NULL )
	{
		send_to_char( "No help on that word.\n\r", ch );
		if (sysdata.ahelp)
			append_file( ch, HELP_FILE, nohelp );
		return ;
	}

	/*
	if( (!is_demon(ch) || (is_demon(ch) && ch->demonrank < 1)) &&
	    (!str_cmp(argument,"greaterdemon") ||
	    !str_cmp(argument,"greater demon"))
	    && !IS_IMMORTAL(ch) )
	{
	  ch_printf(ch,"No help on that word.\n\r");
	  return;
	}
	if( (!is_demon(ch) || (is_demon(ch) && ch->demonrank < 2)) &&
            (!str_cmp(argument,"demonwarlord") ||
            !str_cmp(argument,"demon warlord"))
	    && !IS_IMMORTAL(ch) )
        {
          ch_printf(ch,"No help on that word.\n\r");
          return;
        }
	if( (!is_demon(ch) || (is_demon(ch) && ch->demonrank < 3)) &&
            (!str_cmp(argument,"demonking") ||
            !str_cmp(argument,"demon king"))
	    && !IS_IMMORTAL(ch) )
        {
          ch_printf(ch,"No help on that word.\n\r");
          return;
        }
        */

	sysdata.outBytesFlag = LOGBOUTINFORMATION;
	/* Make newbies do a help start. --Shaddai */
	if ( !IS_NPC(ch) && !str_cmp( argument, "start" ) )
		SET_BIT(ch->pcdata->flags, PCFLAG_HELPSTART);

	if ( pHelp->level >= 0 && str_cmp( argument, "imotd" ) )
	{
		send_to_pager( "\n\r", ch );
		send_to_pager( pHelp->keyword, ch );
		send_to_pager( "\n\r", ch );
	}

	/*
	 * Strip leading '.' to allow initial blanks.
	 */
	if ( pHelp->text[0] == '.' )
		send_to_pager_color( pHelp->text + 1, ch );
	else
		send_to_pager_color( pHelp->text , ch );

	sysdata.outBytesFlag = LOGBOUTNORM;
	return ;
}

void do_news( CHAR_DATA *ch, char *argument )
{
	set_pager_color( AT_NOTE, ch );
	do_help( ch, "news" );
}

extern char * help_greeting;	/* so we can edit the greeting online */

/*
 * Help editor							-Thoric
 */
void do_hedit( CHAR_DATA *ch, char *argument )
{
	HELP_DATA *pHelp;

	if ( !ch->desc )
	{
		send_to_char( "You have no descriptor.\n\r", ch );
		return ;
	}

	switch ( ch->substate )
	{
		default:
			break;
		case SUB_HELP_EDIT:
			if ( (pHelp = ch->dest_buf) == NULL )
			{
				bug( "hedit: sub_help_edit: NULL ch->dest_buf", 0 );
				stop_editing( ch );
				return ;
			}
			if ( help_greeting == pHelp->text )
				help_greeting = NULL;
			STRFREE( pHelp->text );
			pHelp->text = copy_buffer( ch );
			if ( !help_greeting )
				help_greeting = pHelp->text;
			stop_editing( ch );
			return ;
	}
	if ( (pHelp = get_help(ch, argument)) == NULL )     /* new help */
	{
		HELP_DATA * tHelp;
		char argnew[MAX_INPUT_LENGTH];
		int lev;
		bool new_help = TRUE;

		for ( tHelp = first_help; tHelp; tHelp = tHelp->next )
			if ( !str_cmp( argument, tHelp->keyword) )
			{
				pHelp = tHelp;
				new_help = FALSE;
				break;
			}
		if ( new_help )
		{
			if ( isdigit(argument[0]) )
			{
				lev = number_argument( argument, argnew );
				argument = argnew;
			}
			else
				lev = get_trust(ch);
			CREATE( pHelp, HELP_DATA, 1 );
			pHelp->keyword = STRALLOC( strupper(argument) );
			pHelp->text = STRALLOC( "" );
			pHelp->level = lev;
			add_help( pHelp );
		}
	}

	ch->substate = SUB_HELP_EDIT;
	ch->dest_buf = pHelp;
	start_editing( ch, pHelp->text );
}

/*
 * Stupid leading space muncher fix				-Thoric
 */
char *help_fix( char *text )
{
	char *fixed ;

	if ( !text )
		return "";
	fixed = strip_cr(text)
		        ;
	if ( fixed [0] == ' ' )
			fixed [0]
				= '.';
	return fixed ;
}

void do_hset( CHAR_DATA *ch, char *argument )
{
	HELP_DATA *pHelp;
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];

	smash_tilde( argument );
	argument = one_argument( argument, arg1 );
	if ( arg1[0] == '\0' )
	{
		send_to_char( "Syntax: hset <field> [value] [help page]\n\r", ch );
		send_to_char( "\n\r", ch );
		send_to_char( "Field being one of:\n\r", ch );
		send_to_char( "  level keyword remove save\n\r", ch );
		return ;
	}

	if ( !str_cmp( arg1, "save" ) )
	{
		FILE *fpout;

		log_string_plus( "Saving help.are...", LOG_NORMAL, LEVEL_GREATER );

		rename( "help.are", "help.are.bak" );
		fclose( fpReserve );
		if ( ( fpout = fopen( "help.are", "w" ) ) == NULL )
		{
			bug( "hset save: fopen", 0 );
			perror( "help.are" );
			fpReserve = fopen( NULL_FILE, "r" );
			return ;
		}

		fprintf( fpout, "#HELPS\n\n" );
		for ( pHelp = first_help; pHelp; pHelp = pHelp->next )
			fprintf( fpout, "%d %s~\n%s~\n\n",
			         pHelp->level, pHelp->keyword, help_fix(pHelp->text) );

		fprintf( fpout, "0 $~\n\n\n#$\n" );
		fclose( fpout );
		fpReserve = fopen( NULL_FILE, "r" );
		send_to_char( "Saved.\n\r", ch );
		return ;
	}
	if ( str_cmp( arg1, "remove" ) )
		argument = one_argument( argument, arg2 );

	if ( (pHelp = get_help(ch, argument)) == NULL )
	{
		send_to_char( "Cannot find help on that subject.\n\r", ch );
		return ;
	}
	if ( !str_cmp( arg1, "remove" ) )
	{
		UNLINK( pHelp, first_help, last_help, next, prev );
		STRFREE( pHelp->text );
		STRFREE( pHelp->keyword );
		DISPOSE( pHelp );
		send_to_char( "Removed.\n\r", ch );
		return ;
	}
	if ( !str_cmp( arg1, "level" ) )
	{
		pHelp->level = atoi( arg2 );
		send_to_char( "Done.\n\r", ch );
		return ;
	}
	if ( !str_cmp( arg1, "keyword" ) )
	{
		STRFREE( pHelp->keyword );
		pHelp->keyword = STRALLOC( strupper(arg2) );
		send_to_char( "Done.\n\r", ch );
		return ;
	}

	do_hset( ch, "" );
}

void do_hl( CHAR_DATA *ch, char *argument )
{
	send_to_char( "If you want to use HLIST, spell it out.\n\r", ch );
	return ;
}

/*
 * Show help topics in a level range				-Thoric
 * Idea suggested by Gorog
 * prefix keyword indexing added by Fireblade
 */
void do_hlist( CHAR_DATA *ch, char *argument )
{
	int min, max, minlimit, maxlimit, cnt;
	char arg[MAX_INPUT_LENGTH];
	HELP_DATA *help;
	bool minfound, maxfound;
	char *idx;

	maxlimit = get_trust(ch);
	minlimit = maxlimit >= LEVEL_GREATER ? -1 : 0;

	min = minlimit;
	max = maxlimit;

	idx = NULL;
	minfound = FALSE;
	maxfound = FALSE;

	for ( argument = one_argument(argument, arg); arg[0] != '\0';
	      argument = one_argument(argument, arg))
	{
		if ( !isdigit(arg[0]) )
		{
			if ( idx )
			{
				set_char_color(AT_GREEN, ch);
				ch_printf(ch, "You may only use a single keyword to index the list.\n\r");
				return ;
			}
			idx = STRALLOC(arg);
		}
		else
		{
			if ( !minfound )
			{
				min = URANGE(minlimit, atoi(arg), maxlimit);
				minfound = TRUE;
			}
			else
				if ( !maxfound )
				{
					max = URANGE(minlimit, atoi(arg), maxlimit);
					maxfound = TRUE;
				}
				else
				{
					set_char_color(AT_GREEN, ch);
					ch_printf(ch, "You may only use two level limits.\n\r");
					return ;
				}
		}
	}

	if ( min > max )
	{
		int temp = min;

		min = max;
		max = temp;
	}

	set_pager_color( AT_GREEN, ch );
	pager_printf( ch, "Help Topics in level range %d to %d:\n\r\n\r", min, max );
	for ( cnt = 0, help = first_help; help; help = help->next )
		if ( help->level >= min && help->level <= max
		     && (!idx || nifty_is_name_prefix(idx, help->keyword)) )
		{
			pager_printf( ch, "  %3d %s\n\r", help->level, help->keyword );
			++cnt;
		}
	if ( cnt )
		pager_printf( ch, "\n\r%d pages found.\n\r", cnt );
	else
		send_to_char( "None found.\n\r", ch );

	if ( idx )
		STRFREE(idx);

	return ;
}


/*
 * New do_who with WHO REQUEST, clan, race and homepage support.  -Thoric
 *
 * Latest version of do_who eliminates redundant code by using linked lists.
 * Shows imms separately, indicates guest and retired immortals.
 * Narn, Oct/96
 *
 * Who group by Altrag, Feb 28/97
 */
struct whogr_s
{
	struct whogr_s *next;
	struct whogr_s *follower;
	struct whogr_s *l_follow;
	DESCRIPTOR_DATA *d;
	int indent;
}
*first_whogr, *last_whogr;

struct whogr_s *find_whogr(DESCRIPTOR_DATA *d, struct whogr_s *first)
{
	struct whogr_s *whogr, *whogr_t;

	for (whogr = first; whogr; whogr = whogr->next)
		if (whogr->d == d)
			return whogr;
		else if (whogr->follower && (whogr_t = find_whogr(d, whogr->follower)))
			return whogr_t;
	return NULL;
}

void indent_whogr(CHAR_DATA *looker, struct whogr_s *whogr, int ilev)
{
	for ( ; whogr; whogr = whogr->next )
	{
		if (whogr->follower)
		{
			int nlev = ilev;
			CHAR_DATA *wch =
			    (whogr->d->original ? whogr->d->original : whogr->d->character);

			if (can_see(looker, wch) && !IS_IMMORTAL(wch))
				nlev += 3;
			indent_whogr(looker, whogr->follower, nlev);
		}
		whogr->indent = ilev;
	}
}

/* This a great big mess to backwards-structure the ->leader character
   fields */
void create_whogr(CHAR_DATA *looker)
{
	DESCRIPTOR_DATA *d;
	CHAR_DATA *wch;
	struct whogr_s *whogr, *whogr_t;
	int dc = 0, wc = 0;

	while ((whogr = first_whogr) != NULL)
	{
		first_whogr = whogr->next;
		DISPOSE(whogr);
	}
	first_whogr = last_whogr = NULL;
	/* Link in the ones without leaders first */
	for (d = last_descriptor; d; d = d->prev)
	{
		if (d->connected != CON_PLAYING && d->connected != CON_EDITING)
			continue;
		++dc;
		wch = (d->original ? d->original : d->character);
		if (!wch->leader || wch->leader == wch || !wch->leader->desc ||
		    IS_NPC(wch->leader) || IS_IMMORTAL(wch) || IS_IMMORTAL(wch->leader))
		{
			CREATE(whogr, struct whogr_s, 1);
			if (!last_whogr)
				first_whogr = last_whogr = whogr;
			else
			{
				last_whogr->next = whogr;
				last_whogr = whogr;
			}
			whogr->next = NULL;
			whogr->follower = whogr->l_follow = NULL;
			whogr->d = d;
			whogr->indent = 0;
			++wc;
		}
	}
	/* Now for those who have leaders.. */
	while (wc < dc)
		for (d = last_descriptor; d; d = d->prev)
		{
			if (d->connected != CON_PLAYING && d->connected != CON_EDITING)
				continue;
			if (find_whogr(d, first_whogr))
				continue;
			wch = (d->original ? d->original : d->character);
			if (wch->leader && wch->leader != wch && wch->leader->desc &&
			    !IS_NPC(wch->leader) && !IS_IMMORTAL(wch) &&
			    !IS_IMMORTAL(wch->leader) &&
			    (whogr_t = find_whogr(wch->leader->desc, first_whogr)))
			{
				CREATE(whogr, struct whogr_s, 1);
				if (!whogr_t->l_follow)
					whogr_t->follower = whogr_t->l_follow = whogr;
				else
				{
					whogr_t->l_follow->next = whogr;
					whogr_t->l_follow = whogr;
				}
				whogr->next = NULL;
				whogr->follower = whogr->l_follow = NULL;
				whogr->d = d;
				whogr->indent = 0;
				++wc;
			}
		}
	/* Set up indentation levels */
	indent_whogr(looker, first_whogr, 0);

	/* And now to linear link them.. */
	for (whogr_t = NULL, whogr = first_whogr; whogr; )
		if (whogr->l_follow)
		{
			whogr->l_follow->next = whogr;
			whogr->l_follow = NULL;
			if (whogr_t)
				whogr_t->next = whogr = whogr->follower;
			else
				first_whogr = whogr = whogr->follower;
		}
		else
		{
			whogr_t = whogr;
			whogr = whogr->next;
		}
}

void do_who( CHAR_DATA *ch, char *argument )
{
	char *argument2;
	char buf[MAX_STRING_LENGTH];
	char buf2[MAX_STRING_LENGTH];
	char kairank[MAX_STRING_LENGTH];
	bool kaioshin = FALSE;
	char clan_name[MAX_INPUT_LENGTH];
	char council_name[MAX_INPUT_LENGTH];
	char invis_str[MAX_INPUT_LENGTH];
	char ghost_str[MAX_INPUT_LENGTH];
	char incog_str[MAX_INPUT_LENGTH];
	char char_name[MAX_INPUT_LENGTH];
	char last_name[MAX_INPUT_LENGTH];
	char *extra_title;
	char class_text[MAX_INPUT_LENGTH];
	struct whogr_s *whogr, *whogr_p;
	DESCRIPTOR_DATA *d;
	int iClass, iRace;
	int iLevelLower;
	int iLevelUpper;
	int nNumber;
	int nMatch;
	bool rgfClass[MAX_CLASS];
	bool rgfRace[MAX_RACE];
	bool fClassRestrict;
	bool fRaceRestrict;
	bool fImmortalOnly;
	bool fLeader;
	bool fPkill;
	bool fShowHomepage;
	bool fClanMatch; /* SB who clan (order),who guild, and who council */
	bool fCouncilMatch;
	bool fDeityMatch;
	bool fGroup;
	bool fQuesting;

	CLAN_DATA *pClan = NULL;
	COUNCIL_DATA *pCouncil = NULL;
	DEITY_DATA *pDeity = NULL;
	FILE *whoout = NULL;
	char clancolor[MAX_INPUT_LENGTH];

	if( IS_NPC(ch) )
	{
	  ch_printf(ch,"Huh?\n\r");
	  return;
	}

	/*
	#define WT_IMM    0;
	#define WT_MORTAL 1;
	#define WT_DEADLY 2;
	*/

	WHO_DATA *cur_who = NULL;
	WHO_DATA *next_who = NULL;
        WHO_DATA *first_unknown = NULL;
        WHO_DATA *first_grouped = NULL;
        WHO_DATA *first_groupwho = NULL;
	WHO_DATA *first_mortal = NULL;
	WHO_DATA *first_imm = NULL;
	WHO_DATA *first_deadly = NULL;
	WHO_DATA *first_skilled = NULL;
        WHO_DATA *first_experienced = NULL;
	WHO_DATA *first_ultimate = NULL;
	WHO_DATA *first_veteran = NULL;
	WHO_DATA *first_fearsome = NULL;
	WHO_DATA *first_legendary = NULL;
	WHO_DATA *first_epic = NULL;
        WHO_DATA *first_ascendant = NULL;
	WHO_DATA *first_transcendent = NULL;
	WHO_DATA *first_champion = NULL;
	WHO_DATA *first_titan = NULL;
	WHO_DATA *first_mythical = NULL;
	WHO_DATA *first_omnipotent = NULL;
        WHO_DATA *first_demigod = NULL;

	argument2 = strcat(argument,"" );

	/*
	 * Set default arguments.
	 */
	iLevelLower = 0;
	iLevelUpper = 16;
	fClassRestrict = FALSE;
	fRaceRestrict = FALSE;
	fImmortalOnly = FALSE;
	fPkill = FALSE;
	fShowHomepage = FALSE;
	fClanMatch	= FALSE; /* SB who clan (order), who guild, who council */
	fCouncilMatch = FALSE;
	fDeityMatch = FALSE;
	fGroup	= FALSE; /* Alty who group */
	fLeader	= FALSE;
	fQuesting = FALSE;
	for ( iClass = 0; iClass < MAX_CLASS; iClass++ )
		rgfClass[iClass] = FALSE;
	for ( iRace = 0; iRace < MAX_RACE; iRace++ )
		rgfRace[iRace] = FALSE;

#ifdef REQWHOARG
	/*
	 * The who command must have at least one argument because we often
	 * have up to 500 players on. Too much spam if a player accidentally
	 * types "who" with no arguments.           --Gorog
	 */
	if ( ch && argument[0] == '\0' )
	{
		send_to_pager_color(
		    "\n\r&GYou must specify at least one argument.\n\rUse 'who 1' to view the entire who list.\n\r", ch);
		return ;
	}
#endif

	/*
	 * Parse arguments.
	 */
	nNumber = 0;
	for ( ;; )
	{
		char arg[MAX_STRING_LENGTH];

		argument = one_argument( argument, arg );
		if ( arg[0] == '\0' )
			break;

		if ( is_number( arg ) )
		{
			switch ( ++nNumber )
			{
				case 1:
					iLevelLower = atoi( arg );
					break;
				case 2:
					iLevelUpper = atoi( arg );
					break;
				default:
					send_to_char( "Only two rank numbers allowed.\n\r", ch );
					return ;
			}
		}
		else
		{
			if ( strlen(arg) < 3 )
			{
				send_to_char( "Arguments must be longer than that.\n\r", ch );
				return ;
			}

			/*
			 * Look for classes to turn on.
			 */
			if ( !str_cmp( arg, "pkill" ) )
				fPkill = TRUE;
			else if (!str_cmp( arg, "questing"))
				fQuesting = TRUE;
			else
				if ( !str_cmp( arg, "imm" ) || !str_cmp( arg, "gods" ) )
					fImmortalOnly = TRUE;
				else
					if ( !str_cmp( arg, "leader" ) )
						fLeader = TRUE;
					else
						if ( !str_cmp( arg, "www" ) )
							fShowHomepage = TRUE;
						else
							if ( !str_cmp( arg, "group" ) && ch )
								fGroup = TRUE;
							else		 /* SB who clan (order), guild, council */
								if ( ( pClan = get_clan (arg) ) )
									fClanMatch = TRUE;
								else
									if ( ( pCouncil = get_council (arg) ) )
										fCouncilMatch = TRUE;
									else
										if ( ( pDeity = get_deity (arg) ) )
											fDeityMatch = TRUE;
										else
										{
											for ( iClass = 0; iClass < MAX_CLASS; iClass++ )
											{
												if ( !str_cmp( arg, class_table[iClass]->who_name ) )
												{
													rgfClass[iClass] = TRUE;
													break;
												}
											}
											if ( iClass != MAX_CLASS )
												fClassRestrict = TRUE;

											for ( iRace = 0; iRace < MAX_RACE; iRace++ )
											{
												if ( !str_cmp( arg, race_table[iRace]->race_name ) )
												{
													rgfRace[iRace] = TRUE;
													break;
												}
											}
											if ( iRace != MAX_RACE )
												fRaceRestrict = TRUE;

											if ( iClass == MAX_CLASS && iRace == MAX_RACE
											     && fClanMatch == FALSE
											     && fCouncilMatch == FALSE
											     && fDeityMatch == FALSE )
											{
												send_to_char( "That's not a class, race, order, guild,"
												              " council or deity.\n\r", ch );
												return ;
											}
										}
		}
	}

	char rarg[MAX_STRING_LENGTH];
        argument2 = one_argument( argument2, rarg );
    if( !is_number(rarg) )
    {
	if( !str_cmp(rarg,"android") )
	{
	  rgfRace[4] = TRUE; // Regular androids
	  rgfRace[26] = TRUE; // Super androids
	  rgfRace[28] = TRUE; // Type H androids
	  rgfRace[29] = TRUE; // Type E androids
	  rgfRace[30] = TRUE; // Type FM androids
	  rgfClass[4] = TRUE; // Regular androids
          rgfClass[26] = TRUE; // Super androids
          rgfClass[28] = TRUE; // Type H androids
          rgfClass[29] = TRUE; // Type E androids
          rgfClass[30] = TRUE; // Type FM androids
	}
	else if( !str_cmp(rarg,"saiyan") )
	{
	  rgfRace[0] = TRUE; // Normal Saiyans
	  rgfRace[10] = TRUE; // Saiyan-namek fused
	  rgfRace[11] = TRUE; // Saiyan-human fused
	  rgfRace[12] = TRUE; // Saiyan-halfbreed fused
	  rgfRace[13] = TRUE; // Saiyan-saiyan fused
	  rgfClass[0] = TRUE; // Normal Saiyans
          rgfClass[10] = TRUE; // Saiyan-namek fused
          rgfClass[11] = TRUE; // Saiyan-human fused
          rgfClass[12] = TRUE; // Saiyan-halfbreed fused
          rgfClass[13] = TRUE; // Saiyan-saiyan fused
	}
	else if( !str_cmp(rarg,"halfbreed") )
        {
          rgfRace[2] = TRUE; // Normal hbs
          rgfRace[21] = TRUE; // hb-namek fused
          rgfRace[22] = TRUE; // hb-human fused
          rgfRace[19] = TRUE; // hb-halfbreed fused
          rgfRace[20] = TRUE; // hb-saiyan fused
	  rgfClass[2] = TRUE; // Normal hbs
          rgfClass[21] = TRUE; // hb-namek fused
          rgfClass[22] = TRUE; // hb-human fused
          rgfClass[19] = TRUE; // hb-halfbreed fused
          rgfClass[20] = TRUE; // hb-saiyan fused
        }
	else if( !str_cmp(rarg,"human") )
        {
          rgfRace[1] = TRUE; // Normal humans
          rgfRace[14] = TRUE; // h-namek fused
          rgfRace[17] = TRUE; // h-human fused
          rgfRace[15] = TRUE; // h-halfbreed fused
          rgfRace[25] = TRUE; // h-saiyan fused
	  rgfClass[1] = TRUE; // Normal humans
          rgfClass[14] = TRUE; // h-namek fused
          rgfClass[17] = TRUE; // h-human fused
          rgfClass[15] = TRUE; // h-halfbreed fused
          rgfClass[25] = TRUE; // h-saiyan fused
        }
	else if( !str_cmp(rarg,"namek") )
        {
          rgfRace[3] = TRUE; // Normal nameks
          rgfRace[18] = TRUE; // n-namek fused
          rgfRace[23] = TRUE; // n-human fused
          rgfRace[16] = TRUE; // n-halfbreed fused
          rgfRace[24] = TRUE; // n-saiyan fused
	  rgfClass[3] = TRUE; // Normal nameks
          rgfClass[18] = TRUE; // n-namek fused
          rgfClass[23] = TRUE; // n-human fused
          rgfClass[16] = TRUE; // n-halfbreed fused
          rgfClass[24] = TRUE; // n-saiyan fused
        }
    }
	/*
	 * Now find matching chars.
	 */
	nMatch = 0;
	buf[0] = '\0';
	if ( ch )
		set_pager_color( AT_GREEN, ch );
	else
	{
		if ( fShowHomepage )
			whoout = fopen( WEBWHO_FILE, "w" );
		else
			whoout = fopen( WHO_FILE, "w" );
		if ( !whoout )
		{
			bug( "do_who: cannot open who file!" );
			return ;
		}
	}

	/* start from last to first to get it in the proper order */
	if (fGroup)
	{
		create_whogr(ch);
		whogr = first_whogr;
		d = whogr->d;
	}
	else
	{
		whogr = NULL;
		d = last_descriptor;
	}
	whogr_p = NULL;
	for ( ; d; whogr_p = whogr, whogr = (fGroup ? whogr->next : NULL),
	      d = (fGroup ? (whogr ? whogr->d : NULL) : d->prev))
	{
		CHAR_DATA * wch;
		char const *class;

		if ( (d->connected != CON_PLAYING && d->connected != CON_EDITING)
		     || !can_see( ch, d->character ) || d->original)
			continue;
		wch = d->original ? d->original : d->character;
		if ( (!IS_IMMORTAL(wch) && get_rank_number(wch) < iLevelLower)
		     || (!IS_IMMORTAL(wch) && get_rank_number(wch) > iLevelUpper)
		     || ( fPkill && !can_pk(wch) )
		     || ( fQuesting && !xIS_SET(wch->act, PLR_QUESTING))
		     || ( fImmortalOnly && wch->level < LEVEL_IMMORTAL )
		     || ( fClassRestrict && !rgfClass[wch->class] )
		     || ( fRaceRestrict && !rgfRace[wch->race] )
		     || ( fClanMatch && ( pClan != wch->pcdata->clan ))  /* SB */
		     || ( fCouncilMatch && ( pCouncil != wch->pcdata->council )) /* SB */
		     || ( fDeityMatch && ( pDeity != wch->pcdata->deity )) )
			continue;
		if ( fLeader && !(wch->pcdata->council &&
		                  ((wch->pcdata->council->head2 &&
		                    !str_cmp(wch->pcdata->council->head2, wch->name)) ||
		                   (wch->pcdata->council->head &&
		                    !str_cmp(wch->pcdata->council->head, wch->name)))) &&
		     !(wch->pcdata->clan && ((is_deity(wch) )
		                             || (is_leader(wch)))))
			continue;

		if (fGroup && !wch->leader &&
		    !IS_SET(wch->pcdata->flags, PCFLAG_GROUPWHO) &&
		    (!whogr_p || !whogr_p->indent))
			continue;

		nMatch++;

		if ( fShowHomepage
		     && wch->pcdata->homepage
		     && wch->pcdata->homepage[0] != '\0' )
			sprintf( char_name, "<A HREF=\"%s\">%s</A>",
			         show_tilde( wch->pcdata->homepage ), wch->name );
		else
		{
			sprintf(char_name, "%s%s", color_align(wch), wch->name);
			sprintf(last_name, "%s", wch->pcdata->last_name ? wch->pcdata->last_name : "");

			/*		if (IS_GOOD(wch))
						sprintf(char_name, "&C%s", wch->name);
					else if (IS_NEUTRAL(wch))
						sprintf(char_name, "&w%s", wch->name);
					else
						sprintf(char_name, "&R%s", wch->name);
			*/
		}

		//	  strcpy( char_name, wch->name );

		if( is_kaio(wch) && wch->kairank > 0 )
		{
		  kaioshin = TRUE;
		  sprintf(kairank,"%s",get_kai(wch));
		}
		else if( is_demon(wch) && wch->demonrank > 0 )
		{
		  kaioshin = TRUE;
		  sprintf(kairank,"%s",get_demon(wch));
		}
	        else
		  kaioshin = FALSE;

/*		sprintf( class_text, " %s%s%11s", NOT_AUTHED(wch) ? "N" : " ",
			         get_pkColor(wch),
				 (kaioshin) ? kairank : class_table[wch->class]->who_name );
*/
		sprintf( class_text," %s%15s", get_pkColor(wch),
				kaioshin ? kairank : class_table[wch->class]->who_name );

		class = class_text;
		switch ( wch->level )
		{
			default:
				break;
			case MAX_LEVEL - 0:
				class = " &WSupreme Entity";
				break;
			case MAX_LEVEL - 1:
				class = " &WInfinite      ";
				break;
			case MAX_LEVEL - 2:
				class = " &WEternal       ";
				break;
			case MAX_LEVEL - 3:
				class = " &WAncient       ";
				break;
			case MAX_LEVEL - 4:
				class = " &WExalted God   ";
				break;
			case MAX_LEVEL - 5:
				class = " &WAscendant God ";
				break;
			case MAX_LEVEL - 6:
				class = " &WGreater God   ";
				break;
			case MAX_LEVEL - 7:
				class = " &WGod           ";
				break;
			case MAX_LEVEL - 8:
				class = " &WLesser God    ";
				break;
			case MAX_LEVEL - 9:
				class = " &WImmortal      ";
				break;
			case MAX_LEVEL - 10:
				class = " &WDemi God      ";
				break;
			case MAX_LEVEL - 11:
				class = " &WSavior        ";
				break;
			case MAX_LEVEL - 12:
				class = " &WCreator       ";
				break;
			case MAX_LEVEL - 13:
				class = " &WAcolyte       ";
				break;
			case MAX_LEVEL - 14:
				class = " &WNeophyte      ";
				break;
			case MAX_LEVEL - 15:
				class = " &WAvatar        ";
				break;
		}

		if ( wch->level >= LEVEL_IMMORTAL && wch->pcdata->pretitle
		     && wch->pcdata->pretitle[0] != '\0' )
		{
			sprintf( buf2, " &W%14s", wch->pcdata->pretitle );
			class = buf2;
		}

		if ( !str_cmp( wch->name, sysdata.guild_overseer ) )
			extra_title = " [Overseer of Guilds]";
		else if ( !str_cmp( wch->name, sysdata.guild_advisor ) )
			extra_title = " [Advisor to Guilds]";
		else
			extra_title = "";

		if ( IS_RETIRED( wch ) )
			class = "&g        Retired";
		else if ( IS_GUEST( wch ) )
			class = "&g          Guest";
		/*
			else if ( wch->pcdata->clan
		             &&  !str_cmp( wch->name, wch->pcdata->clan->leader )
		             &&   wch->pcdata->clan->leadrank[0] != '\0' )
		                class = wch->pcdata->clan->leadrank;
		        else if ( wch->pcdata->clan
			     &&  !str_cmp( wch->name, wch->pcdata->clan->number1 )
		             &&   wch->pcdata->clan->onerank[0] != '\0' )
		                class = wch->pcdata->clan->onerank;
			else if ( wch->pcdata->clan
		             &&  !str_cmp( wch->name, wch->pcdata->clan->number2 )
		             &&   wch->pcdata->clan->tworank[0] != '\0' )
		                class = wch->pcdata->clan->tworank;
		  	else if ( wch->pcdata->rank && wch->pcdata->rank[0] != '\0' )
			  class = wch->pcdata->rank;
		*/
		sprintf( clancolor, "&w" );

		if ( wch->pcdata->clan )
		{
			CLAN_DATA *pclan = wch->pcdata->clan;

			if (wch->pcdata->clan->alignment == CLANALIGN_GOOD)
				sprintf( clancolor, "&C" );
			else if (wch->pcdata->clan->alignment == CLANALIGN_EVIL)
				sprintf( clancolor, "&R" );
			else
				sprintf( clancolor, "&W" );

			if ( pclan->clan_type == CLAN_GUILD )
				strcpy( clan_name, " <" );
			else
				strcpy( clan_name, " (" );

			if ( pclan->clan_type == CLAN_ORDER )
			{
				if ( is_deity(wch) )
					strcat( clan_name, "Admin, Order of " );
				else
					if ( is_leader(wch) )
						strcat( clan_name, "Leader, Order of " );
							else
								strcat( clan_name, "Order of " );
			}
			else
				if ( pclan->clan_type == CLAN_GUILD )
				{
					if ( is_leader(wch) )
						strcat( clan_name, "Leader, " );
				}
				else
				{
					if( is_owner(wch,pclan) )
						strcat( clan_name, "Owner of " );
					else if ( is_deity(wch) )
						strcat( clan_name, "Admin of " );
					else
					{
					    if( str_cmp( get_clan_rank( wch ), "" ) && str_cmp( get_clan_rank( wch ), "None" ) )
					    {
						strcat( clan_name, get_clan_rank( wch ) );
						strcat( clan_name, " of " );
					    }
					    else
					    {
						if( is_leader( wch ) )
						    strcat( clan_name, "Leader of " );
					    }
					}
				}
			strcat( clan_name, pclan->name );
			if ( pclan->clan_type == CLAN_GUILD )
				strcat( clan_name, ">" );
			else
				strcat( clan_name, ")" );
		}
		else
			clan_name[0] = '\0';

		if ( wch->pcdata->council )
		{
			strcpy( council_name, " &G[" );
			if ( wch->pcdata->council->head2 == NULL )
			{
				if (!str_cmp (wch->name, wch->pcdata->council->head))
					strcat (council_name, "Head of ");
			}
			else
			{
				if (!str_cmp (wch->name, wch->pcdata->council->head)
				    || !str_cmp ( wch->name, wch->pcdata->council->head2) )
					strcat (council_name, "Co-Head of ");
			}
			strcat( council_name, wch->pcdata->council_name );
			strcat( council_name, "]" );
		}
		else
			council_name[0] = '\0';

		if ( xIS_SET(wch->act, PLR_WIZINVIS)
		     || xIS_SET(wch->act, PLR_GHOST)
		     || xIS_SET(wch->act, PLR_INCOG) )
		{
			if ( xIS_SET(wch->act, PLR_WIZINVIS))
				sprintf( invis_str, "(i%d) ", wch->pcdata->wizinvis );
			else
				invis_str[0] = '\0';
			if ( xIS_SET(wch->act, PLR_GHOST))
				sprintf( ghost_str, "(g%d) ", wch->pcdata->ghost_level );
			else
				ghost_str[0] = '\0';
			if ( xIS_SET(wch->act, PLR_INCOG))
				sprintf( incog_str, "(c%d) ", wch->pcdata->incog_level );
			else
				incog_str[0] = '\0';
		}
		else
		{
			invis_str[0] = '\0';
			ghost_str[0] = '\0';
			incog_str[0] = '\0';
		}

		/*
		    if ( xIS_SET(wch->act, PLR_BOUNTY)
		    	&& !str_cmp( wch->name, ch->pcdata->hunting ) )
		         strcat( buf3, "&R(BOUNTY) ");
		    else
		    	strcat( buf3, "");
		*/

		sprintf( buf, "%*s%s%-15s &W%s&w%s&z%s%s%s%s%s%s%s%s&C%s%s&Y%s.%s&D%s%s%s\n\r",
			 0, "",
			 xIS_SET(wch->act, PLR_WAR1) ? "&w&W[&YWAR&W]" :
                         (xIS_SET(wch->act, PLR_WAR2) ? "&w&W[&RWAR&W]" : "     "),
//		         (fGroup ? whogr->indent : 0), "",
		         class,
		         invis_str,
		         ghost_str,
		         incog_str,
		         (wch->desc && wch->desc->connected) ? "&g[WRITING] " : "",
		         xIS_SET(wch->act, PLR_AFK) ? "&g[AFK] " : "",
		         xIS_SET(wch->affected_by, AFF_TAG) ? "&W[IT] " : "",
		         IS_SET(wch->pcdata->flags, PCFLAG_DND) ? "&g[DND] " : "",
		         xIS_SET(wch->act, PLR_ATTACKER) ? "&R(ATTACKER) " : "",
		         xIS_SET(wch->act, PLR_KILLER) ? "&R(KILLER) " : "",
		         xIS_SET(wch->act, PLR_THIEF) ? "&R(THIEF) " : "",
		         char_name, last_name,
		         wch->pcdata->title,
		         extra_title,
		         clancolor,
		         clan_name,
		         council_name );
			 /*
		         xIS_SET(wch->act, PLR_WAR1) ? "&w&W[&YWAR&W] " : "",
		         xIS_SET(wch->act, PLR_WAR2) ? "&w&W[&RWAR&W] " : "");
			 */

		/*
		 * This is where the old code would display the found player to the ch.
		 * What we do instead is put the found data into a linked list
		 */

		/* First make the structure. */
		CREATE( cur_who, WHO_DATA, 1 );
		cur_who->text = str_dup( buf );
		if ( wch->level > 50 && IS_IMMORTAL( wch ) )
			cur_who->type = WT_IMM;
		else if ( fGroup )
			if ( wch->leader || (whogr_p && whogr_p->indent) )
				cur_who->type = WT_GROUPED;
			else
				cur_who->type = WT_GROUPWHO;
		else if (IS_HC(wch))
			cur_who->type = WT_UNKNOWN;
		else if ( CAN_PKILL( wch ) )
		{
			if (wch->exp < 100000 ) /* less than 100k  */
				cur_who->type = WT_DEADLY;
			else if (wch->exp < 1000000 ) /* less than 1 mil */
				cur_who->type = WT_SKILLED;
			else if (wch->exp < 10000000 ) /* less than 10 mil */
				cur_who->type = WT_EXPERIENCED;
			else if (wch->exp < 100000000 ) /* less than 100 mil */
				cur_who->type = WT_ULTIMATE;
			else if (wch->exp < 1000000000 ) /* less than 1 bil */
				cur_who->type = WT_VETERAN;
			else if (wch->exp < 10000000000ULL ) /* less than 10 bil */
				cur_who->type = WT_FEARSOME;
			else if (wch->exp < 50000000000ULL ) /* less than 50 bil */
				cur_who->type = WT_LEGENDARY;
			else if (wch->exp < 100000000000ULL) /* less than 100 bil */
				cur_who->type = WT_EPIC;
			else if (wch->exp < 300000000000ULL) /* less than 300 bil */
				cur_who->type = WT_ASCENDANT;
			else if (wch->exp < 600000000000ULL) /* less than 600 bil */
				cur_who->type = WT_TRANSCENDENT;
			else if (wch->exp < 1000000000000ULL) /* less than 1 trillion */
				cur_who->type = WT_CHAMPION;
                        else if (wch->exp < 10000000000000ULL)/* < 10t */
                                cur_who->type = WT_TITAN;
                        else if (wch->exp < 50000000000000ULL)/* < 50t */
                                cur_who->type = WT_MYTHICAL;
                        else if (wch->exp < 100000000000000ULL)/* < 100t */
                                cur_who->type = WT_OMNIPOTENT;
			else
				cur_who->type = WT_DEMIGOD;
		}
		else
			cur_who->type = WT_MORTAL;

		/* Then put it into the appropriate list. */
		switch ( cur_who->type )
		{
			case WT_MORTAL:
				cur_who->next = first_mortal;
				first_mortal = cur_who;
				break;
			case WT_UNKNOWN:
				cur_who->next = first_unknown;
				first_unknown = cur_who;
				break;
			case WT_DEADLY:
				cur_who->next = first_deadly;
				first_deadly = cur_who;
				break;
			case WT_SKILLED:
				cur_who->next = first_skilled;
				first_skilled = cur_who;
				break;
			case WT_EXPERIENCED:
				cur_who->next = first_experienced;
				first_experienced = cur_who;
				break;
			case WT_ULTIMATE:
				cur_who->next = first_ultimate;
				first_ultimate = cur_who;
				break;
			case WT_VETERAN:
				cur_who->next = first_veteran;
				first_veteran = cur_who;
				break;
			case WT_FEARSOME:
                                cur_who->next = first_fearsome;
                                first_fearsome = cur_who;
                                break;
			case WT_LEGENDARY:
				cur_who->next = first_legendary;
				first_legendary = cur_who;
				break;
			case WT_EPIC:
				cur_who->next = first_epic;
				first_epic = cur_who;
				break;
                        case WT_ASCENDANT:
                                cur_who->next = first_ascendant;
                                first_ascendant = cur_who;
                                break;
			case WT_TRANSCENDENT:
				cur_who->next = first_transcendent;
				first_transcendent = cur_who;
				break;
			case WT_CHAMPION:
				cur_who->next = first_champion;
				first_champion = cur_who;
				break;
			case WT_TITAN:
				cur_who->next = first_titan;
				first_titan = cur_who;
				break;
			case WT_MYTHICAL:
				cur_who->next = first_mythical;
				first_mythical = cur_who;
				break;
			case WT_OMNIPOTENT:
				cur_who->next = first_omnipotent;
				first_omnipotent = cur_who;
				break;
			case WT_DEMIGOD:
                                cur_who->next = first_demigod;
                                first_demigod = cur_who;
                                break;
			case WT_GROUPED:
				cur_who->next = first_grouped;
				first_grouped = cur_who;
				break;
			case WT_GROUPWHO:
				cur_who->next = first_groupwho;
				first_groupwho = cur_who;
				break;
			case WT_IMM:
				cur_who->next = first_imm;
				first_imm = cur_who;
				break;

		}

	}


	/* Ok, now we have three separate linked lists and what remains is to
	 * display the information and clean up.
	 */
	/*
	 * Two extras now for grouped and groupwho (wanting group). -- Alty
	 */

	send_to_pager_color( "\n\r", ch);

	if ( first_mortal )
	{
		if ( !ch )
			fprintf( whoout, "\n\r---------------------------[ FIGHTERS IN TRAINING ]---------------------------\n\r\n\r" );
		else
			send_to_pager_color( "\n\r&z---------------------------[ &PFIGHTERS IN TRAINING &z]---------------------------\n\r", ch );
	}

	for ( cur_who = first_mortal; cur_who; cur_who = next_who )
	{
		if ( !ch )
			fprintf( whoout, cur_who->text );
		else
			send_to_pager_color( cur_who->text, ch );
		next_who = cur_who->next;
		DISPOSE( cur_who->text );
		DISPOSE( cur_who );
	}

	if ( first_unknown )
	{
		if ( !ch )
			fprintf( whoout, "\n\r---------------------------------[ UNKNOWN ]----------------------------------\n\r\n\r" );
		else
			send_to_pager_color( "&z---------------------------------[ &rUNKNOWN &z]----------------------------------\n\r", ch );
	}

	for ( cur_who = first_unknown; cur_who; cur_who = next_who )
	{
		if ( !ch )
			fprintf( whoout, cur_who->text );
		else
			send_to_pager_color( cur_who->text, ch );
		next_who = cur_who->next;
		DISPOSE( cur_who->text );
		DISPOSE( cur_who );
	}

	if ( first_deadly )
	{
		if ( !ch )
			fprintf( whoout, "\n\r-----------------------------[ TRAINED FIGHTERS ]-----------------------------\n\r\n\r" );
		else
			send_to_pager_color( "&z-----------------------------[ &cTRAINED FIGHTERS &z]-----------------------------\n\r", ch );
	}

	for ( cur_who = first_deadly; cur_who; cur_who = next_who )
	{
		if ( !ch )
			fprintf( whoout, cur_who->text );
		else
			send_to_pager_color( cur_who->text, ch );
		next_who = cur_who->next;
		DISPOSE( cur_who->text );
		DISPOSE( cur_who );
	}

	if ( first_skilled )
	{
		if ( !ch )
			fprintf( whoout, "\n\r-----------------------------[ SKILLED FIGHTERS ]-----------------------------\n\r\n\r" );
		else
			send_to_pager_color( "&z-----------------------------[ &OSKILLED FIGHTERS &z]-----------------------------\n\r", ch );
	}

	for ( cur_who = first_skilled; cur_who; cur_who = next_who )
	{
		if ( !ch )
			fprintf( whoout, cur_who->text );
		else
			send_to_pager_color( cur_who->text, ch );
		next_who = cur_who->next;
		DISPOSE( cur_who->text );
		DISPOSE( cur_who );
	}

	if ( first_experienced )
	{
		if ( !ch )
			fprintf( whoout, "\n\r---------------------------[ EXPERIENCED FIGHTERS ]---------------------------\n\r\n\r" );
		else
			send_to_pager_color( "&z---------------------------[ &wEXPERIENCED FIGHTERS &z]---------------------------\n\r", ch );
	}

	for ( cur_who = first_experienced; cur_who; cur_who = next_who )
	{
		if ( !ch )
			fprintf( whoout, cur_who->text );
		else
			send_to_pager_color( cur_who->text, ch );
		next_who = cur_who->next;
		DISPOSE( cur_who->text );
		DISPOSE( cur_who );
	}

	if ( first_ultimate )
	{
		if ( !ch )
			fprintf( whoout, "\n\r----------------------------[ ULTIMATE  FIGHTERS ]----------------------------\n\r\n\r" );
		else
			send_to_pager_color( "&z----------------------------[ &RULTIMATE  FIGHTERS &z]----------------------------\n\r", ch );
	}

	for ( cur_who = first_ultimate; cur_who; cur_who = next_who )
	{
		if ( !ch )
			fprintf( whoout, cur_who->text );
		else
			send_to_pager_color( cur_who->text, ch );
		next_who = cur_who->next;
		DISPOSE( cur_who->text );
		DISPOSE( cur_who );
	}

	if ( first_veteran )
	{
		if ( !ch )
			fprintf( whoout, "\n\r-----------------------------[ VETERAN WARRIORS ]-----------------------------\n\r\n\r" );
		else
			send_to_pager_color( "&z-----------------------------[ &CVETERAN WARRIORS &z]-----------------------------\n\r", ch );
	}

	for ( cur_who = first_veteran; cur_who; cur_who = next_who )
	{
		if ( !ch )
			fprintf( whoout, cur_who->text );
		else
			send_to_pager_color( cur_who->text, ch );
		next_who = cur_who->next;
		DISPOSE( cur_who->text );
		DISPOSE( cur_who );
	}

        if ( first_fearsome )
        {
                if ( !ch )
                        fprintf( whoout,"\n\r-----------------------------[ FEARSOME WARRIORS ]----------------------------\n\r\n\r" );
                else
                        send_to_pager_color("&z-----------------------------[ &rFEARSOME WARRIORS &z]----------------------------\n\r", ch );
        }

        for ( cur_who = first_fearsome; cur_who; cur_who = next_who )
        {
                if ( !ch )
                        fprintf( whoout, cur_who->text );
                else
                        send_to_pager_color( cur_who->text, ch );
                next_who = cur_who->next;
                DISPOSE( cur_who->text );
                DISPOSE( cur_who );
        }

	if ( first_legendary )
	{
		if ( !ch )
			fprintf( whoout, "\n\r----------------------------[ LEGENDARY WARRIORS ]----------------------------\n\r\n\r" );
		else
			send_to_pager_color( "&z----------------------------[ &BLEGENDARY WARRIORS &z]----------------------------\n\r", ch );
	}

	for ( cur_who = first_legendary; cur_who; cur_who = next_who )
	{
		if ( !ch )
			fprintf( whoout, cur_who->text );
		else
			send_to_pager_color( cur_who->text, ch );
		next_who = cur_who->next;
		DISPOSE( cur_who->text );
		DISPOSE( cur_who );
	}

	if ( first_epic )
	{
		if ( !ch )
			fprintf( whoout, "\n\r------------------------------[ EPIC WARRIORS ]-------------------------------\n\r\n\r" );
		else
			send_to_pager_color( "&z------------------------------[ &WEPIC WARRIORS &z]-------------------------------\n\r", ch );
	}

	for ( cur_who = first_epic; cur_who; cur_who = next_who )
	{
		if ( !ch )
			fprintf( whoout, cur_who->text );
		else
			send_to_pager_color( cur_who->text, ch );
		next_who = cur_who->next;
		DISPOSE( cur_who->text );
		DISPOSE( cur_who );
	}

        if ( first_ascendant )
        {
                if ( !ch )
                        fprintf( whoout,"\n\r----------------------------[ ASCENDANT WARRIORS ]----------------------------\n\r\n\r" );
                else
                        send_to_pager_color("&z----------------------------[ &gASCENDANT WARRIORS &z]----------------------------\n\r", ch );
        }

        for ( cur_who = first_ascendant; cur_who; cur_who = next_who )
        {
                if ( !ch )
                        fprintf( whoout, cur_who->text );
                else
                        send_to_pager_color( cur_who->text, ch );
                next_who = cur_who->next;
                DISPOSE( cur_who->text );
                DISPOSE( cur_who );
        }

	if ( first_transcendent )
	{
		if ( !ch )
			fprintf( whoout, "\n\r--------------------------[ TRANSCENDENT WARRIORS ]---------------------------\n\r\n\r" );
		else
			send_to_pager_color( "&z--------------------------[ &GTRANSCENDENT WARRIORS &z]---------------------------\n\r", ch );
	}

	for ( cur_who = first_transcendent; cur_who; cur_who = next_who )
	{
		if ( !ch )
			fprintf( whoout, cur_who->text );
		else
			send_to_pager_color( cur_who->text, ch );
		next_who = cur_who->next;
		DISPOSE( cur_who->text );
		DISPOSE( cur_who );
	}

	if ( first_champion )
	{
		if ( !ch )
			fprintf( whoout, "\n\r------------------------------[ CHAMPIONS ]-------------------------------\n\r\n\r" );
		else
			send_to_pager_color( "&z--------------------------------[ &BCHAMPIONS &z]---------------------------------\n\r", ch );
	}

	for ( cur_who = first_champion; cur_who; cur_who = next_who )
	{
		if ( !ch )
			fprintf( whoout, cur_who->text );
		else
			send_to_pager_color( cur_who->text, ch );
		next_who = cur_who->next;
		DISPOSE( cur_who->text );
		DISPOSE( cur_who );
	}

	if ( first_titan )
	{
		if ( !ch )
			fprintf( whoout, "\n\r-------------------------------[ TITANS ]--------------------------------\n\r\n\r" );
		else
			send_to_pager_color( "&z---------------------------------[ &WTITANS &z]-----------------------------------\n\r", ch );
	}

	for ( cur_who = first_titan; cur_who; cur_who = next_who )
	{
		if ( !ch )
			fprintf( whoout, cur_who->text );
		else
			send_to_pager_color( cur_who->text, ch );
		next_who = cur_who->next;
		DISPOSE( cur_who->text );
		DISPOSE( cur_who );
	}

	if ( first_mythical )
	{
		if ( !ch )
			fprintf( whoout, "\n\r-------------------------[ MYTHICAL WARRIORS ]-----------------------------\n\r\n\r" );
		else
			send_to_pager_color( "&z---------------------------[ &CMYTHICAL WARRIORS &z]------------------------------\n\r", ch );
	}

	for ( cur_who = first_mythical; cur_who; cur_who = next_who )
	{
		if ( !ch )
			fprintf( whoout, cur_who->text );
		else
			send_to_pager_color( cur_who->text, ch );
		next_who = cur_who->next;
		DISPOSE( cur_who->text );
		DISPOSE( cur_who );
	}

	if ( first_omnipotent )
	{
		if ( !ch )
			fprintf( whoout, "\n\r------------------------[ OMNIPOTENT WARRIORS ]----------------------------\n\r\n\r" );
		else
			send_to_pager_color( "&z--------------------------[ &ROMNIPOTENT WARRIORS &z]-----------------------------\n\r", ch );
	}

	for ( cur_who = first_omnipotent; cur_who; cur_who = next_who )
	{
		if ( !ch )
			fprintf( whoout, cur_who->text );
		else
			send_to_pager_color( cur_who->text, ch );
		next_who = cur_who->next;
		DISPOSE( cur_who->text );
		DISPOSE( cur_who );
	}

	if ( first_demigod )
        {
                if ( !ch )
                        fprintf( whoout,"\n\r--------------------------------[ DEMI-GODS ]---------------------------------\n\r\n\r" );
                else
                        send_to_pager_color("&r--------------------------------[ &zDEMI-GODS &r]---------------------------------\n\r", ch );
        }

        for ( cur_who = first_demigod; cur_who; cur_who = next_who )
        {
                if ( !ch )
                        fprintf( whoout, cur_who->text );
                else
                        send_to_pager_color( cur_who->text, ch );
                next_who = cur_who->next;
                DISPOSE( cur_who->text );
                DISPOSE( cur_who );
        }

	if (first_grouped)
	{
		/*      if ( !ch )
		        fprintf( whoout, "\n\r-----------------------------[ GROUPED CHARACTERS ]---------------------------\n\r\n\r" );
		      else*/
		send_to_pager_color( "&z-----------------------------[ GROUPED CHARACTERS ]---------------------------\n\r", ch );
	}
	for ( cur_who = first_grouped; cur_who; cur_who = next_who )
	{
		/*      if ( !ch )
		        fprintf( whoout, cur_who->text );
		      else*/
		send_to_pager_color( cur_who->text, ch );
		next_who = cur_who->next;
		DISPOSE( cur_who->text );
		DISPOSE( cur_who );
	}

	if (first_groupwho)
	{
		if ( !ch )
			fprintf( whoout, "\n\r-------------------------------[ WANTING GROUP ]------------------------------\n\r\n\r" );
		else
			send_to_pager_color( "&z-------------------------------[ WANTING GROUP ]------------------------------\n\r", ch );
	}
	for ( cur_who = first_groupwho; cur_who; cur_who = next_who )
	{
		if ( !ch )
			fprintf( whoout, cur_who->text );
		else
			send_to_pager_color( cur_who->text, ch );
		next_who = cur_who->next;
		DISPOSE( cur_who->text );
		DISPOSE( cur_who );
	}

	if ( first_imm )
	{
		if ( !ch )
			fprintf( whoout, "\n\r------------------------------[ ADMINISTRATORS ]------------------------------\n\r\n\r" );
		else
			send_to_pager_color( "&z------------------------------[ &YADMINISTRATORS &z]------------------------------\n\r", ch );
	}

	for ( cur_who = first_imm; cur_who; cur_who = next_who )
	{
		if ( !ch )
			fprintf( whoout, cur_who->text );
		else
			send_to_pager_color( cur_who->text, ch );
		next_who = cur_who->next;
		DISPOSE( cur_who->text );
		DISPOSE( cur_who );
	}

	if ( !ch )
	{
		fprintf( whoout, "%d player%s.\n\r", nMatch, nMatch == 1 ? "" : "s" );
		fclose( whoout );
		return ;
	}

	send_to_pager_color( "&z------------------------------------------------------------------------------\n\r", ch );
	pager_printf_color( ch, "&Y%d &Wplayer%s.\n\r", nMatch, nMatch == 1 ? "" : "s" );
	return ;
}

void do_compare( CHAR_DATA *ch, char *argument )
{
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	OBJ_DATA *obj1;
	OBJ_DATA *obj2;
	int value1;
	int value2;
	char *msg;

	argument = one_argument( argument, arg1 );
	argument = one_argument( argument, arg2 );
	if ( arg1[0] == '\0' )
	{
		send_to_char( "Compare what to what?\n\r", ch );
		return ;
	}

	if ( ( obj1 = get_obj_carry( ch, arg1 ) ) == NULL )
	{
		send_to_char( "You do not have that item.\n\r", ch );
		return ;
	}

	if ( arg2[0] == '\0' )
	{
		for ( obj2 = ch->first_carrying; obj2; obj2 = obj2->next_content )
		{
			if ( obj2->wear_loc != WEAR_NONE
			     && can_see_obj( ch, obj2 )
			     && obj1->item_type == obj2->item_type
			     && ( obj1->wear_flags & obj2->wear_flags & ~ITEM_TAKE) != 0 )
				break;
		}

		if ( !obj2 )
		{
			send_to_char( "You aren't wearing anything comparable.\n\r", ch );
			return ;
		}
	}
	else
	{
		if ( ( obj2 = get_obj_carry( ch, arg2 ) ) == NULL )
		{
			send_to_char( "You do not have that item.\n\r", ch );
			return ;
		}
	}

	msg	= NULL;
	value1	= 0;
	value2	= 0;

	if ( obj1 == obj2 )
	{
		msg = "You compare $p to itself.  It looks about the same.";
	}
	else if ( obj1->item_type != obj2->item_type )
	{
		msg = "You can't compare $p and $P.";
	}
	else
	{
		switch ( obj1->item_type )
		{
			default:
				msg = "You can't compare $p and $P.";
				break;

			case ITEM_ARMOR:
				value1 = obj1->value[0];
				value2 = obj2->value[0];
				break;

			case ITEM_WEAPON:
				value1 = obj1->value[1] + obj1->value[2];
				value2 = obj2->value[1] + obj2->value[2];
				break;
		}
	}

	if ( !msg )
	{
		if ( value1 == value2 )
			msg = "$p and $P look about the same.";
		else if ( value1 > value2 )
			msg = "$p looks better than $P.";
		else
			msg = "$p looks worse than $P.";
	}

	act( AT_PLAIN, msg, ch, obj1, obj2, TO_CHAR );
	return ;
}



void do_where( CHAR_DATA *ch, char *argument )
{
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;
	CHAR_DATA *v;
	DESCRIPTOR_DATA *d;
	bool found;
	int count = 0;

	one_argument( argument, arg );

	if( !str_cmp(arg,"npc") )
	{
	  ch_printf(ch,"\n\r&cNpc's that currently serve you:\n\r");
	  for( v = first_char; v; v = v->next )
	  {
	    if( !IS_NPC(v) )
	      continue;
	    if( !is_split(v) )
	      continue;
	    if( !v->master )
	      continue;
	    if( v->master != ch )
	      continue;

	    ch_printf(ch,"&w%s",v->short_descr);
	    if( v->in_room )
	      ch_printf(ch,"    &c%s", v->in_room->name);
	    if( v->in_room->area )
	      ch_printf(ch,"    &z(&c%s&z)&D",v->in_room->area->name);

	    ch_printf(ch,"\n\r");
	    count++;
	  }
	  if( count == 0 )
	    ch_printf(ch,"&RNone.\n\r");
	  return;
	}

	if ( arg[0] != '\0'
	     && (victim = get_char_world(ch, arg)) && !IS_NPC(victim)
	     && IS_SET(victim->pcdata->flags, PCFLAG_DND)
	     && get_trust(ch) < get_trust(victim) )
	{
		act( AT_PLAIN, "You didn't find any $T.", ch, NULL, arg, TO_CHAR );
		return ;
	}

	sysdata.outBytesFlag = LOGBOUTINFORMATION;
	set_pager_color( AT_PERSON, ch );
	if ( arg[0] == '\0' )
	{
		pager_printf( ch, "\n\rPlayers near you in %s:\n\r", ch->in_room->area->name );
		found = FALSE;
		for ( d = first_descriptor; d; d = d->next )
			if ( (d->connected == CON_PLAYING || d->connected == CON_EDITING )
			     && ( victim = d->character ) != NULL
			     && !IS_NPC(victim)
			     && victim->in_room
			     && victim->in_room->area == ch->in_room->area
			     && can_see( ch, victim )
			     && ( (!IS_NPC(victim) && !IS_HC(victim))
			       || (!IS_NPC(victim) && IS_HC(victim) &&
				   ch->level >= 63) )
			     && ( get_trust(ch) >= get_trust(victim)
			          || !IS_SET(victim->pcdata->flags, PCFLAG_DND) )
			   ) /* if victim has the DND flag ch must outrank them */

			{
				found = TRUE;
				/*		if ( CAN_PKILL( victim ) )
						  set_pager_color( AT_PURPLE, ch );
						else
						  set_pager_color( AT_PERSON, ch );
				*/
				if( !IS_HC(victim) )
				  pager_printf_color( ch, "&P%-13s  ", victim->name );
				else
				  pager_printf_color( ch, "&R%-13s  ", victim->name );
				if ( IS_IMMORTAL( victim ) && victim->level > 50 )
					send_to_pager_color( "&P(&WImmortal&P)\t", ch );
				else if ( CAN_PKILL( victim ) && victim->pcdata->clan
				          && victim->pcdata->clan->clan_type != CLAN_ORDER
				          && victim->pcdata->clan->clan_type != CLAN_GUILD )
						{
							ch_printf_color( ch, "%-18s\t", victim->pcdata->clan->badge );
						}
				else if (xIS_SET(ch->act, PLR_OUTCAST))
					send_to_char_color( "&c(&zOutcast&c) ", ch );
				else if ( CAN_PKILL( victim ) )
					send_to_pager_color( "(&wRonin&P)\t", ch );
				else
					send_to_pager( "\t\t\t", ch );
				pager_printf_color( ch, "&P%s\n\r", victim->in_room->name );
			}
		if ( !found )
			send_to_char( "None\n\r", ch );
	}
	else
	{
		found = FALSE;
		for ( victim = first_char; victim; victim = victim->next )
			if ( victim->in_room
			     && victim->in_room->area == ch->in_room->area
			     && ( !IS_NPC(victim)
			          && !IS_HC(victim) )
			     && !IS_AFFECTED(victim, AFF_HIDE)
			     && !IS_AFFECTED(victim, AFF_SNEAK)
			     && can_see( ch, victim )
			     && is_name( arg, victim->name ) )
			{
				found = TRUE;
				pager_printf( ch, "%-28s %s\n\r",
				              PERS(victim, ch), victim->in_room->name );
				break;
			}
		if ( !found )
			act( AT_PLAIN, "You didn't find any $T.", ch, NULL, arg, TO_CHAR );
	}

	sysdata.outBytesFlag = LOGBOUTNORM;
	return ;
}




void do_consider( CHAR_DATA *ch, char *argument )
{
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;
	char *msg;
	int diff;

	one_argument( argument, arg );

	if ( arg[0] == '\0' )
	{
		send_to_char( "Consider killing whom?\n\r", ch );
		return ;
	}

	if ( ( victim = get_char_room( ch, arg ) ) == NULL )
	{
		send_to_char( "They're not here.\n\r", ch );
		return ;
	}
	if ( victim == ch )
	{
		send_to_char( "You decide you're pretty sure you could take yourself in a fight.\n\r", ch );
		return ;
	}
	diff = victim->level - ch->level;

	if ( diff <= -10 )
		msg = "You are far more experienced than $N.";
	else if ( diff <= -5 )
		msg = "$N is not nearly as experienced as you.";
	else if ( diff <= -2 )
		msg = "You are more experienced than $N.";
	else if ( diff <= 1 )
		msg = "You are just about as experienced as $N.";
	else if ( diff <= 4 )
		msg = "You are not nearly as experienced as $N.";
	else if ( diff <= 9 )
		msg = "$N is far more experienced than you!";
	else
		msg = "$N would make a great teacher for you!";
	act( AT_CONSIDER, msg, ch, NULL, victim, TO_CHAR );

	diff = (int) (victim->max_hit - ch->max_hit) / 6;

	if ( diff <= -200)
		msg = "$N looks like a feather!";
	else if ( diff <= -150)
		msg = "You could kill $N with your hands tied!";
	else if ( diff <= -100)
		msg = "Hey! Where'd $N go?";
	else if ( diff <= -50)
		msg = "$N is a wimp.";
	else if ( diff <= 0)
		msg = "$N looks weaker than you.";
	else if ( diff <= 50)
		msg = "$N looks about as strong as you.";
	else if ( diff <= 100)
		msg = "It would take a bit of luck...";
	else if ( diff <= 150)
		msg = "It would take a lot of luck, and equipment!";
	else if ( diff <= 200)
		msg = "Why don't you dig a grave for yourself first?";
	else
		msg = "$N is built like a TANK!";
	act( AT_CONSIDER, msg, ch, NULL, victim, TO_CHAR );

	return ;
}

#define STR 1
#define SPD 2
#define INTEL 3
#define CON 4

#define MAX_STR_TRAINING 11
#define MAX_SPD_TRAINING 9
#define MAX_INT_TRAINING 13
#define MAX_CON_TRAINING 8

char * const strTrainUser [MAX_STR_TRAINING] =
    {
        "You do some push-ups.",
        "You do some sit-ups.",
        "You lift some weights.",
        "You bench press a car.",
        "You hurl some boulders around.",
        "You do some one-armed pushups.",
        "You do some pushups on one finger.",
        "You grab a slab of granite and try to crush it into powder.",
        "You attempt to squeeze water out of a rock.",
        "You try to punch through a solid rock wall.",
        "You try to move a mountain."
    };
char * const strTrainRoom [MAX_STR_TRAINING] =
    {
        "$n does some push-ups.",
        "$n does some sit-ups.",
        "$n lifts some weights.",
        "$n bench presses a car.",
        "$n hurls some boulders around.",
        "$n does some one-armed pushups.",
        "$n does some pushups on one finger.",
        "$n grabs a slab of granite and tries to crush it into powder.",
        "$n attempts to squeeze water out of a rock.",
        "$n tries to punch through a solid rock wall.",
        "$n tries to move a mountain."
    };
char * const spdTrainUser [MAX_SPD_TRAINING] =
    {
        "You start running some laps.",
        "You speed around on your tricycle.",
        "You attempt to break the sound barrier while running.",
        "You try to run as fast as light.",
        "You jog backwards all the way around the room.",
        "You attempt to juggle five scouters.",
        "You try to circle rats to make them dizzy.",
        "You throw a deck of cards up into the air and try to catch them all before they hit the ground.",
        "You start back flipping around the room."
    };
char * const spdTrainRoom [MAX_SPD_TRAINING] =
    {
        "$n starts running some laps.",
        "$n starts speeding around on $s tricycle.",
        "$n attempts to break the sound barrier while running.",
        "$n trys to run as fast as light.",
        "$n jogs backwards all the way around the room.",
        "$n attempts to juggle five scouters.",
        "$n trys to circle rats to make them dizzy.",
        "$n throws a deck of cards up into the air and tries to catch them all before they hit the ground.",
        "$n starts back flipping around the room."
    };
char * const intTrainUser [MAX_INT_TRAINING] =
    {
        "You sit down and peacefully meditate.",
        "You sit down and read a good book.",
        "You sit down and contemplate the meaning of life.",
        "You sit down and think, 'If a tree falls in the forest and there is nobody around, does it make a sound?'",
        "You attempt complex math without a calculator.",
        "You try to mentally decipher the human genome.",
        "You attempt to figure out whether the chicken or the egg came first.",
        "You try to name all of the elements... backwards.",
        "You sit down and think 'Why do we drive on parkways and park in driveways?'",
        "You ponder 'Why is one pair of pants, still called a pair?'",
        "You try to bend a spoon with your mind.",
        "You realize that there is no spoon.",
        "You practice telling corny jokes.  Maybe you will get as good as King Kai someday."
    };
char * const intTrainRoom [MAX_INT_TRAINING] =
    {
        "$n sits down and peacefully meditates.",
        "$n sits down and reads a good book.",
        "$n sits down and contemplates the meaning of life.",
        "$n sits down and thinks, 'If a tree falls in the forest and there is nobody around, does it make a sound?'",
        "$n attempts complex math without a calculator.",
        "$n tries to mentally decipher the human genome.",
        "$n attempts to figure out whether the chicken or the egg came first.",
        "$n tries to name all of the elements... backwards.",
        "$n sits down and thinks 'Why do we drive on parkways and park in driveways?'",
        "$n ponders 'Why is one pair of pants, still called a pair?'",
        "$n tries to bend a spoon with $s mind.",
        "$n realizes that there is no spoon.",
        "$n practices telling corny jokes.  Maybe $e will get as good as King Kai someday."
    };
char * const conTrainUser [MAX_CON_TRAINING] =
    {
        "You start doing some jumping jacks.",
        "You start working out to your favorite Tai-Bo video.",
        "You let a gang of thugs pummel you for a bit.",
        "You try to run the marathon.",
        "You attempt to stomach a bad movie.",
        "You repeatedly overwork your gag reflex.",
        "You ask another fighter to throw a car at you.",
        "You crank the gravity up to double what you can handle."
    };
char * const conTrainRoom [MAX_CON_TRAINING] =
    {
        "$n starts doing some jumping jacks.",
        "$n starts working out to $s favorite Tai-Bo video."
        "$n lets a gang of thugs pummel $m for a bit.",
        "$n tries to run the marathon.",
        "$n attempts to stomach a bad movie.",
        "$n repeatedly overworks $s gag reflex.",
        "$n asks another fighter to throw a car at $m.",
        "$n cranks the gravity up to double what $e can handle."
    };

bool gTrainSuccess(CHAR_DATA *ch, int stat, sh_int *tAbility)
{
	char buf[MAX_STRING_LENGTH];
	float xp_mod = 0;
	long double xp_gain = 0;
	int increase = 0;


	increase = number_range(1, 3);
	*tAbility += increase;
	if (xIS_SET(ch->in_room->room_flags, ROOM_TIME_CHAMBER)
	    && number_range(1, 100) < 35)
	{
		switch ( number_range( 1, 4 ) )
		{
			case 1:
				increase *= 1;
				break;

			case 2:
				increase *= 1;
				break;

			case 3:
				increase *= 2;
				break;

			case 4:
				increase *= 3;
				break;
		}
	}

	if (increase >= 2)
	{
		ch->hit--;
	}

	ch->mana -= URANGE(0, ch->mana * 0.005, ch->mana);

	if (ch->race != 6)
	{
		if (is_saiyan(ch)) /* Saiyan */
			xp_mod = 0.66;
		else if (is_namek(ch)) /* Namek */
			xp_mod = 0.665;
		else if (is_hb_hb(ch)) /* Halfbreed-hb */
                        xp_mod = 0.69;
		else if (is_hb(ch)) /* Halfbreed */
			xp_mod = 0.68;
		else				/* Everyone Else */
			xp_mod = 0.67;
	}
	else
	{
		if (ch->pcdata->absorb_pl_mod == 0) /* Saiyan */
			xp_mod = 0.66;
		else if (ch->pcdata->absorb_pl_mod == 3) /* Namek */
			xp_mod = 0.665;
		else if (ch->pcdata->absorb_pl_mod == 2) /* Halfbreed */
			xp_mod = 0.68;
		else if (ch->pcdata->absorb_pl_mod == 6)
			xp_mod = 0.65;
		else								/* Everyone Else */
			xp_mod = 0.67;
	}

	xp_mod = (float) xp_mod - 0.01;
	xp_gain = (long double) increase / 100 * pow(ch->pl, xp_mod);

	/* PL Gains cut if player is powered up */

	if (ch->exp != ch->pl && ch->exp < ch->pl)
	{
		int pl_exp = 0;

		pl_exp = (ch->pl / ch->exp);
		xp_gain = xp_gain - ( (long double) pl_exp * 0.025 * xp_gain );
	}

	sprintf( buf, "Your power level increases by %s points.", num_punct(xp_gain) );
	act( AT_HIT, buf, ch, NULL, NULL, TO_CHAR );

	gain_exp(ch, xp_gain);

	if (*tAbility >= 100)
		return TRUE;
	else
		return FALSE;
}

void do_train( CHAR_DATA *ch, char *argument )
{
	char buf[MAX_STRING_LENGTH];
	char tBuf[MAX_STRING_LENGTH];
	char arg1[MAX_INPUT_LENGTH];
	sh_int *tAbility;
	sh_int *pAbility;
	sh_int *permTstat;
	char *pOutput;
	int stat;
	bool checkLearn = FALSE;
	int minLF;
	int trainTime = 0;
	int gravLevel = 0;
	int activity = 0;

	/* Lifeforce to quit training at */
	minLF = 5;

	if ( IS_NPC(ch) )
		return ;

	argument = one_argument( argument, arg1 );

	if (is_android(ch) || is_superandroid(ch) )
	{
		send_to_char( "Androids can't learn anything by training like that.\n\r", ch );
		return ;
	}

	if (!xIS_SET((ch)->in_room->room_flags, ROOM_GRAV))
	{
		send_to_char( "Hmm... I don't see a grav training room around here...\n\r", ch );
		return ;
	}

	if (IS_IMMORTAL(ch))
	{
		sprintf( buf, "Administrators don't need to train.\n\r");
		send_to_char(buf, ch);
		return ;
	}

	if (ch->pcdata->tStat == 0)
	{
		if (ch->substate != SUB_TIMER_DO_ABORT)
		{
			/* -------- DO NON-ABORTING STUFF -------- */
			if ( arg1[0] == '\0')
			{
				sprintf( buf, "Syntax: gravtrain <attribute> <minutes>\n\r");
				send_to_char(buf, ch);
				pager_printf(ch, "You have %d training points and %d extended training points\n\r",
				             ch->train, ch->pcdata->xTrain);
				return ;
			}
			if ( argument[0] == '\0')
			{
				sprintf( buf, "Syntax: gravtrain <attribute> <minutes>\n\rYou have %d training sessions left.\n\r", ch->train );
				send_to_char(buf, ch);
				return ;
			}
			if (!is_number(argument))
			{
				pager_printf(ch, "Those aren't minutes...\n\r");
				return ;
			}

			trainTime = atoi(argument);

			if (trainTime <= 0)
			{
				pager_printf(ch, "Last time I checked, time travel wasn't an option...\n\r");
				return ;
			}
			if (trainTime >= 480)
			{
				pager_printf(ch, "Training for more than 8 hours at a time isn't a good idea.\n\r");
				return ;
			}
		}

		/* -------- END NON-ABORTING STUFF -------- */
	}


	if ( !str_cmp( arg1, "str" ) || !str_cmp( arg1, "strength" )
	     || ch->pcdata->tStat == STR)
	{
		stat	= ch->pcdata->tStat = STR;
		tAbility = &ch->pcdata->tStr;
		pAbility = &ch->perm_str;
		permTstat = &ch->pcdata->permTstr;
		pOutput = "strength";
	}
	else if ( !str_cmp( arg1, "int" ) || !str_cmp( arg1, "intelligence" )
	          || ch->pcdata->tStat == INTEL)
	{
		stat	= ch->pcdata->tStat = INTEL;
		tAbility = &ch->pcdata->tInt;
		pAbility = &ch->perm_int;
		permTstat = &ch->pcdata->permTint;
		pOutput = "intelligence";
	}
	else if ( !str_cmp( arg1, "spd" ) || !str_cmp( arg1, "speed" )
	          || ch->pcdata->tStat == SPD)
	{
		stat	= ch->pcdata->tStat = SPD;
		tAbility = &ch->pcdata->tSpd;
		pAbility = &ch->perm_dex;
		permTstat = &ch->pcdata->permTspd;
		pOutput = "speed";
	}
	else if ( !str_cmp( arg1, "con" ) || !str_cmp( arg1, "constitution" )
	          || ch->pcdata->tStat == CON )
	{
		stat	= ch->pcdata->tStat = CON;
		tAbility = &ch->pcdata->tCon;
		pAbility = &ch->perm_con;
		permTstat = &ch->pcdata->permTcon;
		pOutput = "constitution";
	}
	else
	{
		pager_printf(ch, "You have %d training points and %d extended training points\n\r",
		             ch->train, ch->pcdata->xTrain);
		ch->pcdata->tStat = 0;
		ch->pcdata->tRounds = 0;
		return ;
	}

	if ( *permTstat >= 100 && *tAbility >= 99 && ch->pcdata->xTrain < 1)
	{
		act(AT_RED, "Your $T is already at maximum.", ch, NULL, pOutput, TO_CHAR );
		ch->pcdata->tStat = 0;
		ch->pcdata->tRounds = 0;
		return ;
	}

	if ( ch->train < 1 && ch->pcdata->xTrain < 1 && *tAbility >= 99)
	{
		pager_printf(ch, "You don't have enough training points to work on your %s anymore.\n\r", pOutput );
		ch->pcdata->tStat = 0;
		ch->pcdata->tRounds = 0;
		return ;
	}


	sysdata.outBytesFlag = LOGBOUTCOMBAT;

	switch (ch->substate)
	{
		default:
			bug( "do_train: illegal substate (%s)", ch->name );
			sysdata.outBytesFlag = LOGBOUTNORM;
			return ;

		case SUB_NONE:
			ch->pcdata->tRounds = trainTime;
			if (ch->pcdata->tRounds <= 0)
			{
				ch->pcdata->tRounds = 0;
				break;
			}
			if (*tAbility >= 100)
			{
				checkLearn = TRUE;
				break;
			}
			if (ch->hit <= minLF)
				break;

			gravLevel = ((get_curr_str(ch) + get_curr_dex(ch) + get_curr_int(ch) + get_curr_con(ch)) - 40) * 5;
			pager_printf(ch, "&GYou turn the dial to %d times gravity and begin training your %s.\n\r",
			             gravLevel, pOutput);
			act(AT_SKILL, "$n starts training $s $T.", ch, NULL, pOutput, TO_ROOM );
			WAIT_STATE( ch, 2 * PULSE_VIOLENCE );

			if ( (checkLearn = gTrainSuccess(ch, stat, tAbility)) )
				break;

			/*		    WAIT_STATE( ch, 6 );*/
			add_timer( ch, TIMER_DO_FUN, 1, do_train, 1);
			sysdata.outBytesFlag = LOGBOUTNORM;
			return ;

		case 1:
			ch->pcdata->tRounds--;
			if (ch->pcdata->tRounds <= 0)
			{
				ch->pcdata->tRounds = 0;
				act(AT_SKILL, "You stop training your $T.", ch, NULL, pOutput, TO_CHAR );
				act(AT_SKILL, "$n stops training $s $T.", ch, NULL, pOutput, TO_ROOM );
				break;
			}
			if (*tAbility >= 100)
			{
				checkLearn = TRUE;
				break;
			}
			if (ch->hit <= minLF)
			{
				act(AT_SKILL, "You almost pass out from training your $T to hard.", ch, NULL, pOutput, TO_CHAR );
				act(AT_SKILL, "$n almost passes out from training $s $T to hard.", ch, NULL, pOutput, TO_ROOM );
				break;
			}

			if (number_range(1, 100) < 25)
			{
				pager_printf(ch, "&G");
				switch (ch->pcdata->tStat)
				{
					default:
						activity = -1;
						break;
					case 1:
						activity = number_range(0, MAX_STR_TRAINING - 1);
						pager_printf(ch, strTrainUser[activity]);
						act(AT_SKILL, strTrainRoom[activity], ch, NULL, pOutput, TO_ROOM );
						break;
					case 2:
						activity = number_range(0, MAX_SPD_TRAINING - 1);
						pager_printf(ch, spdTrainUser[activity]);
						act(AT_SKILL, spdTrainRoom[activity], ch, NULL, pOutput, TO_ROOM );
						break;
					case 3:
						activity = number_range(0, MAX_INT_TRAINING - 1);
						pager_printf(ch, intTrainUser[activity]);
						act(AT_SKILL, intTrainRoom[activity], ch, NULL, pOutput, TO_ROOM );
						break;
					case 4:
						activity = number_range(0, MAX_CON_TRAINING - 1);
						pager_printf(ch, conTrainUser[activity]);
						act(AT_SKILL, conTrainRoom[activity], ch, NULL, pOutput, TO_ROOM );
						break;
				}
				pager_printf(ch, "&w\n\r");
			}

			if ( (checkLearn = gTrainSuccess(ch, stat, tAbility)) )
				break;

			add_timer( ch, TIMER_DO_FUN, 1, do_train, 2);
			sysdata.outBytesFlag = LOGBOUTNORM;
			return ;

		case 2:
			ch->pcdata->tRounds--;
			if (ch->pcdata->tRounds <= 0)
			{
				ch->pcdata->tRounds = 0;
				act(AT_SKILL, "You take a break and stop training your $T.", ch, NULL, pOutput, TO_CHAR );
				act(AT_SKILL, "$n takes a break and stops training $s $T.", ch, NULL, pOutput, TO_ROOM );
				break;
			}
			if (*tAbility >= 100)
			{
				checkLearn = TRUE;
				break;
			}
			if (ch->hit <= minLF)
			{
				act(AT_SKILL, "You almost pass out from training your $T to hard.", ch, NULL, pOutput, TO_CHAR );
				act(AT_SKILL, "$n almost passes out from training $s $T to hard.", ch, NULL, pOutput, TO_ROOM );
				break;
			}

			if (number_range(1, 100) < 25)
			{
				pager_printf(ch, "&G");
				switch (ch->pcdata->tStat)
				{
					default:
						activity = -1;
						break;
					case 1:
						activity = number_range(0, MAX_STR_TRAINING - 1);
						pager_printf(ch, strTrainUser[activity]);
						act(AT_SKILL, strTrainRoom[activity], ch, NULL, pOutput, TO_ROOM );
						break;
					case 2:
						activity = number_range(0, MAX_SPD_TRAINING - 1);
						pager_printf(ch, spdTrainUser[activity]);
						act(AT_SKILL, spdTrainRoom[activity], ch, NULL, pOutput, TO_ROOM );
						break;
					case 3:
						activity = number_range(0, MAX_INT_TRAINING - 1);
						pager_printf(ch, intTrainUser[activity]);
						act(AT_SKILL, intTrainRoom[activity], ch, NULL, pOutput, TO_ROOM );
						break;
					case 4:
						activity = number_range(0, MAX_CON_TRAINING - 1);
						pager_printf(ch, conTrainUser[activity]);
						act(AT_SKILL, conTrainRoom[activity], ch, NULL, pOutput, TO_ROOM );
						break;
				}
				pager_printf(ch, "&w\n\r");
			}

			if ( (checkLearn = gTrainSuccess(ch, stat, tAbility)) )
				break;

			add_timer( ch, TIMER_DO_FUN, 1, do_train, 1);
			sysdata.outBytesFlag = LOGBOUTNORM;
			return ;

		case SUB_TIMER_DO_ABORT:
			ch->pcdata->tRounds = 0;
			act(AT_SKILL, "You stop training your $T.", ch, NULL, pOutput, TO_CHAR );
			act(AT_SKILL, "$n stops training $s $T.", ch, NULL, pOutput, TO_ROOM );
			break;
	}

	ch->pcdata->tStat = 0;
	ch->pcdata->tRounds = 0;

	if ( *permTstat >= 100 && ch->pcdata->xTrain < 1 && *tAbility >= 99)
	{
		act(AT_SKILL, "Your $T has reached it's peak for now.", ch, NULL, pOutput, TO_CHAR );
		*tAbility = 99;
		sysdata.outBytesFlag = LOGBOUTNORM;
		return ;
	}

	if (checkLearn)
	{
		if (ch->train >= 1 && *permTstat < 100)
		{
			ch->train -= 1;
			*tAbility = 0;
			*pAbility += 1;
			*permTstat += 1;
			sprintf( tBuf, "Your $T increases to %d!", *pAbility );
			/*		act(AT_RED, "Your $T increases!", ch, NULL, pOutput, TO_CHAR );*/
			act(AT_RED, tBuf, ch, NULL, pOutput, TO_CHAR );
			act(AT_RED, "$n's $T increases!", ch, NULL, pOutput, TO_ROOM );
			act(AT_SKILL, "You take a break and stop training your $T.", ch, NULL, pOutput, TO_CHAR );
			act(AT_SKILL, "$n takes a break and stops training $s $T.", ch, NULL, pOutput, TO_ROOM );
		}
		else if (ch->pcdata->xTrain >= 1)
		{
			ch->pcdata->xTrain -= 1;
			*tAbility = 0;
			*pAbility += 1;
			sprintf( tBuf, "Your $T increases to %d!", *pAbility );
			act(AT_RED, tBuf, ch, NULL, pOutput, TO_CHAR );
			act(AT_RED, "$n's $T increases!", ch, NULL, pOutput, TO_ROOM );
			act(AT_SKILL, "You take a break and stop training your $T.", ch, NULL, pOutput, TO_CHAR );
			act(AT_SKILL, "$n takes a break and stops training $s $T.", ch, NULL, pOutput, TO_ROOM );
		}
		else
		{
			act(AT_SKILL, "Your $T has reached its peak for now.", ch, NULL, pOutput, TO_CHAR );
			*tAbility = 99;
		}
	}

	sysdata.outBytesFlag = LOGBOUTNORM;
	return ;
}

#undef STR
#undef SPD
#undef INT
#undef CON
#undef MAX_STR_TRAINING
#undef MAX_SPD_TRAINING
#undef MAX_INT_TRAINING
#undef MAX_CON_TRAINING

/*
 * Place any skill types you don't want them to be able to practice
 * normally in this list.  Separate each with a space.
 * (Uses an is_name check). -- Altrag
 */
#define CANT_PRAC "Tongue"

void do_practice( CHAR_DATA *ch, char *argument )
{
	char buf[MAX_STRING_LENGTH];
	int  sn;
        bool bioPrac = FALSE;
        int  ii      = 0;

	if ( IS_NPC(ch) )
		return ;

	if (is_android(ch) || is_superandroid(ch))
	{
		send_to_char( "Androids can't practice things like that.\n\r", ch );
		return ;
	}

	if( (!str_cmp(argument,"mystic") || !str_prefix(argument,"mystic"))
	     && !is_kaio(ch) && argument[0] != '\0' )
	{
	    ch_printf(ch,"You can't.\n\r");
	    return;
	}

	/*
	    if ( ch->level < 2 )
	    {
		send_to_char(
		"You must be second level to practice.  Seek out monsters to kill!\n\r",
		    ch );
		return;
	    }
	*/
	if ( argument[0] == '\0' )
	{
		int	col;
		sh_int	lasttype, cnt;

		col = cnt = 0;
		lasttype = SKILL_SPELL;
		set_pager_color( AT_MAGIC, ch );
		for ( sn = 0; sn < top_sn; sn++ )
		{
			if ( !skill_table[sn]->name )
				break;

			if (skill_table[sn]->type == SKILL_SPELL || skill_table[sn]->type == SKILL_WEAPON
			    || skill_table[sn]->type == SKILL_TONGUE )
				continue;

			if ( strcmp(skill_table[sn]->name, "reserved") == 0
			     && ( IS_IMMORTAL(ch) || CAN_CAST(ch) ) )
			{
				if ( col % 3 != 0 )
					send_to_pager( "\n\r", ch );
				set_pager_color( AT_MAGIC, ch );
				send_to_pager_color(
				    " ----------------------------------[&CSpells&B]----------------------------------\n\r", ch);
				col = 0;
			}
			if ( skill_table[sn]->type != lasttype )
			{
				if ( !cnt )
					send_to_pager( "\n\r", ch );
				else
					if ( col % 3 != 0 )
						send_to_pager( "\n\r", ch );
				set_pager_color( AT_MAGIC, ch );
				if (skill_table[sn]->type == SKILL_SKILL)
					pager_printf_color( ch,
					                    " ------------------------------------&CSkills&B-----------------------------------\n\r");
				else if (skill_table[sn]->type == SKILL_ABILITY)
					pager_printf_color( ch,
					                    " ----------------------------------&CAbilities&B----------------------------------\n\r");
				else
					pager_printf_color( ch,
					                    " -----------------------------------&C%ss&B-----------------------------------\n\r",
					                    skill_tname[skill_table[sn]->type]);
				col = cnt = 0;
			}
			lasttype = skill_table[sn]->type;

			if (( skill_table[sn]->guild != CLASS_NONE
			      && ( !IS_GUILDED(ch)
			           || (ch->pcdata->clan->class != skill_table[sn]->guild) ) ) )
				continue;

			if ( ( ch->level < skill_table[sn]->min_level[ch->class]
			       && skill_table[sn]->skill_level[ch->class] > ch->exp )
			     || ( skill_table[sn]->min_level[ch->class] == 0 )
			     || (skill_table[sn]->skill_level[ch->class] == 0)
			     || (ch->pcdata->learned[sn] == 0)
			   )
				continue;

			if ( ch->pcdata->learned[sn] <= 0
			     && SPELL_FLAG(skill_table[sn], SF_SECRETSKILL) )
				continue;

			++cnt;
			set_pager_color( AT_MAGIC, ch );
			pager_printf( ch, "%20.20s", skill_table[sn]->name );
			if ( ch->pcdata->learned[sn] > 0 )
				set_pager_color( AT_SCORE, ch );
			pager_printf( ch, " %3.0f%% ", ch->pcdata->learned[sn] );
			if ( ++col % 3 == 0 )
				send_to_pager( "\n\r", ch );
		}

		if ( col % 3 != 0 )
			send_to_pager( "\n\r", ch );
		set_pager_color( AT_MAGIC, ch );
		send_to_pager_color(
		    " -----------------------------------------------------------------------------\n\r", ch);
		/*
			pager_printf( ch, "You have %d practice sessions left.\n\r", ch->practice );
		*/
		pager_printf( ch, "You have %d training sessions left.\n\r", ch->train );
	}
	else
	{
		CHAR_DATA *mob;
		int adept;
		bool can_prac = TRUE;

		if ( !IS_AWAKE(ch) )
		{
			send_to_char( "In your dreams, or what?\n\r", ch );
			return ;
		}

		for ( mob = ch->in_room->first_person; mob; mob = mob->next_in_room )
			if ( IS_NPC(mob) && xIS_SET(mob->act, ACT_PRACTICE) )
				break;

		if ( !mob )
		{
			send_to_char( "You can't do that here.\n\r", ch );
			return ;
		}

		if (IS_IMMORTAL(ch))
		{
			sprintf( buf, "Administrators don't need to practice.\n\r");
			send_to_char(buf, ch);
			return ;
		}
		/*
			if ( ch->practice <= 0 )
			{
			    act( AT_TELL, "$n tells you 'You must earn some more practice sessions.'",
				mob, NULL, ch, TO_VICT );
			    return;
			}
		*/
		sn = skill_lookup( argument );

		if (sn == -1)
		{
			act( AT_TELL, "$n tells you 'That is not a skill.'", mob, NULL, ch, TO_VICT );
			return ;
		}

		if (skill_table[sn]->min_level[ch->class] == 0
		    || skill_table[sn]->skill_level[ch->class] == 0
		    || (sn == gsn_wss && !demonranked(ch)) )
		{
			act( AT_TELL, "$n tells you 'You can not learn that.'",
			     mob, NULL, ch, TO_VICT );
			return ;
		}

		/*	    ( ( ch->level < skill_table[sn]->min_level[ch->class]
			    	&& skill_table[sn]->skill_level[ch->class] > ch->exp )
			    || (  skill_table[sn]->min_level[ch->class] == 0 )
			    || (skill_table[sn]->skill_level[ch->class] == 0))
		*/
		if ( can_prac
		     && ( ( sn == -1 )
		          || ( !IS_NPC(ch)
		               && ch->exp < skill_table[sn]->skill_level[ch->class]
		               && ch->level < skill_table[sn]->min_level[ch->class]
		               /* OUT FOR THIS PORT -SHADDAI
		                                   &&  ch->exp < skill_table[sn]->race_level[ch->race]
		               */
		             )
		        )
		   )
		{
			act( AT_TELL, "$n tells you 'You're not ready to learn that yet...'",
			     mob, NULL, ch, TO_VICT );
			return ;
		}

		if ( is_name( skill_tname[skill_table[sn]->type], CANT_PRAC ) )
		{
			act( AT_TELL, "$n tells you 'I do not know how to teach that.'",
			     mob, NULL, ch, TO_VICT );
			return ;
		}

		/*
		 * Skill requires a special teacher
		 */
		if ( skill_table[sn]->teachers && skill_table[sn]->teachers[0] != '\0' )
		{
			sprintf( buf, "%d", mob->pIndexData->vnum );
			if ( !is_name( buf, skill_table[sn]->teachers ) )
			{
				act( AT_TELL, "$n tells you, 'I do not know how to teach that.'",
				     mob, NULL, ch, TO_VICT );
				return ;
			}
		}

		/*
		 * Guild checks - right now, cant practice guild skills - done on
		 * induct/outcast
		 */
		/*
			if ( !IS_NPC(ch) && !IS_GUILDED(ch)
			&&    skill_table[sn]->guild != CLASS_NONE)
			{
			    act( AT_TELL, "$n tells you 'Only guild members can use that..'"
				mob, NULL, ch, TO_VICT );
			    return;
			}

			if ( !IS_NPC(ch) && skill_table[sn]->guild != CLASS_NONE
			     && ch->pcdata->clan->class != skill_table[sn]->guild )
			{
			    act( AT_TELL, "$n tells you 'That can not be used by your guild.'"
				mob, NULL, ch, TO_VICT );
			    return;
			}
		*/
		if ( !IS_NPC(ch) && skill_table[sn]->guild != CLASS_NONE)
		{
			act( AT_TELL, "$n tells you 'That is only for members of guilds...'",
			     mob, NULL, ch, TO_VICT );
			return ;
		}

		/*
		 * Disabled for now
		if ( mob->level < skill_table[sn]->skill_level[ch->class]
		||   mob->level < skill_table[sn]->skill_level[mob->class] )
		{
		    act( AT_TELL, "$n tells you 'You must seek another to teach you that...'",
			mob, NULL, ch, TO_VICT );
		    return;
		}
		 */

                if( ch->race == race_lookup( "bio-android" ) )
                {
                  if( sn == gsn_semiperfect
                   || sn == gsn_perfect
                   || sn == gsn_ultraperfect )
                  {
                    bioPrac = FALSE;
                  }
		  else if( sn == gsn_ki_burst
		   || sn == gsn_clone )
		  {
		    bioPrac = TRUE;
		  }
                  else
                  {
                    bioPrac = TRUE;
                    for( ii = 0; ii < MAX_RACE; ii++ )
                    {
                      if( ii == ch->class )
                        continue;
                      else
                      {
                        if( skill_table[sn]->skill_level[ii] > 0 )
                        {
                          bioPrac = FALSE;
                        }
                      }
                    }
                  }
                }
		if (ch->race == 6 && skill_table[sn]->skill_level[ch->class] >= 100000
                 && !bioPrac)
		{
			send_to_char( "You will have to absorb that.\n\r", ch );
			return ;
		}

		adept = class_table[ch->class]->skill_adept * 0.2;

		if ( ch->pcdata->learned[sn] > 0 )
		{
			sprintf( buf, "$n tells you, 'I've taught you everything I can about %s.'",
			         skill_table[sn]->name );
			act( AT_TELL, buf, mob, NULL, ch, TO_VICT );
			act( AT_TELL, "$n tells you, 'You'll have to practice it on your own now...'",
			     mob, NULL, ch, TO_VICT );
		}
		else
		{
			//	    ch->practice--;
			ch->pcdata->learned[sn] = START_ADEPT;
                        if( sn == gsn_preservation )
                        {
                          ch->pcdata->learned[sn] = 95;
                        }
			if( sn == gsn_regeneration )
                        {
                          ch->pcdata->learned[sn] = 95;
                        }
			if( sn == gsn_kaiocreate )
			{
			  ch->pcdata->learned[sn] = 95;
			}
			if( sn == gsn_demonweapon )
                        {
                          ch->pcdata->learned[sn] = 95;
                        }
			if( sn == gsn_wss )
                        {
                          ch->pcdata->learned[sn] = 95;
                        }
			if( sn == gsn_clone )
                        {
                          ch->pcdata->learned[sn] = 100;
                        }
			if( sn == gsn_monkey_gun )
			{
			  ch->pcdata->learned[sn] = 95;
			}
			act( AT_ACTION, "You practice $T.",
			     ch, NULL, skill_table[sn]->name, TO_CHAR );
			act( AT_ACTION, "$n practices $T.",
			     ch, NULL, skill_table[sn]->name, TO_ROOM );
                        if( sn == gsn_preservation )
                        {
			  act( AT_TELL,
			       "$n tells you. 'That's as good as it gets!'",
			       mob, NULL, ch, TO_VICT );
                        }
			else if( sn == gsn_regeneration )
                        {
                          act( AT_TELL,
                               "$n tells you. 'That's as good as it gets!'",
                               mob, NULL, ch, TO_VICT );
                        }
                        else
                        {
			  act( AT_TELL,
			       "$n tells you. 'You'll have to practice it on your own now...'",
			       mob, NULL, ch, TO_VICT );
                        }
		}
	}
	return ;
}

void do_wimpy( CHAR_DATA *ch, char *argument )
{
	char arg[MAX_INPUT_LENGTH];
	int wimpy;

	set_char_color( AT_YELLOW, ch );
	one_argument( argument, arg );
	if ( !str_cmp( arg, "max" ) )
	{
		if ( IS_PKILL( ch ) )
			wimpy = (int) ch->max_hit / 2.25;
		else
			wimpy = (int) ch->max_hit / 1.2;
	}
	else
		if ( arg[0] == '\0' )
			wimpy = (int) ch->max_hit / 5;
		else
			wimpy = atoi( arg );

	if ( wimpy < 0 )
	{
		send_to_char( "Your courage exceeds your wisdom.\n\r", ch );
		return ;
	}
	if ( IS_PKILL( ch ) && wimpy > (int) ch->max_hit / 2.25 )
	{
		send_to_char( "Such cowardice ill becomes you.\n\r", ch );
		return ;
	}
	else if ( wimpy > (int) ch->max_hit / 1.2 )
	{
		send_to_char( "Such cowardice ill becomes you.\n\r", ch );
		return ;
	}
	ch->wimpy	= wimpy;
	ch_printf( ch, "Wimpy set to %d life force.\n\r", wimpy );
	return ;
}



void do_password( CHAR_DATA *ch, char *argument )
{
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	char arg3[MAX_INPUT_LENGTH];
	char log_buf[MAX_STRING_LENGTH];
	char *pArg;
	char *pwdnew;
	char *p;
	char cEnd;

	if ( IS_NPC(ch) )
		return ;

	/*
	 * Can't use one_argument here because it smashes case.
	 * So we just steal all its code.  Bleagh.
	 */
	pArg = arg1;
	while ( isspace(*argument) )
		argument++;

	cEnd = ' ';
	if ( *argument == '\'' || *argument == '"' )
		cEnd = *argument++;

	while ( *argument != '\0' )
	{
		if ( *argument == cEnd )
		{
			argument++;
			break;
		}
		*pArg++ = *argument++;
	}
	*pArg = '\0';

	pArg = arg2;
	while ( isspace(*argument) )
		argument++;

	cEnd = ' ';
	if ( *argument == '\'' || *argument == '"' )
		cEnd = *argument++;

	while ( *argument != '\0' )
	{
		if ( *argument == cEnd )
		{
			argument++;
			break;
		}
		*pArg++ = *argument++;
	}
	*pArg = '\0';

	pArg = arg3;
	while ( isspace(*argument) )
		argument++;

	cEnd = ' ';
	if ( *argument == '\'' || *argument == '"' )
		cEnd = *argument++;

	while ( *argument != '\0' )
	{
		if ( *argument == cEnd )
		{
			argument++;
			break;
		}
		*pArg++ = *argument++;
	}
	*pArg = '\0';

	if ( arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0' )
	{
		send_to_char( "Syntax: password <old> <new> <again>.\n\r", ch );
		return ;
	}

	/*
	    if ( strcmp( crypt( arg1, ch->pcdata->pwd ), ch->pcdata->pwd ) )
	    {
		WAIT_STATE( ch, 40 );
		send_to_char( "Wrong password.  Wait 10 seconds.\n\r", ch );
		return;
	    }
	*/

	/* This should stop all the mistyped password problems --Shaddai */
	if( str_cmp( sha256_crypt( arg1 ), ch->pcdata->pwd ) )
	{
		send_to_char("Old password is incorrect, try again.\n\r", ch );
		return ;
	}
	if ( strcmp( arg2, arg3 ))
	{
		send_to_char("New passwords don't match try again.\n\r", ch );
		return ;
	}
	if ( strlen(arg2) < 5 )
	{
		send_to_char(
		    "New password must be at least five characters long.\n\r", ch );
		return ;
	}

	/*
	 * No tilde allowed because of player file format.
	 */
	pwdnew = sha256_crypt( arg2 );
	for ( p = pwdnew; *p != '\0'; p++ )
	{
		if ( *p == '~' )
		{
			send_to_char(
			    "New password not acceptable, try again.\n\r", ch );
			return ;
		}
	}

	DISPOSE( ch->pcdata->pwd );
	ch->pcdata->pwd = str_dup( pwdnew );
	if ( IS_SET(sysdata.save_flags, SV_PASSCHG) )
		save_char_obj( ch );
	if ( ch->desc && ch->desc->host[0] != '\0' )
		sprintf(log_buf, "%s changing password from site %s\n", ch->name,
		        ch->desc->host );
	else
		sprintf(log_buf, "%s changing thier password with no descriptor!", ch->name);
	log_string( log_buf );
	send_to_char( "Ok.\n\r", ch );
	return ;
}



void do_socials( CHAR_DATA *ch, char *argument )
{
	int iHash;
	int col = 0;
	SOCIALTYPE *social;

	set_pager_color( AT_PLAIN, ch );
	for ( iHash = 0; iHash < 27; iHash++ )
		for ( social = social_index[iHash]; social; social = social->next )
		{
			pager_printf( ch, "%-12s", social->name );
			if ( ++col % 6 == 0 )
				send_to_pager( "\n\r", ch );
		}

	if ( col % 6 != 0 )
		send_to_pager( "\n\r", ch );
	return ;
}


void do_commands( CHAR_DATA *ch, char *argument )
{
	int col;
	bool found;
	int hash;
	CMDTYPE *command;

	col = 0;
	set_pager_color( AT_PLAIN, ch );
	if ( argument[0] == '\0' )
	{
		for ( hash = 0; hash < 126; hash++ )
			for ( command = command_hash[hash]; command; command = command->next )
				if ( command->level < LEVEL_HERO
				     && command->level <= get_trust( ch )
				     && (command->name[0] != 'm'
				         || command->name[1] != 'p') )
				{
					pager_printf( ch, "%-12s", command->name );
					if ( ++col % 6 == 0 )
						send_to_pager( "\n\r", ch );
				}
		if ( col % 6 != 0 )
			send_to_pager( "\n\r", ch );
	}
	else
	{
		found = FALSE;
		for ( hash = 0; hash < 126; hash++ )
			for ( command = command_hash[hash]; command; command = command->next )
				if ( command->level < LEVEL_HERO
				     && command->level <= get_trust( ch )
				     && !str_prefix(argument, command->name)
				     && (command->name[0] != 'm'
				         || command->name[1] != 'p') )
				{
					pager_printf( ch, "%-12s", command->name );
					found = TRUE;
					if ( ++col % 6 == 0 )
						send_to_pager( "\n\r", ch );
				}

		if ( col % 6 != 0 )
			send_to_pager( "\n\r", ch );
		if ( !found )
			ch_printf( ch, "No command found under %s.\n\r", argument);
	}
	return ;
}

void do_channels( CHAR_DATA *ch, char *argument )
{
	char arg[MAX_INPUT_LENGTH];
	one_argument( argument, arg );

	if ( IS_NPC( ch ) )
		return ;

	if ( arg[0] == '\0' )
	{
		if ( !IS_NPC(ch) && xIS_SET(ch->act, PLR_SILENCE) )
		{
			set_char_color( AT_GREEN, ch );
			send_to_char( "You are silenced.\n\r", ch );
			return ;
		}

		/* Channels everyone sees regardless of affiliation --Blodkai */
		send_to_char_color( "\n\r &gPublic channels  (severe penalties for abuse)&G:\n\r  ", ch );
		/*
		        if ( IS_IMMORTAL(ch) || ( ch->pcdata->council
			&&   !str_cmp( ch->pcdata->council->name, "Newbie Council" ) ) )
		*/
		ch_printf_color( ch, "%s", !xIS_SET( ch->deaf, CHANNEL_NEWBIE) ?
		                 " &G+NEWBIE"	:
		                 " &g-newbie" );
		ch_printf_color( ch, "%s", !xIS_SET( ch->deaf, CHANNEL_RACETALK ) ?
		                 " &G+RACETALK" :
		                 " &g-racetalk" );
		ch_printf_color( ch, "%s", !xIS_SET( ch->deaf, CHANNEL_CHAT ) ?
		                 " &G+CHAT" :
		                 " &g-chat" );
		ch_printf_color( ch, "%s", !xIS_SET( ch->deaf, CHANNEL_CHAT_OOC ) ?
				 " &G+CHATOOC" :
                                 " &g-chatooc" );
		ch_printf_color( ch, "%s", !xIS_SET( ch->deaf, CHANNEL_AUCTION ) ?
		                 " &G+AUCTION" :
		                 " &g-auction" );
		ch_printf_color( ch, "%s", !xIS_SET( ch->deaf, CHANNEL_TRAFFIC ) ?
		                 " &G+TRAFFIC"	:
		                 " &g-traffic" );
		ch_printf_color( ch, "%s", !xIS_SET( ch->deaf, CHANNEL_QUEST ) ?
		                 " &G+QUEST" :
		                 " &g-quest" );
		ch_printf_color( ch, "%s", !xIS_SET( ch->deaf, CHANNEL_WARTALK ) ?
		                 " &G+WARTALK" :
		                 " &g-wartalk" );
		ch_printf_color( ch, "%s", !xIS_SET( ch->deaf, CHANNEL_MUSIC ) ?
		                 " &G+MUSIC" :
		                 " &g-music" );
		ch_printf_color( ch, "%s", !xIS_SET( ch->deaf, CHANNEL_ASK ) ?
		                 " &G+ASK" :
		                 " &g-ask" );
		ch_printf_color( ch, "%s", !xIS_SET( ch->deaf, CHANNEL_SHOUT ) ?
		                 " &G+SHOUT"	:
		                 " &g-shout" );
		ch_printf_color( ch, "%s", !xIS_SET( ch->deaf, CHANNEL_YELL ) ?
		                 " &G+YELL"	:
		                 " &g-yell" );
		ch_printf_color( ch, "%s", !xIS_SET( ch->deaf, CHANNEL_OOC ) ?
		                 " &G+OOC"	:
		                 " &g-ooc" );
		ch_printf_color( ch, "%s", !xIS_SET( ch->deaf, CHANNEL_INFO ) ?
		                 " &G+INFO"	:
		                 " &g-info" );
		if (IS_HC(ch) || IS_IMMORTAL(ch))
			ch_printf_color( ch, "%s", !xIS_SET( ch->deaf, CHANNEL_HC ) ?
			                 " &G+HARD-CORE"	:
			                 " &g-hard-core" );
		ch_printf_color( ch, "%s", !xIS_SET( ch->deaf, CHANNEL_ROLEPLAY ) ?
		                 " &G+ROLEPLAY"	:
		                 " &g-roleplay" );

		send_to_char_color( "\n\r &gPublic channels  (Not Moderated)&G:\n\r  ", ch );
		ch_printf_color( ch, "%s", !xIS_SET( ch->deaf, CHANNEL_FOS ) ?
		                 " &G+FOS"	:
		                 " &g-fos" );
		/* For organization channels (orders, clans, guilds, councils) */
		send_to_char_color( "\n\r &gPrivate channels (severe penalties for abuse)&G:\n\r ", ch );
		ch_printf_color( ch, "%s", !xIS_SET( ch->deaf, CHANNEL_TELLS ) ?
		                 " &G+TELLS" :
		                 " &g-tells" );
		ch_printf_color( ch, "%s", !xIS_SET( ch->deaf, CHANNEL_WHISPER ) ?
		                 " &G+WHISPER"	:
		                 " &g-whisper" );
		if ( !IS_NPC( ch ) && ch->pcdata->clan )
		{
			if ( ch->pcdata->clan->clan_type == CLAN_ORDER )
				send_to_char_color( !xIS_SET( ch->deaf, CHANNEL_ORDER ) ?
				                    " &G+ORDER"	: " &g-order", ch );

			else if ( ch->pcdata->clan->clan_type == CLAN_GUILD )
				send_to_char_color( !xIS_SET( ch->deaf, CHANNEL_GUILD ) ?
				                    " &G+GUILD"	: " &g-guild", ch );
			else
				send_to_char_color( !xIS_SET( ch->deaf, CHANNEL_CLAN ) ?
				                    " &G+CLAN"	: " &g-clan", ch );
		}
		if ( !IS_NPC( ch ) && ch->pcdata->council )
			ch_printf_color( ch, "%s", !xIS_SET( ch->deaf, CHANNEL_COUNCIL) ?
			                 " &G+COUNCIL"	:
			                 " &g-council" );

		/* Immortal channels */
		if ( IS_IMMORTAL( ch ) )
		{
			send_to_char_color( "\n\r &gImmortal Channels&G:\n\r  ", ch );
			send_to_char_color( !xIS_SET( ch->deaf, CHANNEL_ADMIN ) ?
			                    " &G+ADMIN"	: " &g-admin", ch );
			send_to_char_color( !xIS_SET( ch->deaf, CHANNEL_IMMTALK ) ?
			                    " &G+IMMTALK"	: " &g-immtalk", ch );
			if ( get_trust( ch ) >= sysdata.muse_level )
				send_to_char_color( !xIS_SET( ch->deaf, CHANNEL_HIGHGOD ) ?
				                    " &G+MUSE"	: " &g-muse", ch );
			send_to_char_color( !xIS_SET( ch->deaf, CHANNEL_MONITOR ) ?
			                    " &G+MONITOR"	: " &g-monitor", ch );
			send_to_char_color( !xIS_SET( ch->deaf, CHANNEL_AUTH )	?
			                    " &G+AUTH"	: " &g-auth", ch );
			ch_printf_color( ch, "%s", !xIS_SET( ch->deaf, CHANNEL_AINFO ) ?
			                 " &G+AINFO"	:
			                 " &g-ainfo" );
		}
		if ( get_trust( ch ) >= sysdata.log_level )
		{
			send_to_char_color( !xIS_SET( ch->deaf, CHANNEL_LOG ) ?
			                    " &G+LOG"	: " &g-log", ch);
			send_to_char_color( !xIS_SET( ch->deaf, CHANNEL_BUILD) ?
			                    " &G+BUILD"	: " &g-build", ch );
			send_to_char_color( !xIS_SET( ch->deaf, CHANNEL_COMM ) ?
			                    " &G+COMM"	: " &g-comm", ch );
			send_to_char_color( !xIS_SET (ch->deaf, CHANNEL_WARN)
			                    ? " &G+WARN" : " &g-warn", ch);
			if ( get_trust( ch ) >= sysdata.think_level )
				send_to_char_color( !xIS_SET( ch->deaf, CHANNEL_HIGH ) ?
				                    " &G+HIGH"	: " &g-high", ch );
		}
		send_to_char( "\n\r", ch );
	}
	else
	{
		bool fClear;
		bool ClearAll;
		int bit;

		bit = 0;
		ClearAll = FALSE;

		if ( arg[0] == '+' )
			fClear = TRUE;
		else if ( arg[0] == '-' )
			fClear = FALSE;
		else
		{
			send_to_char( "Channels -channel or +channel?\n\r", ch );
			return ;
		}

		if ( !str_cmp( arg + 1, "auction" ) )
			bit = CHANNEL_AUCTION;
		else if ( !str_cmp( arg + 1, "traffic" ) )
			bit = CHANNEL_TRAFFIC;
		else if ( !str_cmp( arg + 1, "chat" ) )
			bit = CHANNEL_CHAT;
		else if ( !str_cmp( arg + 1, "chatooc" ) )
			bit = CHANNEL_CHAT_OOC;
		else if ( !str_cmp( arg + 1, "clan" ) )
			bit = CHANNEL_CLAN;
		else if ( !str_cmp( arg + 1, "council" ) )
			bit = CHANNEL_COUNCIL;
		else if ( !str_cmp( arg + 1, "guild" ) )
			bit = CHANNEL_GUILD;
		else if ( !str_cmp( arg + 1, "quest" ) )
			bit = CHANNEL_QUEST;
		else if ( !str_cmp( arg + 1, "tells" ) )
			bit = CHANNEL_TELLS;
		else if ( !str_cmp( arg + 1, "immtalk" ) )
			bit = CHANNEL_IMMTALK;
		else if ( !str_cmp( arg + 1, "log" ) )
			bit = CHANNEL_LOG;
		else if ( !str_cmp( arg + 1, "build" ) )
			bit = CHANNEL_BUILD;
		else if ( !str_cmp( arg + 1, "high" ) )
			bit = CHANNEL_HIGH;
		else if ( !str_cmp( arg + 1, "avatar" ) )
			bit = CHANNEL_AVTALK;
		else if ( !str_cmp( arg + 1, "monitor" ) )
			bit = CHANNEL_MONITOR;
		else if ( !str_cmp( arg + 1, "auth" ) )
			bit = CHANNEL_AUTH;
		else if ( !str_cmp( arg + 1, "newbie" ) )
			bit = CHANNEL_NEWBIE;
		else if ( !str_cmp( arg + 1, "music" ) )
			bit = CHANNEL_MUSIC;
		else if ( !str_cmp( arg + 1, "muse" ) )
			bit = CHANNEL_HIGHGOD;
		else if ( !str_cmp( arg + 1, "ask" ) )
			bit = CHANNEL_ASK;
		else if ( !str_cmp( arg + 1, "shout" ) )
			bit = CHANNEL_SHOUT;
		else if ( !str_cmp( arg + 1, "yell" ) )
			bit = CHANNEL_YELL;
		else if ( !str_cmp( arg + 1, "comm" ) )
			bit = CHANNEL_COMM;
		else if ( !str_cmp( arg + 1, "warn" ) )
			bit = CHANNEL_WARN;
		else if ( !str_cmp( arg + 1, "order" ) )
			bit = CHANNEL_ORDER;
		else if ( !str_cmp( arg + 1, "wartalk" ) )
			bit = CHANNEL_WARTALK;
		else if ( !str_cmp( arg + 1, "whisper" ) )
			bit = CHANNEL_WHISPER;
		else if ( !str_cmp( arg + 1, "racetalk" ) )
			bit = CHANNEL_RACETALK;
		else if ( !str_cmp( arg + 1, "ooc" ) )
			bit = CHANNEL_OOC;
		else if ( !str_cmp( arg + 1, "info" ) )
			bit = CHANNEL_INFO;
		else if ( !str_cmp( arg + 1, "ainfo" ) )
			bit = CHANNEL_AINFO;
		else if ( !str_cmp( arg + 1, "hard-core" ) )
			bit = CHANNEL_HC;
		else if ( !str_cmp( arg + 1, "fos" ) )
			bit = CHANNEL_FOS;
		else if ( !str_cmp( arg + 1, "roleplay" ) )
			bit = CHANNEL_ROLEPLAY;
		else if ( !str_cmp( arg + 1, "all" ) )
			ClearAll = TRUE;
		else if ( !str_cmp( arg + 1, "admin" ) && IS_IMMORTAL(ch))
			bit = CHANNEL_ADMIN;
		else
		{
			send_to_char( "Set or clear which channel?\n\r", ch );
			return ;
		}

		if (( fClear ) && ( ClearAll ))
		{
			xREMOVE_BIT (ch->deaf, CHANNEL_NEWBIE);
			xREMOVE_BIT (ch->deaf, CHANNEL_RACETALK);
			xREMOVE_BIT (ch->deaf, CHANNEL_AUCTION);
			xREMOVE_BIT (ch->deaf, CHANNEL_CHAT);
			xREMOVE_BIT (ch->deaf, CHANNEL_CHAT_OOC);
			xREMOVE_BIT (ch->deaf, CHANNEL_QUEST);
			xREMOVE_BIT (ch->deaf, CHANNEL_WARTALK);
			xREMOVE_BIT (ch->deaf, CHANNEL_TRAFFIC);
			xREMOVE_BIT (ch->deaf, CHANNEL_MUSIC);
			xREMOVE_BIT (ch->deaf, CHANNEL_ASK);
			xREMOVE_BIT (ch->deaf, CHANNEL_SHOUT);
			xREMOVE_BIT (ch->deaf, CHANNEL_YELL);
			xREMOVE_BIT (ch->deaf, CHANNEL_OOC);
			xREMOVE_BIT (ch->deaf, CHANNEL_INFO);
			xREMOVE_BIT (ch->deaf, CHANNEL_FOS);
			xREMOVE_BIT (ch->deaf, CHANNEL_HC);
			xREMOVE_BIT (ch->deaf, CHANNEL_ROLEPLAY);

			/*     if (ch->pcdata->clan)
			       REMOVE_BIT (ch->deaf, CHANNEL_CLAN);

			if (ch->pcdata->council)
			REMOVE_BIT (ch->deaf, CHANNEL_COUNCIL);

			     if (ch->pcdata->guild)
			       REMOVE_BIT (ch->deaf, CHANNEL_GUILD);
			*/
			if (ch->level >= LEVEL_IMMORTAL)
				xREMOVE_BIT (ch->deaf, CHANNEL_AVTALK);

			/*
			if (ch->level >= sysdata.log_level )
			  REMOVE_BIT (ch->deaf, CHANNEL_COMM);
			*/

		}
		else if ((!fClear) && (ClearAll))
		{
			xSET_BIT (ch->deaf, CHANNEL_NEWBIE);
			xSET_BIT (ch->deaf, CHANNEL_RACETALK);
			xSET_BIT (ch->deaf, CHANNEL_AUCTION);
			xSET_BIT (ch->deaf, CHANNEL_TRAFFIC);
			xSET_BIT (ch->deaf, CHANNEL_CHAT);
			xSET_BIT (ch->deaf, CHANNEL_CHAT_OOC);
			xSET_BIT (ch->deaf, CHANNEL_QUEST);
			xSET_BIT (ch->deaf, CHANNEL_MUSIC);
			xSET_BIT (ch->deaf, CHANNEL_ASK);
			xSET_BIT (ch->deaf, CHANNEL_SHOUT);
			xSET_BIT (ch->deaf, CHANNEL_WARTALK);
			xSET_BIT (ch->deaf, CHANNEL_YELL);
			xSET_BIT (ch->deaf, CHANNEL_OOC);
			xSET_BIT (ch->deaf, CHANNEL_INFO);
			xSET_BIT (ch->deaf, CHANNEL_FOS);
			xSET_BIT (ch->deaf, CHANNEL_HC);
			xSET_BIT (ch->deaf, CHANNEL_ROLEPLAY);

			/*     if (ch->pcdata->clan)
			       SET_BIT (ch->deaf, CHANNEL_CLAN);

			if (ch->pcdata->council)
			SET_BIT (ch->deaf, CHANNEL_COUNCIL);

			     if ( IS_GUILDED(ch) )
			       SET_BIT (ch->deaf, CHANNEL_GUILD);
			*/
			if (ch->level >= LEVEL_IMMORTAL)
				xSET_BIT (ch->deaf, CHANNEL_AVTALK);

			/*
			if (ch->level >= sysdata.log_level)
			  SET_BIT (ch->deaf, CHANNEL_COMM);
			*/

		}
		else if (fClear)
		{
			if (bit == CHANNEL_FOS)
			{
				send_to_char( "&RThe 'Freedom of Speech' channel is &WNOT &Rmoderated by administrators.\n\r", ch );
				send_to_char( "&RThis channel may contain vulgar language, obscene gestures, adult situations,\n\r", ch );
				send_to_char( "&Rfull frontal nudity, and graphic violence.  Then again...it may not.\n\r", ch );
				send_to_char( "&RYou &WMUST &Rbe at least 18 years of age to use or have this channel active.\n\r", ch );
				send_to_char( "&RIf you are not 18, please type '&Wchannels -fos&R' to turn off this channel.&w\n\r", ch );
			}
			xREMOVE_BIT (ch->deaf, bit);
		}
		else
		{
			xSET_BIT (ch->deaf, bit);
		}

		send_to_char( "Ok.\n\r", ch );
	}

	return ;
}


/*
 * display WIZLIST file						-Thoric
 */
void do_wizlist( CHAR_DATA *ch, char *argument )
{
	set_pager_color( AT_IMMORT, ch );
	show_file( ch, WIZLIST_FILE );
}

/*
 * Contributed by Grodyn.
 * Display completely overhauled, 2/97 -- Blodkai
 */
void do_config( CHAR_DATA *ch, char *argument )
{
	char arg[MAX_INPUT_LENGTH];
	int prompt = 0;

	if ( IS_NPC(ch) )
		return ;

	argument = one_argument( argument, arg );

	set_char_color( AT_GREEN, ch );

	if ( ch->position == POS_FIGHTING || ch->position == POS_BERSERK
	     || ch->position == POS_AGGRESSIVE || ch->position == POS_DEFENSIVE
	     || ch->position == POS_EVASIVE )
	{
		send_to_char( "Wait until you're not so busy fighting.\n\r", ch );
		return ;
	}
	if ( arg[0] == '\0' )
	{
		set_char_color( AT_DGREEN, ch );
		send_to_char( "\n\rConfigurations\n\r", ch );
		set_char_color( AT_GREEN, ch );
		send_to_char( "(use 'config +/- <keyword>' to toggle, or 'config nprompt/bprompt #' see 'help config')\n\r\n\r", ch );
		set_char_color( AT_DGREEN, ch );
		send_to_char( "Allow Swearing:   ", ch );
		set_char_color( AT_GREY, ch );
		ch_printf( ch, "%-12s   %-12s   %-12s",
		           xIS_SET(ch->act, PLR_SAY_SWEAR ) ? "[+] SAY"
		           : "[-] say",
		           xIS_SET(ch->act, PLR_TELL_SWEAR ) ? "[+] TELL"
		           : "[-] tell",
		           xIS_SET(ch->act, PLR_CLAN_SWEAR ) ? "[+] CLAN"
		           : "[-] clan" );
		set_char_color( AT_DGREEN, ch );
		send_to_char( "\n\r\n\rDisplay:   ", ch );
		set_char_color( AT_GREY, ch );
		ch_printf( ch, "%-12s   %-12s   %-12s   %-12s\n\r           %-12s   %-12s   %-12s   %-12s",
		           IS_SET( ch->pcdata->flags, PCFLAG_PAGERON ) ? "[+] PAGER"
		           : "[-] pager",
		           IS_SET( ch->pcdata->flags, PCFLAG_GAG ) ? "[+] GAG"
		           : "[-] gag",
		           xIS_SET(ch->act, PLR_BRIEF ) ? "[+] BRIEF"
		           : "[-] brief",
		           xIS_SET(ch->act, PLR_COMBINE ) ? "[+] COMBINE"
		           : "[-] combine",
		           xIS_SET(ch->act, PLR_BLANK ) ? "[+] BLANK"
		           : "[-] blank",
		           xIS_SET(ch->act, PLR_PROMPT ) ? "[+] PROMPT"
		           : "[-] prompt",
		           xIS_SET(ch->act, PLR_ANSI ) ? "[+] ANSI"
		           : "[-] ansi",
		           xIS_SET(ch->act, PLR_RIP ) ? "[+] RIP"
		           : "[-] rip" );
		set_char_color( AT_DGREEN, ch );
		send_to_char( "\n\r\n\rNormal Prompt:   ", ch );
		set_char_color( AT_GREY, ch );
		ch_printf(ch, "%sDefault(0)&w ", ch->pcdata->normalPromptConfig == 0 ? "&W" : "&w");
		ch_printf(ch, "%sPL Extended(1)&w ", ch->pcdata->normalPromptConfig == 1 ? "&W" : "&w");
		set_char_color( AT_DGREEN, ch );
		send_to_char( "\n\r\n\rBattle Prompt:   ", ch );
		set_char_color( AT_GREY, ch );
		ch_printf(ch, "%sDefault(0)&w ", ch->pcdata->battlePromptConfig == 0 ? "&W" : "&w");
		ch_printf(ch, "%sDefault No Armor(1)&w ", ch->pcdata->battlePromptConfig == 1 ? "&W" : "&w");
		ch_printf(ch, "%sPL Extended(2)&w ", ch->pcdata->battlePromptConfig == 2 ? "&W" : "&w");
		ch_printf(ch, "%sPL Extended No Armor(3)&w ", ch->pcdata->battlePromptConfig == 3 ? "&W" : "&w");
		set_char_color( AT_DGREEN, ch );
		send_to_char( "\n\r\n\rAuto:      ", ch );
		set_char_color( AT_GREY, ch );
		ch_printf( ch, "%-12s   %-12s   %-12s   %-12s\n\r           %-12s",
		           xIS_SET(ch->act, PLR_AUTOSAC ) ? "[+] AUTOSAC"
		           : "[-] autosac",
		           xIS_SET(ch->act, PLR_AUTOGOLD ) ? "[+] AUTOZENI"
		           : "[-] autozeni",
		           xIS_SET(ch->act, PLR_AUTOLOOT ) ? "[+] AUTOLOOT"
		           : "[-] autoloot",
		           xIS_SET(ch->act, PLR_AUTOEXIT ) ? "[+] AUTOEXIT"
		           : "[-] autoexit",
		           xIS_SET(ch->act, PLR_AUTO_COMPASS ) ? "[+] AUTOCOMPASS"
		           : "[-] autocompass" );

		set_char_color( AT_DGREEN, ch );
		send_to_char( "\n\r\n\rSafeties:  ", ch );
		set_char_color( AT_GREY, ch );
		ch_printf( ch, "%-12s   %-12s",
		           IS_SET( ch->pcdata->flags, PCFLAG_NORECALL ) ? "[+] NORECALL"
		           : "[-] norecall",
		           IS_SET( ch->pcdata->flags, PCFLAG_NOSUMMON ) ? "[+] NOSUMMON"
		           : "[-] nosummon",
		           xIS_SET(ch->act, PLR_NOFOLLOW ) ? "[+] NOFOLLOW"
		           : "[-] nofollow" );

		if ( !IS_SET( ch->pcdata->flags, PCFLAG_DEADLY ) )
			ch_printf( ch, "   %-12s   %-12s",
			           xIS_SET(ch->act, PLR_SHOVEDRAG ) ? "[+] DRAG"
			           : "[-] drag",
			           xIS_SET(ch->act, PLR_NICE ) ? "[+] NICE"
			           : "[-] nice" );

		set_char_color( AT_DGREEN, ch );
		send_to_char( "\n\r\n\rMisc:      ", ch );
		set_char_color( AT_GREY, ch );
		ch_printf( ch, "%-12s   %-12s   %-12s   %-12s\n\r",
		           xIS_SET(ch->act, PLR_TELNET_GA )	? "[+] TELNETGA"
		           : "[-] telnetga",
		           IS_SET( ch->pcdata->flags, PCFLAG_GROUPWHO ) ? "[+] GROUPWHO"
		           : "[-] groupwho",
		           IS_SET( ch->pcdata->flags, PCFLAG_NOINTRO ) ? "[+] NOINTRO"
		           : "[-] nointro",
		           xIS_SET(ch->act, PLR_SPAR ) ? "[+] SPAR"
		           : "[-] spar" );
// Fuck leet --Saiyr	   xIS_SET(ch->act, PLR_1337 ) ? "[+] 1337"
//                           : "[-] 1337" );

		set_char_color( AT_DGREEN, ch );
		send_to_char( "\n\r\n\rSettings:  ", ch );
		set_char_color( AT_GREY, ch );
		ch_printf_color( ch, "Pager Length (&W%d&w)    Wimpy (&W%d&w)    Auction Limit (&W%s&w)",
		                 ch->pcdata->pagerlen,
		                 ch->wimpy, num_punct(ch->pcdata->auction_pl) );

		if ( IS_IMMORTAL( ch ) )
		{
			set_char_color( AT_DGREEN, ch );
			send_to_char( "\n\r\n\rImmortal toggles:  ", ch );
			set_char_color( AT_GREY, ch );
			ch_printf( ch, "Roomvnum [%s]    Automap [%s]",
			           xIS_SET(ch->act, PLR_ROOMVNUM ) ? "+"
			           : " ",
			           xIS_SET(ch->act, PLR_AUTOMAP ) ? "+"
			           : " " );
			ch_printf( ch, "\n\r&R&gChan while edit (chanedit):&w [%s]",
				xIS_SET(ch->act, PLR_CHANEDIT) ? "+" : " " );
		}

		set_char_color( AT_DGREEN, ch );
		send_to_char( "\n\r\n\rSentences imposed on you (if any):", ch );
		set_char_color( AT_YELLOW, ch );
		ch_printf( ch, "\n\r%s%s%s%s%s%s",
		           xIS_SET(ch->act, PLR_SILENCE ) ?
		           " For your abuse of channels, you are currently silenced.\n\r" : "",
		           xIS_SET(ch->act, PLR_NO_EMOTE ) ?
		           " The gods have removed your emotes.\n\r" : "",
		           xIS_SET(ch->act, PLR_NO_TELL ) ?
		           " You are not permitted to send 'tells' to others.\n\r" : "",
		           xIS_SET(ch->act, PLR_LITTERBUG ) ?
		           " A convicted litterbug.  You cannot drop anything.\n\r" : "",
		           xIS_SET(ch->act, PLR_THIEF ) ?
		           " A proven thief, you will be hunted by the authorities.\n\r" : "",
		           xIS_SET(ch->act, PLR_KILLER ) ?
		           " For the crime of murder you are sentenced to death...\n\r" : "" );
		if( xIS_SET(ch->act, PLR_NOGBOARD ) )
		  ch_printf(ch," You are no longer able to use the global boards.\n\r");
		if( xIS_SET(ch->act, PLR_SILENCE ) )
		  ch_printf(ch," You have %d minutes left on your silence.\n\r",
			ch->pcdata->silence );
		if( ch->pcdata->release_date != 0 )
		  ch_printf(ch," You will be released from hell on %24.24s. You were helled by %s.\n\r",
			ctime(&ch->pcdata->release_date),
			ch->pcdata->helled_by);

	}
	else if (!strcmp(arg, "nprompt") || !strcmp(arg, "bprompt"))
	{
		if (!is_number(argument))
		{
			send_to_char( "You must type a number.\n\r", ch );
			return ;
		}
		prompt = atoi(argument);
		if (!strcmp(arg, "nprompt"))
		{
			if (prompt < 0 || prompt > 1)
			{
				send_to_char( "Number must be 0 or 1.\n\r", ch );
				return ;
			}
			ch->pcdata->normalPromptConfig = prompt;
		}
		if (!strcmp(arg, "bprompt"))
		{
			if (prompt < 0 || prompt > 3)
			{
				send_to_char( "Number must be 0, 1, 2, or 3.\n\r", ch );
				return ;
			}
			ch->pcdata->battlePromptConfig = prompt;
		}

		send_to_char( "Ok.\n\r", ch );
		return;
	}
	else
	{
		bool fSet;
		int bit = 0;

		if ( arg[0] == '+' )
			fSet = TRUE;
		else if ( arg[0] == '-' )
			fSet = FALSE;
		else
		{
			send_to_char( "Config -option or +option?\n\r", ch );
			return ;
		}

		if ( !str_prefix( arg + 1, "autoexit" ) )
			bit = PLR_AUTOEXIT;
		else if ( !str_prefix( arg + 1, "autoloot" ) )
			bit = PLR_AUTOLOOT;
		else if ( !str_prefix( arg + 1, "autosac" ) )
			bit = PLR_AUTOSAC;
		else if ( !str_prefix( arg + 1, "autozeni" ) )
			bit = PLR_AUTOGOLD;
		else if ( !str_prefix( arg + 1, "blank" ) )
			bit = PLR_BLANK;
		else if ( !str_prefix( arg + 1, "brief" ) )
			bit = PLR_BRIEF;
		else if ( !str_prefix( arg + 1, "combine" ) )
			bit = PLR_COMBINE;
		else if ( !str_prefix( arg + 1, "prompt" ) )
			bit = PLR_PROMPT;
		else if ( !str_prefix( arg + 1, "telnetga" ) )
			bit = PLR_TELNET_GA;
		else if ( !str_prefix( arg + 1, "ansi" ) )
			bit = PLR_ANSI;
		else if ( !str_prefix( arg + 1, "spar"	) )
			bit = PLR_SPAR;
//		else if ( !str_prefix( arg + 1, "1337"  ) )
//                        bit = PLR_1337;
		else if ( !str_prefix( arg + 1, "rip" ) )
			bit = PLR_RIP;
		/*	else if ( !str_prefix( arg+1, "flee"     ) ) bit = PLR_FLEE; */
		else if ( !str_prefix( arg + 1, "nice" ) )
			bit = PLR_NICE;
		else if ( !str_prefix( arg + 1, "drag" ) )
			bit = PLR_SHOVEDRAG;
		else if ( IS_IMMORTAL( ch )
		          && !str_prefix( arg + 1, "vnum" ) )
			bit = PLR_ROOMVNUM;
		else if ( IS_IMMORTAL( ch )
			  && !str_prefix( arg + 1, "chanedit" ) )
			bit = PLR_CHANEDIT;
		else if ( IS_IMMORTAL( ch )
		          && !str_prefix( arg + 1, "map" ) )
			bit = PLR_AUTOMAP;     /* maps */
		else if ( !str_prefix( arg + 1, "say" ) )
			bit = PLR_SAY_SWEAR;
		else if ( !str_prefix( arg + 1, "tell" ) )
			bit = PLR_TELL_SWEAR;
		else if ( !str_prefix( arg + 1, "clan" ) )
			bit = PLR_CLAN_SWEAR;
		else if ( !str_prefix( arg + 1, "autocompass" ) )
			bit = PLR_AUTO_COMPASS;
		else if ( !str_prefix( arg + 1, "nofollow" ) )
			bit = PLR_NOFOLLOW;

		if (bit)
		{
			if ( (bit == PLR_FLEE || bit == PLR_NICE || bit == PLR_SHOVEDRAG)
			     && IS_SET( ch->pcdata->flags, PCFLAG_DEADLY ) )
			{
				send_to_char( "Pkill characters can not config that option.\n\r", ch );
				return ;
			}

			if ( (bit == PLR_ANSI) && xIS_SET(ch->act, PLR_ANSI ))
				ch->desc->ansi = FALSE;
			else if ( (bit == PLR_ANSI) && !xIS_SET(ch->act, PLR_ANSI))
				ch->desc->ansi = TRUE;

			if ( fSet )
				xSET_BIT (ch->act, bit);
			else
				xREMOVE_BIT(ch->act, bit);
			send_to_char( "Ok.\n\r", ch );
			return ;
		}
		else
		{
			if ( !str_prefix( arg + 1, "norecall" ) )
				bit = PCFLAG_NORECALL;
			else if ( !str_prefix( arg + 1, "nointro" ) )
				bit = PCFLAG_NOINTRO;
			else if ( !str_prefix( arg + 1, "nosummon" ) )
				bit = PCFLAG_NOSUMMON;
			else if ( !str_prefix( arg + 1, "gag" ) )
				bit = PCFLAG_GAG;
			else if ( !str_prefix( arg + 1, "pager" ) )
				bit = PCFLAG_PAGERON;
			else if ( !str_prefix( arg + 1, "groupwho" ) )
				bit = PCFLAG_GROUPWHO;
			else if ( !str_prefix( arg + 1, "@hgflag_" ) )
				bit = PCFLAG_HIGHGAG;
			else
			{
				send_to_char( "Config which option?\n\r", ch );
				return ;
			}

			if ( fSet )
				SET_BIT (ch->pcdata->flags, bit);
			else
				REMOVE_BIT (ch->pcdata->flags, bit);

			send_to_char( "Ok.\n\r", ch );
			return ;
		}
	}

	return ;
}

/*
 * Combat toggle flags -- Melora
 */
void do_combat( CHAR_DATA *ch, char *argument )
{
	char arg[MAX_INPUT_LENGTH];

	if ( IS_NPC(ch) )
		return ;

	one_argument( argument, arg );

	set_char_color( AT_GREEN, ch );

	if ( ch->position == POS_FIGHTING || ch->position == POS_BERSERK
	     || ch->position == POS_AGGRESSIVE || ch->position == POS_DEFENSIVE
	     || ch->position == POS_EVASIVE )
	{
		send_to_char( "Wait until you're not so busy fighting.\n\r", ch );
		return ;
	}
	if ( arg[0] == '\0' )
	{
		set_char_color( AT_DGREEN, ch );
		send_to_char( "\n\rCombat Configurations ", ch );
		set_char_color( AT_GREEN, ch );
		send_to_char( "(use 'combat +/- <keyword>' to toggle, see 'help combat')\n\r\n\r", ch );
		set_char_color( AT_DGREEN, ch );
		send_to_char( "\n\r\n\rDefenses:   \n\r", ch );
		set_char_color( AT_GREY, ch );
		//      ch_printf( ch, "%-12s   %-12s   %-12s   %-12s\n\r           %-12s   %-12s   %-12s   %-12s",
		if (is_android(ch) || is_superandroid(ch))
		{
		ch_printf( ch, "%-12s\n\r",
		           IS_SET( ch->pcdata->combatFlags, CMB_NO_KI_ABSORB ) ? "[-] kiabsorb"
		           : "[+] KIABSORB" );
		}
		if (is_human(ch))
		{
			ch_printf( ch, "%-12s\n\r",
		           IS_SET( ch->pcdata->combatFlags, CMB_NO_HEART ) ? "[+] HEART"
		           : "[-] heart" );
		}
		if (ch->pcdata->learned[gsn_dcd] > 0)
                {
                        ch_printf( ch, "%-12s\n\r",
                           IS_SET( ch->pcdata->combatFlags, CMB_NO_DCD) ? "[-] dcd"
                           : "[+] DCD" );
                }
		if (ch->pcdata->learned[gsn_dodge] > 0)
                {
                        ch_printf( ch, "%-12s\n\r",
                           IS_SET( ch->pcdata->combatFlags, CMB_NO_DODGE) ? "[-] dodge"
                           : "[+] DODGE" );
                }
		if (ch->pcdata->learned[gsn_block] > 0)
                {
                        ch_printf( ch, "%-12s\n\r",
                           IS_SET( ch->pcdata->combatFlags, CMB_NO_BLOCK) ? "[-] block"
                           : "[+] BLOCK" );
                }

		/*      if ( IS_IMMORTAL( ch ) )
		      {
			set_char_color( AT_DGREEN, ch );
		        send_to_char( "\n\r\n\rImmortal toggles:  ", ch );
		        set_char_color( AT_GREY, ch );
		        ch_printf( ch, "Roomvnum [%s]    Automap [%s]",
			  xIS_SET(ch->act, PLR_ROOMVNUM ) 		? "+"
									: " ",
		          xIS_SET(ch->act, PLR_AUTOMAP  ) 		? "+"
									: " " );
		      }
		 */
	}
	else
	{
		bool fSet;
		int bit = 0;

		if ( arg[0] == '+' )
			fSet = TRUE;
		else if ( arg[0] == '-' )
			fSet = FALSE;
		else
		{
			send_to_char( "Combat -option or +option?\n\r", ch );
			return ;
		}
		bool reverse = FALSE;
		if ( !str_prefix( arg + 1, "kiabsorb" ) )
		{
			reverse = TRUE;
			bit = CMB_NO_KI_ABSORB;
		}
		else if ( !str_prefix( arg + 1, "dcd" ) )
                {
                        reverse = TRUE;
                        bit = CMB_NO_DCD;
                }
		else if ( !str_prefix( arg + 1, "dodge" ) )
                {
                        reverse = TRUE;
                        bit = CMB_NO_DODGE;
                }
		else if ( !str_prefix( arg + 1, "block" ) )
                {
                        reverse = TRUE;
                        bit = CMB_NO_BLOCK;
                }
		else if ( !str_prefix( arg + 1, "heart" ) )
		{
			if (xIS_SET((ch)->affected_by, AFF_EXTREME))
			{
				send_to_char("You cannot toggle heart while using Extreme.\n\r",ch);
				return;
			}
			if (xIS_SET((ch)->affected_by, AFF_MYSTIC))
                        {
                                send_to_char("You cannot toggle heart while using Mystic.\n\r",ch);
                                return;
                        }

			if (arg[0] == '-')
			{
				if (xIS_SET((ch)->affected_by, AFF_HEART))
				{
					xREMOVE_BIT(ch->affected_by, AFF_HEART);
					ch->pl = ch->heart_pl;
					send_to_char("Your power decreases as you suppress your inner strength.\n\r",ch);
				}
			}
			bit = CMB_NO_HEART;
		}
		/*	else if ( !str_prefix( arg+1, "autoloot" ) ) bit = PLR_AUTOLOOT;
			else if ( !str_prefix( arg+1, "autosac"  ) ) bit = PLR_AUTOSAC;
			else if ( !str_prefix( arg+1, "autozeni" ) ) bit = PLR_AUTOGOLD;
			else if ( !str_prefix( arg+1, "blank"    ) ) bit = PLR_BLANK;
			else if ( !str_prefix( arg+1, "brief"    ) ) bit = PLR_BRIEF;
			else if ( !str_prefix( arg+1, "combine"  ) ) bit = PLR_COMBINE;
			else if ( !str_prefix( arg+1, "prompt"   ) ) bit = PLR_PROMPT;
			else if ( !str_prefix( arg+1, "telnetga" ) ) bit = PLR_TELNET_GA;
			else if ( !str_prefix( arg+1, "ansi"     ) ) bit = PLR_ANSI;
			else if ( !str_prefix( arg+1, "spar"	 ) ) bit = PLR_SPAR;
			else if ( !str_prefix( arg+1, "rip"      ) ) bit = PLR_RIP;
			else if ( !str_prefix( arg+1, "nice"     ) ) bit = PLR_NICE;
			else if ( !str_prefix( arg+1, "drag"     ) ) bit = PLR_SHOVEDRAG;
			else if ( IS_IMMORTAL( ch )
			     &&   !str_prefix( arg+1, "vnum"     ) ) bit = PLR_ROOMVNUM;
			else if ( IS_IMMORTAL( ch )
			     &&   !str_prefix( arg+1, "map"      ) ) bit = PLR_AUTOMAP;
			else if ( !str_prefix( arg+1, "say"     ) ) bit = PLR_SAY_SWEAR;
			else if ( !str_prefix( arg+1, "tell"     ) ) bit = PLR_TELL_SWEAR;
			else if ( !str_prefix( arg+1, "clan"     ) ) bit = PLR_CLAN_SWEAR;
			else if ( !str_prefix( arg+1, "autocompass"     ) ) bit = PLR_AUTO_COMPASS;
			else if ( !str_prefix( arg+1, "nofollow"     ) ) bit = PLR_NOFOLLOW;
		 */

		if (bit)
		{

		  if( reverse )
		  {
			if ( fSet )
				REMOVE_BIT (ch->pcdata->combatFlags, bit);
			else
				SET_BIT(ch->pcdata->combatFlags, bit);
			send_to_char( "Ok.\n\r", ch );
			return ;
		  }
		  else
                  {
                        if ( fSet )
                                SET_BIT (ch->pcdata->combatFlags, bit);
                        else
                                REMOVE_BIT(ch->pcdata->combatFlags, bit);
                        send_to_char( "Ok.\n\r", ch );
                        return ;
                  }
		}
	}

	return ;
}



void do_credits( CHAR_DATA *ch, char *argument )
{
	do_help( ch, "credits" );
}


extern int top_area;

/*
void do_areas( CHAR_DATA *ch, char *argument )
{
    AREA_DATA *pArea1;
    AREA_DATA *pArea2;
    int iArea;
    int iAreaHalf;

    iAreaHalf = (top_area + 1) / 2;
    pArea1    = first_area;
    pArea2    = first_area;
    for ( iArea = 0; iArea < iAreaHalf; iArea++ )
	pArea2 = pArea2->next;

    for ( iArea = 0; iArea < iAreaHalf; iArea++ )
    {
	ch_printf( ch, "%-39s%-39s\n\r",
	    pArea1->name, pArea2 ? pArea2->name : "" );
	pArea1 = pArea1->next;
	if ( pArea2 )
	    pArea2 = pArea2->next;
    }

    return;
}
*/

/*
 * New do_areas with soft/hard level ranges
void do_areas( CHAR_DATA *ch, char *argument )
{
    AREA_DATA *pArea;

    set_pager_color( AT_PLAIN, ch );
    send_to_pager("\n\r   Author    |             Area                     | Recommended |  Enforced\n\r", ch);
    send_to_pager("-------------+--------------------------------------+-------------+-----------\n\r", ch);

    for ( pArea = first_area; pArea; pArea = pArea->next )
	pager_printf(ch, "%-12s | %-36s | %4d - %-4d | %3d - %-3d \n\r",
	 	pArea->author, pArea->name, pArea->low_soft_range,
		pArea->hi_soft_range, pArea->low_hard_range,
		pArea->hi_hard_range);
    return;
}
*/

/*
 * New do_areas, written by Fireblade, last modified - 4/27/97
 *
 *   Syntax: area            ->      lists areas in alphanumeric order
 *           area <a>        ->      lists areas with soft max less than
 *                                                    parameter a
 *           area <a> <b>    ->      lists areas with soft max bewteen
 *                                                    numbers a and b
 *           area old        ->      list areas in order loaded
 *
 */
void do_areas( CHAR_DATA *ch, char *argument )
{
	char *header_string1 = "\n\r   Author    |             Area"
	                       "                     | "
	                       "Recommended |  Enforced\n\r";
	char *header_string2 = "-------------+-----------------"
	                       "---------------------+----"
	                       "---------+-----------\n\r";
	char *print_string = "%-12s | %-36s | %4d - %-4d | %3d - "
	                     "%-3d \n\r";

	AREA_DATA *pArea;
	int lower_bound = 0;
	int upper_bound = MAX_LEVEL + 1;
	/* make sure is to init. > max area level */
	char arg[MAX_STRING_LENGTH];
	int planetSort;
	bool showPlanetName;

	argument = one_argument(argument, arg);

	upper_bound = ch->level;

	if (arg[0] != '\0')
	{
		if (!is_number(arg))
		{
			if (!strcmp(arg, "old"))
			{
				set_pager_color( AT_PLAIN, ch );
				send_to_pager(header_string1, ch);
				send_to_pager(header_string2, ch);
				for (pArea = first_area; pArea;pArea = pArea->next)
				{
					pager_printf(ch, print_string,
					             pArea->author, pArea->name,
					             pArea->low_soft_range,
					             pArea->hi_soft_range,
					             pArea->low_hard_range,
					             pArea->hi_hard_range);
				}
				return ;
			}
			else
			{
				send_to_char("Area may only be followed by numbers, or 'old'.\n\r", ch);
				return ;
			}
		}

		upper_bound = atoi(arg);
		lower_bound = upper_bound;

		argument = one_argument(argument, arg);

		if (arg[0] != '\0')
		{
			if (!is_number(arg))
			{
				send_to_char("Area may only be followed by numbers.\n\r", ch);
				return ;
			}

			upper_bound = atoi(arg);

			argument = one_argument(argument, arg);
			if (arg[0] != '\0')
			{
				send_to_char("Only two level numbers allowed.\n\r", ch);
				return ;
			}
		}
	}

	if (lower_bound > upper_bound)
	{
		int swap = lower_bound;
		lower_bound = upper_bound;
		upper_bound = swap;
	}

	set_pager_color( AT_PLAIN, ch );
	send_to_pager(header_string1, ch);
	send_to_pager(header_string2, ch);

	for (planetSort = 1; planetSort <= MAX_PLANETS; planetSort++)
	{
		showPlanetName = TRUE;
		for (pArea = first_area_name; pArea; pArea = pArea->next_sort_name)
		{
			if (pArea->hi_soft_range >= lower_bound
			    && pArea->low_soft_range <= upper_bound
			    && pArea->areaPlanet == planetSort)
			{
				if (showPlanetName)
				{
					showPlanetName = FALSE;
					pager_printf(ch, "&z-------------&w|&G%-37s&w |&z-------------&w|&z-----------&w\n\r", planet_table[pArea->areaPlanet]);
				}

				pager_printf(ch, print_string,
				             pArea->author, pArea->name,
				             pArea->low_soft_range,
				             pArea->hi_soft_range,
				             pArea->low_hard_range,
				             pArea->hi_hard_range);
			}
		}
	}
	return ;
}

void do_afk( CHAR_DATA *ch, char *argument )
{
	if ( IS_NPC(ch) )
		return ;

	if ( xIS_SET(ch->act, PLR_AFK) )
	{
		xREMOVE_BIT(ch->act, PLR_AFK);
		send_to_char( "You are no longer afk.\n\r", ch );
		/*	act(AT_GREY,"$n is no longer afk.", ch, NULL, NULL, TO_ROOM);*/
		act(AT_GREY, "$n is no longer afk.", ch, NULL, NULL, TO_CANSEE);
	}
	else
	{
		xSET_BIT(ch->act, PLR_AFK);
		send_to_char( "You are now afk.\n\r", ch );
		/*	act(AT_GREY,"$n is now afk.", ch, NULL, NULL, TO_ROOM);*/
		act(AT_GREY, "$n is now afk.", ch, NULL, NULL, TO_CANSEE);
		return;
	}

}

void swap(int *element1Ptr, int *element2Ptr)
{
	int temp = *element1Ptr;
	*element1Ptr = *element2Ptr;
	*element2Ptr = temp;
	return ;
}

void slistSort(int *array, const int size, int chRace)
{
	int pass;
	int j;

	for (pass = 1; pass < size ; pass++)
		for (j = 0; j < size - 1; j++)
			if (skill_table[array[j]]->skill_level[chRace] > skill_table[array[j + 1]]->skill_level[chRace])
				swap(&array[j], &array[j + 1]);
	return ;
}

void do_alist( CHAR_DATA *ch, char *argument )
{
	int	snRace[gsn_top_sn];
	int	sn;
	int	size = 0;
	int	i;
	bool	canUse;
        bool    bioPrac = FALSE;
	bool    mystic = FALSE;
	bool    canMystic = FALSE;
        int     ii = 0;

	if( IS_NPC(ch) )
	  return;

	for (sn = gsn_first_ability;sn < gsn_first_weapon;sn++)
	{
		if (( skill_table[sn]->guild != CLASS_NONE
		      && ( !IS_GUILDED(ch)
		           || (ch->pcdata->clan->class != skill_table[sn]->guild) ) ) )
			continue;

		if( sn == gsn_wss && !demonranked(ch) )
			continue;

		if( sn == gsn_instant_trans && ((is_android(ch)
		    || is_superandroid(ch)) &&
		    !wearing_sentient_chip(ch)) )
			continue;

		if ( skill_table[sn]->min_level[ch->class] == 0
		     || skill_table[sn]->skill_level[ch->class] == 0)
			continue;

		if ( ch->pcdata->learned[sn] <= 0
		     && SPELL_FLAG(skill_table[sn], SF_SECRETSKILL) )
			continue;

                if( ch->race == race_lookup( "bio-android" ) )
                {
                  if( sn == gsn_semiperfect
                   || sn == gsn_perfect
                   || sn == gsn_ultraperfect )
                  {
                    bioPrac = FALSE;
                  }
		  else if( sn == gsn_clone
		   || sn == gsn_ki_burst )
		  {
		    bioPrac = TRUE;
		  }
                  else
                  {
                    bioPrac = TRUE;
                    for( ii = 0; ii < MAX_RACE; ii++ )
                    {
                      if( ii == ch->class )
                        continue;
                      else
                      {
                        if( skill_table[sn]->skill_level[ii] > 0 )
                        {
                          bioPrac = FALSE;
                        }
                      }
                    }
                  }
                }
		if (ch->race == 6 && ch->pcdata->learned[sn] <= 0 && skill_table[sn]->skill_level[ch->class] >= 100000
                 && !bioPrac)
			continue;

		snRace[size] = sn;
		size++;
	}

	slistSort(snRace, size, ch->race);

	pager_printf_color( ch, "\n\r&B----------------------------&CAbilities&B---------------------------\n\r");
	if (is_android(ch) || is_superandroid(ch) )
		pager_printf_color( ch, "-----------&CName&B----------------------&CTL Needed&B--&CCurrent&B---&CMax&B---\n\r");
	else
		pager_printf_color( ch, "-----------&CName&B----------------------&CPL Needed&B--&CCurrent&B---&CMax&B---\n\r");

	for (i = 0;i < size;i++)
	{
		if (skill_table[snRace[i]]->skill_level[ch->class] > ch->exp)
			canUse = FALSE;
		else
			canUse = TRUE;

		if( !str_cmp(skill_table[snRace[i]]->name,"mystic"))
		{
		  mystic = TRUE;
		  if( IS_SET(ch->pcdata->flags, PCFLAG_KNOWSMYSTIC) ||
		      is_kaio(ch) )
		    canMystic = TRUE;
		  else
                    canMystic = FALSE;
		}
		else
		  mystic = FALSE;


		if (canUse)
		{
		    if( (!mystic && !canMystic) || (mystic && canMystic) )
			pager_printf_color( ch, "&w %22.22s       %16s   &Y%3.0f%%    &W%3d%%\n\r",
			                    skill_table[snRace[i]]->name, num_punct_ld(skill_table[snRace[i]]->skill_level[ch->class]),
			                    ch->pcdata->learned[snRace[i]], skill_table[snRace[i]]->skill_adept[ch->class]);
		}
		else
		{
		    if( (!mystic && !canMystic) || (mystic && canMystic) )
			pager_printf_color( ch, "&z %22.22s       %16s   %3.0f%%    %3d%%\n\r",
			                    skill_table[snRace[i]]->name, num_punct_ld(skill_table[snRace[i]]->skill_level[ch->class]),
			                    ch->pcdata->learned[snRace[i]], skill_table[snRace[i]]->skill_adept[ch->class]);
		}
	}

}

void do_slist( CHAR_DATA *ch, char *argument )
{
	int	snRace[gsn_top_sn];
	int	sn;
	int	size = 0;
	int	i;
	bool	canUse;
        bool    bioPrac = FALSE;
        int     ii = 0;

	if( IS_NPC(ch) )
	  return;

	for (sn = gsn_first_skill;sn < gsn_first_ability;sn++)
	{
		if (( skill_table[sn]->guild != CLASS_NONE
		      && ( !IS_GUILDED(ch)
		           || (ch->pcdata->clan->class != skill_table[sn]->guild) ) ) )
			continue;

		if ( skill_table[sn]->min_level[ch->class] == 0
		     || skill_table[sn]->skill_level[ch->class] == 0)
			continue;

		if ( ch->pcdata->learned[sn] <= 0
		     && SPELL_FLAG(skill_table[sn], SF_SECRETSKILL) )
			continue;

                if( ch->race == race_lookup( "bio-android" ) )
                {
                  if( sn == gsn_semiperfect
                   || sn == gsn_perfect
                   || sn == gsn_ultraperfect )
                  {
                    bioPrac = FALSE;
                  }
                  else
                  {
                    bioPrac = TRUE;
                    for( ii = 0; ii < MAX_RACE; ii++ )
                    {
                      if( ii == ch->class )
                        continue;
                      else
                      {
                        if( skill_table[sn]->skill_level[ii] > 0 )
                        {
                          bioPrac = FALSE;
                        }
                      }
                    }
                  }
                }
		if (ch->race == 6 && ch->pcdata->learned[sn] <= 0 && skill_table[sn]->skill_level[ch->class] >= 100000
                 && !bioPrac)
			continue;

		snRace[size] = sn;
		size++;
	}

	slistSort(snRace, size, ch->race);

	pager_printf_color( ch, "&B--------------------------&CSkills&B----------------------------\n\r");
//	pager_printf_color( ch, "&B------------------------&CAbilities&B---------------------\n\r");
	if (is_android(ch))
		pager_printf_color( ch, "-----------&CName&B--------------------&CTL Needed&B--&CCurrent&B---&CMax&B---\n\r");
	else
		pager_printf_color( ch, "-----------&CName&B--------------------&CPL Needed&B--&CCurrent&B---&CMax&B---\n\r");

	for (i = 0;i < size;i++)
	{
		if (skill_table[snRace[i]]->skill_level[ch->class] > ch->exp)
			canUse = FALSE;
		else
			canUse = TRUE;

		if (canUse)
		{
			pager_printf_color( ch, "&w %20.20s       %16s   &Y%3.0f%%    &W%3d%%\n\r",
			                    skill_table[snRace[i]]->name, num_punct_ld(skill_table[snRace[i]]->skill_level[ch->class]),
			                    ch->pcdata->learned[snRace[i]], skill_table[snRace[i]]->skill_adept[ch->class]);
		}
		else
		{
			pager_printf_color( ch, "&z %20.20s       %16s   %3.0f%%    %3d%%\n\r",
			                    skill_table[snRace[i]]->name, num_punct_ld(skill_table[snRace[i]]->skill_level[ch->class]),
			                    ch->pcdata->learned[snRace[i]], skill_table[snRace[i]]->skill_adept[ch->class]);
		}
	}

}

void do_whois( CHAR_DATA *ch, char *argument)
{
	CHAR_DATA *victim;
	char buf[MAX_STRING_LENGTH];
	char buf2[MAX_STRING_LENGTH];

	buf[0] = '\0';

	if (IS_NPC(ch))
		return ;

	if (argument[0] == '\0')
	{
		send_to_pager("You must input the name of an online character.\n\r", ch);
		return ;
	}

	strcat(buf, "0.");
	strcat(buf, argument);
	if ( ( ( victim = get_char_world(ch, buf) ) == NULL ))
	{
		send_to_pager("No such character online.\n\r", ch);
		return ;
	}

	if (IS_NPC(victim))
	{
		send_to_pager("That's not a player!\n\r", ch);
		return ;
	}

	set_pager_color( AT_GREY, ch );
	pager_printf(ch, "\n\r'%s%s&D.'\n\r %s is a %s %s, %d years of age.\n\r",
	             victim->name,
	             victim->pcdata->title,
	             victim->sex == SEX_MALE ? "He" :
	             victim->sex == SEX_FEMALE ? "She" : "It",
	             victim->sex == SEX_MALE ? "male" :
	             victim->sex == SEX_FEMALE ? "female" : "neutral",
		     capitalize(race_table[victim->race]->race_name),
	             get_newage(victim) );

	if( victim->pcdata->spouse && victim->pcdata->spouse[0] != '\0' )
	pager_printf(ch, " %s is married to %s.\n\r",
		victim->sex == SEX_MALE ? "He" : victim->sex == SEX_FEMALE ? "She" : "It", victim->pcdata->spouse );

	pager_printf(ch, " %s is a %sdeadly player",
	             victim->sex == SEX_MALE ? "He" :
	             victim->sex == SEX_FEMALE ? "She" : "It",
	             IS_SET(victim->pcdata->flags, PCFLAG_DEADLY) ? "" : "non-");

	if ( victim->pcdata->clan )
	{
		if ( victim->pcdata->clan->clan_type == CLAN_ORDER )
			send_to_pager( ", and belongs to the Order ", ch );
		else
			if ( victim->pcdata->clan->clan_type == CLAN_GUILD )
				send_to_pager( ", and belongs to the ", ch );
			else
				send_to_pager( ", and belongs to Clan ", ch );
		send_to_pager( victim->pcdata->clan->name, ch );
	}
	send_to_pager( ".\n\r", ch );

	if (victim->pcdata->council)
		pager_printf(ch, " %s holds a seat on:  %s\n\r",
		             victim->sex == SEX_MALE ? "He" :
		             victim->sex == SEX_FEMALE ? "She" : "It",
		             victim->pcdata->council->name );

	if (victim->pcdata->deity)
		pager_printf(ch, " %s has found succor in the deity %s.\n\r",
		             victim->sex == SEX_MALE ? "He" :
		             victim->sex == SEX_FEMALE ? "She" : "It",
		             victim->pcdata->deity->name);

	if (victim->pcdata->homepage && victim->pcdata->homepage[0] != '\0')
		pager_printf(ch, " %s homepage can be found at %s\n\r",
		             victim->sex == SEX_MALE ? "His" :
		             victim->sex == SEX_FEMALE ? "Her" : "Its",
		             show_tilde( victim->pcdata->homepage ) );

	if (victim->pcdata->bio && victim->pcdata->bio[0] != '\0')
		pager_printf(ch, " %s's personal bio:\n\r%s",
		             victim->name,
		             victim->pcdata->bio);
	else
		pager_printf(ch, " %s has yet to create a bio.\n\r",
		             victim->name );
/*
	if( victim->description && victim->description[0] != '\0' )
		pager_printf( ch, " %s's description:\n\r%s",
			     victim->name, victim->description );
*/
	if (IS_IMMORTAL(ch))
	{
		send_to_pager("-------------------\n\r", ch);
		send_to_pager("Info for immortals:\n\r", ch);

		pager_printf( ch, "%s has played for %d hours.\n\r",
			victim->name, ((get_age(victim) - 4) * 2) );

		if ( victim->pcdata->authed_by && victim->pcdata->authed_by[0] != '\0' )
			pager_printf(ch, "%s was authorized by %s.\n\r",
			             victim->name, victim->pcdata->authed_by );

		pager_printf(ch, "%s has killed %d mobiles, and been killed by a mobile %d times.\n\r",
		             victim->name, victim->pcdata->mkills, victim->pcdata->mdeaths );
		if ( victim->pcdata->pkills || victim->pcdata->pdeaths )
			pager_printf(ch, "%s has killed %d players, and been killed by a player %d times.\n\r",
			             victim->name, victim->pcdata->pkills, victim->pcdata->pdeaths );
		if ( victim->pcdata->illegal_pk )
			pager_printf(ch, "%s has committed %d illegal player kills.\n\r",
			             victim->name, victim->pcdata->illegal_pk );

		pager_printf(ch, "%s is %shelled at the moment.\n\r",
		             victim->name,
		             (victim->pcdata->release_date == 0) ? "not " : "");

		if (victim->pcdata->nuisance )
		{
			pager_printf_color( ch, "&RNuisance   &cStage: (&R%d&c/%d)  Power:  &w%d  &cTime:  &w%s.\n\r", victim->pcdata->nuisance->flags,
			                    MAX_NUISANCE_STAGE, victim->pcdata->nuisance->power,
			                    ctime(&victim->pcdata->nuisance->time));
		}
		if (victim->pcdata->release_date != 0)
			pager_printf(ch, "%s was helled by %s, and will be released on %24.24s.\n\r",
			             victim->sex == SEX_MALE ? "He" :
			             victim->sex == SEX_FEMALE ? "She" : "It",
			             victim->pcdata->helled_by,
			             ctime(&victim->pcdata->release_date));

		if( victim->pcdata->silence != 0 )
		{
		    pager_printf( ch, "%s was silenced by %s for %d minutes.\n\r",
			victim->sex == SEX_MALE ? "He" :
			victim->sex == SEX_FEMALE ? "She" : "It",
			victim->pcdata->silencedby,
			victim->pcdata->silence );
		}

		if (get_trust(victim) < get_trust(ch))
		{
			sprintf(buf2, "list %s", buf);
			do_comment(ch, buf2);
		}

		if (xIS_SET(victim->act, PLR_SILENCE) || xIS_SET(victim->act, PLR_NO_EMOTE)
		    || xIS_SET(victim->act, PLR_NO_TELL) || xIS_SET(victim->act, PLR_THIEF)
		    || xIS_SET(victim->act, PLR_KILLER) )
		{
			sprintf(buf2, "This player has the following flags set:");
			if (xIS_SET(victim->act, PLR_SILENCE))
				strcat(buf2, " silence");
			if (xIS_SET(victim->act, PLR_NO_EMOTE))
				strcat(buf2, " noemote");
			if (xIS_SET(victim->act, PLR_NO_TELL) )
				strcat(buf2, " notell");
			if (xIS_SET(victim->act, PLR_THIEF) )
				strcat(buf2, " thief");
			if (xIS_SET(victim->act, PLR_KILLER) )
				strcat(buf2, " killer");
			strcat(buf2, ".\n\r");
			send_to_pager(buf2, ch);
		}
		if ( victim->desc && victim->desc->host[0] != '\0' )   /* added by Gorog */
		{
			sprintf (buf2, "%s's IP info: %s ", victim->name, victim->desc->host);
			if (get_trust(ch) >= LEVEL_GOD)
				strcat (buf2, victim->desc->user);
			strcat (buf2, "\n\r");
			send_to_pager(buf2, ch);
		}
	}
}

void do_pager( CHAR_DATA *ch, char *argument )
{
	char arg[MAX_INPUT_LENGTH];

	if ( IS_NPC(ch) )
		return ;
	set_char_color( AT_NOTE, ch );
	argument = one_argument(argument, arg);
	if ( !*arg )
	{
		if ( IS_SET(ch->pcdata->flags, PCFLAG_PAGERON) )
		{
			send_to_char( "Pager disabled.\n\r", ch );
			do_config(ch, "-pager");
		}
		else
		{
			ch_printf( ch, "Pager is now enabled at %d lines.\n\r", ch->pcdata->pagerlen );
			do_config(ch, "+pager");
		}
		return ;
	}
	if ( !is_number(arg) )
	{
		send_to_char( "Set page pausing to how many lines?\n\r", ch );
		return ;
	}
	ch->pcdata->pagerlen = atoi(arg);
	if ( ch->pcdata->pagerlen < 5 )
		ch->pcdata->pagerlen = 5;
	ch_printf( ch, "Page pausing set to %d lines.\n\r", ch->pcdata->pagerlen );
	return ;
}

/*
 * The ignore command allows players to ignore up to MAX_IGN
 * other players. Players may ignore other characters whether
 * they are online or not. This is to prevent people from
 * spamming someone and then logging off quickly to evade
 * being ignored.
 * Syntax:
 *	ignore		-	lists players currently ignored
 *	ignore none	-	sets it so no players are ignored
 *	ignore <player>	-	start ignoring player if not already
 *				ignored otherwise stop ignoring player
 *	ignore reply	-	start ignoring last player to send a
 *				tell to ch, to deal with invis spammers
 * Last Modified: June 26, 1997
 * - Fireblade
 */
void do_ignore(CHAR_DATA *ch, char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	IGNORE_DATA *temp, *next;
	char fname[1024];
	struct stat fst;
	CHAR_DATA *victim;

	if (IS_NPC(ch))
		return ;

	argument = one_argument(argument, arg);

	sprintf(fname, "%s%c/%s", PLAYER_DIR,
	        tolower(arg[0]), capitalize(arg));

	victim = NULL;

	/* If no arguements, then list players currently ignored */
	if (arg[0] == '\0')
	{
		set_char_color(AT_DIVIDER, ch);
		ch_printf(ch, "\n\r----------------------------------------\n\r");
		set_char_color(AT_DGREEN, ch);
		ch_printf(ch, "You are currently ignoring:\n\r");
		set_char_color(AT_DIVIDER, ch);
		ch_printf(ch, "----------------------------------------\n\r");
		set_char_color(AT_IGNORE, ch);

		if (!ch->pcdata->first_ignored)
		{
			ch_printf(ch, "\t    no one\n\r");
			return ;
		}

		for (temp = ch->pcdata->first_ignored; temp;
		     temp = temp->next)
		{
			ch_printf(ch, "\t  - %s\n\r", temp->name);
		}

		return ;
	}
	/* Clear players ignored if given arg "none" */
	else if (!strcmp(arg, "none"))
	{
		for (temp = ch->pcdata->first_ignored; temp; temp = next)
		{
			next = temp->next;
			UNLINK(temp, ch->pcdata->first_ignored,
			       ch->pcdata->last_ignored,
			       next, prev);
			STRFREE(temp->name);
			DISPOSE(temp);
		}

		set_char_color(AT_IGNORE, ch);
		ch_printf(ch, "You now ignore no one.\n\r");

		return ;
	}
	/* Prevent someone from ignoring themself... */
	else if (!strcmp(arg, "self") || nifty_is_name(arg, ch->name))
	{
		set_char_color(AT_IGNORE, ch);
		ch_printf(ch, "Did you type something?\n\r");
		return ;
	}
	else
	{
		int i;

		/* get the name of the char who last sent tell to ch */
		if (!strcmp(arg, "reply"))
		{
			if (!ch->reply)
			{
				set_char_color(AT_IGNORE, ch);
				ch_printf(ch, "They're not here.\n\r");
				return ;
			}
			else
			{
				strcpy(arg, ch->reply->name);
			}
		}

		/* Loop through the linked list of ignored players */
		/* 	keep track of how many are being ignored     */
		for (temp = ch->pcdata->first_ignored, i = 0; temp;
		     temp = temp->next, i++)
		{
			/* If the argument matches a name in list remove it */
			if (!strcmp(temp->name, capitalize(arg)))
			{
				UNLINK(temp, ch->pcdata->first_ignored,
				       ch->pcdata->last_ignored,
				       next, prev);
				set_char_color(AT_IGNORE, ch);
				ch_printf(ch, "You no longer ignore %s.\n\r",
				          temp->name);
				STRFREE(temp->name);
				DISPOSE(temp);
				return ;
			}
		}

		/* if there wasn't a match check to see if the name   */
		/* is valid. This if-statement may seem like overkill */
		/* but it is intended to prevent people from doing the*/
		/* spam and log thing while still allowing ya to      */
		/* ignore new chars without pfiles yet...             */
		if ( stat(fname, &fst) == -1 &&
		     (!(victim = get_char_world(ch, arg)) ||
		      IS_NPC(victim) ||
		      strcmp(capitalize(arg), victim->name) != 0))
		{
			set_char_color(AT_IGNORE, ch);
			ch_printf(ch, "No player exists by that"
			          " name.\n\r");
			return ;
		}

		if (victim)
		{
			strcpy(capitalize(arg), victim->name);
		}

		/* If its valid and the list size limit has not been */
		/* reached create a node and at it to the list	     */
		if (i < MAX_IGN)
		{
			IGNORE_DATA *new;
			CREATE(new, IGNORE_DATA, 1);
			new->name = STRALLOC(capitalize(arg));
			new->next = NULL;
			new->prev = NULL;
			LINK(new, ch->pcdata->first_ignored,
			     ch->pcdata->last_ignored, next, prev);
			set_char_color(AT_IGNORE, ch);
			ch_printf(ch, "You now ignore %s.\n\r", new->name);
			return ;
		}
		else
		{
			set_char_color(AT_IGNORE, ch);
			ch_printf(ch, "You may only ignore %d players.\n\r",
			          MAX_IGN);
			return ;
		}
	}
}

/*
 * This function simply checks to see if ch is ignoring ign_ch.
 * Last Modified: October 10, 1997
 * - Fireblade
 */
bool is_ignoring(CHAR_DATA *ch, CHAR_DATA *ign_ch)
{
	IGNORE_DATA *temp;

	if (IS_NPC(ch) || IS_NPC(ign_ch))
		return FALSE;

        if( !ch->pcdata->first_ignored )
          return FALSE;

	if( IS_IMMORTAL(ign_ch) )
	  return FALSE;

	for (temp = ch->pcdata->first_ignored; temp; temp = temp->next)
	{
		if (nifty_is_name(temp->name, ign_ch->name))
			return TRUE;
	}

	return FALSE;
}

/* Version info -- Scryn */

void do_version(CHAR_DATA* ch, char* argument)
{
	if (IS_NPC(ch))
		return ;

	set_char_color(AT_YELLOW, ch);
	ch_printf(ch, "SMAUG %s.%s\n\r", SMAUG_VERSION_MAJOR, SMAUG_VERSION_MINOR);

	if (IS_IMMORTAL(ch))
		ch_printf(ch, "Compiled on %s at %s.\n\r", __DATE__, __TIME__);

	return ;
}

void do_upgrade( CHAR_DATA *ch, char *argument )
{
	char buf[MAX_STRING_LENGTH];
	int sn;
	bool prac = FALSE;

	if ( IS_NPC(ch) )
		return ;

	if ( !is_android(ch) && !is_superandroid(ch) )
	{
		send_to_char( "Huh?\n\r", ch );
		return ;
	}

	if ( argument[0] == '\0' )
	{
		int	col;
		sh_int	lasttype, cnt;

		col = cnt = 0;
		lasttype = SKILL_SPELL;
		set_pager_color( AT_MAGIC, ch );
		for ( sn = 0; sn < top_sn; sn++ )
		{
			if ( !skill_table[sn]->name )
				break;

			if( sn == gsn_instant_trans && ((is_android(ch)
                    		|| is_superandroid(ch)) &&
                    		!wearing_sentient_chip(ch)) )
                          continue;

			if (skill_table[sn]->type == SKILL_SPELL || skill_table[sn]->type == SKILL_WEAPON
			    || skill_table[sn]->type == SKILL_TONGUE )
				continue;

			if ( strcmp(skill_table[sn]->name, "reserved") == 0
			     && ( IS_IMMORTAL(ch) || CAN_CAST(ch) ) )
			{
				if ( col % 3 != 0 )
					send_to_pager( "\n\r", ch );
				set_pager_color( AT_MAGIC, ch );
				send_to_pager_color(
				    " ----------------------------------[&CSpells&B]----------------------------------\n\r", ch);
				col = 0;
			}
			if ( skill_table[sn]->type != lasttype )
			{
				if ( !cnt )
					send_to_pager( "\n\r", ch );
				else
					if ( col % 3 != 0 )
						send_to_pager( "\n\r", ch );
				set_pager_color( AT_MAGIC, ch );
				if (skill_table[sn]->type == SKILL_SKILL)
					pager_printf_color( ch,
					                    " ------------------------------------&CSkills&B-----------------------------------\n\r");
				else if (skill_table[sn]->type == SKILL_ABILITY)
					pager_printf_color( ch,
					                    " ----------------------------------&CAbilities&B----------------------------------\n\r");
				else
					pager_printf_color( ch,
					                    " -----------------------------------&C%ss&B-----------------------------------\n\r",
					                    skill_tname[skill_table[sn]->type]);
				col = cnt = 0;
			}
			lasttype = skill_table[sn]->type;

			if (( skill_table[sn]->guild != CLASS_NONE
			      && ( !IS_GUILDED(ch)
			           || (ch->pcdata->clan->class != skill_table[sn]->guild) ) ) )
				continue;

			if ( ( ch->level < skill_table[sn]->min_level[ch->class]
			       && skill_table[sn]->skill_level[ch->class] > ch->exp )
			     || ( skill_table[sn]->min_level[ch->class] == 0 )
			     || (skill_table[sn]->skill_level[ch->class] == 0)
			     || (ch->pcdata->learned[sn] == 0)
			   )
				continue;

			if ( ch->pcdata->learned[sn] <= 0
			     && SPELL_FLAG(skill_table[sn], SF_SECRETSKILL) )
				continue;

			++cnt;
			set_pager_color( AT_MAGIC, ch );
			pager_printf( ch, "%20.20s", skill_table[sn]->name );
			if ( ch->pcdata->learned[sn] > 0 )
				set_pager_color( AT_SCORE, ch );
			pager_printf( ch, " %3.0f%% ", ch->pcdata->learned[sn] );
			if ( ++col % 3 == 0 )
				send_to_pager( "\n\r", ch );
		}

		if ( col % 3 != 0 )
			send_to_pager( "\n\r", ch );
		set_pager_color( AT_MAGIC, ch );
		send_to_pager_color(
		    " -----------------------------------------------------------------------------\n\r", ch);
		//	pager_printf( ch, "You have %d software upgrade points left.\n\r", ch->practice );
		pager_printf( ch, "You have %d hardware upgrade points left.\n\r", ch->train );
	}
	else
	{
		CHAR_DATA *mob;
		int adept;
		bool can_prac = TRUE;

		if ( !IS_AWAKE(ch) )
		{
			send_to_char( "In your dreams, or what?\n\r", ch );
			return ;
		}

		for ( mob = ch->in_room->first_person; mob; mob = mob->next_in_room )
		{
			if ( IS_NPC(mob) && xIS_SET(mob->act, ACT_UPGRADE) )
			{
				prac = TRUE;
				break;
			}
			if ( IS_NPC(mob) && xIS_SET(mob->act, ACT_TRAIN) )
			{
				prac = FALSE;
				break;
			}
		}

		if ( !mob )
		{
			send_to_char( "You can't do that here.\n\r", ch );
			return ;
		}

		if (IS_IMMORTAL(ch))
		{
			sprintf( buf, "Administrators don't need to upgrade.\n\r");
			send_to_char(buf, ch);
			return ;
		}

		/* 'practice' section */

		if ( prac )
		{
			/*
				if (ch->practice <= 0 )
				{
				    act( AT_TELL, "$n tells you 'You must get some more software upgrade points.'",
					mob, NULL, ch, TO_VICT );
				    return;
				}
			*/
			sn = skill_lookup( argument );

			if (sn == -1)
			{
				act( AT_TELL, "$n tells you 'That is not a skill.'", mob, NULL, ch, TO_VICT );
				return ;
			}

			if (skill_table[sn]->min_level[ch->class] == 0
			    || skill_table[sn]->skill_level[ch->class] == 0)
			{
				act( AT_TELL, "$n tells you 'You can not add that.'",
				     mob, NULL, ch, TO_VICT );
				return ;
			}
			if( sn == gsn_instant_trans && ((is_android(ch)
                    		|| is_superandroid(ch)) &&
                    		!wearing_sentient_chip(ch)) )
                        {
				act( AT_TELL, "$n tells you 'You can not add that.'",
                                     mob, NULL, ch, TO_VICT );
                                return;
			}

			/*	    ( ( ch->level < skill_table[sn]->min_level[ch->class]
				    	&& skill_table[sn]->skill_level[ch->class] > ch->exp )
				    || (  skill_table[sn]->min_level[ch->class] == 0 )
				    || (skill_table[sn]->skill_level[ch->class] == 0))
			*/
			if ( can_prac
			     && ( ( sn == -1 )
			          || ( !IS_NPC(ch)
			               && ch->exp < skill_table[sn]->skill_level[ch->class]
			               && ch->level < skill_table[sn]->min_level[ch->class]
			               /* OUT FOR THIS PORT -SHADDAI
			                                   &&  ch->exp < skill_table[sn]->race_level[ch->race]
			               */
			             )
			        )
			   )
			{
				act( AT_TELL, "$n tells you 'You're not ready to add that yet...'",
				     mob, NULL, ch, TO_VICT );
				return ;
			}

			if ( is_name( skill_tname[skill_table[sn]->type], CANT_PRAC ) )
			{
				act( AT_TELL, "$n tells you 'I do not know how make that upgrade.'",
				     mob, NULL, ch, TO_VICT );
				return ;
			}

			/*
			 * Skill requires a special teacher
			 */
			if ( skill_table[sn]->teachers && skill_table[sn]->teachers[0] != '\0' )
			{
				sprintf( buf, "%d", mob->pIndexData->vnum );
				if ( !is_name( buf, skill_table[sn]->teachers ) )
				{
					act( AT_TELL, "$n tells you, 'I do not know how to make that upgrade.'",
					     mob, NULL, ch, TO_VICT );
					return ;
				}
			}

			/*
			 * Guild checks - right now, cant practice guild skills - done on
			 * induct/outcast
			 */
			/*
				if ( !IS_NPC(ch) && !IS_GUILDED(ch)
				&&    skill_table[sn]->guild != CLASS_NONE)
				{
				    act( AT_TELL, "$n tells you 'Only guild members can use that..'"
					mob, NULL, ch, TO_VICT );
				    return;
				}

				if ( !IS_NPC(ch) && skill_table[sn]->guild != CLASS_NONE
				     && ch->pcdata->clan->class != skill_table[sn]->guild )
				{
				    act( AT_TELL, "$n tells you 'That can not be used by your guild.'"
					mob, NULL, ch, TO_VICT );
				    return;
				}
			*/
			if ( !IS_NPC(ch) && skill_table[sn]->guild != CLASS_NONE)
			{
				act( AT_TELL, "$n tells you 'That is only for members of guilds...'",
				     mob, NULL, ch, TO_VICT );
				return ;
			}

			/*
			 * Disabled for now
			if ( mob->level < skill_table[sn]->skill_level[ch->class]
			||   mob->level < skill_table[sn]->skill_level[mob->class] )
			{
			    act( AT_TELL, "$n tells you 'You must seek another to teach you that...'",
				mob, NULL, ch, TO_VICT );
			    return;
			}
			 */

			adept = class_table[ch->class]->skill_adept * 0.2;

			if ( ch->pcdata->learned[sn] > 0 )
			{
				sprintf( buf, "$n tells you, 'I've already installed all the data I can for %s.'",
				         skill_table[sn]->name );
				act( AT_TELL, buf, mob, NULL, ch, TO_VICT );
				act( AT_TELL, "$n tells you, 'You'll have to let it adapt to your system on your own now...'",
				     mob, NULL, ch, TO_VICT );
			}
			else
			{
				//	    ch->practice--;
				//ch->pcdata->learned[sn] = START_ADEPT;
				/* Androids, being purely machines,
				   do not have to learn skills in the same
				   mannor as flesh beings. - Karma */
				ch->pcdata->learned[sn] = 95;
                                if( sn == gsn_self_destruct )
                                {
                                  ch->pcdata->learned[sn] = 95;
                                }
				act( AT_ACTION, "You get $T installed.",
				     ch, NULL, skill_table[sn]->name, TO_CHAR );
				act( AT_ACTION, "$n gets $T installed.",
				     ch, NULL, skill_table[sn]->name, TO_ROOM );
                                if( sn == gsn_self_destruct )
                                {
				  act( AT_TELL,
				       "$n tells you. 'This upgrade comes fully functional out of the box!'",
				       mob, NULL, ch, TO_VICT );
                                }
                                else
                                {
				  act( AT_TELL,
				       "$n tells you. 'You'll have to let it adapt to your system on your own now...'",
				       mob, NULL, ch, TO_VICT );
                                }
			}
		}

		/* 'train' section */

		else
		{
			sh_int *pAbility;
			char *pOutput;

			if ( !str_cmp( argument, "str" ) || !str_cmp( argument, "strength" ) )
			{
				pAbility = &ch->perm_str;
				pOutput = "strength";
			}

			else if ( !str_cmp( argument, "int" ) || !str_cmp( argument, "intelligence" ))
			{
				pAbility = &ch->perm_int;
				pOutput = "intelligence";
			}

			else if ( !str_cmp( argument, "spd" ) || !str_cmp( argument, "speed" ))
			{
				pAbility = &ch->perm_dex;
				pOutput = "speed";
			}

			else if ( !str_cmp( argument, "con" ) || !str_cmp( argument, "constitution" ) )
			{
				pAbility = &ch->perm_con;
				pOutput = "constitution";
			}

			else
			{
				strcpy( buf, "You are already at your max" );
				if ( ch->perm_str >= 100 )
					strcat( buf, " strength" );
				if ( ch->perm_int >= 100 )
					strcat( buf, " intelligence" );
				if ( ch->perm_dex >= 100 )
					strcat( buf, " speed" );
				if ( ch->perm_con >= 100 )
					strcat( buf, " constitution" );

				strcat( buf, ".\n\r" );
				send_to_char( buf, ch );


				return ;
			}


			if ( *pAbility >= 100 )  /* 18 is the max you can train something to unless you change it here  -Joe */
			{
				act(AT_RED, "Your $T is already at maximum.", ch, NULL, pOutput, TO_CHAR );
				return ;
			}

			if ( ch->train < 1 )
			{
				send_to_char( "You don't have enough hardware upgrade points.\n\r", ch );
				return ;
			}

			ch->train	-= 1;
			*pAbility += 1;
			act(AT_RED, "You have your $T upgraded!", ch, NULL, pOutput, TO_CHAR );
			act(AT_RED, "$n's $T has been upgraded!", ch, NULL, pOutput, TO_ROOM );

		}
	}
	return ;
}

/* Find non-existant help files. -Sadiq */
void do_nohelps(CHAR_DATA *ch, char *argument)
{
	CMDTYPE *command;
	AREA_DATA *tArea;
	char arg[MAX_STRING_LENGTH];
	int hash, col = 0, sn = 0;

	argument = one_argument( argument, arg );

	if (!IS_IMMORTAL(ch) || IS_NPC(ch) )
	{
		send_to_char("Huh?\n\r", ch);
		return ;
	}

	if ( arg[0] == '\0' || !str_cmp(arg, "all") )
	{
		do_nohelps(ch, "commands");
		send_to_char( "\n\r", ch);
		do_nohelps(ch, "skills");
		send_to_char( "\n\r", ch);
		do_nohelps(ch, "areas");
		send_to_char( "\n\r", ch);
		return ;
	}

	if (!str_cmp(arg, "commands") )
	{
		send_to_char_color("&C&YCommands for which there are no help files:\n\r\n\r", ch);

		for ( hash = 0; hash < 126; hash++ )
		{
			for ( command = command_hash[hash]; command; command = command->next )
			{
				if (!get_help(ch, command->name) )
				{
					ch_printf_color(ch, "&W%-15s", command->name);
					if ( ++col % 5 == 0 )
					{
						send_to_char( "\n\r", ch );
					}
				}
			}
		}

		send_to_char("\n\r", ch);
		return ;
	}

	if (!str_cmp(arg, "skills") || !str_cmp(arg, "spells") )
	{
		send_to_char_color("&CSkills/Spells for which there are no help files:\n\r\n\r", ch);

		for ( sn = 0; sn < top_sn && skill_table[sn] && skill_table[sn]->name; sn++ )
		{
			if (!get_help(ch, skill_table[sn]->name))
			{
				ch_printf_color(ch, "&W%-20s", skill_table[sn]->name);
				if ( ++col % 4 == 0 )
				{
					send_to_char("\n\r", ch);
				}
			}
		}

		send_to_char("\n\r", ch);
		return ;
	}

	if (!str_cmp(arg, "areas") )
	{
		send_to_char_color("&GAreas for which there are no help files:\n\r\n\r", ch);

		for (tArea = first_area; tArea;tArea = tArea->next)
		{
			if (!get_help(ch, tArea->name) )
			{
				ch_printf_color(ch, "&W%-35s", tArea->name);
				if ( ++col % 2 == 0 )
				{
					send_to_char("\n\r", ch);
				}
			}
		}

		send_to_char( "\n\r", ch);
		return ;
	}

	send_to_char("Syntax:  nohelps <all|areas|commands|skills>\n\r", ch);
	return ;
}

void do_partner( CHAR_DATA *ch, char *argument )
{
	CHAR_DATA *victim;

	if ( !ch->desc )
		return ;

	if ( IS_NPC( ch ) )
		return ;

	if ( argument[0] == '\0' )
	{
		send_to_char( "You have to pick someone as your sparring partner!\n\r", ch );
		return ;
	}

	if ( (victim = get_char_room(ch, argument)) != NULL )
	{
		if ( ch->pcdata->HBTCPartner )
			STRFREE( ch->pcdata->HBTCPartner );
		ch->pcdata->HBTCPartner = STRALLOC( victim->name );
		act( AT_ACTION, "You've picked $N as your sparring partner.",
		     ch, NULL, victim, TO_CHAR );
		act( AT_ACTION, "$n has picked you as $s sparring partner.",
		     ch, NULL, victim, TO_VICT );
	}
	else
	{
		send_to_char( "No one by that name is here!\n\r", ch );
	}

	return ;
}

void do_analyze( CHAR_DATA *ch, char *argument )
{
    OBJ_DATA *obj;
    AFFECT_DATA *paf;
    long double range = 0;
    char buf[MAX_STRING_LENGTH];

    if( (obj = get_obj_carry(ch,argument)) == NULL )
    {
	ch_printf(ch,"You aren't carrying anything like that.\n\r");
	return;
    }
    ch_printf(ch,"\n\r&zObject: &C%s\n\r", obj->short_descr );
    ch_printf(ch,"&zPl Req: &C%s\n\r", num_punct_ld(obj->level) );
    ch_printf(ch,"\n\r");

    ch_printf(ch,"&zSpecial properties: &C%s\n\r",
		extra_bit_name( &obj->extra_flags ) );

    if ( obj->item_type != ITEM_LIGHT && obj->wear_flags - 1 > 0 )
       ch_printf( ch, "&zItem's wear location: &C%s&C\n\r",
            flag_string(obj->wear_flags -1, w_flags ) );

    ch_printf(ch,"\n\r");

    switch ( obj->item_type )
    {
      case ITEM_CONTAINER:
      case ITEM_KEYRING:
      case ITEM_QUIVER:
          ch_printf( ch, "%s appears to %s.\n\r",capitalize(obj->short_descr),
                   obj->value[0] < 76 ? "have a small capacity" :
                   obj->value[0] < 150 ? "have a small to medium capacity" :
                   obj->value[0] < 300 ? "have a medium capacity" :
                   obj->value[0] < 500 ? "have a medium to large capacity" :
                   obj->value[0] < 751 ? "have a large capacity" :
                   "have a giant capacity" );
          break;

      case ITEM_PILL:
      case ITEM_SCROLL:
      case ITEM_POTION:
           send_to_char( ".\n\r", ch );
           break;

      case ITEM_WAND:
      case ITEM_STAFF:
           sprintf( buf, "Has %d(%d) charges of level %d",
                    obj->value[1], obj->value[2], obj->value[0] );
           send_to_char( buf, ch );

           if ( obj->value[3] >= 0 && obj->value[3] < top_sn )
           {
             send_to_char( " '", ch );
             send_to_char( skill_table[obj->value[3]]->name, ch );
             send_to_char( "'", ch );
           }
           send_to_char( ".\n\r", ch );
           break;

      case ITEM_MISSILE_WEAPON:
      case ITEM_WEAPON:
           sprintf( buf, "Damage is %d to %d (average %d).%s\n\r",
                    obj->value[1], obj->value[2],
                    ( obj->value[1] + obj->value[2] ) / 2,
                    IS_OBJ_STAT( obj, ITEM_POISONED) ?
                    "\n\rThis weapon is poisoned." : "" );
           send_to_char( buf, ch );
           break;

      case ITEM_ARMOR:
           sprintf( buf, "Armor rating is %d/%d.\n\r", obj->value[4], obj->value[5] );
           send_to_char( buf, ch );
           break;

      case ITEM_SCOUTER:
           range = obj->value[2];
           range *= 100;
           sprintf( buf,"Scouter pl range is 1 to %s.\n\r", num_punct_ld(range) );
           send_to_char( buf, ch );
           break;
      case ITEM_DRAGONBALL:
           sprintf( buf, "This is a real dragonball.\n\r" );
           send_to_char( buf, ch );
           break;

      default:
	   break;
    }

    ch_printf(ch,"\n\r");

    for ( paf = obj->pIndexData->first_affect; paf; paf = paf->next )
          showaffect( ch, paf );

    for ( paf = obj->first_affect; paf; paf = paf->next )
          showaffect( ch, paf );


}

/*eof*/
