#include <sys/types.h>
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "mud.h"

extern void transStatApply args((CHAR_DATA *ch, int strMod, int spdMod, int intMod, int conMod));
extern void transStatRemove args((CHAR_DATA *ch));

bool removeGenieTrans( CHAR_DATA *ch )
{
  if( xIS_SET( ch->affected_by, AFF_EVIL_TRANS ) )
  {
    xREMOVE_BIT( ch->affected_by, AFF_EVIL_TRANS );
    transStatRemove( ch );
    return TRUE;
  }
  if( xIS_SET( ch->affected_by, AFF_SUPER_TRANS ) )
  {
    xREMOVE_BIT( ch->affected_by, AFF_SUPER_TRANS );
    transStatRemove( ch );
    return TRUE;
  }
  if( xIS_SET( ch->affected_by, AFF_KID_TRANS ) )
  {
    xREMOVE_BIT( ch->affected_by, AFF_KID_TRANS );
    transStatRemove( ch );
    return TRUE;
  }
  return FALSE;
}

void do_transform( CHAR_DATA *ch, char *argument )
{
    if( !is_genie( ch ) )
    {
	send_to_char( "You can't do that.\n\r", ch );
	return;
    }

    if( !argument || argument[0] == '\0' )
    {
	send_to_char( "Syntax: transform [thin|super|kid]\n\r", ch );
	return;
    }

    if( !str_prefix( argument, "thin" ) )
    {
	if( ch->exp < skill_table[gsn_thin_trans]->min_level[ch->race] )
	{
	    send_to_char( "You can't do that at your level.\n\r", ch );
	    return;
	}

	if( ch->mana < skill_table[gsn_thin_trans]->min_mana )
	{
	    send_to_char( "You don't have enough energy.\n\r", ch );
	    return;
	}

	WAIT_STATE( ch, skill_table[gsn_thin_trans]->beats );

	if ( can_use_skill( ch, number_percent(), gsn_thin_trans ) )
	{
	    removeGenieTrans( ch );
	    act( AT_PINK, "Steam courses through the series of holes along your head, as you begin", ch, NULL, NULL, TO_CHAR );
	    act( AT_PINK, "to let out a high-pitched shriek.  Your rolling layers of pink flesh", ch, NULL, NULL, TO_CHAR );
	    act( AT_PINK, "rapidly compact, giving you a leaner, more sinister look.", ch, NULL, NULL, TO_CHAR );

	    act( AT_PINK, "Steam courses through the series of holes along $n's head as $e begins", ch, NULL, NULL, TO_NOTVICT );
	    act( AT_PINK, "to let out a high-pitched shriek.  $*s rolling layers of pink flesh rapidly", ch, NULL, NULL, TO_NOTVICT );
	    act( AT_PINK, "compact, giving $m a leaner, more sinister look.", ch, NULL, NULL, TO_NOTVICT );
	    xSET_BIT( ch->affected_by, AFF_EVIL_TRANS );
	    ch->pl = ch->exp * 12;
	    transStatApply(ch, 9, 6, 6, 9);
	    learn_from_success( ch, gsn_thin_trans );
	}
	else
	{
	    act( AT_PLAIN, "You failed.", ch, NULL, NULL, TO_CHAR );
	    learn_from_failure( ch, gsn_thin_trans );
	}

	ch->mana -= skill_table[gsn_thin_trans]->min_mana;
	return;
    }

    if( !str_prefix( argument, "super" ) )
    {
	if( ch->exp < skill_table[gsn_super_trans]->min_level[ch->race] )
	{
	    send_to_char( "You can't do that at your level.\n\r", ch );
	    return;
	}

	if( !xIS_SET( ch->affected_by, AFF_EVIL_TRANS )
	&&  !xIS_SET( ch->affected_by, AFF_KID_TRANS ) )
	{
	    send_to_char( "You can't do that in this form.\n\r", ch );
	    return;
	}

	if( ch->mana < skill_table[gsn_super_trans]->min_mana )
	{
	    send_to_char( "You don't have enough energy.\n\r", ch );
	    return;
	}

	WAIT_STATE( ch, skill_table[gsn_super_trans]->beats );

	if ( can_use_skill( ch, number_percent(), gsn_super_trans ) )
	{
	    removeGenieTrans( ch );
	    act( AT_PINK, "You utter a low-pitched growl, steam blasting through the porting in", ch, NULL, NULL, TO_CHAR );
	    act( AT_PINK, "your head, as you draw energy from your absorbed victims.  The tentacle", ch, NULL, NULL, TO_CHAR );
	    act( AT_PINK, "atop your head lengthens to mid-back level and your slender body bulks", ch, NULL, NULL, TO_CHAR );
	    act( AT_PINK, "up, taking on a powerful, muscular look.  An aura of dark purple energy", ch, NULL, NULL, TO_CHAR );
	    act( AT_PINK, "surrounds you.", ch, NULL, NULL, TO_CHAR );

	    act( AT_PINK, "$n utters a low-pitched growl, steam blasting through the porting", ch, NULL, NULL, TO_NOTVICT );
	    act( AT_PINK, "in $s head, as $e draws energy from $s absorbed victims.  The tentacle", ch, NULL, NULL, TO_NOTVICT );
	    act( AT_PINK, "atop $s head lengthens to mid-back level and $s slender body bulks up,", ch, NULL, NULL, TO_NOTVICT );
	    act( AT_PINK, "taking on a powerful, muscular look.  An aura of dark purple energy", ch, NULL, NULL, TO_NOTVICT );
	    act( AT_PINK, "surrounds $m.", ch, NULL, NULL, TO_NOTVICT );

	    xSET_BIT( ch->affected_by, AFF_SUPER_TRANS );
	    ch->pl = ch->exp * 20;
	    transStatApply(ch, 18, 12, 12, 18);
	    learn_from_success( ch, gsn_super_trans );
	}
	else
	{
	    act( AT_PLAIN, "You failed.", ch, NULL, NULL, TO_CHAR );
	    learn_from_failure( ch, gsn_super_trans );
	}

	ch->mana -= skill_table[gsn_super_trans]->min_mana;
	return;
    }

    if( !str_prefix( argument, "kid" ) )
    {
	if( ch->exp < skill_table[gsn_kid_trans]->min_level[ch->race] )
	{
	    send_to_char( "You can't do that at your level.\n\r", ch );
	    return;
	}

	if( !xIS_SET( ch->affected_by, AFF_SUPER_TRANS ) )
	{
	    send_to_char( "You can't do that in this form.\n\r", ch );
	    return;
	}

	if( ch->mana < skill_table[gsn_kid_trans]->min_mana )
	{
	    send_to_char( "You don't have enough energy.\n\r", ch );
	    return;
	}

	WAIT_STATE( ch, skill_table[gsn_kid_trans]->beats );

	if ( can_use_skill( ch, number_percent(), gsn_kid_trans ) )
	{
	    removeGenieTrans( ch );
	    act( AT_PINK, "You bellow loudly, your voice starting deep and gradually becoming more", ch, NULL, NULL, TO_CHAR );
	    act( AT_PINK, "shrill as the stolen energy gives way to a terrifying strength of your", ch, NULL, NULL, TO_CHAR );
	    act( AT_PINK, "own.  Steam shrieks through the holes along your head, completely masking", ch, NULL, NULL, TO_CHAR );
	    act( AT_PINK, "your presence.  Your form shrinks and compacts and when the steam finally", ch, NULL, NULL, TO_CHAR );
	    act( AT_PINK, "clears, you have transformed into a child-sized version of yourself,", ch, NULL, NULL, TO_CHAR );
	    act( AT_PINK, "radiating a horrifying power.", ch, NULL, NULL, TO_CHAR );

	    act( AT_PINK, "$n bellows loudly, $s voice starting deep and gradually becoming more", ch, NULL, NULL, TO_NOTVICT );
	    act( AT_PINK, "shrill as the stolen energy gives way to a terrifying strength of $s own.", ch, NULL, NULL, TO_NOTVICT );
	    act( AT_PINK, "Steam shrieks through the holes along $s head, completely masking $s", ch, NULL, NULL, TO_NOTVICT );
	    act( AT_PINK, "presence.  $*s form shrinks and compacts and when the steam finally", ch, NULL, NULL, TO_NOTVICT );
	    act( AT_PINK, "clears, $e has transformed into a child-sized version of $mself,", ch, NULL, NULL, TO_NOTVICT );
	    act( AT_PINK, "radiating a horrifiyng power.", ch, NULL, NULL, TO_NOTVICT );

	    xSET_BIT( ch->affected_by, AFF_KID_TRANS );
	    ch->pl = ch->exp * 28;
	    transStatApply(ch, 27, 18, 18, 27);
	    learn_from_success( ch, gsn_kid_trans );
	}
	else
	{
	    act( AT_PLAIN, "You failed.", ch, NULL, NULL, TO_CHAR );
	    learn_from_failure( ch, gsn_kid_trans );
	}

	ch->mana -= skill_table[gsn_kid_trans]->min_mana;
	return;
    }

    do_transform( ch, "" );

    return;
}

