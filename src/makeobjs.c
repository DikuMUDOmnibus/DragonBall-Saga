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
 *			Specific object creation module			    *
 ****************************************************************************/

#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "mud.h"

extern int	top_affect;
int affectLocation1 = 0;
int affectLocation2 = 0;

#define MAX_ARMOR_TYPE 8
/* Armor types for Armor Generator */
struct armorgenT
{
   int type;	/* Armor type */
   char *name;	/* Descriptive name */
   int weight;	/* Base weight */
   double armorMod;		/* armor */
   int cost;	/* Base value or cost */
   int mlevel;	/* Minimun mob level before this item will drop */
};

const struct armorgenT armor_type[] =
{
	/* Type, Name, Base Weight, Armor Mod, Base Cost, Min Rank */

	{ 0, "Not Defined",	0,  0, 0, 0},
	{ 1, "Padded",		2,  0.70, 500, 0},
	{ 2, "Leather",		3,  0.80, 1000, 2},
	{ 3, "Riot",		8,  0.90, 2500, 3},
	{ 4, "Combat",		9,  1.00, 5000, 4},
	{ 5, "Battle",		7,  1.05, 10000, 5},
	{ 6, "Assault",		7,  1.10, 25000, 6},
	{ 7, "Ancient",		10, 1.15, 50000, 7},
	{ 8, "Dark Wave",	20, 1.25, 100000, 8}

};

const double armor_mods[8]=
{
	0.35,	// Body Armor
	0.25,	// Shields
	0.20,	// Sleeves
	0.20,	// Leggings
	0.20,	// Helmets
	0.15,	// Gloves
	0.15,	// Boots
	0.10 	// Belts
};

const char *armor_wearLoc[8]=
{
	"Armor",
	"Shield",
	"Sleeves",
	"Leggings",
	"Helmet",
	"Gauntlets",
	"Boots",
	"Belt"
};

const char *magic_prefix[8]=
{
	"",
	"Mighty ",
	"Fast ",
	"Wise ",
	"Sturdy ",
	"Fortune ",
	"Glorious ",
	"Brilliant "
};

const char *magic_suffix[8]=
{
	"",
	" of Strength",
	" of Speed",
	" of Mind",
	" of Fortitude",
	" of Chance",
	" of Perfection",
	" of Energy"
};

struct magicA
{
	int location;	/* Apply type */
	double mod;		/* How much to affect user (mod*level) or sn lookup */
	int mlevel;		/* Minimun mob level before this item will drop */
	int bitvector;	/* Affect placed on user */
	int maxmod;		/* The max value of mod*level.  0 = unlimited */
	bool larmor;	/* Will the affect load on Body Armor */
	bool lshield;	/* Will the affect load on Shields */
	bool lsleeve;	/* Will the affect load on Sleeves */
	bool llegging;	/* Will the affect load on Leggings */
	bool lhelmet;	/* Will the affect load on Helmets */
	bool lglove;	/* Will the affect load on Gloves */
	bool lboot;		/* Will the affect load on Boots */
	bool lbelt;		/* Will the affect load on Belts */
	bool lring;		/* Will the affect load on Rings */
	bool lamulet;	/* Will the affect load on Amulets */
	char *prefix;
	char *suffix;
};

const struct magicA item_affect[] =
{
	/* Location, Mod, Min Level, Affect Flag, Max Mod,
	 * Load on: Body Armor, Shields, Sleeves, Leggings, Helmets,
	 *          Gloves, Boots, Belts, Rings, Amulets
	 * Prefix, Suffix */

	{ 0, 0, 0, 0, 0,						0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "", "" },
	{ APPLY_STR, 1, 2, 0, 0,				1, 1, 1, 1, 1, 1, 1, 1, 1, 1, "Mighty ", "of Strength" },
	{ APPLY_DEX, 1, 2, 0, 0,				1, 1, 1, 1, 1, 1, 1, 1, 1, 1, "Fast ", "of Speed" },
	{ APPLY_INT, 1, 2, 0, 0,				1, 1, 1, 1, 1, 1, 1, 1, 1, 1, "Wise ", "of Mind" },
	{ APPLY_CON, 1, 2, 0, 0,				1, 1, 1, 1, 1, 1, 1, 1, 1, 1, "Sturdy ", "of Fortitude" },
	{ APPLY_LCK, 0.25, 4, 0, 0,				1, 1, 1, 1, 1, 1, 1, 1, 1, 1, "Fortune ", "of Chance" },
	{ APPLY_ALLSTATS, 0.25, 4, 0, 0,			1, 1, 1, 1, 1, 1, 1, 1, 1, 1, "Glorious ", "of Perfection" },
	{ APPLY_MANA, 10, 2, 0, 0,				1, 1, 1, 1, 1, 1, 1, 1, 1, 1, "Brilliant ", "of Energy"}
};

