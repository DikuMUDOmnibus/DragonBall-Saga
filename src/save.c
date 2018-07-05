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
 *		     Character saving and loading module		    *
 ****************************************************************************/

#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#ifndef WIN32
  #include <dirent.h>
#endif
#include "mud.h"


/*
 * Increment with every major format change.
 */
#define SAVEVERSION	4

/*
 * Array to keep track of equipment temporarily.		-Thoric
 */
OBJ_DATA *save_equipment[MAX_WEAR][8];
CHAR_DATA *quitting_char, *loading_char, *saving_char;

int file_ver;

/*
 * Externals
 */
void fwrite_comments( CHAR_DATA *ch, FILE *fp );
void fread_comment( CHAR_DATA *ch, FILE *fp );
bool     check_parse_name        args( ( char *name, bool newchar ) );

extern char * GfpName;
extern bool StopFP;

void transStatApply(CHAR_DATA *ch, int strMod, int spdMod, int intMod, int conMod);

/*
 * Array of containers read for proper re-nesting of objects.
 */
static	OBJ_DATA *	rgObjNest	[MAX_NEST];

/*
 * Local functions.
 */
void	fwrite_char	args( ( CHAR_DATA *ch, FILE *fp ) );
void	fread_char	args( ( CHAR_DATA *ch, FILE *fp, bool preload) );
void	write_corpses	args( ( CHAR_DATA *ch, char *name, OBJ_DATA *objrem ) );

#ifdef WIN32                    /* NJG */
UINT timer_code = 0;            /* needed to kill the timer */
/* Note: need to include: WINMM.LIB to link to timer functions */
void caught_alarm();
void CALLBACK alarm_handler(
	UINT IDEvent,		/* identifies timer event */
	UINT uReserved,		/* not used */
	DWORD dwUser,		/* application-defined instance data */
	DWORD dwReserved1,	/* not used */
	DWORD dwReserved2)	/* not used */
{
    caught_alarm();
}

void kill_timer()
{
    if (timer_code)
	timeKillEvent(timer_code);
    timer_code = 0;
}

#endif

void fixTransStatAffects( CHAR_DATA *ch)
{
	AFFECT_DATA *tAff;

	if ( !ch->first_affect )
		return;

	for (tAff = ch->first_affect;tAff;tAff = tAff->next)
	{
		if (tAff->affLocator == LOC_TRANS_STAT_APPLY)
		{
			switch (tAff->location)
			{
				case APPLY_STR:
					ch->mod_str += tAff->modifier;
				break;
				case APPLY_DEX:
					ch->mod_dex += tAff->modifier;
				break;
				case APPLY_INT:
					ch->mod_int += tAff->modifier;
				break;
				case APPLY_CON:
					ch->mod_con += tAff->modifier;
				break;
			}
		}
	}

	return;
}

/*
 * Un-equip character before saving to ensure proper	-Thoric
 * stats are saved in case of changes to or removal of EQ
 */
void de_equip_char( CHAR_DATA *ch )
{
    char buf[MAX_STRING_LENGTH];
    OBJ_DATA *obj;
    int x,y;

    for ( x = 0; x < MAX_WEAR; x++ )
	for ( y = 0; y < MAX_LAYERS; y++ )
	    save_equipment[x][y] = NULL;
    for ( obj = ch->first_carrying; obj; obj = obj->next_content )
	if ( obj->wear_loc > -1 && obj->wear_loc < MAX_WEAR )
	{
	    if ( ch->exp >= obj->level )
	    {
		for ( x = 0; x < MAX_LAYERS; x++ )
		   if ( !save_equipment[obj->wear_loc][x] )
		   {
			save_equipment[obj->wear_loc][x] = obj;
			break;
		   }
		if ( x == MAX_LAYERS )
		{
		    sprintf( buf, "%s had on more than %d layers of clothing in one location (%d): %s",
			ch->name, MAX_LAYERS, obj->wear_loc, obj->name );
		    bug( buf, 0 );
		}
	    }
	    else
	    {
	       sprintf( buf, "%s had on %s:  ch->exp = %.0Lf  obj->level = %.0Lf",
		ch->name, obj->name,
	       	ch->exp, obj->level );
	       bug( buf, 0 );
	    }
	    unequip_char(ch, obj);
	}
}

/*
 * Re-equip character					-Thoric
 */
void re_equip_char( CHAR_DATA *ch )
{
    int x,y;

    for ( x = 0; x < MAX_WEAR; x++ )
	for ( y = 0; y < MAX_LAYERS; y++ )
	   if ( save_equipment[x][y] != NULL )
	   {
		if ( quitting_char != ch )
		   equip_char(ch, save_equipment[x][y], x);
		save_equipment[x][y] = NULL;
	   }
	   else
		break;
}


/*
 * Save a character and inventory.
 * Would be cool to save NPC's too for quest purposes,
 *   some of the infrastructure is provided.
 */
void save_char_obj( CHAR_DATA *ch )
{
    char strsave[MAX_INPUT_LENGTH];
    char strback[MAX_INPUT_LENGTH];
    FILE *fp;

    if ( !ch )
    {
	bug( "Save_char_obj: null ch!", 0 );
	return;
    }

    if ( IS_NPC(ch) || NOT_AUTHED(ch) )
	return;

    saving_char = ch;

    if ( ch->desc && ch->desc->original )
	ch = ch->desc->original;

    de_equip_char( ch );

    ch->save_time = current_time;
    sprintf( strsave, "%s%c/%s",PLAYER_DIR,tolower(ch->pcdata->filename[0]),
				 capitalize( ch->pcdata->filename ) );

    /*
     * Auto-backup pfile (can cause lag with high disk access situtations).
     */
    /* Backup of each pfile on save as above can cause lag in high disk
       access situations on big muds like Realms.  Quitbackup saves most
       of that and keeps an adequate backup -- Blodkai, 10/97 */

    if ( IS_SET( sysdata.save_flags, SV_BACKUP ) ||
       ( IS_SET( sysdata.save_flags, SV_QUITBACKUP ) && quitting_char == ch ))
    {
        sprintf( strback,"%s%c/%s",BACKUP_DIR,tolower(ch->pcdata->filename[0]),
                                 capitalize( ch->pcdata->filename ) );
        rename( strsave, strback );
    }

    /*
     * Save immortal stats, level & vnums for wizlist		-Thoric
     * and do_vnums command
     *
     * Also save the player flags so we the wizlist builder can see
     * who is a guest and who is retired.
     */
    if ( ch->level >= LEVEL_IMMORTAL )
    {
      sprintf( strback, "%s%s", GOD_DIR, capitalize( ch->pcdata->filename ) );

      if ( ( fp = fopen( strback, "w" ) ) == NULL )
      {
	  perror( strsave );
	  bug( "Save_god_level: fopen", 0 );
      }
      else
      {
	fprintf( fp, "Level        %d\n", ch->level );
	fprintf( fp, "Pcflags      %d\n", ch->pcdata->flags );
	if ( ch->pcdata->r_range_lo && ch->pcdata->r_range_hi )
	  fprintf( fp, "RoomRange    %d %d\n", ch->pcdata->r_range_lo,
	  				       ch->pcdata->r_range_hi	);
	if ( ch->pcdata->o_range_lo && ch->pcdata->o_range_hi )
	  fprintf( fp, "ObjRange     %d %d\n", ch->pcdata->o_range_lo,
	  				       ch->pcdata->o_range_hi	);
	if ( ch->pcdata->m_range_lo && ch->pcdata->m_range_hi )
	  fprintf( fp, "MobRange     %d %d\n", ch->pcdata->m_range_lo,
	  				       ch->pcdata->m_range_hi	);
      if ( ch->pcdata->email && ch->pcdata->email[0] != '\0' )
	  fprintf( fp, "Email        %s~\n", ch->pcdata->email );
      if ( ch->pcdata->icq > 0 )
	  fprintf( fp, "ICQ          %d\n", ch->pcdata->icq );
	fprintf( fp, "End\n" );
	fclose( fp );
      }
    }

    if ( ( fp = fopen( TEMP_FILE, "w" ) ) == NULL )
    {
	perror( strsave );
	bug( "Save_char_obj: fopen", 0 );
    }
    else
    {
	bool ferr;

	fwrite_char( ch, fp );
        if ( ch->morph )
        	fwrite_morph_data ( ch, fp );
	if ( ch->first_carrying )
	  fwrite_obj( ch, ch->last_carrying, fp, 0, OS_CARRY );

	if ( sysdata.save_pets && ch->pcdata->pet )
		fwrite_mobile( fp, ch->pcdata->pet );
	if ( ch->comments )                 /* comments */
	  fwrite_comments( ch, fp );        /* comments */
	fprintf( fp, "#END\n" );
	ferr = ferror(fp);
	fclose( fp );
	if (ferr)
	{
	  perror(strsave);
	  bug("Error writing temp file for %s -- not copying", strsave);
	}
	else
	  rename(TEMP_FILE, strsave);
    }

    re_equip_char( ch );

    quitting_char = NULL;
    saving_char   = NULL;
    return;
}



/*
 * Write the char.
 */