void do_demon_wave( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *victim;
    int dam = 0;

    if( !IS_NPC( ch ) && ch->exp < skill_table[gsn_demonwave]->skill_level[ch->class] )
    {
	send_to_char( "You can't do that.\n\r", ch );
	return;
    }

    if( ( victim = who_fighting( ch ) ) == NULL )
    {
	send_to_char( "You aren't fighting anyone.\n\r", ch );
	return;
    }

    if( ch->mana < skill_table[gsn_demonwave]->min_mana )
    {
	send_to_char( "You don't have enough energy.\n\r", ch );
	return;
    }

    if( ch->focus < skill_table[gsn_demonwave]->focus )
    {
	send_to_char( "You need to focus more.\n\r", ch );
	return;
    }

    ch->focus -= skill_table[gsn_demonwave]->focus;
    WAIT_STATE( ch, skill_table[gsn_demonwave]->beats );

    if( can_use_skill( ch, number_percent(), gsn_demonwave ) )
    {
	dam = get_attmod( ch, victim ) * number_range( 22, 28 );
	if( ch->charge > 0 )
	    dam = chargeDamMult( ch, dam );

	act( AT_SKILL, "You cup your hands and grin at $N, \"You go bye bye now!\"  A ball of", ch, NULL, victim, TO_CHAR );
	act( AT_SKILL, "sparkling purple energy appears in your palms and you push your hands", ch, NULL, victim, TO_CHAR );
	act( AT_SKILL, "forward, creating a massive wave of energy that flies at $N! &W[$t]", ch, num_punct(dam), victim, TO_CHAR );

	act( AT_SKILL, "$n cups $s hands and grins at you, \"You go bye bye now!\"  A ball of", ch, NULL, victim, TO_VICT );
	act( AT_SKILL, "sparkling purple energy appears in $s palms and $e pushes $s hands", ch, NULL, victim, TO_VICT );
	act( AT_SKILL, "forward, creating a massive wave of energy that flies at you! &W[$t]", ch, num_punct(dam), victim, TO_VICT );

	act( AT_SKILL, "$n cups $s hands and grins at $N, \"You go bye bye now!\"  A ball of", ch, NULL, victim, TO_NOTVICT );
	act( AT_SKILL, "sparkling purple energy appears in $s palms and $e pushes $s hands", ch, NULL, victim, TO_NOTVICT );
	act( AT_SKILL, "forward, creating a massive wave of energy that flies at $N! &W[$t]", ch, num_punct(dam), victim, TO_NOTVICT );

	dam = ki_absorb( victim, ch, dam, gsn_demonwave );
	learn_from_success( ch, gsn_demonwave );
	global_retcode = damage( ch, victim, dam, TYPE_HIT );
    }
    else
    {
	act( AT_SKILL, "You missed $N with your demon wave.", ch, NULL, victim, TO_CHAR );
	act( AT_SKILL, "$n misses you with $s demon wave.", ch, NULL, victim, TO_VICT );
	act( AT_SKILL, "$n missed $N with a demon wave.", ch, NULL, victim, TO_NOTVICT );
	learn_from_failure( ch, gsn_demonwave );
	global_retcode = damage( ch, victim, 0, TYPE_HIT );
    }

    ch->mana -= skill_table[gsn_demonwave]->min_mana;
    return;
}