int get_total_armor( long double level )
{
	long double chklvl = 0;
	int zeros = 0;
	int i;
	int armorBase = 0;
	int prevArmor = 0;
	int armor = 0;
	double ratio = 0.00;

	for(i=0; i < 100; i++)	// i'm "cheeping" out and caping this at 1 googol -Goku 10.05.04
	{
		if( level / pow( 10, i ) < 10 )
		{
			break;
		}
	}

	chklvl = pow(10,i);
	zeros = i;

	switch (zeros)
	{
		case 0:	// 10
		case 1:	// 10
			armorBase = 10;
			prevArmor = 1;
			break;
		case 2:	// 100
			armorBase = 100;
			prevArmor = 10;
			break;
		case 3:	// 1000
			armorBase = 500;
			prevArmor = 100;
			break;
		case 4:	// 10k
			armorBase = 1000;
			prevArmor = 500;
			break;
		case 5:	// 100k
			armorBase = 2000;
			prevArmor = 1000;
			break;
		default:
			armorBase = 2000 + ((zeros - 5) * 1000);
			prevArmor = 1000 + ((zeros - 5) * 1000);
			break;
	}

	ratio = level/chklvl;

	armor = prevArmor + ((armorBase - prevArmor) * ratio);

	armor =  armor * (double)number_range(500, 1200)/1000;

	return armor;

}

int get_rank_by_pl( long double level )
{

	if (level < 5000)
		return 1;
	else if (level < 100000)
		return 2;
	else if (level < 1000000)
		return 3;
	else if (level < 10000000)
		return 4;
	else if (level < 100000000)
		return 5;
	else if (level < 1000000000)
		return 6;
	else if (level < 10000000000ULL)
		return 7;
	else if (level < 50000000000ULL)
		return 8;
	else if (level < 100000000000ULL)
		return 9;
	else if (level < 300000000000ULL)
		return 10;
	else if (level < 600000000000ULL)
		return 11;
	else if (level < 1000000000000ULL)
		return 12;
	else if (level < 10000000000000ULL)
		return 13;
	else if (level < 50000000000000ULL)
                return 14;
	else if (level < 100000000000000ULL)
                return 15;
	else
		return 16;

	return 0;
}