void fwrite_char( CHAR_DATA *ch, FILE *fp )
{
    AFFECT_DATA *paf;
    int sn, track,i;
    sh_int pos;
    SKILLTYPE *skill = NULL;

    fprintf( fp, "#PLAYER\n"		);

    fprintf( fp, "Version        %d\n",   SAVEVERSION		);
    fprintf( fp, "Name           %s~\n",	ch->name		);
    if (ch->pcdata->last_name && strlen(ch->pcdata->last_name) >= 3)
    fprintf( fp, "Last_Name     %s~\n",   ch->pcdata->last_name              );
	if (ch->pcdata->creation_date == 0)
		fprintf( fp, "Creation_date  %d\n", (int)current_time );
	else
		fprintf( fp, "Creation_date  %d\n", (int)ch->pcdata->creation_date );
		fprintf( fp, "C_date_word    %s~\n", ctime(&ch->pcdata->creation_date) );
    if ( ch->description[0] != '\0' )
      fprintf( fp, "Description    %s~\n",	ch->description	);
    if ( ch->pcdata->description1[0] != '\0' )
      fprintf( fp, "Description1    %s~\n",	ch->pcdata->description1	);
    if ( ch->pcdata->description2[0] != '\0' )
      fprintf( fp, "Description2    %s~\n",	ch->pcdata->description2	);
    if ( ch->pcdata->description3[0] != '\0' )
      fprintf( fp, "Description3    %s~\n",	ch->pcdata->description3	);
    if ( ch->pcdata->description4[0] != '\0' )
      fprintf( fp, "Description4    %s~\n",	ch->pcdata->description4	);
    if ( ch->pcdata->description5[0] != '\0' )
      fprintf( fp, "Description5    %s~\n",	ch->pcdata->description5	);
    fprintf( fp, "Sex            %d\n",	ch->sex			);
    fprintf( fp, "Class          %d\n",	ch->class		);
    fprintf( fp, "Race           %d\n",	ch->race		);
    fprintf( fp, "Languages      %d %d\n", ch->speaks, ch->speaking );
    fprintf( fp, "Level          %d\n",	ch->level		);
    fprintf( fp, "UpgradeL       %d\n",	ch->pcdata->upgradeL	);

    fprintf( fp, "Played         %d\n",
	ch->played + (int) (current_time - ch->logon)		);
    fprintf( fp, "Room           %d\n",
	(  ch->in_room == get_room_index( ROOM_VNUM_LIMBO )
	&& ch->was_in_room )
	    ? ch->was_in_room->vnum
	    : ch->in_room->vnum );

    fprintf( fp, "HpManaMove     %d %d %d %d %d %d\n",
	ch->hit, ch->max_hit, ch->mana, ch->max_mana, ch->move, ch->max_move );
    fprintf( fp, "Zeni           %d\n",	ch->gold		);
    fprintf( fp, "Exp            %.0Lf\n",	ch->exp			);
    fprintf( fp, "PL             %.0Lf\n",	ch->pl			);
    if (ch->race == 1)
	    fprintf( fp, "HeartPL        %.0Lf\n",	ch->heart_pl			);
    if ( ((is_saiyan(ch) || is_hb(ch)) && ch->exp > 8000000)
    	|| (is_bio(ch) && ch->exp > 7500000))
	    fprintf( fp, "Rage           %d\n",	ch->rage			);
    fprintf( fp, "Corespl	%0.Lf\n",	ch->corespl	);
    fprintf( fp, "Cores %d %d %d\n",
		((ch->fm_core) ? 1 : 0),
		((ch->e_core) ? 1 : 0),
		((ch->h_core) ? 1 : 0) );
    fprintf( fp, "Fusionflags  %d\n",           ch->fusionflags );
    fprintf( fp, "Fusions %d\n",                ch->fusions     );
    fprintf( fp, "Fusiontimer  %d\n",           ch->fusiontimer );
    if( ch->fusions > 0 )
    {
	int b = 0;
        for( b = 0; b < ch->fusions; b++ )
        {
          fprintf( fp, "Fused %d %s~\n", b, ch->fused[b] );
        }
	fprintf( fp, "Bck_name %s~\n",	ch->bck_name );
	fprintf( fp, "Bck_pl   %0.Lf\n", ch->bck_pl );
	fprintf( fp, "Bck_race  %d\n",	ch->bck_race );
    }
    fprintf( fp, "Kairank	 %d\n", ch->kairank	);
    if( ch->demonrank > -1 && is_demon( ch ) )
    fprintf( fp, "Demonrank	 %d\n", ch->demonrank   );
    fprintf( fp, "Evilmod	 %d\n", ch->evilmod	);
    fprintf( fp, "Height         %d\n",	ch->height	);
    fprintf( fp, "Weight         %d\n",	ch->weight	);
    if ( !xIS_EMPTY(ch->act) )
      fprintf( fp, "Act            %s\n", print_bitvector(&ch->act) );
    if ( ch->pcdata->combatFlags != 0 )
      fprintf( fp, "CombatFlags    %d\n",	ch->pcdata->combatFlags );
    if ( !xIS_EMPTY(ch->affected_by) )
      fprintf( fp, "AffectedBy     %s\n",	print_bitvector(&ch->affected_by) );
    if ( !xIS_EMPTY(ch->no_affected_by) )
      fprintf( fp, "NoAffectedBy   %s\n",	print_bitvector(&ch->no_affected_by) );

    /*
     * Strip off fighting positions & store as
     * new style (pos>=100 flags new style in character loading)
     */
    pos = ch->position;
    if (  pos == POS_BERSERK
       || pos == POS_AGGRESSIVE
       || pos == POS_FIGHTING
       || pos == POS_DEFENSIVE
       || pos == POS_EVASIVE
       )
    pos = POS_STANDING;
    pos +=100;
    fprintf( fp, "Position       %d\n", pos);

    fprintf( fp, "Style          %d\n", ch->style);

/*
    fprintf( fp, "Practice       %d\n",	ch->practice		);
    fprintf( fp, "MaxPrac        %d\n",	ch->max_prac		);
*/
    fprintf( fp, "Train          %d\n",	ch->train			);
    fprintf( fp, "MaxTrain       %d\n",	ch->max_train		);
    fprintf( fp, "XTrain         %d\n",	ch->pcdata->xTrain	);
    fprintf( fp, "TotalXTrain    %d\n",	ch->pcdata->total_xTrain		);
    fprintf( fp, "MaxEnergy      %d\n",	ch->max_energy		);
    fprintf( fp, "Powerup        %d\n",	ch->powerup			);

    fprintf( fp, "SavingThrows   %d %d %d %d %d\n",
    		  ch->saving_poison_death,
			  ch->saving_wand,
    		  ch->saving_para_petri,
    		  ch->saving_breath,
    		  ch->saving_spell_staff			);
    fprintf( fp, "Alignment      %d\n",	ch->alignment		);
    fprintf( fp, "Favor          %d\n",	ch->pcdata->favor	);
    fprintf( fp, "Glory          %d\n",   ch->pcdata->quest_curr  );
    fprintf( fp, "MGlory         %d\n",   ch->pcdata->quest_accum );
    fprintf( fp, "Hitroll        %d\n",	ch->hitroll		);
    fprintf( fp, "Damroll        %d\n",	ch->damroll		);
    fprintf( fp, "Armor          %d\n",	ch->armor		);
    fprintf( fp, "NaturalAC      %d\n",	ch->pcdata->natural_ac_max		);
    if ( ch->wimpy )
      fprintf( fp, "Wimpy          %d\n",	ch->wimpy		);
    if ( !xIS_EMPTY(ch->deaf) )
      fprintf( fp, "Deaf           %s\n",	print_bitvector(&ch->deaf)		);
    if ( ch->resistant )
      fprintf( fp, "Resistant      %d\n",	ch->resistant		);
    if ( ch->no_resistant )
      fprintf( fp, "NoResistant    %d\n",	ch->no_resistant	);
    if ( ch->immune )
      fprintf( fp, "Immune         %d\n",	ch->immune		);
    if ( ch->no_immune )
      fprintf( fp, "NoImmune       %d\n",	ch->no_immune		);
    if ( ch->susceptible )
      fprintf( fp, "Susceptible    %d\n",	ch->susceptible		);
    if ( ch->no_susceptible )
      fprintf( fp, "NoSusceptible  %d\n",ch->no_susceptible	);
    if ( ch->pcdata && ch->pcdata->outcast_time )
      fprintf( fp, "Outcast_time   %ld\n",ch->pcdata->outcast_time );
    if ( ch->pcdata && ch->pcdata->nuisance )
      fprintf( fp, "NuisanceNew    %ld %ld %d %d\n", ch->pcdata->nuisance->time,
		ch->pcdata->nuisance->max_time,ch->pcdata->nuisance->flags,
		ch->pcdata->nuisance->power );
    if ( ch->mental_state != -10 )
      fprintf( fp, "Mentalstate    %d\n",	ch->mental_state	);

      fprintf( fp, "Password       %s~\n",	ch->pcdata->pwd		);
	if ( ch->pcdata->rank && ch->pcdata->rank[0] != '\0' )
	  fprintf( fp, "Rank           %s~\n",	ch->pcdata->rank	);
	if ( ch->pcdata->bestowments && ch->pcdata->bestowments[0] != '\0' )
	  fprintf( fp, "Bestowments    %s~\n", 	ch->pcdata->bestowments );
	if ( ch->pcdata->pretitle && ch->pcdata->pretitle[0] != '\0' )
	  fprintf( fp, "Pretitle       %s~\n",	ch->pcdata->pretitle 	);
	  fprintf( fp, "Title         %s~\n",	ch->pcdata->title	);
	if ( ch->pcdata->homepage && ch->pcdata->homepage[0] != '\0' )
	  fprintf( fp, "Homepage       %s~\n",	ch->pcdata->homepage	);
    if ( ch->pcdata->email && ch->pcdata->email[0] != '\0' ) /* Samson 4-19-98 */
	  fprintf( fp, "Email          %s~\n", 	ch->pcdata->email );
	if ( ch->pcdata->icq > 0 ) /* Samson 1-4-99 */
	  fprintf( fp, "ICQ            %d\n",		ch->pcdata->icq );
	if ( ch->pcdata->bio && ch->pcdata->bio[0] != '\0' )
	  fprintf( fp, "Bio            %s~\n",	ch->pcdata->bio 	);
	if ( ch->pcdata->authed_by && ch->pcdata->authed_by[0] != '\0' )
	  fprintf( fp, "AuthedBy       %s~\n",	ch->pcdata->authed_by	);
	if ( ch->pcdata->min_snoop )
	  fprintf( fp, "Minsnoop       %d\n",	ch->pcdata->min_snoop	);
	if ( ch->pcdata->prompt && *ch->pcdata->prompt )
	  fprintf( fp, "Prompt         %s~\n",	ch->pcdata->prompt	);
 	if ( ch->pcdata->fprompt && *ch->pcdata->fprompt )
	  fprintf( fp, "FPrompt	       %s~\n",    ch->pcdata->fprompt	);
	if ( ch->pcdata->pagerlen != 24 )
	  fprintf( fp, "Pagerlen       %d\n",	ch->pcdata->pagerlen	);

        /* Save note board status */
        /* Save number of boards in case that number changes */
        fprintf (fp, "Boards         %d ", MAX_BOARD);
        for (i = 0; i < MAX_BOARD; i++)
            fprintf (fp, "%s %ld ", boards[i].short_name, ch->pcdata->last_note[i]);
        fprintf (fp, "\n");



	/* If ch is ignoring players then store those players */
	{
    	IGNORE_DATA *temp;
	for(temp = ch->pcdata->first_ignored; temp; temp = temp->next)
	{
	  fprintf(fp,"Ignored        %s~\n", temp->name);
	}
        }

	if ( IS_IMMORTAL( ch ) )
	{
	  if ( ch->pcdata->bamfin && ch->pcdata->bamfin[0] != '\0' )
	    fprintf( fp, "Bamfin         %s~\n",	ch->pcdata->bamfin	);
	  if ( ch->pcdata->bamfout && ch->pcdata->bamfout[0] != '\0' )
	    fprintf( fp, "Bamfout        %s~\n",	ch->pcdata->bamfout	);
          if ( ch->trust )
            fprintf( fp, "Trust          %d\n",	ch->trust		);
          if ( ch->pcdata && ch->pcdata->restore_time )
            fprintf( fp, "Restore_time   %ld\n",ch->pcdata->restore_time );
	  	fprintf( fp, "WizInvis       %d\n", ch->pcdata->wizinvis );
	  	fprintf( fp, "Ghost          %d\n", ch->pcdata->ghost_level );
	  	fprintf( fp, "Incog          %d\n", ch->pcdata->incog_level );
	  if ( ch->pcdata->r_range_lo && ch->pcdata->r_range_hi )
	    fprintf( fp, "RoomRange      %d %d\n", ch->pcdata->r_range_lo,
	  					 ch->pcdata->r_range_hi	);
	  if ( ch->pcdata->o_range_lo && ch->pcdata->o_range_hi )
	    fprintf( fp, "ObjRange       %d %d\n", ch->pcdata->o_range_lo,
	  					 ch->pcdata->o_range_hi	);
	  if ( ch->pcdata->m_range_lo && ch->pcdata->m_range_hi )
	    fprintf( fp, "MobRange       %d %d\n", ch->pcdata->m_range_lo,
	  					 ch->pcdata->m_range_hi	);
	}
	if ( ch->pcdata->council)
	  fprintf( fp, "Council        %s~\n", 	ch->pcdata->council_name );
        if ( ch->pcdata->deity_name && ch->pcdata->deity_name[0] != '\0' )
	  fprintf( fp, "Deity          %s~\n",	ch->pcdata->deity_name	 );
	if ( ch->pcdata->clan_name && ch->pcdata->clan_name[0] != '\0' )
	{
	  fprintf( fp, "Clan           %s~\n",	ch->pcdata->clan_name	);
      fprintf( fp, "ClanRank       %d\n",	ch->pcdata->clanRank	);
      fprintf( fp, "ClanZeniDon    %f\n",	ch->pcdata->clanZeniDonated	);
      fprintf( fp, "ClanZeniTax    %f\n",	ch->pcdata->clanZeniClanTax	);
      fprintf( fp, "ClanItemsDon   %d\n",	ch->pcdata->clanItemsDonated	);
	}
      fprintf( fp, "Flags          %d\n",	ch->pcdata->flags	);
        if ( ch->pcdata->release_date )
           fprintf( fp, "Helled         %d %s~\n",
        	(int)ch->pcdata->release_date, ch->pcdata->helled_by );
	if( ch->pcdata->gnote_date )
	  fprintf( fp, "Nognote         %d\n",
		(int)ch->pcdata->gnote_date );
        if( ch->pcdata->nextHBTCDate > 0 )
          fprintf( fp, "NextHBTC	%d\n",
                   (int)ch->pcdata->nextHBTCDate );
        if( ch->pcdata->nextspartime > 0 )
          fprintf( fp, "Nextspar	%d\n",
                   (int)ch->pcdata->nextspartime );
        if( ch->pcdata->HBTCTimeLeft > 0 )
          fprintf( fp, "HBTCLeft	%d\n",
                   ch->pcdata->HBTCTimeLeft );
        if( ch->pcdata->lastTaxation > 0 )
          fprintf( fp, "LastTax	 %d\n",
                       (int)ch->pcdata->lastTaxation );
	fprintf( fp, "PKills         %d\n",	ch->pcdata->pkills	);
	fprintf( fp, "PDeaths        %d\n",	ch->pcdata->pdeaths	);
 	if ( get_timer( ch , TIMER_PKILLED)
         && ( get_timer( ch , TIMER_PKILLED) > 0 ) )
    	fprintf( fp, "PTimer         %d\n",     get_timer(ch, TIMER_PKILLED));
    fprintf( fp, "MKills         %d\n",	ch->pcdata->mkills	);
	fprintf( fp, "MDeaths        %d\n",	ch->pcdata->mdeaths	);
	fprintf( fp, "IllegalPK      %d\n",	ch->pcdata->illegal_pk	);
    fprintf( fp, "SparWins       %d\n",	ch->pcdata->spar_wins	);
    fprintf( fp, "SparLoss       %d\n",	ch->pcdata->spar_loss	);
	fprintf( fp, "Sparcount	   %d\n",   ch->pcdata->sparcount   );
    fprintf( fp, "Bounty         %d\n",   ch->pcdata->bounty              );
    fprintf( fp, "Bowed          %d\n",   ch->pcdata->bowed               );
    fprintf( fp, "Bkills         %d\n",   ch->pcdata->bkills              );
    fprintf( fp, "B_timeleft     %d\n",   ch->pcdata->b_timeleft              );

    if (ch->pcdata->bounty_by)
    fprintf( fp, "BountyBy       %s~\n",   ch->pcdata->bounty_by              );
    if (ch->pcdata->hunting)
    fprintf( fp, "Hunting        %s~\n",   ch->pcdata->hunting              );
    if( ch->pcdata->gohometimer != 0 )
    fprintf( fp, "Gohome         %d\n", ch->pcdata->gohometimer );
    if( get_true_rank( ch ) >= 11 )
    {
    fprintf( fp, "KaiTimer       %d\n", ch->pcdata->eKTimer	);
    fprintf( fp, "KaiMessage     %s~\n",ch->pcdata->kaiRestoreMsg );
    }
    fprintf( fp, "Silence        %d\n",	ch->pcdata->silence	);
    if( ch->pcdata->silence != 0 )
    fprintf( fp, "Silencedby     %s~\n", ch->pcdata->silencedby	);
    fprintf( fp, "PK_timer       %d\n",	ch->pcdata->pk_timer	);
    fprintf( fp, "Age            %d\n",	ch->pcdata->age	);
    fprintf( fp, "Build          %d\n",	ch->pcdata->build	);
    fprintf( fp, "Haircolor      %d\n",	ch->pcdata->haircolor	);
    fprintf( fp, "Orignalhaircolor %d\n",	ch->pcdata->orignalhaircolor	);
    fprintf( fp, "Eyes           %d\n",	ch->pcdata->eyes	);
    fprintf( fp, "Orignaleyes    %d\n",	ch->pcdata->orignaleyes	);
    fprintf( fp, "AuraPowerUp	 %d\n", ch->pcdata->auraColorPowerUp );
    fprintf( fp, "Complexion     %d\n",	ch->pcdata->complexion	);
    fprintf( fp, "Secondarycolor %d\n",	ch->pcdata->secondarycolor	);
    fprintf( fp, "Hairstyle      %d\n",	ch->pcdata->hairstyle	);
    fprintf( fp, "Hairlen        %d\n",	ch->pcdata->hairlen	);
    fprintf( fp, "Tail           %d\n",	ch->pcdata->tail	);
    fprintf( fp, "Suppress       %.0Lf\n",	ch->pcdata->suppress	);
    fprintf( fp, "Spouse         %s~\n",	ch->pcdata->spouse	);
    fprintf( fp, "Auction_PL     %.0Lf\n",	ch->pcdata->auction_pl	);
    fprintf( fp, "NPromptCFG     %d\n",	ch->pcdata->normalPromptConfig	);
    fprintf( fp, "BPromptCFG     %d\n",	ch->pcdata->battlePromptConfig	);
	fprintf( fp, "AttrPerm       %d %d %d %d %d\n",
	    ch->perm_str,
	    ch->perm_int,
	    ch->perm_dex,
	    ch->perm_con,
	    ch->perm_lck );
	/* For the android enhance command, so that the base
	   stats can be modified without interfering with
	   transformations. At the time, only str and dex
	   are needed here. The other stats are added just in
	   case of any future need for them. I also realise that
	   this is the sort of thing that AttrMod is for, however
	   I am creating this seperately to avoid any conflicts with
	   pre-existing code. -Karma.
	*/
	fprintf( fp, "AttrAdd       %d %d %d %d %d\n",
            ch->add_str,
            ch->add_dex,
	    ch->add_int,
	    ch->add_con,
	    ch->add_lck );

	fprintf( fp, "PermTStats     %d %d %d %d\n",
	    ch->pcdata->permTstr,
	    ch->pcdata->permTint,
	    ch->pcdata->permTspd,
	    ch->pcdata->permTcon );
	fprintf( fp, "AttrTrain      %d %d %d %d\n",
	    ch->pcdata->tStr,
	    ch->pcdata->tInt,
	    ch->pcdata->tSpd,
	    ch->pcdata->tCon );

	fprintf( fp, "AttrMod        0 0 0 0 0\n" );
/*
	fprintf( fp, "AttrMod        %d %d %d %d %d\n",
	    ch->mod_str - ch->perm_str,
	    ch->mod_int - ch->perm_int,
	    ch->mod_dex - ch->perm_dex,
	    ch->mod_con - ch->perm_con,
	    ch->mod_lck - ch->perm_lck );
*/
	fprintf( fp, "Condition      %d %d %d %d\n",
	    ch->pcdata->condition[0],
	    ch->pcdata->condition[1],
	    ch->pcdata->condition[2],
	    ch->pcdata->condition[3] );
	if (ch->race == 6)
	{
	fprintf( fp, "Absorb_Race    %d %d %d %d %d %d %d\n",
	    ch->pcdata->absorb_race[0],
	    ch->pcdata->absorb_race[1],
	    ch->pcdata->absorb_race[2],
	    ch->pcdata->absorb_race[3],
	    ch->pcdata->absorb_race[4],
	    ch->pcdata->absorb_race[5],
	    ch->pcdata->absorb_race[6] );
    fprintf( fp, "Absorb_Pc      %d\n",	ch->pcdata->absorb_pc	);
    fprintf( fp, "Absorb_Mob     %d\n",	ch->pcdata->absorb_mob	);
    fprintf( fp, "Absorb_Pl_Mod  %d\n",	ch->pcdata->absorb_pl_mod	);
	}
	fprintf( fp, "LastIntrest    %d %d\n",
	    ch->pcdata->interestLastMonth,
	    ch->pcdata->interestLastYear );

	if (ch->race == 6 || is_android(ch) )
	    fprintf( fp, "SD_Charge  %d\n",	ch->pcdata->sd_charge	);

	if( is_android(ch) )
	    fprintf( fp, "Battery %d\n",	ch->battery );

	if ( ch->desc && ch->desc->host )
      fprintf( fp, "Site           %s\n", ch->desc->host );
	else
      fprintf( fp, "Site           %s\n", ch->pcdata->lasthost );
      fprintf( fp, "LastOn         %d\n", (int)current_time );
        for (sn = 0; sn < AT_MAXCOLOR; ++sn)
          if (ch->pcdata->colorize[sn] != -1)
      		fprintf( fp, "Color          %s %d\n", at_color_table[sn].name,
            		ch->pcdata->colorize[sn] );

	for ( sn = 1; sn < top_sn; sn++ )
	{
	    if ( skill_table[sn]->name && ch->pcdata->learned[sn] > 0 )
		switch( skill_table[sn]->type )
		{
		    default:
			fprintf( fp, "Skill        %f '%s'\n",
			  ch->pcdata->learned[sn], skill_table[sn]->name );
			break;
		    case SKILL_ABILITY:
			fprintf( fp, "Ability      %f '%s'\n",
			  ch->pcdata->learned[sn], skill_table[sn]->name );
			break;
		    case SKILL_SPELL:
			fprintf( fp, "Spell        %.0f '%s'\n",
			  ch->pcdata->learned[sn], skill_table[sn]->name );
			break;
		    case SKILL_WEAPON:
			fprintf( fp, "Weapon       %.0f '%s'\n",
			  ch->pcdata->learned[sn], skill_table[sn]->name );
			break;
		    case SKILL_TONGUE:
			fprintf( fp, "Tongue       %.0f '%s'\n",
			  ch->pcdata->learned[sn], skill_table[sn]->name );
			break;
		}
	}

    for ( paf = ch->first_affect; paf; paf = paf->next )
    {
	if ( paf->type >= 0 && (skill=get_skilltype(paf->type)) == NULL )
	    continue;

	if ( paf->type >= 0 && paf->type < TYPE_PERSONAL )
	  fprintf( fp, "AffectData   '%s' %3d %3d %3d %3d %s\n",
	    skill->name,
	    paf->duration,
	    paf->modifier,
	    paf->location,
	    paf->affLocator,
	    print_bitvector(&paf->bitvector)
	    );
	else
	  fprintf( fp, "Affect       %3d %3d %3d %3d %3d %s\n",
	    paf->type,
	    paf->duration,
	    paf->modifier,
	    paf->location,
	    paf->affLocator,
	    print_bitvector(&paf->bitvector)
	    );
    }

    track = URANGE( 2, ((ch->level+3) * MAX_KILLTRACK)/LEVEL_AVATAR, MAX_KILLTRACK );
    for ( sn = 0; sn < track; sn++ )
    {
	if ( ch->pcdata->killed[sn].vnum == 0 )
	  break;
	fprintf( fp, "Killed       %d %d\n",
		ch->pcdata->killed[sn].vnum,
		ch->pcdata->killed[sn].count );
    }

#ifdef IMC
    imc_savechar( ch, fp );
#endif

    fprintf( fp, "End\n\n" );
    return;
}