void do_candy_blast( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *victim;
    int dam = 0;

    if( is_namek( ch ) )
    {
	ch_printf( ch, "You are not allowed.\n\r" );
	return;
    }

    if( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) )
    {
	send_to_char( "You can't concentrate enough for that.\n\r", ch );
	return;
    }

    if( !IS_NPC( ch ) && ch->exp < skill_table[gsn_candyblast]->skill_level[ch->class] )
    {
	send_to_char( "You can't do that.\n\r", ch );
	return;
    }

    if( ( victim = who_fighting( ch ) ) == NULL )
    {
	send_to_char( "You aren't fighting anyone.\n\r", ch );
	return;
    }

    if( ch->mana < skill_table[gsn_candyblast]->min_mana )
    {
	send_to_char( "You don't have enough energy.\n\r", ch );
	return;
    }
    if( ch->focus < skill_table[gsn_candyblast]->focus )
    {
	send_to_char( "You need to focus more.\n\r", ch );
	return;
    }
    else
	ch->focus -= skill_table[gsn_candyblast]->focus;

    WAIT_STATE( ch, skill_table[gsn_candyblast]->beats );
    if( can_use_skill( ch, number_percent(), gsn_candyblast ) )
    {
/*	switch( number_range( 1, 100 ) )
	{
	    case 100:
		if( IS_NPC( victim ) || ( !IS_NPC( victim ) && ( !xIS_SET( ch->act, PLR_SPAR ) || !xIS_SET( ch->act, PLR_SPAR ) ) ) )
		{
		    if( victim->pl / ch->pl >= 5 )
		    {
			act( AT_SKILL, "You charge a huge ball of energy on the tip of your finger and then direct it quickly towards $n.", ch, NULL, victim, TO_CHAR );
			act( AT_SKILL, "$n is enveloped by the death ball, but when all is done, seems unaffected.", ch, NULL, victim, TO_CHAR );
			act( AT_SKILL, "$n charges a huge ball of energy on the tip of $s finger and then directs it quickly towards you.", ch, NULL, victim, TO_VICT );
			act( AT_SKILL, "You are enveloped by the death ball, but seem unaffected when it's over.", ch, NULL, victim, TO_VICT );
			act( AT_SKILL, "$n charges a huge ball of energy on the tip of $s finger and then directs it quickly towards $N.", ch, NULL, victim, TO_NOTVICT );
			act( AT_RED, "$N is enveloped by the death ball, but when all is done, seems unaffected.", ch, NULL, victim, TO_NOTVICT );
			dam = 0;
			break;
		    }

		    dam = 999999999;
		    act( AT_SKILL, "You charge a huge ball of energy on the tip of your finger and then direct it quickly towards $N.", ch, NULL, victim, TO_CHAR );
		    act( AT_RED, "$N is slowly enveloped by the death ball, killing $M instantly.", ch, NULL, victim, TO_CHAR );
		    act( AT_SKILL, "$n charges a huge ball of energy on the tip of $s finger and then directs it quickly towards you.", ch, NULL, victim, TO_VICT );
		    act( AT_RED, "You are slowly enveloped by the death ball, killing you instantly.", ch, NULL, victim, TO_VICT );
		    act( AT_SKILL, "$n charges a huge ball of energy on the tip of $s finger and then directs it quickly towards $N.", ch, NULL, victim, TO_NOTVICT );
		    act( AT_RED, "$N is slowly enveloped by the death ball, killing $M instantly.", ch, NULL, victim, TO_NOTVICT );

		    learn_from_success( ch, gsn_candyblast );
		    global_retcode = damage( ch, victim, dam, TYPE_HIT );
		    break;
		}
	    default:
*/		dam = get_attmod( ch, victim ) * number_range( 35, 45 );
		if( ch->charge > 0 )
		    dam = chargeDamMult( ch, dam );

		act( AT_SKILL, "Your head tentacle flips towards $N and fires a pink beam of energy.  The", ch, NULL, victim, TO_CHAR );
		act( AT_SKILL, "ray hits and engulfs $N in a pink blaze that electrifies $S body.  &W[$t]", ch, num_punct(dam), victim, TO_CHAR );

		act( AT_SKILL, "$n's head tentacle flips towards you and fires a pink beam of energy.  The", ch, NULL, victim, TO_VICT );
		act( AT_SKILL, "ray hits and engulfs you in a pink blaze that electrifies your body. &W[$t]", ch, num_punct(dam), victim, TO_VICT );

		act( AT_SKILL, "$n's head tentacle flips towards $N and fires a pink beam of energy.  The", ch, NULL, victim, TO_NOTVICT );
		act( AT_SKILL, "ray hits and engulfs $N in a pink blaze that electrifies $S body. &W[$t]", ch, num_punct(dam), victim, TO_NOTVICT );

		dam = ki_absorb( victim, ch, dam, gsn_candyblast );
		learn_from_success( ch, gsn_candyblast );
		global_retcode = damage( ch, victim, dam, TYPE_HIT );
//		break;
//	}
    }
    else
    {
	act( AT_SKILL, "You missed $N with your candy blast.", ch, NULL, victim, TO_CHAR );
	act( AT_SKILL, "$n misses you with $s candy blast.", ch, NULL, victim, TO_VICT );
	act( AT_SKILL, "$n missed $N with a candy blast.", ch, NULL, victim, TO_NOTVICT );
	learn_from_failure( ch, gsn_candyblast );
	global_retcode = damage( ch, victim, 0, TYPE_HIT );
    }
    ch->mana -= skill_table[gsn_candyblast]->min_mana;
    return;
}