int calc_zeni( long double level, CHAR_DATA *killer )
{
	long double chklvl = 0;
	int zeros = 0;
	int i;
	int zeniBase = 0;
	int prevZeni = 0;
	int zeni = 0;
	double ratio = 0.00;
	long double chklvl2 = 0;

	for(i=0; i < 100; i++)	// i'm "cheeping" out and caping this at 1 googol -Goku 10.05.04
	{
		if( level / pow( 10, i ) < 10 )
		{
			break;
		}
	}

	chklvl = pow(10,i);
	chklvl2 = pow(10,i-1);

	/* double zero count to smooth out increase -Goku 10.05.04 */
	if (level < chklvl - chklvl2 + chklvl2)
		zeros = i*2-1;
	else
		zeros = i*2;

	switch (zeros)
	{
		case 0:	// 0
		case 1:	// 1
			zeniBase = 1;
			prevZeni = 1;
			break;
		case 2:	// 5
			zeniBase = 5;
			prevZeni = 1;
			break;
		case 3:	// 10
			zeniBase = 10;
			prevZeni = 5;
			break;
		case 4:	// 50
			zeniBase = 20;
			prevZeni = 10;
			break;
		case 5:	// 100
		case 6:	// 500
			zeniBase = 30;
			prevZeni = 20;
			break;
		case 7:	// 1000
		case 8:	// 5000
			zeniBase = 40;
			prevZeni = 30;
			break;
		case 9:	// 10k
			zeniBase = 60;
			prevZeni = 40;
			break;
		case 10:	// 50k
			zeniBase = 90;
			prevZeni = 60;
			break;
		case 11:	// 100k
			zeniBase = 150;
			prevZeni = 90;
			break;
		case 12:	// 500k
			zeniBase = 170;
			prevZeni = 150;
			break;
		case 13:	// 1m
			zeniBase = 200;
			prevZeni = 170;
			break;
		case 14:	// 5m
			zeniBase = 300;
			prevZeni = 200;
			break;
		case 15:	// 10m
			zeniBase = 400;
			prevZeni = 300;
			break;
		default:
			zeniBase = 500 + ((zeros - 15) * 100);
			prevZeni = 400 + ((zeros - 15) * 100);
			break;
	}

	ratio = level/chklvl;

	zeni = prevZeni + ((zeniBase - prevZeni) * ratio);
	zeni = (dice(get_rank_by_pl(level), zeni)
			+ (dice(get_rank_by_pl(level), zeni) / 10
			+ dice(get_curr_lck(killer), zeni / 3) ) );

	return zeni/4;

}

void magicAffectGen( OBJ_DATA *obj, int affects, int armorType )
{
	int i;
	int affect = 0;
	int type;
	bool armor = FALSE;
	AFFECT_DATA *paf;
	int justincase = 0;

	if (obj->item_type == ITEM_ARMOR)
	{
		type = obj->value[3];
		armor = TRUE;
	}
	else
		return;

	xSET_BIT(obj->extra_flags, ITEM_MAGIC);

	for (i = 0; i < affects; i++)
	{

		justincase++;
		if (justincase > 1000)
			break;

		affect = number_range( 1, 7 );

			if (item_affect[affect].mlevel > get_rank_by_pl(obj->level))
			{
				i = UMIN(0, i-1);  /* Going back in time! */
				continue;
			}

			switch (armorType)
			{
				case 0:
					if (!item_affect[affect].larmor)
					{
						i = UMIN(0, i-1);  /* Going back in time! */
						continue;
					}
				case 1:
					if (!item_affect[affect].lshield)
					{
						i = UMIN(0, i-1);  /* Going back in time! */
						continue;
					}
				case 2:
					if (!item_affect[affect].lsleeve)
					{
						i = UMIN(0, i-1);  /* Going back in time! */
						continue;
					}
				case 3:
					if (!item_affect[affect].llegging)
					{
						i = UMIN(0, i-1);  /* Going back in time! */
						continue;
					}
				case 4:
					if (!item_affect[affect].lhelmet)
					{
						i = UMIN(0, i-1);  /* Going back in time! */
						continue;
					}
				case 5:
					if (!item_affect[affect].lglove)
					{
						i = UMIN(0, i-1);  /* Going back in time! */
						continue;
					}
				case 6:
					if (!item_affect[affect].lboot)
					{
						i = UMIN(0, i-1);  /* Going back in time! */
						continue;
					}
				case 7:
					if (!item_affect[affect].lbelt)
					{
						i = UMIN(0, i-1);  /* Going back in time! */
						continue;
					}
				default:
					break;
			}

		if (!affectLocation1)
			affectLocation1 = affect;
		else if (!affectLocation2)
			affectLocation2 = affect;

		CREATE( paf, AFFECT_DATA, 1 );
		paf->type			= -1;
		paf->duration		= -1;
		paf->location		= item_affect[affect].location;
		if (item_affect[affect].maxmod > 0)
			paf->modifier = number_range(1,(int) UMAX(item_affect[affect].mod * (get_rank_by_pl(obj->level) - item_affect[affect].mlevel), item_affect[affect].maxmod));
		else
			paf->modifier = number_range(1,(int) (item_affect[affect].mod * (get_rank_by_pl(obj->level) - item_affect[affect].mlevel)));

		xCLEAR_BITS( paf->bitvector );

		if (item_affect[affect].bitvector)
			xSET_BIT( paf->bitvector, item_affect[affect].bitvector );

		LINK( paf, obj->first_affect, obj->last_affect, next, prev );
		++top_affect;
	}
}