/*
 * Write an object and its contents.
 */
void fwrite_obj( CHAR_DATA *ch, OBJ_DATA *obj, FILE *fp, int iNest,
		 sh_int os_type )
{
    EXTRA_DESCR_DATA *ed;
    AFFECT_DATA *paf;
    sh_int wear, wear_loc, x;

    if ( iNest >= MAX_NEST )
    {
	bug( "fwrite_obj: iNest hit MAX_NEST %d", iNest );
	return;
    }

    /*
     * Slick recursion to write lists backwards,
     *   so loading them will load in forwards order.
     */
    if ( obj->prev_content && os_type != OS_CORPSE )
	fwrite_obj( ch, obj->prev_content, fp, iNest, OS_CARRY );

    /*
     * Castrate storage characters.
     * Catch deleted objects                                    -Thoric
     * Do NOT save prototype items!				-Thoric
     */
/*
    if ( (ch && ch->exp < (obj->level * 0.30))
    || ( obj->item_type == ITEM_KEY && !IS_OBJ_STAT(obj, ITEM_CLANOBJECT ))
    ||   obj_extracted(obj)
    ||   IS_OBJ_STAT( obj, ITEM_PROTOTYPE ) )
	return;
*/
    /*    Munch magic flagged containers for now - bandaid */
    if ( obj->item_type == ITEM_CONTAINER
    &&   IS_OBJ_STAT( obj, ITEM_MAGIC ) )
	xTOGGLE_BIT( obj->extra_flags, ITEM_MAGIC );

    /* Corpse saving. -- Altrag */
    fprintf( fp, (os_type == OS_CORPSE ? "#CORPSE\n" : "#OBJECT\n") );

    if ( iNest )
	fprintf( fp, "Nest         %d\n",	iNest		     );
    if ( obj->count > 1 )
	fprintf( fp, "Count        %d\n",	obj->count	     );
    if ( QUICKMATCH( obj->name, obj->pIndexData->name ) == 0 )
	fprintf( fp, "Name         %s~\n",	obj->name	     );
    if ( QUICKMATCH( obj->short_descr, obj->pIndexData->short_descr ) == 0 )
	fprintf( fp, "ShortDescr   %s~\n",	obj->short_descr     );
    if ( QUICKMATCH( obj->description, obj->pIndexData->description ) == 0 )
	fprintf( fp, "Description  %s~\n",	obj->description     );
    if ( QUICKMATCH( obj->action_desc, obj->pIndexData->action_desc ) == 0 )
	fprintf( fp, "ActionDesc   %s~\n",	obj->action_desc     );
    fprintf( fp, "Vnum         %d\n",	obj->pIndexData->vnum	     );
    if (!obj->origin)
	fprintf( fp, "Origin       %d, PRE-ORIGIN~\n",ORIGIN_UNKNOWN	     );
	else
	fprintf( fp, "Origin       %s~\n",obj->origin	     );
    if ( os_type == OS_CORPSE && obj->in_room )
      fprintf( fp, "Room         %d\n",   obj->in_room->vnum         );
    if ( !xSAME_BITS(obj->extra_flags, obj->pIndexData->extra_flags) )
	fprintf( fp, "ExtraFlags   %s\n",	print_bitvector(&obj->extra_flags) );
    if ( obj->wear_flags != obj->pIndexData->wear_flags )
	fprintf( fp, "WearFlags    %d\n",	obj->wear_flags	     );
    wear_loc = -1;
    for ( wear = 0; wear < MAX_WEAR; wear++ )
	for ( x = 0; x < MAX_LAYERS; x++ )
	   if ( obj == save_equipment[wear][x] )
	   {
		wear_loc = wear;
		break;
	   }
	   else
	   if ( !save_equipment[wear][x] )
		break;
    if ( wear_loc != -1 )
	fprintf( fp, "WearLoc      %d\n",	wear_loc	     );
    if ( obj->item_type != obj->pIndexData->item_type )
	fprintf( fp, "ItemType     %d\n",	obj->item_type	     );
    if ( obj->weight != obj->pIndexData->weight )
      fprintf( fp, "Weight       %d\n",	obj->weight		     );
    if ( obj->level )
      fprintf( fp, "Level        %.0Lf\n",	obj->level		     );
    if ( obj->timer )
      fprintf( fp, "Timer        %d\n",	obj->timer		     );
    if ( obj->cost != obj->pIndexData->cost )
      fprintf( fp, "Cost         %d\n",	obj->cost		     );
    if ( obj->value[0] || obj->value[1] || obj->value[2]
    ||   obj->value[3] || obj->value[4] || obj->value[5] )
      fprintf( fp, "Values       %d %d %d %d %d %d\n",
	obj->value[0], obj->value[1], obj->value[2],
	obj->value[3], obj->value[4], obj->value[5]     );

    switch ( obj->item_type )
    {
    case ITEM_PILL: /* was down there with staff and wand, wrongly - Scryn */
    case ITEM_POTION:
    case ITEM_SCROLL:
	if ( IS_VALID_SN(obj->value[1]) )
	    fprintf( fp, "Spell 1      '%s'\n",
		skill_table[obj->value[1]]->name );

	if ( IS_VALID_SN(obj->value[2]) )
	    fprintf( fp, "Spell 2      '%s'\n",
		skill_table[obj->value[2]]->name );

	if ( IS_VALID_SN(obj->value[3]) )
	    fprintf( fp, "Spell 3      '%s'\n",
		skill_table[obj->value[3]]->name );

	break;

    case ITEM_STAFF:
    case ITEM_WAND:
	if ( IS_VALID_SN(obj->value[3]) )
	    fprintf( fp, "Spell 3      '%s'\n",
		skill_table[obj->value[3]]->name );

	break;
    case ITEM_SALVE:
	if ( IS_VALID_SN(obj->value[4]) )
	    fprintf( fp, "Spell 4      '%s'\n",
		skill_table[obj->value[4]]->name );

	if ( IS_VALID_SN(obj->value[5]) )
	    fprintf( fp, "Spell 5      '%s'\n",
		skill_table[obj->value[5]]->name );
	break;
    }

    for ( paf = obj->first_affect; paf; paf = paf->next )
    {
	/*
	 * Save extra object affects				-Thoric
	 */
	if ( paf->type < 0 || paf->type >= top_sn )
	{
	  fprintf( fp, "Affect       %d %d %d %d %s\n",
	    paf->type,
	    paf->duration,
	     ((paf->location == APPLY_WEAPONSPELL
	    || paf->location == APPLY_WEARSPELL
	    || paf->location == APPLY_REMOVESPELL
	    || paf->location == APPLY_STRIPSN
	    || paf->location == APPLY_RECURRINGSPELL)
	    && IS_VALID_SN(paf->modifier))
	    ? skill_table[paf->modifier]->slot : paf->modifier,
	    paf->location,
	    print_bitvector(&paf->bitvector)
	    );
	}
	else
	  fprintf( fp, "AffectData   '%s' %d %d %d %s\n",
	    skill_table[paf->type]->name,
	    paf->duration,
	     ((paf->location == APPLY_WEAPONSPELL
	    || paf->location == APPLY_WEARSPELL
	    || paf->location == APPLY_REMOVESPELL
	    || paf->location == APPLY_STRIPSN
	    || paf->location == APPLY_RECURRINGSPELL)
	    && IS_VALID_SN(paf->modifier))
	    ? skill_table[paf->modifier]->slot : paf->modifier,
	    paf->location,
	    print_bitvector(&paf->bitvector)
	    );
    }

    for ( ed = obj->first_extradesc; ed; ed = ed->next )
	fprintf( fp, "ExtraDescr   %s~ %s~\n",
	    ed->keyword, ed->description );

    fprintf( fp, "End\n\n" );

    if ( obj->first_content )
	fwrite_obj( ch, obj->last_content, fp, iNest + 1, OS_CARRY );

    return;
}



/*
 * Load a char and inventory into a new ch structure.
 */