/*
void do_super_ghost_kamikaze( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    CHAR_DATA *victim;
    int shot = 0;
    int shots = 0;
    int dam = 0;
    int energy = 0;
    int damPerShot = 0;

    if( IS_NPC(ch) && IS_AFFECTED( ch, AFF_CHARM ) )
    {
	send_to_char( "You can't concentrate enough for that.\n\r", ch );
	return;
    }

    if( !IS_NPC(ch) && ch->exp < skill_table[gsn_sgkamikaze]->skill_level[ch->class] )
    {
	send_to_char( "You can't do that.\n\r", ch );
	return;
    }

    if( arg[0] == '\0' || atoi( arg ) <= 0 )
    {
	send_to_char( "Syntax: scatter <# of shots>\n\r", ch );
	return;
    }

    if( ( victim = who_fighting( ch ) ) == NULL )
    {
	send_to_char( "You aren't fighting anyone.\n\r", ch );
	return;
    }

    shot = atoi( arg );
    if( shot > 50 )
	shot = 50;
    energy = shot * skill_table[gsn_sgkamikaze]->min_mana;
    shots = shot;
    strcpy( buf, num_punct( shot ) );

    if( ch->mana < energy )
    {
	send_to_char( "You don't have enough energy.\n\r", ch );
	return;
    }
    if( ch->focus < ( skill_table[gsn_sgkamikaze]->focus * ( 1 + ( shot / 10 ) ) ) )
    {
	send_to_char( "You need to focus more.\n\r", ch );
	return;
    }
    else
	ch->focus -= ( skill_table[gsn_sgkamikaze]->focus * ( 1 + ( shot / 10 ) ) );

    WAIT_STATE( ch, ( skill_table[gsn_sgkamikaze]->beats * ( 1 + ( shot / 10 ) ) ) );

    if( can_use_skill( ch, number_percent(), gsn_sgkamikaze ) )
    {
	while( shots > 0 )
	{
	    switch( number_range( 1, 4 ) )
	    {
		default:
		case 1:
		    break;
		case 2:
		case 3:
		    dam += number_range( 0, 1 );
		    break;
		case 4:
		    dam += number_range( 0, 2 );
		    break;
	    }
	    shots--;
	}
	damPerShot = number_range( 1, 3 );
	strcpy( buf2, num_punct( get_attmod( ch, victim ) * dam * damPerShot ) );

	act( AT_SKILL, "You power up and yell, 'SCATTER SHOT!'", ch, NULL, victim, TO_CHAR );
	act2( AT_SKILL, "You launch a barrage of $t energy balls towards $N, "
		"all exploding on contact. &W[$T]", ch, buf, victim, TO_CHAR, buf2 );
	act( AT_SKILL, "$n powers up and yells, 'SCATTER SHOT!'", ch, NULL, victim, TO_VICT );
	act2( AT_SKILL, "$e launches a barrage of $t energy balls towards you, "
		"all exploding on contact. &W[$T]", ch, buf, victim, TO_VICT, buf2 );
	act( AT_SKILL, "$n powers up and yells, 'SCATTER SHOT!'", ch, NULL, victim, TO_NOTVICT );
	act2( AT_SKILL, "$e launches a barrage of $t energy balls towards $N, "
		"all exploding on contact. &W[$T]", ch, buf, victim, TO_NOTVICT, buf2 );

	learn_from_success( ch, gsn_sgkamikaze );
	global_retcode = damage( ch, victim, (get_attmod(ch, victim) * dam * damPerShot), TYPE_HIT );
    }
    else
    {
	act( AT_SKILL, "You launch a barrage of $t energy balls towards $N but $E is to fast to hit.", ch, buf, victim, TO_CHAR );
	act( AT_SKILL, "$n launches a barrage of $t energy balls towards $N but you are to fast to hit.", ch, buf, victim, TO_VICT $
	act( AT_SKILL, "$n launches a barrage of $t energy balls towards $N but $E is to fast to hit.", ch, buf, victim, TO_NOTVICT$
	learn_from_failure( ch, gsn_sgkamikaze );
	global_retcode = damage( ch, victim, 0, TYPE_HIT );
    }
    ch->mana -= energy;
    return;
}
*/