void finalize_cost( OBJ_DATA *obj )
{
	AFFECT_DATA *iaf;
	int mod = 0;

	for( iaf = obj->first_affect; iaf; iaf = iaf->next )
	{
		if (!iaf)
			break;
		if (iaf->location == APPLY_STR || iaf->location == APPLY_DEX
			|| iaf->location == APPLY_INT || iaf->location == APPLY_CON)
			mod += iaf->modifier * 2500;
		else if (iaf->location == APPLY_LCK)
			mod += iaf->modifier * 5000;
		else if (iaf->location == APPLY_ALLSTATS)
			mod += iaf->modifier * 10000;
		else if (iaf->location == APPLY_MANA)
			mod += iaf->modifier * 500;
	}

	mod += obj->value[5] * 50;
	obj->cost += mod;

	return;
}

OBJ_DATA *generate_item( long double pl, long double killerpl )
{
	OBJ_DATA *newitem = NULL;
	int wearLoc;
	int armor;
	int armorType;
	int x;
	char buf[MAX_STRING_LENGTH];
	long double max = 0;
	long double min = 0;

	if( killerpl > pl )
	{
		max = killerpl;
		min = pl;
	}
	else
	{
		max = pl;
		min = killerpl;
	}

	affectLocation1 = 0;
	affectLocation2 = 0;

	if( !( newitem = create_object( get_obj_index( OBJ_VNUM_TREASURE ), pl ) ) )
	{
		bug( "create_object: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
		return NULL;
	}

	/* Hey look, if it takes more than 50000 iterations to find something.... */
	for( x = 0; x < 50000; x++ )
	{
		armorType = number_range(1,MAX_ARMOR_TYPE);
		if (armor_type[armorType].mlevel <= get_rank_by_pl(pl))
			break;
	}

   newitem->weight   = armor_type[armorType].weight;
   armor = get_total_armor(pl);
   newitem->cost     = armor_type[armorType].cost;
//   newitem->level    = pl;
//   newitem->level    = number_range( min, max );
	newitem->level = min + ( ( min + max ) / 3 );
	if( number_range( 1, 5 ) == 5 )
		newitem->level -= ( min + max ) / 5;
	if( number_range( 1, 75 ) == 23 )
		newitem->level -= ( max - min ) / 3;
	if( number_range( 1, 300 ) == 59 )
		newitem->level = min;
	newitem->level = UMIN( newitem->level, killerpl * 2 );

	wearLoc = number_range(0,7);
	newitem->value[4] = newitem->value[5] = armor * armor_type[armorType].armorMod * armor_mods[wearLoc];

	switch (wearLoc)
	{
		default:
			bug("Invalid armor wear location generated");
		case 0:
			SET_BIT( newitem->wear_flags, ITEM_WEAR_BODY );
		break;
		case 1:
			SET_BIT( newitem->wear_flags, ITEM_WEAR_SHIELD );
		break;
		case 2:
			SET_BIT( newitem->wear_flags, ITEM_WEAR_ARMS );
			break;
		case 3:
			SET_BIT( newitem->wear_flags, ITEM_WEAR_LEGS );
			break;
		case 4:
			SET_BIT( newitem->wear_flags, ITEM_WEAR_HEAD );
			break;
		case 5:
			SET_BIT( newitem->wear_flags, ITEM_WEAR_HANDS );
			break;
		case 6:
			SET_BIT( newitem->wear_flags, ITEM_WEAR_FEET );
			break;
		case 7:
			SET_BIT( newitem->wear_flags, ITEM_WEAR_WAIST );
			break;
	}

	if (number_range(1,4) == 4)	// 75% chance to get a magic item
		magicAffectGen(newitem, number_range(1,2), wearLoc);

	finalize_cost(newitem);

	sprintf(buf, "%s%s %s%s",
		magic_prefix[affectLocation1],
		armor_type[armorType].name,
		armor_wearLoc[wearLoc],
		magic_suffix[affectLocation2]);
	STRFREE( newitem->name );
	newitem->name = STRALLOC( buf );

	sprintf(buf, "%s%s %s%s", magic_prefix[affectLocation1],armor_type[armorType].name,
		armor_wearLoc[wearLoc], magic_suffix[affectLocation2]);
	STRFREE( newitem->short_descr );
	newitem->short_descr = STRALLOC( buf );

	sprintf(buf, "A %s%s %s%s lies here in a heap.", magic_prefix[affectLocation1],armor_type[armorType].name,
		armor_wearLoc[wearLoc], magic_suffix[affectLocation2]);
	STRFREE( newitem->description );
	newitem->description = STRALLOC( buf );

	affectLocation1 = 0;
	affectLocation2 = 0;
	return (newitem);
}

void generate_treasure( CHAR_DATA *killer, CHAR_DATA *ch, OBJ_DATA *corpse )
{
	int tchance;
	int zeni;
	char buf[MAX_STRING_LENGTH];
	char originText[MAX_STRING_LENGTH];

	/* Rolling for the initial check to see if we should be generating anything at all */
	tchance = number_range( 1, 100 );

	if (tchance <= 30)
	{
		return;
	}

	else if (tchance <= 80)
	{
		zeni = calc_zeni(ch->exp, killer);
		if (zeni < 1)
		{
			bug("RTG Made less than 1 zeni: PC(%s) Mob(%d) Room(%d) Zeni(%d)", killer->name, ch->pIndexData->vnum, ch->in_room->vnum, zeni);
			return;
		}
	    if ( ch->in_room )
	    {
	      ch->in_room->area->gold_looted += zeni;
	      sysdata.global_looted += zeni/100;
	    }
		if (economy_has(ch->in_room->area, zeni))
		{
			lower_economy(ch->in_room->area, zeni);
	    	obj_to_obj( create_money( zeni ), corpse );
	    }
		return;
	}

	else
	{
		if( killer->pl / ch->pl > 7 )
			return;

		// make items yo
		OBJ_DATA *item = generate_item( ch->exp, killer->exp );
		if( !item )
		{
			bug( "%s", "generate_treasure: Item object failed to create!" );
			return;
		}
		sprintf(buf, "killer:%sMob%d",killer->name, ch->pIndexData->vnum);
		sprintf(originText, "%d, %s", ORIGIN_RTG, buf);
		STRFREE( item->origin );
		item->origin = STRALLOC( originText );
		obj_to_obj( item, corpse );
		return;
	}


}

/*
 * Make a fire.
 */
void make_fire(ROOM_INDEX_DATA *in_room, sh_int timer)
{
    OBJ_DATA *fire;

    fire = create_object( get_obj_index( OBJ_VNUM_FIRE ), 0 );
    fire->timer = number_fuzzy(timer);
    obj_to_room( fire, in_room );
    return;
}

/*
 * Make a trap.
 */
OBJ_DATA *make_trap(int v0, int v1, int v2, int v3)
{
    OBJ_DATA *trap;

    trap = create_object( get_obj_index( OBJ_VNUM_TRAP ), 0 );
    trap->timer = 0;
    trap->value[0] = v0;
    trap->value[1] = v1;
    trap->value[2] = v2;
    trap->value[3] = v3;
    return trap;
}


/*
 * Turn an object into scraps.		-Thoric
 */
void make_scraps( OBJ_DATA *obj )
{
  char buf[MAX_STRING_LENGTH];
  OBJ_DATA  *scraps, *tmpobj;
  CHAR_DATA *ch = NULL;

  separate_obj( obj );
  scraps	= create_object( get_obj_index( OBJ_VNUM_SCRAPS ), 0 );
  scraps->timer = number_range( 5, 15 );

  /* don't make scraps of scraps of scraps of ... */
  if ( obj->pIndexData->vnum == OBJ_VNUM_SCRAPS )
  {
     STRFREE( scraps->short_descr );
     scraps->short_descr = STRALLOC( "some debris" );
     STRFREE( scraps->description );
     scraps->description = STRALLOC( "Bits of debris lie on the ground here." );
  }
  else
  {
     sprintf( buf, scraps->short_descr, obj->short_descr );
     STRFREE( scraps->short_descr );
     scraps->short_descr = STRALLOC( buf );
     sprintf( buf, scraps->description, obj->short_descr );
     STRFREE( scraps->description );
     scraps->description = STRALLOC( buf );
  }

  if ( obj->carried_by )
  {
    act( AT_OBJECT, "$p falls to the ground in scraps!",
		  obj->carried_by, obj, NULL, TO_CHAR );
    if ( obj == get_eq_char( obj->carried_by, WEAR_WIELD )
    &&  (tmpobj = get_eq_char( obj->carried_by, WEAR_DUAL_WIELD)) != NULL )
       tmpobj->wear_loc = WEAR_WIELD;

    obj_to_room( scraps, obj->carried_by->in_room);
  }
  else
  if ( obj->in_room )
  {
    if ( (ch = obj->in_room->first_person ) != NULL )
    {
      act( AT_OBJECT, "$p is reduced to little more than scraps.",
	   ch, obj, NULL, TO_ROOM );
      act( AT_OBJECT, "$p is reduced to little more than scraps.",
	   ch, obj, NULL, TO_CHAR );
    }
    obj_to_room( scraps, obj->in_room);
  }
  if ( (obj->item_type == ITEM_CONTAINER || obj->item_type == ITEM_KEYRING
  ||    obj->item_type == ITEM_QUIVER    || obj->item_type == ITEM_CORPSE_PC)
  &&    obj->first_content )
  {
    if ( ch && ch->in_room )
    {
	act( AT_OBJECT, "The contents of $p fall to the ground.",
	   ch, obj, NULL, TO_ROOM );
	act( AT_OBJECT, "The contents of $p fall to the ground.",
	   ch, obj, NULL, TO_CHAR );
    }
    if ( obj->carried_by )
	empty_obj( obj, NULL, obj->carried_by->in_room );
    else
    if ( obj->in_room )
	empty_obj( obj, NULL, obj->in_room );
    else
    if ( obj->in_obj )
	empty_obj( obj, obj->in_obj, NULL );
  }
  extract_obj( obj );
}

OBJ_DATA *make_deathCertificate( CHAR_DATA *ch, CHAR_DATA *killer )
{
    char buf[MAX_STRING_LENGTH];
	OBJ_DATA *dc;
    EXTRA_DESCR_DATA *ed;

	dc = create_object(get_obj_index(610),1);

	sprintf( buf, "%s death certificate", ch->name );
    STRFREE( dc->name );
    dc->name = STRALLOC( buf );
	sprintf( buf, "%s's death certificate", ch->name );
    STRFREE( dc->short_descr );
    dc->short_descr = STRALLOC( buf );

	ed = SetOExtra(dc, dc->name);
	sprintf( buf,
	"Certificate of Death\n\r"
	"--------------------\n\r"
	"Name: &W%s&w\n\r"
	"Rank: &W%s&w\n\r"
	"Sex : &W%s&w  Race: &W%s&w\n\r"
	"Cause of Death: &WHomicide&w\n\r"
	"Time of Death : &W%s&w\r"
	"Area of the Crime : &W%s&w\n\r"
	"Scene of the Crime: &W%s&w\n\r"
	"Suspect Information\n\r"
	"-------------------\n\r"
	"Name: &W%s&w\n\r"
	"Rank: &W%s&w\n\r"
	"Sex : &W%s&w  Race: &W%s&w\n\r"
	"Reason for Involvement:\n\r"
	"  &W'Possible' Self Defense&w\n\r",
	ch->name,
	get_rank(ch),
	ch->sex == SEX_MALE ? "Male" : ch->sex == SEX_FEMALE ? "Female" : "Neutral", capitalize(get_race(ch)),
	ctime(&current_time),
	killer->in_room->area->name,
	killer->in_room->name,
	killer->name,
	get_rank(killer),
	killer->sex == SEX_MALE ? "Male" : killer->sex == SEX_FEMALE ? "Female" : "Neutral", capitalize(get_race(killer))
	);
    STRFREE( ed->description );
    ed->description = STRALLOC( buf );

	return dc;
}

/*
 * Goku's global drop code.  Checks if a mob will drop any of
 * the items defined when it's killed.  -Goku 07.28.04
 */
#define GLOBAL_DROP_OBJ_TOKEN 613

void global_drop_check( OBJ_DATA *corpse, CHAR_DATA *ch, CHAR_DATA *killer )
{
    int luckMod;

    luckMod = get_curr_lck(killer);

    /* if luck is lower than 0, no drops */
    if (luckMod < 1)
        return;

    if (number_range(luckMod, 10000) <= 1)
    {
        obj_to_obj( create_object(get_obj_index(GLOBAL_DROP_OBJ_TOKEN), 0), corpse);
    }

    return;
}

#undef GLOBAL_DROP_OBJ_TOKEN

/*
 * Make a corpse out of a character.
 */
void make_corpse( CHAR_DATA *ch, CHAR_DATA *killer )
{
    char buf[MAX_STRING_LENGTH];
    OBJ_DATA *corpse;
    OBJ_DATA *obj;
    OBJ_DATA *obj_next;
    char *name;

	if ( !ch ) {
		if (!killer)
			bug( "make_corpse: NULL TARGET and NULL KILLER", 0 );
		else
			bug( "make_corpse: NULL TARGET, %s is the KILLER", killer->name, 0 );
		return;
	}
	if ( !killer ) {
		if (!ch)
			bug( "make_corpse: NULL KILLER and NULL TARGET", 0 );
		else
			bug( "make_corpse: NULL KILLER, %s as TARGET", ch->name, 0 );
		return;
	}

    if ( IS_NPC(ch) )
    {
	name		= ch->short_descr;
	corpse		= create_object(get_obj_index(OBJ_VNUM_CORPSE_NPC), 0);
	corpse->timer	= 6;
	if ( ch->gold > 0 )
	{
	    if ( ch->in_room )
	    {
	      ch->in_room->area->gold_looted += ch->gold;
	      sysdata.global_looted += ch->gold/100;
	    }
	    obj_to_obj( create_money( ch->gold ), corpse );
	    ch->gold = 0;
	}
	else if (ch->gold < 0 && !IS_NPC( killer ))
		generate_treasure( killer, ch, corpse);

/* Cannot use these!  They are used.
	corpse->value[0] = (int)ch->pIndexData->vnum;
	corpse->value[1] = (int)ch->max_hit;
*/
/*	Using corpse cost to cheat, since corpses not sellable */
	corpse->cost     = (-(int)ch->pIndexData->vnum);
        corpse->value[2] = corpse->timer;
    }
    else
    {
	name		= ch->name;
	corpse		= create_object(get_obj_index(OBJ_VNUM_CORPSE_PC), 0);
        if ( in_arena( ch ) )
	  corpse->timer	= 30;
	else
	  corpse->timer = 15;
        corpse->value[2] = (int)(corpse->timer/8);
	corpse->value[4] = ch->level;
	if ( CAN_PKILL( ch ) && sysdata.pk_loot )
	  xSET_BIT( corpse->extra_flags, ITEM_CLANCORPSE );
	/* Pkill corpses get save timers, in ticks (approx 70 seconds)
	   This should be anough for the killer to type 'get all corpse'. */
	if ( !IS_NPC(ch) && !IS_NPC(killer) )
	  corpse->value[3] = 1;
	else
	  corpse->value[3] = 0;
    }

    if ( CAN_PKILL( ch ) && CAN_PKILL( killer ) && ch != killer )
    {
	sprintf( buf, "%s", killer->name );
	STRFREE( corpse->action_desc );
	corpse->action_desc = STRALLOC( buf );
    }

    /* Added corpse name - make locate easier , other skills */
    sprintf( buf, "corpse %s", name );
    STRFREE( corpse->name );
    corpse->name = STRALLOC( buf );

    sprintf( buf, corpse->short_descr, name );
    STRFREE( corpse->short_descr );
    corpse->short_descr = STRALLOC( buf );

    sprintf( buf, corpse->description, name );
    STRFREE( corpse->description );
    corpse->description = STRALLOC( buf );

    for ( obj = ch->first_carrying; obj; obj = obj_next )
    {
	obj_next = obj->next_content;
	if (!IS_NPC(ch) && obj->wear_loc != -1)
		continue;
	obj_from_char( obj );

	if( obj->item_type == ITEM_DRAGONBALL )
	{
	  obj_to_room(obj,ch->in_room);
	  continue;
	}

	if ( (!IS_NPC(ch) && IS_OBJ_STAT( obj, ITEM_INVENTORY ))
	  || IS_OBJ_STAT( obj, ITEM_DEATHROT ) )
	    extract_obj( obj );
	else
	    obj_to_obj( obj, corpse );
    }

	/* global drop? -Goku 07.28.04 */
	if (!IS_NPC(killer) && IS_NPC(ch)
		&& !xIS_SET(ch->affected_by, AFF_NO_GLOBAL_DROP)
		&& killer->pl/ch->exp <= 5 )
		global_drop_check(corpse, ch, killer);

/*  disabled untill a more efficant way to make these things can be implimented -Goku
	if (!IS_NPC(ch) && !IS_NPC(killer))
	{
		act( AT_SOCIAL, "The Coroner materializes before the corpse of $N.", killer, NULL, ch, TO_CHAR );
		act( AT_SAY, "The Coroner says 'Yep, $E's dead.'", killer, NULL, ch, TO_CHAR );
		act( AT_SOCIAL, "The Coroner stamps a sheet of paper and places it on the corpse.", killer, NULL, ch, TO_CHAR );
		act( AT_SAY, "The Coroner says 'I'll just leave this for the police to take care of.'", killer, NULL, ch, TO_CHAR );

		act( AT_SOCIAL, "The Coroner materializes before the corpse of $N.", killer, NULL, ch, TO_NOTVICT );
		act( AT_SAY, "The Coroner says 'Yep, $E's dead.'", killer, NULL, ch, TO_NOTVICT );
		act( AT_SOCIAL, "The Coroner stamps a sheet of paper and places it on the corpse.", killer, NULL, ch, TO_NOTVICT );
		act( AT_SAY, "The Coroner says 'I'll just leave this for the police to take care of.'", killer, NULL, ch, TO_NOTVICT );

		obj_to_obj( make_deathCertificate(ch, killer), corpse );
	}
*/
    if (spaceDeath)
    	obj_to_room( corpse, get_room_index(ROOM_CORPSE_DROPOFF) );
    else
    	obj_to_room( corpse, ch->in_room );

   	spaceDeath = FALSE;
    return;
}



void make_blood( CHAR_DATA *ch )
{
	OBJ_DATA *obj;

	obj		= create_object( get_obj_index( OBJ_VNUM_BLOOD ), 0 );
	obj->timer	= number_range( 2, 4 );
	obj->value[1]   = number_range( 3, UMIN(5, ch->level) );
	obj_to_room( obj, ch->in_room );
}


void make_bloodstain( CHAR_DATA *ch )
{
	OBJ_DATA *obj;

	obj		= create_object( get_obj_index( OBJ_VNUM_BLOODSTAIN ), 0 );
	obj->timer	= number_range( 1, 2 );
	obj_to_room( obj, ch->in_room );
}


/*
 * make some coinage
 */
OBJ_DATA *create_money( int amount )
{
    char buf[MAX_STRING_LENGTH];
    OBJ_DATA *obj;

    if ( amount <= 0 )
    {
	bug( "Create_money: zero or negative money %d.", amount );
	amount = 1;
    }

    if ( amount == 1 )
    {
	obj = create_object( get_obj_index( OBJ_VNUM_MONEY_ONE ), 0 );
    }
    else
    {
	obj = create_object( get_obj_index( OBJ_VNUM_MONEY_SOME ), 0 );
	sprintf( buf, obj->short_descr, amount );
	STRFREE( obj->short_descr );
	obj->short_descr = STRALLOC( buf );
	obj->value[0]	 = amount;
    }

    return obj;
}