bool load_char_obj( DESCRIPTOR_DATA *d, char *name, bool preload )
{
    char strsave[MAX_INPUT_LENGTH];
    CHAR_DATA *ch;
    FILE *fp;
    bool found;
    struct stat fst;
    int i, x;
    extern FILE *fpArea;
    extern char strArea[MAX_INPUT_LENGTH];
    char buf[MAX_INPUT_LENGTH];

    CREATE( ch, CHAR_DATA, 1 );
    for ( x = 0; x < MAX_WEAR; x++ )
	for ( i = 0; i < MAX_LAYERS; i++ )
	    save_equipment[x][i] = NULL;
    clear_char( ch );
    loading_char = ch;

    CREATE( ch->pcdata, PC_DATA, 1 );
    d->character		= ch;
    ch->desc			= d;
    ch->pcdata->filename	= STRALLOC( name );
    ch->name			= NULL;
    ch->act			= multimeb(PLR_BLANK, PLR_COMBINE, PLR_PROMPT, -1);
    ch->perm_str		= 10;
    ch->perm_int		= 10;
    ch->perm_dex		= 10;
    ch->perm_con		= 10;
    ch->perm_lck		= 0;
    ch->no_resistant 		= 0;
    ch->no_susceptible 		= 0;
    ch->no_immune 		= 0;
    ch->was_in_room		= NULL;
    xCLEAR_BITS(ch->no_affected_by);
    ch->pcdata->condition[COND_THIRST]	= 48;
    ch->pcdata->condition[COND_FULL]	= 48;
    ch->pcdata->nuisance		= NULL;
    ch->pcdata->wizinvis		= 0;
    ch->pcdata->ghost_level		= 0;
    ch->pcdata->incog_level		= 0;
    ch->pcdata->charmies		= 0;
    ch->mental_state			= -10;
    ch->mobinvis			= 0;
	ch->pcdata->admintalk	= 0;
    for(i = 0; i < MAX_SKILL; i++)
        ch->pcdata->learned[i]		= 0;
    ch->pcdata->release_date		= 0;
    ch->pcdata->gnote_date		= 0;
    ch->pcdata->nextHBTCDate            = 0;
	ch->pcdata->nextspartime = 0;
    ch->pcdata->HBTCTimeLeft            = 0;
    ch->pcdata->HBTCPartner		= NULL;
    ch->pcdata->lastTaxation            = 0;
    ch->pcdata->creation_date		= 0;
    ch->pcdata->helled_by			= NULL;
    ch->saving_poison_death 		= 0;
    ch->saving_wand				= 0;
    ch->saving_para_petri		= 0;
    ch->saving_breath			= 0;
    ch->saving_spell_staff		= 0;
    ch->style 					= STYLE_FIGHTING;
    ch->comments                = NULL;    /* comments */
    ch->pcdata->pagerlen		= 24;
    ch->pcdata->first_ignored		= NULL;    /* Ignore list */
    ch->pcdata->last_ignored		= NULL;
    ch->pcdata->tell_history		= NULL;	/* imm only lasttell cmnd */
    ch->pcdata->lt_index		= 0;	/* last tell index */
    ch->morph                   = NULL;
    ch->pcdata->email			= NULL; /* Initialize email address - Samson 1-4-99 */
    ch->pcdata->homepage		= NULL; /* Initialize homepage - Samson 1-4-99 */
    ch->pcdata->icq				= 0; /* Initalize icq# - Samson 1-4-99 */

    ch->pcdata->bounty_by		= str_dup( "" );
    ch->pcdata->hunting		= str_dup( "" );
    ch->pcdata->spouse		= str_dup( "" );

    ch->pcdata->last_name		= str_dup( "" );
    ch->charge					= 0;
    ch->skillGsn				= 0;

#ifdef IMC
    imc_initchar( ch );
#endif

    for (i = 0; i < AT_MAXCOLOR; ++i)
      ch->pcdata->colorize[i] = -1;


	ch->pcdata->absorb_race[0] = 0;
	ch->pcdata->absorb_race[1] = 0;
	ch->pcdata->absorb_race[2] = 0;
	ch->pcdata->absorb_race[3] = 0;
	ch->pcdata->absorb_race[4] = 0;
	ch->pcdata->absorb_race[5] = 0;
	ch->pcdata->absorb_race[6] = 0;
	ch->pcdata->absorb_race[7] = 0;
	ch->pcdata->absorb_race[8] = 0;
	ch->pcdata->absorb_race[9] = 0;
	ch->pcdata->absorb_race[10] = 0;

	ch->pcdata->absorb_pc = 0;
	ch->pcdata->absorb_mob = 0;
	ch->pcdata->absorb_pl_mod = 0;
	ch->pcdata->absorb_pl = 0;
	ch->pcdata->absorb_sn = 0;
	ch->pcdata->absorb_learn = 0;

    ch->pcdata->pIdle[0] = 0;
    ch->pcdata->pIdle[1] = 0;
    ch->pcdata->pIdle[2] = 0;
    ch->pcdata->pIdle[3] = 0;
    ch->pcdata->pIdle[4] = 0;
    ch->pcdata->iIdle = 0;
    ch->pcdata->bot_warn[0] = 0;
    ch->pcdata->bot_warn[1] = 0;
    ch->pcdata->bot_warn[2] = 0;

	ch->pcdata->tStr = 0;
	ch->pcdata->tInt = 0;
	ch->pcdata->tSpd = 0;
	ch->pcdata->tCon = 0;

	ch->pcdata->permTstr = 0;
	ch->pcdata->permTint = 0;
	ch->pcdata->permTspd = 0;
	ch->pcdata->permTcon = 0;

	ch->pcdata->upgradeL = 0;
        ch->pcdata->auraColorPowerUp = -1;

	ch->pcdata->interestLastMonth = 0;
	ch->pcdata->interestLastYear = 0;

	ch->focus = 0;

    found = FALSE;
    sprintf( strsave, "%s%c/%s", PLAYER_DIR, tolower(name[0]),
			capitalize( name ) );

	GfpName = strsave;

    if ( stat( strsave, &fst ) != -1 )
    {
      if ( fst.st_size == 0 )
      {
		sprintf( strsave, "%s%c/%s", BACKUP_DIR, tolower(name[0]),
			capitalize( name ) );
		send_to_char( "Restoring your backup player file...", ch );
      }
      else
      {
		if ( !strcmp(ch->pcdata->filename, "Goku") || !strcmp(ch->pcdata->filename, "Guava") )
		{
			sprintf( buf, "%s player data for: %s (%dK)",
			  preload ? "Preloading" : "Loading", ch->pcdata->filename,
				(int) fst.st_size/1024 );
			log_string_plus( buf, LOG_COMM, LEVEL_SUPREME );
		}
		else
		{
			sprintf( buf, "%s player data for: %s (%dK)",
			  preload ? "Preloading" : "Loading", ch->pcdata->filename,
				(int) fst.st_size/1024 );
			log_string_plus( buf, LOG_COMM, LEVEL_GREATER );
		}
      }
    }
    /* else no player file */

    if ( ( fp = fopen( strsave, "r" ) ) != NULL )
    {
	int iNest;

	for ( iNest = 0; iNest < MAX_NEST; iNest++ )
	    rgObjNest[iNest] = NULL;

	found = TRUE;
	/* Cheat so that bug will show line #'s -- Altrag */
	fpArea = fp;
	strcpy(strArea, strsave);
	for ( ; ; )
	{
	    char letter;
	    char *word;

	if (StopFP)
	{
		bug("Bad Pfile detected.  Stoping proccessing of bad Pfile.");
		StopFP = FALSE;
		return found;
	}

	    letter = fread_letter( fp );
	    if ( letter == '*' )
	    {
		fread_to_eol( fp );
		continue;
	    }

	    if ( letter != '#' )
	    {
		bug( "Load_char_obj: # not found.", 0 );
		bug( name, 0 );
		break;
	    }

	    word = fread_word( fp );

	    if ( !strcmp( word, "PLAYER" ) )
	    {
			fread_char ( ch, fp, preload );
			if ( preload )
				break;
	    }
	    else if ( !strcmp( word, "OBJECT" ) ) /* Objects	*/
			fread_obj  ( ch, fp, OS_CARRY );
		else if ( !strcmp( word, "MorphData") ) /* Morphs */
			fread_morph_data ( ch, fp );
		else if ( !strcmp( word, "COMMENT") )
			fread_comment(ch, fp );		/* Comments	*/
	    else if ( !strcmp( word, "MOBILE") )
		{
			CHAR_DATA *mob;
			mob = fread_mobile( fp );
			ch->pcdata->pet = mob;
			mob->master = ch;
			xSET_BIT(mob->affected_by, AFF_CHARM);
		}
		else if ( !strcmp( word, "END"    ) )	/* Done		*/
			break;
		else
		{
			bug( "Load_char_obj: bad section.", 0 );
			bug( name, 0 );
			break;
		}
	}
	fclose( fp );
	fpArea = NULL;
	strcpy(strArea, "$");
    }

    if ( !found )
    {
        ch->name	   		= STRALLOC( name );
	ch->short_descr			= STRALLOC( "" );
	ch->long_descr			= STRALLOC( "" );
	ch->description			= STRALLOC( "" );
	ch->pcdata->description1			= STRALLOC( "" );
	ch->pcdata->description2			= STRALLOC( "" );
	ch->pcdata->description3			= STRALLOC( "" );
	ch->pcdata->description4			= STRALLOC( "" );
	ch->pcdata->description5			= STRALLOC( "" );
	ch->editor			= NULL;
	ch->pcdata->clan_name		= STRALLOC( "" );
	ch->pcdata->clan		= NULL;
	ch->pcdata->council_name 	= STRALLOC( "" );
	ch->pcdata->council 		= NULL;
  	ch->pcdata->deity_name		= STRALLOC( "" );
	ch->pcdata->deity		= NULL;
	ch->pcdata->pet			= NULL;
	ch->pcdata->pwd			= str_dup( "" );

        /* every characters starts at default board from login.. this board
         should be read_level == 0 !
        */
        ch->pcdata->board		= &boards[DEFAULT_BOARD];

        ch->pcdata->bamfin		= str_dup( "" );
	ch->pcdata->bamfout		= str_dup( "" );
	ch->pcdata->rank		= str_dup( "" );
	ch->pcdata->bestowments		= str_dup( "" );
	ch->pcdata->pretitle		= STRALLOC( "" );
	ch->pcdata->title		= STRALLOC( "" );
	ch->pcdata->homepage		= str_dup( "" );
	ch->pcdata->email		= str_dup( "" ); /* Samson 4-19-98 */
	ch->pcdata->icq			= 0;			 /* Samson 1-4-99 */
	ch->pcdata->bio 		= STRALLOC( "" );
	ch->pcdata->authed_by		= STRALLOC( "" );
	ch->pcdata->prompt		= STRALLOC( "" );
	ch->pcdata->fprompt		= STRALLOC( "" );
	ch->pcdata->kaiRestoreMsg	= str_dup( "" );
    ch->pcdata->bounty_by		= str_dup( "" );
    ch->pcdata->hunting			= str_dup( "" );
    ch->pcdata->spouse			= str_dup( "" );

    ch->pcdata->last_name			= str_dup( "" );

	ch->pcdata->r_range_lo		= 0;
	ch->pcdata->r_range_hi		= 0;
	ch->pcdata->m_range_lo		= 0;
	ch->pcdata->m_range_hi		= 0;
	ch->pcdata->o_range_lo		= 0;
	ch->pcdata->o_range_hi		= 0;
	ch->pcdata->wizinvis		= 0;
	ch->pcdata->ghost_level		= 0;
	ch->pcdata->incog_level		= 0;
	ch->pcdata->admintalk		= 0;
    ch->pcdata->silence			= 0;
    ch->pcdata->eKTimer			= 0;
    ch->pcdata->pk_timer		= 0;
    ch->pcdata->auction_pl		= 0;
    ch->pcdata->pIdle[0] = 0;
    ch->pcdata->pIdle[1] = 0;
    ch->pcdata->pIdle[2] = 0;
    ch->pcdata->pIdle[3] = 0;
    ch->pcdata->pIdle[4] = 0;
    ch->pcdata->iIdle = 0;
    ch->pcdata->bot_warn[0] = 0;
    ch->pcdata->bot_warn[1] = 0;
    ch->pcdata->bot_warn[2] = 0;
    ch->pcdata->auraColorPowerUp = -1;
	ch->pcdata->interestLastMonth = 0;
	ch->pcdata->interestLastYear = 0;

    }
    else
    {
	if ( !ch->name )
    		ch->name	= STRALLOC( name );
	if ( !ch->pcdata->clan_name )
	{
	  ch->pcdata->clan_name	= STRALLOC( "" );
	  ch->pcdata->clan	= NULL;
	}
	if ( !ch->pcdata->council_name )
	{
	  ch->pcdata->council_name = STRALLOC( "" );
	  ch->pcdata->council 	= NULL;
	}
	if ( !ch->pcdata->deity_name )
	{
	  ch->pcdata->deity_name = STRALLOC( "" );
	  ch->pcdata->deity	 = NULL;
	}
        if ( !ch->pcdata->bio )
          ch->pcdata->bio	 = STRALLOC( "" );

	if ( !ch->pcdata->authed_by )
	  ch->pcdata->authed_by	 = STRALLOC( "" );

	if ( xIS_SET(ch->act, PLR_FLEE) )
	  xREMOVE_BIT(ch->act, PLR_FLEE);

	if ( IS_IMMORTAL( ch ) )
	{
	  if ( ch->pcdata->wizinvis < 2 )
	    ch->pcdata->wizinvis = ch->level;
	  if ( ch->pcdata->ghost_level < 2 )
	    ch->pcdata->ghost_level = ch->level;
	  if ( ch->pcdata->incog_level < 2 )
	    ch->pcdata->incog_level = ch->level;
 	  assign_area( ch );
	}
	if ( file_ver > 1 ) {
	  for ( i = 0; i < MAX_WEAR; i++ )
	    for ( x = 0; x < MAX_LAYERS; x++ )
		if ( save_equipment[i][x] )
		{
		    equip_char( ch, save_equipment[i][x], i );
		    save_equipment[i][x] = NULL;
		}
		else
		    break;
	}

	/* Must be done *AFTER* eq is worn because of wis/int modifiers */
/*	if ( !IS_IMMORTAL(ch) )
		REMOVE_BIT(ch->speaks, LANG_COMMON | race_table[ch->race]->language);
	if ( countlangs(ch->speaks) < (ch->level / 10) && !IS_IMMORTAL(ch) )
	{
		int prct = 5 + (get_curr_int(ch) / 6) + (get_curr_wis(ch) / 7);

		do
		{
			int iLang;
			int lang = 1;
			int need = (ch->level / 10) - countlangs(ch->speaks);
			int prac = 2 - (get_curr_cha(ch) / 17) * (70 / prct) * need;

			if ( ch->practice >= prac )
				break;

			for ( iLang = 1; lang_array[iLang] != LANG_UNKNOWN; iLang++ )
				if ( number_range( 1, iLang ) == 1 )
					lang = iLang;
			if ( (iLang = bsearch_skill_exact( lang_names[lang], gsn_first_tongue, gsn_top_sn-1  )) < 0 )
				continue;
			if ( ch->pcdata->learned[iLang] > 0 )
				continue;
			SET_BIT(ch->speaks, lang_array[lang]);
			ch->pcdata->learned[iLang] = 70;
			ch->speaks &= VALID_LANGS;
			REMOVE_BIT(ch->speaks,
					   LANG_COMMON | race_table[ch->race]->language);
		}
	}*/
    }

    /* Rebuild affected_by and RIS to catch errors - FB */
    update_aris(ch);
    loading_char = NULL;
    return found;
}



/*
 * Read in a char.
 */

#if defined(KEY)
#undef KEY
#endif

#define KEY( literal, field, value )					\
				if ( !strcmp( word, literal ) )	\
				{					\
				    field  = value;			\
				    fMatch = TRUE;			\
				    break;				\
				}