void do_tentacle_attack( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *victim;

    if( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) )
    {
	send_to_char( "You can't concentrate enough for that.\n\r", ch );
	return;
    }

    if( !IS_NPC( ch ) && ch->exp < skill_table[gsn_tentacle]->skill_level[ch->class] )
    {
	send_to_char( "You better leave the martial arts to fighters.\n\r", ch );
	return;
    }

    if( ( victim = who_fighting( ch ) ) == NULL )
    {
	send_to_char( "You aren't fighting anyone.\n\r", ch );
	return;
    }

    if( ch->mana < skill_table[gsn_tentacle]->min_mana )
    {
	send_to_char( "You don't have enough energy.\n\r", ch );
	return;
    }

    if( ch->focus < skill_table[gsn_tentacle]->focus )
    {
	send_to_char( "You need to focus more.\n\r", ch );
	return;
    }
    else
	ch->focus -= skill_table[gsn_tentacle]->focus;

    WAIT_STATE( ch, skill_table[gsn_tentacle]->beats );

    if( can_use_skill( ch, number_percent(), gsn_tentacle ) )
    {
	learn_from_success( ch, gsn_tentacle );
	global_retcode = damage( ch, victim, ( get_attmod( ch, victim ) * number_range( 1, 5 ) ), gsn_tentacle );
    }
    else
    {
	learn_from_failure( ch, gsn_tentacle );
	global_retcode = damage( ch, victim, 0, gsn_tentacle );
    }

    ch->mana -= skill_table[gsn_tentacle]->min_mana;

    return;
}