void fread_char( CHAR_DATA *ch, FILE *fp, bool preload )
{
    char buf[MAX_STRING_LENGTH];
    char *line;
    char *word;
    int x1, x2, x3, x4, x5, x6, x7;
    sh_int killcnt;
    bool fMatch;

    file_ver = 0;
    killcnt = 0;
    for ( ; ; )
    {
	word   = feof( fp ) ? "End" : fread_word( fp );
	fMatch = FALSE;

	if (StopFP)
	{
		bug("Bad Pfile detected.  Stoping proccessing of bad Pfile.");
		StopFP = FALSE;
		return;
	}

	switch ( UPPER(word[0]) )
	{
	case '*':
	    fMatch = TRUE;
	    fread_to_eol( fp );
	    break;

	case 'A':
	    if ( !strcmp( word, "Ability" ) )
	    {
		int sn;
		double value;

		if ( preload )
		  word = "End";
		else
		{
		  value = fread_number_skill( fp );
		  if ( file_ver < 3 )
		    sn = skill_lookup( fread_word( fp ) );
		  else
		    sn = bsearch_skill_exact( fread_word( fp ), gsn_first_ability, gsn_first_weapon-1 );
		  if ( sn < 0 )
		    bug( "Fread_char: unknown ability.", 0 );
		  else
		  {
		    ch->pcdata->learned[sn] = value;
		    /* Take care of people who have stuff they shouldn't     *
		     * Assumes class and level were loaded before. -- Altrag *
		     * Assumes practices are loaded first too now. -- Altrag */
	      if ( skill_table[sn]->skill_level[ch->class] <= 0 )
		      {
		        ch->pcdata->learned[sn] = 0;
		        ch->practice++;
		      }

		  }
		  fMatch = TRUE;
		  break;
		}
	    }

	    if ( !str_cmp( word, "Absorb_Race" ) )
	    {
		line = fread_line( fp );
		sscanf( line, "%d %d %d %d %d %d %d",
		      &x1, &x2, &x3, &x4, &x5, &x6, &x7 );
		ch->pcdata->absorb_race[0] = x1;
		ch->pcdata->absorb_race[1] = x2;
		ch->pcdata->absorb_race[2] = x3;
		ch->pcdata->absorb_race[3] = x4;
		ch->pcdata->absorb_race[4] = x5;
		ch->pcdata->absorb_race[5] = x6;
		ch->pcdata->absorb_race[6] = x7;
		fMatch = TRUE;
		break;
	    }
	    KEY( "Absorb_Pc",	ch->pcdata->absorb_pc,		fread_number( fp ) );
	    KEY( "Absorb_Mob",	ch->pcdata->absorb_mob,		fread_number( fp ) );
	    KEY( "Absorb_Pl_Mod",	ch->pcdata->absorb_pl_mod,		fread_number( fp ) );

	    KEY( "Act",		ch->act,		fread_bitvector( fp ) );
	    KEY( "AffectedBy",	ch->affected_by,	fread_bitvector( fp ) );
	    KEY( "Alignment",	ch->alignment,		fread_number( fp ) );
	    KEY( "Armor",	ch->armor,		fread_number( fp ) );
	    KEY( "Age",	ch->pcdata->age,	fread_number( fp ) );
            KEY( "AuraPowerUp", ch->pcdata->auraColorPowerUp, fread_number( fp ) );

	    if ( !strcmp( word, "Affect" ) || !strcmp( word, "AffectData" ) )
	    {
		AFFECT_DATA *paf;

		if ( preload )
		{
		    fMatch = TRUE;
		    fread_to_eol( fp );
		    break;
		}
		CREATE( paf, AFFECT_DATA, 1 );
		if ( !strcmp( word, "Affect" ) )
		{
		    paf->type	= fread_number( fp );
		}
		else
		{
		    int sn;
		    char *sname = fread_word(fp);

		    if ( (sn=skill_lookup(sname)) < 0 )
		    {
			if ( (sn=herb_lookup(sname)) < 0 )
			    bug( "Fread_char: unknown skill.", 0 );
			else
			    sn += TYPE_HERB;
		    }
		    paf->type = sn;
		}

		paf->duration	= fread_number( fp );
		paf->modifier	= fread_number( fp );
		paf->location	= fread_number( fp );
		if ( paf->location == APPLY_WEAPONSPELL
		||   paf->location == APPLY_WEARSPELL
		||   paf->location == APPLY_REMOVESPELL
		||   paf->location == APPLY_STRIPSN
		||   paf->location == APPLY_RECURRINGSPELL )
		  paf->modifier	= slot_lookup( paf->modifier );
		paf->affLocator	= fread_number( fp );

		paf->bitvector	= fread_bitvector( fp );
		LINK(paf, ch->first_affect, ch->last_affect, next, prev );
		fMatch = TRUE;
		break;
	    }


	    if ( !strcmp( word, "AttrMod"  ) )
	    {
		line = fread_line( fp );
		x1=x2=x3=x4=x5=13;
		sscanf( line, "%d %d %d %d %d",
		      &x1, &x2, &x3, &x4, &x5 );
		ch->mod_str = x1;
		ch->mod_int = x2;
		ch->mod_dex = x3;
		ch->mod_con = x4;
		ch->mod_lck = x5;
		if (!x5)
		ch->mod_lck = 0;
		fMatch = TRUE;
		break;
	    }

	    if ( !strcmp( word, "AttrPerm" ) )
	    {
		line = fread_line( fp );
		x1=x2=x3=x4=x5=0;
		sscanf( line, "%d %d %d %d %d",
		      &x1, &x2, &x3, &x4, &x5 );
		ch->perm_str = x1;
		ch->perm_int = x2;
		ch->perm_dex = x3;
		ch->perm_con = x4;
		ch->perm_lck = x5;
		if (!x5)
		  ch->perm_lck = 0;
		fMatch = TRUE;
		break;
	    }

	    if ( !strcmp( word, "AttrAdd" ) )
            {
                line = fread_line( fp );
                x1=x2=x3=x4=x5=0;
                sscanf( line, "%d %d %d %d %d",
                      &x1, &x2, &x3, &x4, &x5 );
                ch->add_str = x1;
                ch->add_dex = x2;
                ch->add_int = x3;
                ch->add_con = x4;
                ch->add_lck = x5;
                if (!x5)
                  ch->add_lck = 0;
                fMatch = TRUE;
                break;
            }

	    if ( !strcmp( word, "AttrTrain" ) )
	    {
		line = fread_line( fp );
		x1=x2=x3=x4=0;
		sscanf( line, "%d %d %d %d",
		      &x1, &x2, &x3, &x4 );
		ch->pcdata->tStr = x1;
		ch->pcdata->tInt = x2;
		ch->pcdata->tSpd = x3;
		ch->pcdata->tCon = x4;
		fMatch = TRUE;
		break;
	    }

		KEY( "Auction_PL", ch->pcdata->auction_pl, fread_number_ld( fp ) );
	    KEY( "AuthedBy",	ch->pcdata->authed_by,	fread_string( fp ) );
	    break;

	case 'B':
	    KEY( "Bamfin",	ch->pcdata->bamfin,	fread_string_nohash( fp ) );
	    KEY( "Bamfout",	ch->pcdata->bamfout,	fread_string_nohash( fp ) );
            /* Read in board status */
            if (!str_cmp(word, "Boards" ))
            {
                int i,num = fread_number (fp); /* number of boards saved */
                char *boardname;

                for (; num ; num-- ) /* for each of the board saved */
                {
                    boardname = fread_word (fp);
                    i = board_lookup (boardname); /* find board number */

                    if (i == BOARD_NOTFOUND) /* Does board still exist ? */
                    {
                        sprintf (buf, "fread_char: %s had unknown board name: %s. Skipped.", ch->name, boardname);
                        log_string (buf);
                        fread_number (fp); /* read last_note and skip info */
                    }
                    else /* Save it */
                        ch->pcdata->last_note[i] = fread_number (fp);
                }	 /* for */
		        if( ch->pcdata->board == NULL )
    		    {
	    		    for (i = 0; i < MAX_BOARD; i++)
	        		{
						ch->pcdata->board = &boards[i];
					}
				ch->pcdata->board = &boards[0];
				}

                fMatch = TRUE;
            } /* Boards */

	    KEY( "Battery",	ch->battery, fread_number( fp ) );
            KEY( "BPromptCFG",      ch->pcdata->battlePromptConfig,  fread_number( fp ) );
            KEY( "Bkills",      ch->pcdata->bkills,   fread_number( fp ) );
            KEY( "Bounty",      ch->pcdata->bounty,   fread_number( fp ) );
            KEY( "B_timeleft",      ch->pcdata->b_timeleft, fread_number( fp ) );
            KEY( "BountyBy",      ch->pcdata->bounty_by, fread_string_nohash( fp ) );
            KEY( "Bowed",       ch->pcdata->bowed,   fread_number( fp ) );
            KEY( "Bestowments", ch->pcdata->bestowments, fread_string_nohash( fp ) );
	    KEY( "Bio",		ch->pcdata->bio,	fread_string( fp ) );
	    KEY( "Build",	ch->pcdata->build,	fread_number( fp ) );
	    KEY( "Bck_name",	ch->bck_name, fread_string( fp ) );
	    KEY( "Bck_pl",	ch->bck_pl, fread_number_ld( fp ) );
	    KEY( "Bck_race",	ch->bck_race, fread_number( fp ) );
	    break;

	case 'C':
	    if ( !strcmp( word, "Clan" ) )
	    {
		ch->pcdata->clan_name = fread_string( fp );

		if ( !preload
		&&   ch->pcdata->clan_name[0] != '\0'
		&& ( ch->pcdata->clan = get_clan( ch->pcdata->clan_name )) == NULL )
		{
		  sprintf( buf, "Warning: the organization %s no longer exists, and therefore you no longer\n\rbelong to that organization.\n\r",
		           ch->pcdata->clan_name );
		  send_to_char( buf, ch );
		  STRFREE( ch->pcdata->clan_name );
		  ch->pcdata->clan_name = STRALLOC( "" );
		}
		fMatch = TRUE;
		break;
	    }
	    KEY( "ClanRank",	ch->pcdata->clanRank,		fread_number( fp ) );
	    KEY( "ClanZeniDon",	ch->pcdata->clanZeniDonated,		fread_number_skill( fp ) );
	    KEY( "ClanZeniTax",	ch->pcdata->clanZeniClanTax,		fread_number_skill( fp ) );
	    KEY( "ClanItemsDon",	ch->pcdata->clanItemsDonated,		fread_number( fp ) );
	    KEY( "Class",	ch->class,		fread_number( fp ) );
	    KEY( "CombatFlags",	ch->pcdata->combatFlags,			fread_number( fp ) );
	    KEY( "Corespl",	ch->corespl,	fread_number_ld( fp ) );
	    if ( !str_cmp( word, "Cores" ) )
	    {
		int fm_core = fread_number(fp);
		int e_core = fread_number(fp);
		int h_core = fread_number(fp);

		if( fm_core == 1 )
		  ch->fm_core = TRUE;
		else
		  ch->fm_core = FALSE;

		if( e_core == 1 )
                  ch->e_core = TRUE;
                else
                  ch->e_core = FALSE;

		if( h_core == 1 )
                  ch->h_core = TRUE;
                else
                  ch->h_core = FALSE;

		fMatch = TRUE;
		break;
	    }
	    KEY( "Creation_date",	ch->pcdata->creation_date,		fread_number( fp ) );

	    if ( !str_cmp( word, "C_date_word" ) )
		{
		fread_string_nohash( fp );
		fMatch = TRUE;
		break;
	    }

	    if ( !str_cmp( word, "Color" ) )
	    {
		char *cword;
		int at;

		cword = fread_word(fp);
		for (at = 0; at < AT_MAXCOLOR; ++at)
		  if (!str_cmp(cword, at_color_table[at].name))
		    break;
		if (at < AT_MAXCOLOR)
		  ch->pcdata->colorize[at] = fread_number(fp);
		else
		{
		  bug("Fread_char: color %s invalid.", cword);
		  fread_number(fp);
		}
		fMatch = TRUE;
		break;
	    }

	    if ( !str_cmp( word, "Condition" ) )
	    {
		line = fread_line( fp );
		sscanf( line, "%d %d %d %d",
		      &x1, &x2, &x3, &x4 );
		ch->pcdata->condition[0] = x1;
		ch->pcdata->condition[1] = x2;
		ch->pcdata->condition[2] = x3;
		ch->pcdata->condition[3] = x4;
		fMatch = TRUE;
		break;
	    }

	    if ( !strcmp( word, "Council" ) )
	    {
		ch->pcdata->council_name = fread_string( fp );
		if ( !preload
		&&   ch->pcdata->council_name[0] != '\0'
		&& ( ch->pcdata->council = get_council( ch->pcdata->council_name )) == NULL )
		{
		  sprintf( buf, "Warning: the council %s no longer exists, and herefore you no longer\n\rbelong to a council.\n\r",
		           ch->pcdata->council_name );
		  send_to_char( buf, ch );
		  STRFREE( ch->pcdata->council_name );
		  ch->pcdata->council_name = STRALLOC( "" );
		}
		fMatch = TRUE;
		break;
	    }
	    KEY( "Complexion",	ch->pcdata->complexion,	fread_number( fp ) );
	    break;

	case 'D':
	    KEY( "Damroll",	ch->damroll,		fread_number( fp ) );
	    KEY( "Deaf",	ch->deaf,		fread_bitvector( fp ) );
	    if ( !strcmp( word, "Deity" ) )
            {
                ch->pcdata->deity_name = fread_string( fp );

                if ( !preload
                &&   ch->pcdata->deity_name[0] != '\0'
                && ( ch->pcdata->deity = get_deity( ch->pcdata->deity_name )) == NULL )
                {
                  sprintf( buf, "Warning: the deity %s no longer exists.\n\r",
                           ch->pcdata->deity_name );
                  send_to_char( buf, ch );
                  STRFREE( ch->pcdata->deity_name );
                  ch->pcdata->deity_name = STRALLOC( "" );
		  ch->pcdata->favor = 0;
                }
                fMatch = TRUE;
                break;
            }
	    KEY( "Demonrank", ch->demonrank,	fread_number( fp ) );
	    KEY( "Description",	ch->description,	fread_string( fp ) );
	    KEY( "Description1",ch->pcdata->description1,	fread_string( fp ) );
	    KEY( "Description2",ch->pcdata->description2,	fread_string( fp ) );
	    KEY( "Description3",ch->pcdata->description3,	fread_string( fp ) );
	    KEY( "Description4",ch->pcdata->description4,	fread_string( fp ) );
	    KEY( "Description5",ch->pcdata->description5,	fread_string( fp ) );
	    break;

	/* 'E' was moved to after 'S' */
        case 'F':
	    KEY( "Favor",	ch->pcdata->favor,	fread_number( fp ) );
	    if ( !strcmp( word, "Filename" ) )
	    {
		/*
		 * File Name already set externally.
		 */
		fread_to_eol( fp );
		fMatch = TRUE;
		break;
	    }
	    KEY( "Flags",	ch->pcdata->flags,	fread_number( fp ) );
	    KEY( "FPrompt",	ch->pcdata->fprompt,	fread_string( fp ) );
            KEY( "Fusionflags", ch->fusionflags, fread_number( fp ) );
            KEY( "Fusions",     ch->fusions, fread_number( fp ) );
            KEY( "Fusiontimer", ch->fusiontimer, fread_number( fp ) );
            if ( !strcmp( word, "Fused" ) )
            {
                fMatch = TRUE;
                x1 = fread_number(fp);
                ch->fused[x1] = fread_string(fp);
                break;
            }
            break;

	case 'G':
	    KEY( "Ghost",	ch->pcdata->ghost_level,	fread_number( fp ) );
	    KEY( "Glory",       ch->pcdata->quest_curr, fread_number( fp ) );
	    KEY( "Gold",	ch->gold,		fread_number( fp ) );
	    KEY( "Gohome",	ch->pcdata->gohometimer,	fread_number( fp ) );
	    /* temporary measure */
	    if ( !strcmp( word, "Guild" ) )
	    {
		ch->pcdata->clan_name = fread_string( fp );

		if ( !preload
		&&   ch->pcdata->clan_name[0] != '\0'
		&& ( ch->pcdata->clan = get_clan( ch->pcdata->clan_name )) == NULL )
		{
		  sprintf( buf, "Warning: the organization %s no longer exists, and therefore you no longer\n\rbelong to that organization.\n\r",
		           ch->pcdata->clan_name );
		  send_to_char( buf, ch );
		  STRFREE( ch->pcdata->clan_name );
		  ch->pcdata->clan_name = STRALLOC( "" );
		}
		fMatch = TRUE;
		break;
	    }
            break;

	case 'H':
		KEY( "HeartPL", ch->heart_pl, fread_number_ld( fp ) );
            KEY( "Height",        ch->height,   fread_number( fp ) );
            KEY( "HBTCLeft",      ch->pcdata->HBTCTimeLeft, fread_number( fp ) );

	    if ( !strcmp(word, "Helled") )
	    {
	      ch->pcdata->release_date = fread_number(fp);
	      ch->pcdata->helled_by = fread_string(fp);
	      fMatch = TRUE;
	      break;
	    }

	    KEY( "Hitroll",	ch->hitroll,		fread_number( fp ) );
	    KEY( "Homepage",	ch->pcdata->homepage,	fread_string_nohash( fp ) );

	    if ( !strcmp( word, "HpManaMove" ) )
	    {
		ch->hit		= fread_number( fp );
		ch->max_hit	= fread_number( fp );
		ch->mana	= fread_number( fp );
		ch->max_mana	= fread_number( fp );
		ch->move	= fread_number( fp );
		ch->max_move	= fread_number( fp );
		ch->mod 	= 1;
		fMatch = TRUE;
		break;
	    }
	    KEY( "Haircolor",	ch->pcdata->haircolor,	fread_number( fp ) );
	    KEY( "Hairstyle",	ch->pcdata->hairstyle,	fread_number( fp ) );
	    KEY( "Hairlen",	ch->pcdata->hairlen,	fread_number( fp ) );
        KEY( "Hunting",      ch->pcdata->hunting,             fread_string_nohash( fp ) );
	    break;

	case 'I':
		KEY( "ICQ",	ch->pcdata->icq,	fread_number( fp ) );
	    if(!strcmp(word, "Ignored"))
	    {
	    	char *temp;
	    	char fname[1024];
	    	struct stat fst;
	    	int ign;
	    	IGNORE_DATA *inode;

	    	/* Get the name */
	    	temp = fread_string(fp);

	    	sprintf(fname, "%s%c/%s", PLAYER_DIR,
	    		tolower(temp[0]), capitalize(temp));

	    	/* If there isn't a pfile for the name */
	    	/* then don't add it to the list       */
	    	if(stat(fname, &fst) == -1)
	    	{
	    		fMatch = TRUE;
	    		break;
	    	}

	    	/* Count the number of names already ignored */
	    	for(ign = 0, inode = ch->pcdata->first_ignored; inode;
	    				inode = inode->next)
	    	{
	    		ign++;
	    	}

	    	/* Add the name unless the limit has been reached */
	    	if(ign >= MAX_IGN)
	    	{
	    		bug("fread_char: too many ignored names");
	    	}
	    	else
	    	{
	    		/* Add the name to the list */
	    		CREATE(inode, IGNORE_DATA, 1);
	    		inode->name = STRALLOC(temp);
	    		inode->next = NULL;
	    		inode->prev = NULL;

	    		LINK(inode, ch->pcdata->first_ignored,
	    			ch->pcdata->last_ignored, next,
	    			prev);
	    	}

	    	fMatch = TRUE;
	    	break;
	    }
	    KEY( "IllegalPK",	ch->pcdata->illegal_pk,	fread_number( fp ) );
	    KEY( "Immune",	ch->immune,		fread_number( fp ) );
	    KEY( "Incog",	ch->pcdata->incog_level,	fread_number( fp ) );
#ifdef IMC
           if( ( fMatch = imc_loadchar( ch, fp, word ) ) )
                break;
#endif
	    break;

	case 'K':
	    KEY( "Kairank",	ch->kairank,	fread_number( fp ) );
	    KEY( "KaiTimer",	ch->pcdata->eKTimer,	fread_number( fp ) );
	    KEY( "KaiMessage",	ch->pcdata->kaiRestoreMsg,	fread_string_nohash( fp ) );
	    if ( !strcmp( word, "Killed" ) )
	    {
		fMatch = TRUE;
		if ( killcnt >= MAX_KILLTRACK )
		    bug( "fread_char: killcnt (%d) >= MAX_KILLTRACK", killcnt );
		else
		{
		    ch->pcdata->killed[killcnt].vnum    = fread_number( fp );
		    ch->pcdata->killed[killcnt++].count = fread_number( fp );
		}
	    }
	    break;

	case 'L':

	    KEY( "LastOn",	ch->pcdata->lastlogon,		fread_number( fp ) );
	    if ( !strcmp( word, "LastIntrest" ) )
	    {
		line = fread_line( fp );
		x1=x2=0;
		sscanf( line, "%d %d", &x1, &x2 );
		ch->pcdata->interestLastMonth = x1;
		ch->pcdata->interestLastYear = x2;
		fMatch = TRUE;
		break;
		}

	    if ( !strcmp( word, "Last_Name" ) )
	    {
		ch->pcdata->last_name = fread_string_nohash( fp );
		{
		    sprintf( buf, " %s", ch->pcdata->last_name );
		    if ( ch->pcdata->last_name )
		    {
				DISPOSE(ch->pcdata->last_name);
				ch->pcdata->last_name = str_dup( "" );
		    }
		ch->pcdata->last_name = strdup(buf);
		}
		fMatch = TRUE;
		break;
	    }
	    KEY( "Level",	ch->level,		fread_number( fp ) );
	    KEY( "LongDescr",	ch->long_descr,		fread_string( fp ) );
	    if ( !strcmp( word, "Languages" ) )
	    {
	    	ch->speaks = fread_number( fp );
	    	ch->speaking = fread_number( fp );
	    	fMatch = TRUE;
	    }
            if( !strcmp( "LastTax", word ) )
            {
	      ch->pcdata->lastTaxation = fread_number(fp);
	      fMatch = TRUE;
            }
	    break;

	case 'M':
		KEY( "MaxEnergy", ch->max_energy, fread_number( fp ) );
		KEY( "MaxPrac", ch->max_prac, fread_number( fp ) );
		KEY( "MaxTrain", ch->max_train, fread_number( fp ) );
	    KEY( "MDeaths",	ch->pcdata->mdeaths,	fread_number( fp ) );
	    KEY( "Mentalstate", ch->mental_state,	fread_number( fp ) );
	    KEY( "MGlory",      ch->pcdata->quest_accum,fread_number( fp ) );
	    KEY( "Minsnoop",	ch->pcdata->min_snoop,	fread_number( fp ) );
	    KEY( "MKills",	ch->pcdata->mkills,	fread_number( fp ) );
	    KEY( "Mobinvis",	ch->mobinvis,		fread_number( fp ) );
	    if ( !strcmp( word, "MobRange" ) )
	    {
		ch->pcdata->m_range_lo = fread_number( fp );
		ch->pcdata->m_range_hi = fread_number( fp );
		fMatch = TRUE;
	    }
	    break;

	case 'N':
	    KEY( "NPromptCFG",	ch->pcdata->normalPromptConfig,		fread_number( fp ) );
	    KEY( "NaturalAC",	ch->pcdata->natural_ac_max,		fread_number( fp ) );
	    KEY ("Name", ch->name, fread_string( fp ) );
            KEY ("NoAffectedBy", ch->no_affected_by, fread_bitvector( fp ) );
	    KEY( "Nognote",	ch->pcdata->gnote_date,			fread_number( fp ) );
            KEY ("NoImmune", ch->no_immune, fread_number( fp ) );
            KEY ("NoResistant", ch->no_resistant, fread_number( fp ) );
            KEY ("NoSusceptible", ch->no_susceptible, fread_number( fp ) );
	    if ( !strcmp ( "Nuisance", word ) )
            {
                fMatch = TRUE;
                CREATE( ch->pcdata->nuisance, NUISANCE_DATA, 1 );
                ch->pcdata->nuisance->time = fread_number( fp );
                ch->pcdata->nuisance->max_time = fread_number( fp );
                ch->pcdata->nuisance->flags = fread_number( fp );
		ch->pcdata->nuisance->power = 1;
            }
	    if ( !strcmp ( "NuisanceNew", word ) )
	    {
		fMatch = TRUE;
		CREATE( ch->pcdata->nuisance, NUISANCE_DATA, 1 );
		ch->pcdata->nuisance->time = fread_number( fp );
		ch->pcdata->nuisance->max_time = fread_number( fp );
		ch->pcdata->nuisance->flags = fread_number( fp );
		ch->pcdata->nuisance->power = fread_number( fp );
	    }

            if( !strcmp( "NextHBTC", word ) )
            {
	      ch->pcdata->nextHBTCDate = fread_number(fp);
	      fMatch = TRUE;
            }
			if( !strcmp( "Nextspar", word ) )
            {
	      ch->pcdata->nextspartime = fread_number(fp);
	      fMatch = TRUE;
            }

	    break;
	case 'O':
	    KEY( "Orignaleyes",	ch->pcdata->orignaleyes,	fread_number( fp ) );
	    KEY( "Orignalhaircolor",	ch->pcdata->orignalhaircolor,	fread_number( fp ) );
	    KEY( "Outcast_time", ch->pcdata->outcast_time, fread_number( fp ) );
	    if ( !strcmp( word, "ObjRange" ) )
	    {
		ch->pcdata->o_range_lo = fread_number( fp );
		ch->pcdata->o_range_hi = fread_number( fp );
		fMatch = TRUE;
	    }
	    break;

	case 'P':
	    KEY( "Pagerlen",	ch->pcdata->pagerlen,	fread_number( fp ) );
	    KEY( "Password",	ch->pcdata->pwd,	fread_string_nohash( fp ) );
	    if ( !strcmp( word, "PermTStats" ) )
	    {
		line = fread_line( fp );
		x1=x2=x3=x4=0;
		sscanf( line, "%d %d %d %d",
		      &x1, &x2, &x3, &x4 );
		ch->pcdata->permTstr = x1;
		ch->pcdata->permTint = x2;
		ch->pcdata->permTspd = x3;
		ch->pcdata->permTcon = x4;
		fMatch = TRUE;
		break;
	    }
	    KEY( "PDeaths",	ch->pcdata->pdeaths,	fread_number( fp ) );
	    KEY( "PK_timer",	ch->pcdata->pk_timer,	fread_number( fp ) );
	    KEY( "PKills",	ch->pcdata->pkills,	fread_number( fp ) );
		KEY( "PL", ch->pl, fread_number_ld( fp ) );
	    KEY( "Played",	ch->played,		fread_number( fp ) );
	    /* KEY( "Position",	ch->position,		fread_number( fp ) );*/
            /*
             *  new positions are stored in the file from 100 up
             *  old positions are from 0 up
             *  if reading an old position, some translation is necessary
             */
            if (!strcmp ( word, "Position" ) )
            {
               ch->position          = fread_number( fp );
               if(ch->position<100){
                  switch(ch->position){
                      default: ;
                      case 0: ;
                      case 1: ;
                      case 2: ;
                      case 3: ;
                      case 4: break;
                      case 5: ch->position=6; break;
                      case 6: ch->position=8; break;
                      case 7: ch->position=9; break;
                      case 8: ch->position=12; break;
                      case 9: ch->position=13; break;
                      case 10: ch->position=14; break;
                      case 11: ch->position=15; break;
                  }
                  fMatch = TRUE;
               } else {
                  ch->position-=100;
                  fMatch = TRUE;
               }
            }
	    KEY( "Practice",	ch->practice,		fread_number( fp ) );
	    KEY( "Powerup",	ch->powerup,		fread_number( fp ) );
	    KEY( "Pretitle",		ch->pcdata->pretitle,	fread_string( fp ) );
	    KEY( "Prompt",	ch->pcdata->prompt,	fread_string( fp ) );
	    if (!strcmp ( word, "PTimer" ) )
	    {
		add_timer( ch , TIMER_PKILLED, fread_number(fp), NULL, 0 );
		fMatch = TRUE;
		break;
	    }
	    break;

	case 'R':
	    KEY( "Race",        ch->race,		fread_number( fp ) );
	    KEY( "Rage",        ch->rage,		fread_number( fp ) );
	    KEY( "Rank",        ch->pcdata->rank,	fread_string_nohash( fp ) );
	    KEY( "Resistant",	ch->resistant,		fread_number( fp ) );
	    KEY( "Restore_time",ch->pcdata->restore_time, fread_number( fp ) );

	    if ( !strcmp( word, "Room" ) )
	    {
		ch->in_room = get_room_index( fread_number( fp ) );
		if ( !ch->in_room )
		    ch->in_room = get_room_index( ROOM_VNUM_LIMBO );
		fMatch = TRUE;
		break;
	    }
	    if ( !strcmp( word, "RoomRange" ) )
	    {
		ch->pcdata->r_range_lo = fread_number( fp );
		ch->pcdata->r_range_hi = fread_number( fp );
		fMatch = TRUE;
	    }
	    break;

	case 'S':
	    KEY( "SD_Charge",		ch->pcdata->sd_charge,		fread_number( fp ) );
	    KEY( "Secondarycolor",		ch->pcdata->secondarycolor,		fread_number( fp ) );
	    KEY( "Sex",		ch->sex,		fread_number( fp ) );
	    KEY( "Silence",	ch->pcdata->silence,	fread_number( fp ) );
	    KEY( "Silencedby",	ch->pcdata->silencedby,	fread_string( fp ) );
	    KEY( "ShortDescr",	ch->short_descr,	fread_string( fp ) );
		KEY( "Sparcount",   ch->pcdata->sparcount, fread_number( fp ) );
        KEY( "Spouse",      ch->pcdata->spouse,             fread_string_nohash( fp ) );
	    KEY( "Style",	ch->style,		fread_number( fp ) );
	    KEY( "Suppress",	ch->pcdata->suppress,	fread_number_ld( fp ) );
	    KEY( "Susceptible",	ch->susceptible,	fread_number( fp ) );
	    if ( !strcmp( word, "SavingThrow" ) )
	    {
		ch->saving_wand 	= fread_number( fp );
		ch->saving_poison_death = ch->saving_wand;
		ch->saving_para_petri 	= ch->saving_wand;
		ch->saving_breath 	= ch->saving_wand;
		ch->saving_spell_staff 	= ch->saving_wand;
		fMatch = TRUE;
		break;
	    }

	    if ( !strcmp( word, "SavingThrows" ) )
	    {
		ch->saving_poison_death = fread_number( fp );
		ch->saving_wand 	= fread_number( fp );
		ch->saving_para_petri 	= fread_number( fp );
		ch->saving_breath 	= fread_number( fp );
		ch->saving_spell_staff 	= fread_number( fp );
		fMatch = TRUE;
		break;
	    }

	    if ( !strcmp( word, "Site" ) )
	    {
		if ( !preload )
		{
		  ch->pcdata->lasthost = STRALLOC( fread_word(fp) );
		  sprintf( buf, "Last connected from: %s\n\r", ch->pcdata->lasthost );
		  send_to_char( buf, ch );
		  if (ch->desc && ch->desc->host)
		  	ch->pcdata->lasthost = STRALLOC( ch->desc->host );
		}
		else
		  fread_to_eol( fp );
		fMatch = TRUE;
		if ( preload )
		  word = "End";
		else
		  break;
	    }

	    if ( !strcmp( word, "Skill" ) )
	    {
		int sn;
		double value;

		if ( preload )
		  word = "End";
		else
		{
		  value = fread_number_skill( fp );
		  if ( file_ver < 3 )
		    sn = skill_lookup( fread_word( fp ) );
		  else
		    sn = bsearch_skill_exact( fread_word( fp ), gsn_first_skill, gsn_first_ability-1 );
		  if ( sn < 0 )
		    bug( "Fread_char: unknown skill.", 0 );
		  else
		  {
		    ch->pcdata->learned[sn] = value;
		    /* Take care of people who have stuff they shouldn't     *
		     * Assumes class and level were loaded before. -- Altrag *
		     * Assumes practices are loaded first too now. -- Altrag */
	      if ( skill_table[sn]->skill_level[ch->class] <= 0 )
		      {
		        ch->pcdata->learned[sn] = 0;
		        ch->practice++;
		      }
		  }
		  fMatch = TRUE;
		  break;
		}
	    }

	    KEY( "SparLoss",	ch->pcdata->spar_loss,	fread_number( fp ) );
	    KEY( "SparWins",	ch->pcdata->spar_wins,	fread_number( fp ) );
	    if ( !strcmp( word, "Spell" ) )
	    {
		int sn;
		int value;

		if ( preload )
		  word = "End";
		else
		{
		  value = fread_number( fp );

		  sn = bsearch_skill_exact( fread_word( fp ), gsn_first_spell, gsn_first_skill-1 );
		  if ( sn < 0 )
		    bug( "Fread_char: unknown spell.", 0 );
		  else
		  {
		    ch->pcdata->learned[sn] = value;
		    if ( ch->level < LEVEL_IMMORTAL )
			if ( skill_table[sn]->skill_level[ch->class] >= LEVEL_IMMORTAL )
			{
			    ch->pcdata->learned[sn] = 0;
		        ch->practice++;
			}
		  }
		  fMatch = TRUE;
		  break;
		}
	    }
	    if ( strcmp( word, "End" ) )
		break;

	case 'E':
	    if ( !strcmp( word, "End" ) )
	    {
		ch->logon_start = ch->exp;
		if (!ch->short_descr)
		  ch->short_descr	= STRALLOC( "" );
		if (!ch->long_descr)
		  ch->long_descr	= STRALLOC( "" );
		if (!ch->description)
		  ch->description	= STRALLOC( "" );
		if (!ch->pcdata->description1)
		  ch->pcdata->description1	= STRALLOC( "" );
		if (!ch->pcdata->description2)
		  ch->pcdata->description2	= STRALLOC( "" );
		if (!ch->pcdata->description3)
		  ch->pcdata->description3	= STRALLOC( "" );
		if (!ch->pcdata->description4)
		  ch->pcdata->description4	= STRALLOC( "" );
		if (!ch->pcdata->description5)
		  ch->pcdata->description5	= STRALLOC( "" );
		if (!ch->pcdata->pwd)
		  ch->pcdata->pwd	= str_dup( "" );
		if (!ch->pcdata->bamfin)
		  ch->pcdata->bamfin	= str_dup( "" );
		if (!ch->pcdata->bamfout)
		  ch->pcdata->bamfout	= str_dup( "" );
		if (!ch->pcdata->bounty_by)
		  ch->pcdata->bounty_by	= str_dup( "" );
		if (!ch->pcdata->hunting)
		  ch->pcdata->hunting	= str_dup( "" );
		if (!ch->pcdata->last_name)
		  ch->pcdata->last_name	= str_dup( "" );
		if (!ch->pcdata->spouse)
		  ch->pcdata->spouse	= str_dup( "" );
		if (!ch->pcdata->bio)
		  ch->pcdata->bio	= STRALLOC( "" );
		if (!ch->pcdata->rank)
		  ch->pcdata->rank	= str_dup( "" );
		if (!ch->pcdata->bestowments)
		  ch->pcdata->bestowments = str_dup( "" );
		if (!ch->pcdata->pretitle)
		  ch->pcdata->pretitle	= STRALLOC( "" );
		if (!ch->pcdata->title)
		  ch->pcdata->title	= STRALLOC( "" );
		if (!ch->pcdata->homepage)
		  ch->pcdata->homepage	= str_dup( "" );
        if (!ch->pcdata->email)
		  ch->pcdata->email = str_dup( "" );
		if (!ch->pcdata->authed_by)
		  ch->pcdata->authed_by = STRALLOC( "" );
		if (!ch->pcdata->prompt )
		  ch->pcdata->prompt	= STRALLOC( "" );
	        if (!ch->pcdata->fprompt )
		  ch->pcdata->fprompt   = STRALLOC( "" );
		ch->editor		= NULL;
		killcnt = URANGE( 2, ((ch->level+3) * MAX_KILLTRACK)/LEVEL_AVATAR, MAX_KILLTRACK );
		if ( killcnt < MAX_KILLTRACK )
		  ch->pcdata->killed[killcnt].vnum = 0;

		/* no good for newbies at all */
		if ( !IS_IMMORTAL( ch ) && !ch->speaking )
			ch->speaking = LANG_COMMON;
		/*	ch->speaking = race_table[ch->race]->language; */
		if ( IS_IMMORTAL( ch ) )
		{
			int i;

			ch->speaks = ~0;
			if ( ch->speaking == 0 )
				ch->speaking = ~0;

			CREATE(ch->pcdata->tell_history, char *, 26);
			for(i = 0; i < 26; i++)
				ch->pcdata->tell_history[i] = NULL;
		}
		if ( !ch->pcdata->prompt )
		  ch->pcdata->prompt = STRALLOC("");

		/* reapply stat bonuses */
		fixTransStatAffects(ch);

		return;
	    }
	    KEY( "Email",	ch->pcdata->email,	fread_string_nohash( fp ) );
	    KEY( "Evilmod",	ch->evilmod,		fread_number( fp ) );
	    KEY( "Exp",		ch->exp,		fread_number_ld( fp ) );
	    KEY( "Eyes",	ch->pcdata->eyes,	fread_number( fp ) );
	    break;

	case 'T':
	    KEY( "Tail",	ch->pcdata->tail,	fread_number( fp ) );
	    if ( !strcmp( word, "Tongue" ) )
	    {
		int sn;
		int value;

		if ( preload )
		  word = "End";
		else
		{
		  value = fread_number( fp );

		  sn = bsearch_skill_exact( fread_word( fp ), gsn_first_tongue, gsn_top_sn-1 );
		  if ( sn < 0 )
		    bug( "Fread_char: unknown tongue.", 0 );
		  else
		  {
		    ch->pcdata->learned[sn] = value;
		    if ( ch->level < LEVEL_IMMORTAL )
			if ( skill_table[sn]->skill_level[ch->class] >= LEVEL_IMMORTAL )
			{
			    ch->pcdata->learned[sn] = 0;
		        ch->practice++;
			}
		  }
		  fMatch = TRUE;
		}
		break;
	    }
		KEY( "Train", ch->train, fread_number( fp ) );
	    KEY( "Trust", ch->trust, fread_number( fp ) );
            /* Let no character be trusted higher than one below maxlevel -- Narn */
	    ch->trust = UMIN( ch->trust, MAX_LEVEL - 1 );

	    KEY( "TotalXTrain", ch->pcdata->total_xTrain, fread_number( fp ) );

	    if ( !strcmp( word, "Title" ) )
	    {
		ch->pcdata->title = fread_string( fp );
		if ( isalpha(ch->pcdata->title[0])
		||   isdigit(ch->pcdata->title[0]) )
		{
		    sprintf( buf, " %s", ch->pcdata->title );
		    if ( ch->pcdata->title )
		      STRFREE( ch->pcdata->title );
		    ch->pcdata->title = STRALLOC( buf );
		}
		fMatch = TRUE;
		break;
	    }

	    break;

	case 'U':
		KEY( "UpgradeL",	ch->pcdata->upgradeL,	fread_number( fp ) );
		break;

	case 'V':
	    if ( !strcmp( word, "Vnum" ) )
	    {
		ch->pIndexData = get_mob_index( fread_number( fp ) );
		fMatch = TRUE;
		break;
	    }
            if( !str_cmp( word, "Version" ) )
            {
               file_ver = fread_number( fp );
               ch->pcdata->version = file_ver;
               fMatch = TRUE;
               break;
            }
	    break;

	case 'W':
            KEY( "Weight",        ch->weight,   fread_number( fp ) );
	    if ( !strcmp( word, "Weapon" ) )
	    {
		int sn;
		int value;

		if ( preload )
		  word = "End";
		else
		{
		  value = fread_number( fp );

		  sn = bsearch_skill_exact( fread_word( fp ), gsn_first_weapon, gsn_first_tongue-1 );
		  if ( sn < 0 )
		    bug( "Fread_char: unknown weapon.", 0 );
		  else
		  {
		    ch->pcdata->learned[sn] = value;
		    if ( ch->level < LEVEL_IMMORTAL )
			if ( skill_table[sn]->skill_level[ch->class] >= LEVEL_IMMORTAL )
			{
			    ch->pcdata->learned[sn] = 0;
		        ch->practice++;
			}
		  }
		  fMatch = TRUE;
		}
		break;
	    }
	    KEY( "Wimpy",	ch->wimpy,		fread_number( fp ) );
	    KEY( "WizInvis",	ch->pcdata->wizinvis,	fread_number( fp ) );
	    break;
	case 'X':
	    KEY( "XTrain",	ch->pcdata->xTrain,	fread_number( fp ) );
	    break;

	case 'Z':
	    KEY( "Zeni",	ch->gold,		fread_number( fp ) );
	}

	if ( !fMatch )
	{
	    sprintf( buf, "Fread_char: no match: %s", word );
	    bug( buf, 0 );
	}
    }
}


void fread_obj( CHAR_DATA *ch, FILE *fp, sh_int os_type )
{
    OBJ_DATA *obj;
    char *word;
    char buf[MAX_STRING_LENGTH];
    int iNest;
    bool fMatch;
    bool fNest;
    bool fVnum;
    ROOM_INDEX_DATA *room = NULL;

    if ( ch )
	room = ch->in_room;
    CREATE( obj, OBJ_DATA, 1 );
    obj->count		= 1;
    obj->wear_loc	= -1;
    obj->weight		= 1;

    fNest		= TRUE;		/* Requiring a Nest 0 is a waste */
    fVnum		= TRUE;
    iNest		= 0;

    for ( ; ; )
    {
	word   = feof( fp ) ? "End" : fread_word( fp );
	fMatch = FALSE;

	switch ( UPPER(word[0]) )
	{
	case '*':
	    fMatch = TRUE;
	    fread_to_eol( fp );
	    break;

	case 'A':
	    KEY( "ActionDesc", obj->action_desc, 	fread_string( fp ) );
	    if ( !strcmp( word, "Affect" ) || !strcmp( word, "AffectData" ) )
	    {
		AFFECT_DATA *paf;
		int pafmod;

		CREATE( paf, AFFECT_DATA, 1 );
		if ( !strcmp( word, "Affect" ) )
		{
		    paf->type	= fread_number( fp );
		}
		else
		{
		    int sn;

		    sn = skill_lookup( fread_word( fp ) );
		    if ( sn < 0 )
			bug( "Fread_obj: unknown skill.", 0 );
		    else
			paf->type = sn;
		}
		paf->duration	= fread_number( fp );
		pafmod		= fread_number( fp );
		paf->location	= fread_number( fp );
		paf->bitvector	= fread_bitvector( fp );
		if ( paf->location == APPLY_WEAPONSPELL
		||   paf->location == APPLY_WEARSPELL
		||   paf->location == APPLY_STRIPSN
		||   paf->location == APPLY_REMOVESPELL
		||   paf->location == APPLY_RECURRINGSPELL )
		  paf->modifier		= slot_lookup( pafmod );
		else
		  paf->modifier		= pafmod;
		LINK(paf, obj->first_affect, obj->last_affect, next, prev );
		fMatch				= TRUE;
		break;
	    }
	    break;

	case 'C':
	    KEY( "Cost",	obj->cost,		fread_number( fp ) );
	    KEY( "Count",	obj->count,		fread_number( fp ) );
	    break;

	case 'D':
	    KEY( "Description",	obj->description,	fread_string( fp ) );
	    break;

	case 'E':
	    KEY( "ExtraFlags",	obj->extra_flags,	fread_bitvector( fp ) );

	    if ( !strcmp( word, "ExtraDescr" ) )
	    {
		EXTRA_DESCR_DATA *ed;

		CREATE( ed, EXTRA_DESCR_DATA, 1 );
		ed->keyword		= fread_string( fp );
		ed->description		= fread_string( fp );
		LINK(ed, obj->first_extradesc, obj->last_extradesc, next, prev );
		fMatch 				= TRUE;
	    }

	    if ( !strcmp( word, "End" ) )
	    {
		if ( !fNest || !fVnum )
		{
		    if ( obj->name )
			sprintf ( buf, "Fread_obj: %s incomplete object.",
				obj->name );
		    else
			sprintf ( buf, "Fread_obj: incomplete object." );
		    bug( buf, 0 );
		    if ( obj->name )
		      STRFREE( obj->name        );
		    if ( obj->description )
		      STRFREE( obj->description );
		    if ( obj->short_descr )
		      STRFREE( obj->short_descr );
		    DISPOSE( obj );
		    return;
		}
		else
		{
		    sh_int wear_loc = obj->wear_loc;

		    if ( !obj->name )
			obj->name = QUICKLINK( obj->pIndexData->name );
		    if ( !obj->description )
			obj->description = QUICKLINK( obj->pIndexData->description );
		    if ( !obj->short_descr )
			obj->short_descr = QUICKLINK( obj->pIndexData->short_descr );
		    if ( !obj->action_desc )
			obj->action_desc = QUICKLINK( obj->pIndexData->action_desc );
		    LINK(obj, first_object, last_object, next, prev );
                    if ( ( !IS_OBJ_STAT( obj, ITEM_RARE )
                        && !IS_OBJ_STAT( obj, ITEM_UNIQUE ) )
                        ||  IS_NPC( ch ) )
                    {
		            obj->pIndexData->count += obj->count;
                    }
		    if ( !obj->serial )
		    {
			cur_obj_serial = UMAX((cur_obj_serial + 1 ) & (BV30-1), 1);
			obj->serial = obj->pIndexData->serial = cur_obj_serial;
		    }
		    if ( fNest )
		      rgObjNest[iNest] = obj;
		    numobjsloaded += obj->count;
		    ++physicalobjects;
		    if ( file_ver > 1 || obj->wear_loc < -1
		    ||   obj->wear_loc >= MAX_WEAR )
		      obj->wear_loc = -1;
		    /* Corpse saving. -- Altrag */
		    if ( os_type == OS_CORPSE )
		    {
		        if ( !room )
		        {
		          bug( "Fread_obj: Corpse without room", 0);
		          room = get_room_index(ROOM_VNUM_LIMBO);
		        }
			/* Give the corpse a timer if there isn't one */

			if ( obj->timer < 1 )
			   obj->timer = 40;
			if ( room->vnum == ROOM_VNUM_HALLOFFALLEN
				&& obj->first_content )
			   obj->timer = -1;
		        obj = obj_to_room( obj, room );
		    }
		    else if ( iNest == 0 || rgObjNest[iNest] == NULL )
		    {
			int slot = -1;
			bool reslot = FALSE;

			if ( file_ver > 1
			&&   wear_loc > -1
			&&   wear_loc < MAX_WEAR )
			{
			   int x;

			   for ( x = 0; x < MAX_LAYERS; x++ )
			      if ( !save_equipment[wear_loc][x] )
			      {
				  save_equipment[wear_loc][x] = obj;
				  slot = x;
				  reslot = TRUE;
				  break;
			      }
			   if ( x == MAX_LAYERS )
				bug( "Fread_obj: too many layers %d", wear_loc );
			}
			obj = obj_to_char( obj, ch );
			if ( reslot && slot != -1 )
			  save_equipment[wear_loc][slot] = obj;
		    }
		    else
		    {
			if ( rgObjNest[iNest-1] )
			{
			   separate_obj( rgObjNest[iNest-1] );
			   obj = obj_to_obj( obj, rgObjNest[iNest-1] );
			}
			else
			   bug( "Fread_obj: nest layer missing %d", iNest-1 );
		    }
		    if ( fNest )
		      rgObjNest[iNest] = obj;
		    return;
		}
	    }
	    break;

	case 'I':
	    KEY( "ItemType",	obj->item_type,		fread_number( fp ) );
	    break;

	case 'L':
	    KEY( "Level",	obj->level,		fread_number_ld( fp ) );
	    break;

	case 'N':
	    KEY( "Name",	obj->name,		fread_string( fp ) );

	    if ( !strcmp( word, "Nest" ) )
	    {
		iNest = fread_number( fp );
		if ( iNest < 0 || iNest >= MAX_NEST )
		{
		    bug( "Fread_obj: bad nest %d.", iNest );
		    iNest = 0;
		    fNest = FALSE;
		}
		fMatch = TRUE;
	    }
	    break;
	case 'O':
		KEY( "Origin",	obj->origin,		fread_string( fp ) );
	case 'R':
	    KEY( "Room", room, get_room_index(fread_number(fp)) );

	case 'S':
	    KEY( "ShortDescr",	obj->short_descr,	fread_string( fp ) );

	    if ( !strcmp( word, "Spell" ) )
	    {
		int iValue;
		int sn;

		iValue = fread_number( fp );
		sn     = skill_lookup( fread_word( fp ) );
		if ( iValue < 0 || iValue > 5 )
		    bug( "Fread_obj: bad iValue %d.", iValue );
		else if ( sn < 0 )
		    bug( "Fread_obj: unknown skill.", 0 );
		else
		    obj->value[iValue] = sn;
		fMatch = TRUE;
		break;
	    }

	    break;

	case 'T':
	    KEY( "Timer",	obj->timer,		fread_number( fp ) );
	    break;

	case 'V':
	    if ( !strcmp( word, "Values" ) )
	    {
		int x1,x2,x3,x4,x5,x6;
		char *ln = fread_line( fp );

		x1=x2=x3=x4=x5=x6=0;
		sscanf( ln, "%d %d %d %d %d %d", &x1, &x2, &x3, &x4, &x5, &x6 );
		/* clean up some garbage */
		if ( file_ver < 3 )
		   x5=x6=0;

		obj->value[0]	= x1;
		obj->value[1]	= x2;
		obj->value[2]	= x3;
		obj->value[3]	= x4;
		obj->value[4]	= x5;
		obj->value[5]	= x6;
		fMatch		= TRUE;
		break;
	    }

	    if ( !strcmp( word, "Vnum" ) )
	    {
		int vnum;

		vnum = fread_number( fp );
		/*  bug( "Fread_obj: bad vnum %d.", vnum );  */
		if ( ( obj->pIndexData = get_obj_index( vnum ) ) == NULL )
		{
			fVnum = FALSE;
			break;
		}
		else
		{
		    fVnum = TRUE;
		    obj->cost = obj->pIndexData->cost;
		    obj->weight = obj->pIndexData->weight;
		    obj->item_type = obj->pIndexData->item_type;
		    obj->wear_flags = obj->pIndexData->wear_flags;
		    obj->extra_flags = obj->pIndexData->extra_flags;
		}
		fMatch = TRUE;
		break;
	    }
	    break;

	case 'W':
	    KEY( "WearFlags",	obj->wear_flags,	fread_number( fp ) );
	    KEY( "WearLoc",	obj->wear_loc,		fread_number( fp ) );
	    KEY( "Weight",	obj->weight,		fread_number( fp ) );
	    break;

	}

	if ( !fMatch )
	{
	    EXTRA_DESCR_DATA *ed;
	    AFFECT_DATA *paf;

	    bug( "Fread_obj: no match.", 0 );
	    bug( word, 0 );
	    fread_to_eol( fp );
	    if ( obj->name )
		STRFREE( obj->name        );
	    if ( obj->description )
		STRFREE( obj->description );
	    if ( obj->short_descr )
		STRFREE( obj->short_descr );
	    while ( (ed=obj->first_extradesc) != NULL )
	    {
		STRFREE( ed->keyword );
		STRFREE( ed->description );
		UNLINK( ed, obj->first_extradesc, obj->last_extradesc, next, prev );
		DISPOSE( ed );
	    }
	    while ( (paf=obj->first_affect) != NULL )
	    {
		UNLINK( paf, obj->first_affect, obj->last_affect, next, prev );
		DISPOSE( paf );
	    }
	    DISPOSE( obj );
	    return;
	}
    }
}