/*
void do_genocide_attack( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    CHAR_DATA *victim;
    int shot = 0;
    int shots = 0;
    int dam = 0;
    int energy = 0;
    int damPerShot = 0;

    if( IS_NPC(ch) && IS_AFFECTED( ch, AFF_CHARM ) )
    {
	send_to_char( "You can't concentrate enough for that.\n\r", ch );
	return;
    }

    if( !IS_NPC(ch) && ch->exp < skill_table[gsn_sgkamikaze]->skill_level[ch->class] )
    {
	send_to_char( "You can't do that.\n\r", ch );
	return;
    }

    if( arg[0] == '\0' || atoi( arg ) <= 0 )
    {
	send_to_char( "Syntax: scatter <# of shots>\n\r", ch );
	return;
    }

    if( ( victim = who_fighting( ch ) ) == NULL )
    {
	send_to_char( "You aren't fighting anyone.\n\r", ch );
	return;
    }

    shot = atoi( arg );
    if( shot > 50 )
	shot = 50;
    energy = shot * skill_table[gsn_sgkamikaze]->min_mana;
    shots = shot;
    strcpy( buf, num_punct( shot ) );

    if( ch->mana < energy )
    {
	send_to_char( "You don't have enough energy.\n\r", ch );
	return;
    }
    if( ch->focus < ( skill_table[gsn_sgkamikaze]->focus * ( 1 + ( shot / 10 ) ) ) )
    {
	send_to_char( "You need to focus more.\n\r", ch );
	return;
    }
    else
	ch->focus -= ( skill_table[gsn_sgkamikaze]->focus * ( 1 + ( shot / 10 ) ) );

    WAIT_STATE( ch, ( skill_table[gsn_sgkamikaze]->beats * ( 1 + ( shot / 10 ) ) ) );

    if( can_use_skill( ch, number_percent(), gsn_sgkamikaze ) )
    {
	while( shots > 0 )
	{
	    switch( number_range( 1, 4 ) )
	    {
		default:
		case 1:
		    break;
		case 2:
		case 3:
		    dam += number_range( 0, 1 );
		    break;
		case 4:
		    dam += number_range( 0, 2 );
		    break;
	    }
	    shots--;
	}
	damPerShot = number_range( 1, 3 );
	strcpy( buf2, num_punct( get_attmod( ch, victim ) * dam * damPerShot ) );

	act( AT_SKILL, "You power up and yell, 'SCATTER SHOT!'", ch, NULL, victim, TO_CHAR );
	act2( AT_SKILL, "You launch a barrage of $t energy balls towards $N, "
		"all exploding on contact. &W[$T]", ch, buf, victim, TO_CHAR, buf2 );
	act( AT_SKILL, "$n powers up and yells, 'SCATTER SHOT!'", ch, NULL, victim, TO_VICT );
	act2( AT_SKILL, "$e launches a barrage of $t energy balls towards you, "
		"all exploding on contact. &W[$T]", ch, buf, victim, TO_VICT, buf2 );
	act( AT_SKILL, "$n powers up and yells, 'SCATTER SHOT!'", ch, NULL, victim, TO_NOTVICT );
	act2( AT_SKILL, "$e launches a barrage of $t energy balls towards $N, "
		"all exploding on contact. &W[$T]", ch, buf, victim, TO_NOTVICT, buf2 );

	learn_from_success( ch, gsn_sgkamikaze );
	global_retcode = damage( ch, victim, (get_attmod(ch, victim) * dam * damPerShot), TYPE_HIT );
    }
    else
    {
	act( AT_SKILL, "You launch a barrage of $t energy balls towards $N but $E is to fast to hit.", ch, buf, victim, TO_CHAR );
	act( AT_SKILL, "$n launches a barrage of $t energy balls towards $N but you are to fast to hit.", ch, buf, victim, TO_VICT $
	act( AT_SKILL, "$n launches a barrage of $t energy balls towards $N but $E is to fast to hit.", ch, buf, victim, TO_NOTVICT$
	learn_from_failure( ch, gsn_sgkamikaze );
	global_retcode = damage( ch, victim, 0, TYPE_HIT );
    }
    ch->mana -= energy;
    return;
}
*/