void set_alarm( long seconds )
{
#ifdef WIN32
    kill_timer ();    /* kill old timer */
    timer_code = timeSetEvent(seconds * 1000L, 1000, alarm_handler, 0, TIME_PERIODIC);
#else
    alarm( seconds );
#endif
}

/*
 * Based on last time modified, show when a player was last on	-Thoric
 */
void do_last( CHAR_DATA *ch, char *argument )
{
    char buf [MAX_STRING_LENGTH];
    char arg [MAX_INPUT_LENGTH];
    char name[MAX_INPUT_LENGTH];
    struct stat fst;

    one_argument( argument, arg );
    if ( arg[0] == '\0' )
    {
	send_to_char( "Usage: last <playername>\n\r", ch );
	return;
    }
    strcpy( name, capitalize(arg) );
    sprintf( buf, "%s%c/%s", PLAYER_DIR, tolower(arg[0]), name );
    if ( stat( buf, &fst ) != -1 && check_parse_name( capitalize(name), FALSE ))
      sprintf( buf, "%s was last on: %s\r", name, ctime( &fst.st_mtime ) );
    else
      sprintf( buf, "%s was not found.\n\r", name );
   send_to_char( buf, ch );
}

/*
 * Added support for removeing so we could take out the write_corpses
 * so we could take it out of the save_char_obj function. --Shaddai
 */