void do_skin_trap( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *victim;
    int dam = 0;

    if( !IS_NPC( ch ) && ch->exp < skill_table[gsn_skin_trap]->skill_level[ch->class] )
    {
	send_to_char( "You can't do that.\n\r", ch );
	return;
    }

    if( ( victim = who_fighting( ch ) ) == NULL )
    {
	send_to_char( "You aren't fighting anyone.\n\r", ch );
	return;
    }

    if( ch->mana < skill_table[gsn_skin_trap]->min_mana )
    {
	send_to_char( "You don't have enough energy.\n\r", ch );
	return;
    }

    if( ch->focus < skill_table[gsn_skin_trap]->focus )
    {
	send_to_char( "You need to focus more.\n\r", ch );
	return;
    }

    ch->focus -= skill_table[gsn_skin_trap]->focus;
    WAIT_STATE( ch, skill_table[gsn_skin_trap]->beats );

    if( can_use_skill( ch, number_percent(), gsn_skin_trap ) )
    {
	dam = get_attmod( ch, victim ) * number_range( 20, 25 );
	if (ch->charge > 0)
		dam = chargeDamMult(ch, dam);

	act( AT_SKILL, "A nodule of flesh detaches from you and soars towards $N, entrapping", ch, NULL, victim, TO_CHAR );
	act( AT_SKILL, "$m in its rubbery depths. &W[$t]", ch, num_punct(dam), victim, TO_CHAR );

	act( AT_SKILL, "Amidst the raging battle, a flicker of movement catches the corner", ch, NULL, victim, TO_VICT );
	act( AT_SKILL, "of your eye.  A large nodule of severed pink flesh soars towards", ch, NULL, victim, TO_VICT );
	act( AT_SKILL, "you, entrapping you within its rubbery depths. &W[$t]", ch, num_punct(dam), victim, TO_VICT );

	act( AT_SKILL, "A nodule of flesh detaches from $n and soars towards $N, entrapping", ch, NULL, victim, TO_NOTVICT );
	act( AT_SKILL, "$m in its rubbery depths. &W[$t]", ch, num_punct(dam), victim, TO_NOTVICT );

	dam = ki_absorb( victim, ch, dam, gsn_skin_trap );
	learn_from_success( ch, gsn_skin_trap );
	global_retcode = damage( ch, victim, dam, TYPE_HIT );
    }
    else
    {
	act( AT_SKILL, "You missed $N with your skin trap.", ch, NULL, victim, TO_CHAR );

	act( AT_SKILL, "A nodule of flesh detaches from you and soars towards $N,", ch, NULL, victim, TO_CHAR );
	act( AT_SKILL, "but $E manages to avoid it, and you re-absorb your skin. &W[$t]", ch, num_punct(0), victim, TO_CHAR );
	
	act( AT_SKILL, "Amidst the raging battle, a flicker of movement catches the", ch, NULL, victim, TO_VICT );
	act( AT_SKILL, "corner of your eye.  A large nodule of severed pink flesh soars", ch, NULL, victim, TO_VICT );
	act( AT_SKILL, "towards you.  You narrowly avoid it, and $N re-absorbs it into", ch, NULL, victim, TO_VICT );
	act( AT_SKILL, "self. &W[$t]", ch, num_punct(0), victim, TO_VICT );

	act( AT_SKILL, "A nodule of flesh detaches from $n and soars towards $N,", ch, NULL, victim, TO_NOTVICT );
	act( AT_SKILL, "but $E manages to avoid it, and you re-absorb your skin. &W[$t]", ch, num_punct(0), victim, TO_CHAR );

	learn_from_failure( ch, gsn_skin_trap );
	global_retcode = damage( ch, victim, 0, TYPE_HIT );
    }

    ch->mana -= skill_table[gsn_skin_trap]->min_mana;
    return;
}