void write_corpses( CHAR_DATA *ch, char *name, OBJ_DATA *objrem )
{
  OBJ_DATA *corpse;
  FILE *fp = NULL;

  /* Name and ch support so that we dont have to have a char to save their
     corpses.. (ie: decayed corpses while offline) */
  if ( ch && IS_NPC(ch) )
  {
    bug( "Write_corpses: writing NPC corpse.", 0 );
    return;
  }
  if ( ch )
    name = ch->name;
  /* Go by vnum, less chance of screwups. -- Altrag */
  for ( corpse = first_object; corpse; corpse = corpse->next )
    if ( corpse->pIndexData->vnum == OBJ_VNUM_CORPSE_PC &&
         corpse->in_room != NULL &&
        !str_cmp(corpse->short_descr+14, name) &&
	objrem != corpse )
    {
      if ( !fp )
      {
        char buf[127];

        sprintf(buf, "%s%s", CORPSE_DIR, capitalize(name));
        if ( !(fp = fopen(buf, "w")) )
        {
          bug( "Write_corpses: Cannot open file.", 0 );
          perror(buf);
          return;
        }
      }
      fwrite_obj(ch, corpse, fp, 0, OS_CORPSE);
    }
  if ( fp )
  {
    fprintf(fp, "#END\n\n");
    fclose(fp);
  }
  else
  {
    char buf[127];

    sprintf(buf, "%s%s", CORPSE_DIR, capitalize(name));
    remove(buf);
  }
  return;
}

void load_corpses( void )
{
  DIR *dp;
  struct dirent *de;
  extern FILE *fpArea;
  extern char strArea[MAX_INPUT_LENGTH];
  extern int falling;

  if ( !(dp = opendir(CORPSE_DIR)) )
  {
    bug( "Load_corpses: can't open CORPSE_DIR", 0);
    perror(CORPSE_DIR);
    return;
  }

  falling = 1; /* Arbitrary, must be >0 though. */
  while ( (de = readdir(dp)) != NULL )
  {
    if ( de->d_name[0] != '.' )
    {
      sprintf(strArea, "%s%s", CORPSE_DIR, de->d_name );
      fprintf(stderr, "Corpse -> %s\n", strArea);
      if ( !(fpArea = fopen(strArea, "r")) )
      {
        perror(strArea);
        continue;
      }
      for ( ; ; )
      {
        char letter;
        char *word;

        letter = fread_letter( fpArea );
        if ( letter == '*' )
        {
          fread_to_eol(fpArea);
          continue;
        }
        if ( letter != '#' )
        {
          bug( "Load_corpses: # not found.", 0 );
          break;
        }
        word = fread_word( fpArea );
        if ( !strcmp(word, "CORPSE" ) )
          fread_obj( NULL, fpArea, OS_CORPSE );
        else if ( !strcmp(word, "OBJECT" ) )
          fread_obj( NULL, fpArea, OS_CARRY );
        else if ( !strcmp( word, "END" ) )
          break;
        else
        {
          bug( "Load_corpses: bad section.", 0 );
          break;
        }
      }
      fclose(fpArea);
    }
  }
  fpArea = NULL;
  strcpy(strArea, "$");
  closedir(dp);
  falling = 0;
  return;
}

/*
 * This will write one mobile structure pointed to be fp --Shaddai
 */

void fwrite_mobile( FILE *fp, CHAR_DATA *mob )
{
  if ( !IS_NPC( mob ) || !fp )
	return;
  fprintf( fp, "#MOBILE\n" );
  fprintf( fp, "Vnum	%d\n", mob->pIndexData->vnum );
  if ( mob->in_room )
	fprintf( fp, "Room	%d\n",
       		(  mob->in_room == get_room_index( ROOM_VNUM_LIMBO )
        		&& mob->was_in_room )
            		? mob->was_in_room->vnum
            		: mob->in_room->vnum );
  if ( QUICKMATCH( mob->name, mob->pIndexData->player_name) == 0 )
        fprintf( fp, "Name     %s~\n", mob->name );
  if ( QUICKMATCH( mob->short_descr, mob->pIndexData->short_descr) == 0 )
  	fprintf( fp, "Short	%s~\n", mob->short_descr );
  if ( QUICKMATCH( mob->long_descr, mob->pIndexData->long_descr) == 0 )
  	fprintf( fp, "Long	%s~\n", mob->long_descr );
  if ( QUICKMATCH( mob->description, mob->pIndexData->description) == 0 )
  	fprintf( fp, "Description %s~\n", mob->description );
  fprintf( fp, "Position %d\n", mob->position );
  fprintf( fp, "Flags %s\n",   print_bitvector(&mob->act) );
/* Might need these later --Shaddai
  de_equip_char( mob );
  re_equip_char( mob );
  */
  if ( mob->first_carrying )
	fwrite_obj( mob, mob->last_carrying, fp, 0, OS_CARRY );
  fprintf( fp, "EndMobile\n" );
  return;
}

/*
 * This will read one mobile structure pointer to by fp --Shaddai
 */
CHAR_DATA *  fread_mobile( FILE *fp )
{
  CHAR_DATA *mob = NULL;
  char *word;
  bool fMatch;
  int inroom = 0;
  ROOM_INDEX_DATA *pRoomIndex = NULL;

  word   = feof( fp ) ? "EndMobile" : fread_word( fp );
  if ( !strcmp(word, "Vnum") )
  {
    int vnum;

    vnum = fread_number( fp );
    mob = create_mobile( get_mob_index(vnum) );
    if ( !mob )
    {
	for ( ; ; ) {
	  word   = feof( fp ) ? "EndMobile" : fread_word( fp );
	  /* So we don't get so many bug messages when something messes up
	   * --Shaddai
	   */
	  if ( !strcmp( word, "EndMobile" ) )
		break;
        }
	bug("Fread_mobile: No index data for vnum %d", vnum );
	return NULL;
    }
  }
  else
  {
	for ( ; ; ) {
	  word   = feof( fp ) ? "EndMobile" : fread_word( fp );
	  /* So we don't get so many bug messages when something messes up
	   * --Shaddai
	   */
	  if ( !strcmp( word, "EndMobile" ) )
		break;
        }
	extract_char(mob, TRUE);
	bug("Fread_mobile: Vnum not found", 0 );
	return NULL;
  }
  for ( ; ;) {
       word   = feof( fp ) ? "EndMobile" : fread_word( fp );
       fMatch = FALSE;
       switch ( UPPER(word[0]) ) {
	case '*':
           fMatch = TRUE;
           fread_to_eol( fp );
           break;
	case '#':
		if ( !strcmp( word, "#OBJECT" ) )
			fread_obj ( mob, fp, OS_CARRY );
	case 'D':
		KEY( "Description", mob->description, fread_string(fp));
		break;
	case 'E':
		if ( !strcmp( word, "EndMobile" ) )
		{
		if ( inroom == 0 )
			inroom = ROOM_VNUM_TEMPLE;
		pRoomIndex = get_room_index( inroom );
		if ( !pRoomIndex )
			pRoomIndex = get_room_index( ROOM_VNUM_TEMPLE );
		char_to_room(mob, pRoomIndex);
		return mob;
		}
		break;
 	case 'F':
		KEY( "Flags", mob->act, fread_bitvector(fp));
	case 'L':
		KEY( "Long", mob->long_descr, fread_string(fp ) );
		break;
	case 'N':
		KEY( "Name", mob->name, fread_string( fp ) );
		break;
	case 'P':
		KEY( "Position", mob->position, fread_number( fp ) );
		break;
	case 'R':
		KEY( "Room",  inroom, fread_number(fp));
		break;
	case 'S':
		KEY( "Short", mob->short_descr, fread_string(fp));
		break;
	}
	if ( !fMatch )
	{
	   bug ( "Fread_mobile: no match.", 0 );
	   bug ( word, 0 );
	}
  }
  return NULL;
}

/*
 * This will write in the saved mobile for a char --Shaddai
 */
void write_char_mobile( CHAR_DATA *ch , char *argument )
{
  FILE *fp;
  CHAR_DATA *mob;
  char buf[MAX_STRING_LENGTH];

  if ( IS_NPC( ch ) || !ch->pcdata->pet )
	return;

  fclose( fpReserve );
  if ( (fp = fopen( argument, "w")) == NULL )
  {
	sprintf(buf, "Write_char_mobile: couldn't open %s for writing!\n\r",
		argument );
	bug(buf, 0 );
	fpReserve = fopen( NULL_FILE, "r" );
	return;
  }
  mob = ch->pcdata->pet;
  xSET_BIT( mob->affected_by, AFF_CHARM );
  fwrite_mobile( fp, mob );
  fclose( fp );
  fpReserve = fopen( NULL_FILE, "r" );
  return;
}

/*
 * This will read in the saved mobile for a char --Shaddai
 */

void read_char_mobile( char *argument )
{
  FILE *fp;
  CHAR_DATA *mob;
  char buf[MAX_STRING_LENGTH];

  fclose( fpReserve );
  if ( (fp = fopen( argument, "r")) == NULL )
  {
	sprintf(buf, "Read_char_mobile: couldn't open %s for reading!\n\r",
		argument );
	bug(buf, 0 );
	fpReserve = fopen( NULL_FILE, "r" );
	return;
  }
  mob = fread_mobile( fp );
  fclose( fp );
  fpReserve = fopen( NULL_FILE, "r" );
  return;
}
