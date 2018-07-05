/****************************************************************************
 * [S]imulated [M]edieval [A]dventure multi[U]ser [G]ame      |   \\._.//   *
 * -----------------------------------------------------------|   (0...0)   *
 * SMAUG 1.4 (C) 1994, 1995, 1996, 1998  by Derek Snider      |    ).:.(    *
 * -----------------------------------------------------------|    {o o}    *
 * SMAUG code team: Thoric, Altrag, Blodkai, Narn, Haus,      |   / ' ' \   *
 * Scryn, Rennard, Swordbearer, Gorog, Grishnakh, Nivek,      |~'~.VxvxV.~'~*
 * Tricops and Fireblade                                      |             *
 * ------------------------------------------------------------------------ *
 *			     Special clan module			    *
 ****************************************************************************/

#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
/* #include <stdlib.h> */
#include <time.h>
#include "mud.h"



#define MAX_NEST	100
static	OBJ_DATA *	rgObjNest	[MAX_NEST];

CLAN_DATA * first_clan;
CLAN_DATA * last_clan;

COUNCIL_DATA * first_council;
COUNCIL_DATA * last_council;

MEMBER_LIST * first_member_list;
MEMBER_LIST * last_member_list;

CENSOR_DATA	  *	first_censor;
CENSOR_DATA	  *	last_censor;

ALLIANCE_DATA * first_alliance;
ALLIANCE_DATA * last_alliance;

/* local routines */
void	fread_clan	args( ( CLAN_DATA *clan, FILE *fp ) );
bool	load_clan_file	args( ( char *clanfile ) );
void	write_clan_list	args( ( void ) );

void	fread_council	args( ( COUNCIL_DATA *council, FILE *fp ) );
bool	load_council_file	args( ( char *councilfile ) );
void	write_council_list	args( ( void ) );

void	save_member_list	args( ( MEMBER_LIST *members_list ) );
void	show_members		args( ( CHAR_DATA *ch, char *argument, char *format ) );
/*void	remove_member		args( ( CHAR_DATA *ch ) );*/

char *	const	clan_alliance_type [] =
{ "&wNeutral", "&cFriendly", "&CAllied", "&YHostile", "&RAt War" };

char * get_clanTitle( CHAR_DATA *ch )
{
	if (IS_NPC(ch))
		return ("");
	if (!ch->pcdata->clan)
		return ("");

	if (ch->sex == SEX_FEMALE)
	{
		switch (ch->pcdata->clanRank)
		{
			default:
				return ("");
			case 1:
				return ch->pcdata->clan->fRank1;
			case 2:
				return ch->pcdata->clan->fRank2;
			case 3:
				return ch->pcdata->clan->fRank3;
			case 4:
				return ch->pcdata->clan->fRank4;
			case 5:
				return ch->pcdata->clan->fRank5;
			case 6:
				return ch->pcdata->clan->fRank6;
			case 7:
				return ch->pcdata->clan->fRank7;
		}
	}
	else
	{
		switch (ch->pcdata->clanRank)
		{
			default:
				return ("");
			case 1:
				return ch->pcdata->clan->mRank1;
			case 2:
				return ch->pcdata->clan->mRank2;
			case 3:
				return ch->pcdata->clan->mRank3;
			case 4:
				return ch->pcdata->clan->mRank4;
			case 5:
				return ch->pcdata->clan->mRank5;
			case 6:
				return ch->pcdata->clan->mRank6;
			case 7:
				return ch->pcdata->clan->mRank7;
		}
	}
	return ("");
}
/*
 * Get pointer to alliance structure for 2 clans.
 */

ALLIANCE_DATA * get_alliance(CLAN_DATA * clan, CLAN_DATA * vclan)
{
	ALLIANCE_DATA * alliance;
	bool found = FALSE;

	if (clan == vclan)
		return NULL;

	for(alliance = first_alliance; alliance; alliance = alliance->next)
	{
		if (alliance->clan == clan && alliance->vclan == vclan)
		{
			found = TRUE;
			break;
		}

	}

	if (!found)
	{
		bug("get_alliance: No alliance data found for %s and %s", clan->name, vclan->name);
		return NULL;
	}

	return alliance;
}

/*
 * Adds alliance data to all clans when a new clan is created
 */
void newClanAlliance(CLAN_DATA * newClan)
{
	ALLIANCE_DATA * alliance;
	CLAN_DATA *clan;

	for ( clan = first_clan; clan; clan = clan->next )
	{
		if (clan == newClan)
			continue;

		CREATE( alliance, ALLIANCE_DATA, 1 );
		alliance->clan = clan;
		alliance->vclan = newClan;
		alliance->status = 0;
		alliance->vclanStatus = 0;
		alliance->votes = 0;
		alliance->leader1Vote = STRALLOC( "None" );
		alliance->leader2Vote = STRALLOC( "None" );
		LINK( alliance, first_alliance, last_alliance, next, prev );
	}
	return;
}

/*
 * Checks to make sure that one clan is totaly agreed on war
 */

bool clan_agreed_war(CLAN_DATA *clan, CLAN_DATA *vclan)
{
	ALLIANCE_DATA *alliance;
	ALLIANCE_DATA *valliance;

	alliance = get_alliance(clan, vclan);
	valliance = get_alliance(vclan, clan);

	if (!alliance)
		return FALSE;
	if (!valliance)
                return FALSE;
	
	if (alliance->status != ALLIANCE_ATWAR)
		return FALSE;
	
	if (valliance->status != ALLIANCE_ATWAR)
                return FALSE;
	
	return TRUE;

	/*if ((!str_cmp(alliance->leader1Vote, clan->leader1)
		|| !str_cmp(alliance->leader1Vote, clan->leader2)
		|| !str_cmp(alliance->leader1Vote, clan->leader3)
		|| !str_cmp(alliance->leader1Vote, clan->leader4)
		|| !str_cmp(alliance->leader1Vote, clan->leader5)
		|| !str_cmp(alliance->leader1Vote, clan->leader6))
		&&(!str_cmp(alliance->leader2Vote, clan->leader1)
		|| !str_cmp(alliance->leader2Vote, clan->leader2)
		|| !str_cmp(alliance->leader2Vote, clan->leader3)
		|| !str_cmp(alliance->leader2Vote, clan->leader4)
		|| !str_cmp(alliance->leader2Vote, clan->leader5)
		|| !str_cmp(alliance->leader2Vote, clan->leader6))
		&& alliance->votes >= 2)
		return TRUE;*/

	return FALSE;
}

/*
 * Returns the alliance status for game affects (like if truly at war)
 */

int allianceStatus(CLAN_DATA *clan, CLAN_DATA *vclan)
{
	ALLIANCE_DATA *alliance;
	ALLIANCE_DATA *valliance;

	alliance = get_alliance(clan, vclan);
	valliance = get_alliance(vclan, clan);

	if (!alliance || !valliance)
	{
		if (clan == vclan)
		{
			return ALLIANCE_ALLIED;
		}
		else
		{
			bug("allianceStatus: couldn't get alliance data.");
			return ALLIANCE_NEUTRAL;
		}
	}

	if (alliance->status == ALLIANCE_ALLIED)
	{
		if (alliance->vclanStatus == ALLIANCE_ALLIED)
			return ALLIANCE_ALLIED;
		else
			return ALLIANCE_FRIENDLY;
	}

	if (alliance->status == ALLIANCE_ATWAR)
	{
		if (clan_agreed_war(clan, vclan) && 
		    clan_agreed_war(vclan, clan) )
			return ALLIANCE_ATWAR;
		else
			return ALLIANCE_HOSTILE;
	}

	switch (alliance->status)
	{
		case ALLIANCE_FRIENDLY:
			return ALLIANCE_NEUTRAL;
		case ALLIANCE_ALLIED:
			return ALLIANCE_ALLIED;
		case ALLIANCE_HOSTILE:
			return ALLIANCE_HOSTILE;
		case ALLIANCE_ATWAR:
			return ALLIANCE_ATWAR;
		default:
                        return ALLIANCE_NEUTRAL;
	}
	return ALLIANCE_NEUTRAL;
}

/* To check if ch is the leader of a clan */
bool is_leader(CHAR_DATA *ch)
{
	CLAN_DATA *clan;

	clan = ch->pcdata->clan;

	if (!clan)
		return FALSE;

	if (!str_cmp( ch->name, clan->leader1) || !str_cmp( ch->name, clan->leader2)
		|| !str_cmp( ch->name, clan->leader3) || !str_cmp( ch->name, clan->leader4)
		|| !str_cmp( ch->name, clan->leader5) || !str_cmp( ch->name, clan->leader6))
		return TRUE;

	return FALSE;
}

/* To check if ch is the admin of a clan */
bool is_deity(CHAR_DATA *ch)
{
	CLAN_DATA *clan;

	clan = ch->pcdata->clan;

	if (!clan)
		return FALSE;

	if (!str_cmp( ch->name, clan->deity))
		return TRUE;

	return FALSE;
}

/*
 * Changes alliance status for clans
 */

bool change_alliance(CLAN_DATA *clan, CLAN_DATA *vclan, int newStatus)
{
	ALLIANCE_DATA *alliance;
	ALLIANCE_DATA *valliance;
	char buf[MAX_STRING_LENGTH];

	if ((alliance = get_alliance(clan, vclan)) == NULL)
		return FALSE;

	if ((valliance = get_alliance(vclan, clan)) == NULL)
		return FALSE;

	if( alliance->changeTimer > 0 || valliance->changeTimer > 0)
	{
	  sprintf(buf, "The clan alliance cannot be changed till the current change timer reaches 0 again.");
	  echo_to_clan(clan, buf);
	  return FALSE;
	}

	alliance->changeTimer = ALLIANCE_WAIT_TIME;
	alliance->newStatus = newStatus;

	sprintf(buf, "%s has decided to change their alliance with your clan to %s.", clan->name, clan_alliance_type[alliance->newStatus]);
	echo_to_clan(vclan, buf);
	sprintf(buf, "This status change will take effect in %d minutes.", alliance->changeTimer);
	echo_to_clan(vclan, buf);
	sprintf(buf, "Your clan has decided to change their alliance with %s to %s.", vclan->name, clan_alliance_type[alliance->newStatus]);
	echo_to_clan(clan, buf);
	sprintf(buf, "This status change will take effect in %d minutes.", alliance->changeTimer);
	echo_to_clan(clan, buf);

	save_clan(clan);
	save_clan(vclan);

	return TRUE;
}

/*
 * Does all the stuff when war status is voted for
 */

void alliance_towar(CHAR_DATA *ch, CLAN_DATA *clan, CLAN_DATA *vclan)
{
	ALLIANCE_DATA *alliance;
	ALLIANCE_DATA *valliance;
	char buf[MAX_STRING_LENGTH];

	alliance = get_alliance(clan, vclan);
	valliance = get_alliance(vclan, clan);

	alliance->votes++;

	if (alliance->votes >= 2 && valliance->votes >= 2)
	{
		pager_printf(ch, "Your clan is now at war with %s.\n\r", vclan->name );
		sprintf(buf, "%s and %s have begun a massive clan war!\n\r", clan->name, vclan->name);
		echo_to_all(AT_RED, buf, ECHOTAR_ALL);
	}
	else
	{
		pager_printf(ch, "You have put your vote in for war with %s, but both clans have yet to agree on war.\n\r", vclan->name );
		sprintf(buf, "%s has voted to go to war with %s!", ch->name, clan->name);
		echo_to_clan(clan, buf);
		sprintf(buf, "%s has voted to go to war with %s!", clan->name, vclan->name);
		echo_to_clan(vclan, buf);
	}

	if (!str_cmp(alliance->leader1Vote, "None"))
	{
		STRFREE(alliance->leader1Vote);
		alliance->leader1Vote = STRALLOC(ch->name);
	}
	else if (!str_cmp(alliance->leader2Vote, "None"))
	{
		STRFREE(alliance->leader2Vote);
		alliance->leader2Vote = STRALLOC(ch->name);
	}

	save_clan(clan);
	return;
}

/*
 * Prints a clans alliances with other clans
 */

void show_alliances(CHAR_DATA *ch, CLAN_DATA *clan)
{
	ALLIANCE_DATA *alliance;
	bool found = FALSE;
	char buf[MAX_STRING_LENGTH];

	pager_printf(ch, "&cClan                                        Alliance Status Their Status\n\r");
	pager_printf(ch, "&g------------------------------------------- --------------- ------------\n\r");

	for (alliance = first_alliance; alliance; alliance = alliance->next)
	{
		if (alliance->clan != clan)
			continue;

		if( alliance->vclan == NULL )
			continue;

		found = TRUE;

		pager_printf(ch, "&w%-43s %15s %12s\n\r", alliance->vclan->name,
			clan_alliance_type[alliance->status],
			clan_alliance_type[alliance->vclanStatus]);
		if (alliance->changeTimer)
		{
			sprintf(buf, "&RStatus changing to %s in %d minutes\n\r",
		 		clan_alliance_type[alliance->newStatus], alliance->changeTimer);
		 	pager_printf(ch, "%72s", buf);
		}
	}

	if (!found)
		pager_printf(ch, "No clan alliances formed.\n\r");
	else
		pager_printf(ch, "&g------------------------------------------------------------------------\n\r");
	return;
}

/*
 * Get pointer to clan structure from clan name.
 */
CLAN_DATA *get_clan( char *name )
{
    CLAN_DATA *clan;

    for ( clan = first_clan; clan; clan = clan->next )
       if ( !str_cmp( name, clan->name ) || !str_cmp( name, clan->short_name ))
         return clan;
    return NULL;
}

COUNCIL_DATA *get_council( char *name )
{
    COUNCIL_DATA *council;

    for ( council = first_council; council; council = council->next )
       if ( !str_cmp( name, council->name ) )
         return council;
    return NULL;
}

void save_alliance( )
{
     ALLIANCE_DATA *alliance;
     FILE		*fp;
     CLAN_DATA    *clan;
     char         buf[MAX_STRING_LENGTH];

     sprintf( buf, "%salliance", CLAN_DIR );

     if( ( fp = fopen( buf, "w" ) ) == NULL )
     {
         bug( "Cannot open alliance file for writing", 0 );
         perror( buf );
         return;
     }

	/*
	 * Sorts and saves alliance data
	 */
	for (clan = first_clan; clan; clan = clan->next)
	{
		for( alliance = first_alliance; alliance; alliance = alliance->next )
		{
			if (clan == alliance->clan)
				fprintf( fp, "Alliance %s~ %s~ %d %d %d %s~ %s~\n", alliance->clan->short_name,
					alliance->vclan->short_name, alliance->status,
					alliance->vclanStatus, alliance->votes,
					alliance->leader1Vote,alliance->leader2Vote);
		}
	}
	fprintf( fp, "End\n\n" );
	fclose( fp );
	return;
}

bool load_alliance( )
{
     FILE *fp;
     char buf[MAX_STRING_LENGTH];
	ALLIANCE_DATA * alliance = NULL;

     log_string( "Loading alliance data" );

     sprintf( buf, "%salliance", CLAN_DIR );

     if( ( fp = fopen( buf, "r" ) ) == NULL )
     {
         bug( "Cannot open alliance list for reading" );
         return FALSE;
     }

     for( ; ; )
     {
         char *word;

         word = fread_word( fp );

         if( !str_cmp( word, "Alliance" ) )
         {
             CREATE( alliance, ALLIANCE_DATA, 1 );
             alliance->clan = get_clan(fread_string( fp ));
             alliance->vclan = get_clan(fread_string( fp ));
             alliance->status = fread_number( fp );
             alliance->vclanStatus = fread_number( fp );
             alliance->votes = fread_number( fp );
             alliance->leader1Vote = fread_string( fp );
             alliance->leader2Vote = fread_string( fp );
	     if ( !alliance->leader1Vote || 
		   alliance->leader1Vote[0] == '\0' ||
		   alliance->leader1Vote == NULL )
	       alliance->leader1Vote = STRALLOC( "None" );
	     if ( !alliance->leader2Vote ||
		   alliance->leader2Vote[0] == '\0' ||
		   alliance->leader2Vote == NULL )
               alliance->leader2Vote = STRALLOC( "None" );
             LINK( alliance, first_alliance, last_alliance, next, prev );
             continue;
         }
         else
         if( !str_cmp( word, "End" ) )
         {
             LINK( alliance, first_alliance, last_alliance, next, prev );
             fclose( fp );
             return TRUE;
         }
         else
         {
             bug( "load_alliance: bad section" );
             fclose( fp );
             return FALSE;
         }
     }

}

void write_clan_list( )
{
    CLAN_DATA *tclan;
    FILE *fpout;
    char filename[256];

    sprintf( filename, "%s%s", CLAN_DIR, CLAN_LIST );
    fpout = fopen( filename, "w" );
    if ( !fpout )
    {
	bug( "FATAL: cannot open clan.lst for writing!\n\r", 0 );
 	return;
    }
    for ( tclan = first_clan; tclan; tclan = tclan->next )
	fprintf( fpout, "%s\n", tclan->filename );
    fprintf( fpout, "$\n" );
    fclose( fpout );
}

void write_council_list( )
{
    COUNCIL_DATA *tcouncil;
    FILE *fpout;
    char filename[256];

    sprintf( filename, "%s%s", COUNCIL_DIR, COUNCIL_LIST );
    fpout = fopen( filename, "w" );
    if ( !fpout )
    {
	bug( "FATAL: cannot open council.lst for writing!\n\r", 0 );
 	return;
    }
    for ( tcouncil = first_council; tcouncil; tcouncil = tcouncil->next )
	fprintf( fpout, "%s\n", tcouncil->filename );
    fprintf( fpout, "$\n" );
    fclose( fpout );
}

/*
 * Save a clan's data to its data file
 */
void save_clan( CLAN_DATA *clan )
{
    FILE *fp;
    char filename[256];
    char buf[MAX_STRING_LENGTH];

    if ( !clan )
    {
	bug( "save_clan: null clan pointer!", 0 );
	return;
    }

    if ( !clan->filename || clan->filename[0] == '\0' )
    {
	sprintf( buf, "save_clan: %s has no filename", clan->name );
	bug( buf, 0 );
	return;
    }

    sprintf( filename, "%s%s", CLAN_DIR, clan->filename );

    fclose( fpReserve );
    if ( ( fp = fopen( filename, "w" ) ) == NULL )
    {
    	bug( "save_clan: fopen", 0 );
    	perror( filename );
    }
    else
    {
	fprintf( fp, "#CLAN\n" );
	fprintf( fp, "Name         %s~\n",	clan->name		);
	fprintf( fp, "ShortName    %s~\n",	clan->short_name		);
	fprintf( fp, "Filename     %s~\n",	clan->filename		);
	fprintf( fp, "Motto        %s~\n",	clan->motto		);
	fprintf( fp, "Description  %s~\n",	clan->description	);
	if (clan->clanHQ)
		fprintf( fp, "ClanHQ       %s~\n",	clan->clanHQ->filename		);
	fprintf( fp, "Deity        %s~\n",	clan->deity		);
	fprintf( fp, "Owner	   %s~\n",	clan->owner		);
	fprintf( fp, "Leader1      %s~\n",	clan->leader1		);
	fprintf( fp, "Leader2      %s~\n",	clan->leader2		);
	fprintf( fp, "Leader3      %s~\n",	clan->leader3		);
	fprintf( fp, "Leader4      %s~\n",	clan->leader4		);
	fprintf( fp, "Leader5      %s~\n",	clan->leader5		);
	fprintf( fp, "Leader6      %s~\n",	clan->leader6		);
	fprintf( fp, "Badge        %s~\n",	clan->badge		);
	fprintf( fp, "MaleRank1    %s~\n",	clan->mRank1		);
	fprintf( fp, "MaleRank2    %s~\n",	clan->mRank2		);
	fprintf( fp, "MaleRank3    %s~\n",	clan->mRank3		);
	fprintf( fp, "MaleRank4    %s~\n",	clan->mRank4		);
	fprintf( fp, "MaleRank5    %s~\n",	clan->mRank5		);
	fprintf( fp, "MaleRank6    %s~\n",	clan->mRank6		);
	fprintf( fp, "MaleRank7    %s~\n",	clan->mRank7		);
	fprintf( fp, "FemaleRank1  %s~\n",	clan->fRank1		);
	fprintf( fp, "FemaleRank2  %s~\n",	clan->fRank2		);
	fprintf( fp, "FemaleRank3  %s~\n",	clan->fRank3		);
	fprintf( fp, "FemaleRank4  %s~\n",	clan->fRank4		);
	fprintf( fp, "FemaleRank5  %s~\n",	clan->fRank5		);
	fprintf( fp, "FemaleRank6  %s~\n",	clan->fRank6		);
	fprintf( fp, "FemaleRank7  %s~\n",	clan->fRank7		);
	fprintf( fp, "MaleRank1C   %d\n",	clan->mRank1Count		);
	fprintf( fp, "MaleRank2C   %d\n",	clan->mRank2Count		);
	fprintf( fp, "MaleRank3C   %d\n",	clan->mRank3Count		);
	fprintf( fp, "MaleRank4C   %d\n",	clan->mRank4Count		);
	fprintf( fp, "MaleRank5C   %d\n",	clan->mRank5Count		);
	fprintf( fp, "MaleRank6C   %d\n",	clan->mRank6Count		);
	fprintf( fp, "MaleRank7C   %d\n",	clan->mRank7Count		);
	fprintf( fp, "FemaleRank1C %d\n",	clan->fRank1Count		);
	fprintf( fp, "FemaleRank2C %d\n",	clan->fRank2Count		);
	fprintf( fp, "FemaleRank3C %d\n",	clan->fRank3Count		);
	fprintf( fp, "FemaleRank4C %d\n",	clan->fRank4Count		);
	fprintf( fp, "FemaleRank5C %d\n",	clan->fRank5Count		);
	fprintf( fp, "FemaleRank6C %d\n",	clan->fRank6Count		);
	fprintf( fp, "FemaleRank7C %d\n",	clan->fRank7Count		);

	fprintf( fp, "PKillRangeNew   %d %d %d %d %d %d %d\n",
		clan->pkills[0], clan->pkills[1], clan->pkills[2],
		clan->pkills[3], clan->pkills[4], clan->pkills[5],
		clan->pkills[6]);
	fprintf( fp, "PDeathRangeNew  %d %d %d %d %d %d %d\n",
		clan->pdeaths[0], clan->pdeaths[1], clan->pdeaths[2],
		clan->pdeaths[3], clan->pdeaths[4], clan->pdeaths[5],
		clan->pdeaths[6]);
	fprintf( fp, "SparWinsRangeNew   %d %d %d %d %d %d %d\n",
		clan->spar_wins[0], clan->spar_wins[1], clan->spar_wins[2],
		clan->spar_wins[3], clan->spar_wins[4], clan->spar_wins[5],
		clan->spar_wins[6]);
	fprintf( fp, "SparLossRangeNew   %d %d %d %d %d %d %d\n",
		clan->spar_loss[0], clan->spar_loss[1], clan->spar_loss[2],
		clan->spar_loss[3], clan->spar_loss[4], clan->spar_loss[5],
		clan->spar_loss[6]);
	fprintf( fp, "MKills       %d\n",	clan->mkills		);
	fprintf( fp, "MDeaths      %d\n",	clan->mdeaths		);
	fprintf( fp, "IllegalPK    %d\n",	clan->illegal_pk	);
	fprintf( fp, "Score        %d\n",	clan->score		);
	/*fprintf( fp, "ClanPL       %.0Lf\n",	clan->clanPL		);*/
	fprintf( fp, "Type         %d\n",	clan->clan_type		);
	fprintf( fp, "Class        %d\n",	clan->class		);
	fprintf( fp, "Favour       %d\n",	clan->favour		);
	fprintf( fp, "Strikes      %d\n",	clan->strikes		);
	fprintf( fp, "Members      %d\n",	clan->members		);
	fprintf( fp, "Active       %d\n",	clan->activeMem		);
	fprintf( fp, "MemLimit     %d\n",	clan->mem_limit		);
	fprintf( fp, "Alignment    %d\n",	clan->alignment		);
	fprintf( fp, "Board        %d\n",	clan->board		);
	fprintf( fp, "ClanObjOne   %d\n",	clan->clanobj1		);
	fprintf( fp, "ClanObjTwo   %d\n",	clan->clanobj2		);
	fprintf( fp, "ClanObjThree %d\n",	clan->clanobj3		);
    fprintf( fp, "ClanObjFour  %d\n",	clan->clanobj4		);
	fprintf( fp, "ClanObjFive  %d\n", 	clan->clanobj5		);
	fprintf( fp, "Recall       %d\n",	clan->recall		);
	fprintf( fp, "Storeroom    %d\n",	clan->storeroom		);
	fprintf( fp, "DeathRecall  %d\n",	clan->deathRecall		);
	fprintf( fp, "Morgue       %d\n",	clan->clanMorgue		);
	fprintf( fp, "Donate       %d\n",	clan->clanDonate		);
	fprintf( fp, "Jail         %d\n",	clan->clanJail		);
	fprintf( fp, "GuardOne     %d\n",	clan->guard1		);
	fprintf( fp, "GuardTwo     %d\n",	clan->guard2		);
	fprintf( fp, "ClanSkill1   %d\n",	clan->clanSkill1		);
	fprintf( fp, "ClanSkill2   %d\n",	clan->clanSkill2		);
	fprintf( fp, "ClanSkill3   %d\n",	clan->clanSkill3		);
	fprintf( fp, "URL          %s~\n",	clan->clanWebSite		);
	fprintf( fp, "TaxRate      %d\n",	clan->tax		);
	fprintf( fp, "Bank         %.0Lf\n",	clan->bank		);
	fprintf( fp, "End\n\n"						);
	fprintf( fp, "#END\n"						);
    }
    fclose( fp );
    fpReserve = fopen( NULL_FILE, "r" );
    return;
}

/*
 * Save a council's data to its data file
 */
void save_council( COUNCIL_DATA *council )
{
    FILE *fp;
    char filename[256];
    char buf[MAX_STRING_LENGTH];

    if ( !council )
    {
	bug( "save_council: null council pointer!", 0 );
	return;
    }

    if ( !council->filename || council->filename[0] == '\0' )
    {
	sprintf( buf, "save_council: %s has no filename", council->name );
	bug( buf, 0 );
	return;
    }

    sprintf( filename, "%s%s", COUNCIL_DIR, council->filename );

    fclose( fpReserve );
    if ( ( fp = fopen( filename, "w" ) ) == NULL )
    {
    	bug( "save_council: fopen", 0 );
    	perror( filename );
    }
    else
    {

        fprintf( fp, "#COUNCIL\n" );
        if ( council-> name )
           fprintf( fp, "Name         %s~\n",   council->name           );
        if ( council->filename )
           fprintf( fp, "Filename     %s~\n",   council->filename       );
        if ( council->description )
           fprintf( fp, "Description  %s~\n",   council->description    );
        if ( council->head )
           fprintf( fp, "Head         %s~\n",   council->head           );
        if ( council->head2 != NULL)
                fprintf (fp, "Head2        %s~\n", council->head2);
        fprintf( fp, "Members      %d\n",       council->members        );
        fprintf( fp, "Board        %d\n",       council->board          );
        fprintf( fp, "Meeting      %d\n",       council->meeting        );
        if ( council->powers )
           fprintf( fp, "Powers       %s~\n",   council->powers         );
        fprintf( fp, "End\n\n"                                          );
        fprintf( fp, "#END\n"						);
    }
    fclose( fp );
    fpReserve = fopen( NULL_FILE, "r" );
    return;
}


/*
 * Read in actual clan data.
 */

#if defined(KEY)
#undef KEY
#endif

#define KEY( literal, field, value )					\
				if ( !str_cmp( word, literal ) )	\
				{					\
				    field  = value;			\
				    fMatch = TRUE;			\
				    break;				\
				}

/*
 * Reads in PKill and PDeath still for backward compatibility but now it
 * should be written to PKillRange and PDeathRange for multiple level pkill
 * tracking support. --Shaddai
 * Added a hardcoded limit memlimit to the amount of members a clan can
 * have set using setclan.  --Shaddai
 */

void fread_clan( CLAN_DATA *clan, FILE *fp )
{
    char buf[MAX_STRING_LENGTH];
    char *word;
    bool fMatch;


    clan->mem_limit = 0;  /* Set up defaults */
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
	    KEY( "Alignment",	clan->alignment,	fread_number( fp ) );
	    KEY( "Active",	clan->activeMem,	fread_number( fp ) );
	    break;

	case 'B':
        KEY( "Badge",       clan->badge,            fread_string( fp ) );
	KEY( "Bank",    clan->bank,             fread_number_ld( fp ) );
	/*
        KEY( "Bank",	clan->bank,		fread_number_skill( fp ) ); //used skill since it reads doubles
	*/
	    KEY( "Board",	clan->board,		fread_number( fp ) );
	    break;

	case 'C':
        if (!str_cmp(word, "ClanHQ"))
        {
        	AREA_DATA * clanHQ;
        	char fname[MAX_STRING_LENGTH];

        	sprintf(fname, "%s", fread_string( fp ));

        	for (clanHQ = first_area; clanHQ; clanHQ = clanHQ->next)
        	{
        		if (!str_cmp(clanHQ->filename, fname))
        		{
        			clan->clanHQ = clanHQ;
        			break;
        		}
        	}
        	fMatch = TRUE;
        }
	    KEY( "ClanSkill1",	clan->clanSkill1,		fread_number( fp ) );
	    KEY( "ClanSkill2",	clan->clanSkill2,		fread_number( fp ) );
	    KEY( "ClanSkill3",	clan->clanSkill3,		fread_number( fp ) );
	    KEY( "ClanObjOne",	clan->clanobj1,		fread_number( fp ) );
	    KEY( "ClanObjTwo",	clan->clanobj2,		fread_number( fp ) );
	    KEY( "ClanObjThree",clan->clanobj3,		fread_number( fp ) );
        KEY( "ClanObjFour", clan->clanobj4,         fread_number( fp ) );
        KEY( "ClanObjFive", clan->clanobj5,         fread_number( fp ) );
	    KEY( "Class",	clan->class,		fread_number( fp ) );
	    /*KEY( "ClanPL",	clan->clanPL,		fread_number_ld( fp ) );*/
	    break;

	case 'D':
	    KEY( "DeathRecall",	clan->deathRecall,		fread_number( fp ) );
	    KEY( "Donate",	clan->clanDonate,		fread_number( fp ) );
	    KEY( "Deity",	clan->deity,		fread_string( fp ) );
	    KEY( "Description",	clan->description,	fread_string( fp ) );
	    break;

	case 'E':
	    if ( !str_cmp( word, "End" ) )
	    {
		if (!clan->name)
		  clan->name		= STRALLOC( "" );
		if (!clan->leader1)
		  clan->leader1		= STRALLOC( "" );
		if (!clan->leader2)
		  clan->leader2		= STRALLOC( "" );
		if (!clan->leader3)
		  clan->leader3		= STRALLOC( "" );
		if (!clan->leader4)
		  clan->leader4		= STRALLOC( "" );
		if (!clan->leader5)
		  clan->leader5		= STRALLOC( "" );
		if (!clan->leader6)
		  clan->leader6		= STRALLOC( "" );
		if (!clan->description)
		  clan->description 	= STRALLOC( "" );
		if (!clan->motto)
		  clan->motto		= STRALLOC( "" );
		if (!clan->deity)
		  clan->deity		= STRALLOC( "" );
		if (!clan->badge)
	  	  clan->badge		= STRALLOC( "" );
		if (!clan->clanWebSite)
	  	  clan->clanWebSite		= STRALLOC( "" );
		if (!clan->clanHQ)
	  	  clan->clanHQ		= NULL;
		if (!clan->mRank1)
	  	  clan->mRank1		= STRALLOC( "" );
		if (!clan->mRank2)
	  	  clan->mRank2		= STRALLOC( "" );
		if (!clan->mRank3)
	  	  clan->mRank3		= STRALLOC( "" );
		if (!clan->mRank4)
	  	  clan->mRank4		= STRALLOC( "" );
		if (!clan->mRank5)
	  	  clan->mRank5		= STRALLOC( "" );
		if (!clan->mRank6)
	  	  clan->mRank6		= STRALLOC( "" );
		if (!clan->mRank7)
	  	  clan->mRank7		= STRALLOC( "" );
		if (!clan->fRank1)
	  	  clan->fRank1		= STRALLOC( "" );
		if (!clan->fRank2)
	  	  clan->fRank2		= STRALLOC( "" );
		if (!clan->fRank3)
	  	  clan->fRank3		= STRALLOC( "" );
		if (!clan->fRank4)
	  	  clan->fRank4		= STRALLOC( "" );
		if (!clan->fRank5)
	  	  clan->fRank5		= STRALLOC( "" );
		if (!clan->fRank6)
	  	  clan->fRank6		= STRALLOC( "" );
		if (!clan->fRank7)
	  	  clan->fRank7		= STRALLOC( "" );
    		clan->members = clan->fRank1Count + clan->fRank2Count + clan->fRank3Count + clan->fRank4Count;
  		clan->members+= clan->fRank5Count + clan->fRank6Count + clan->fRank7Count;
		clan->members+= clan->mRank1Count + clan->mRank2Count + clan->mRank3Count + clan->mRank4Count;
		clan->members+= clan->mRank5Count + clan->mRank6Count + clan->mRank7Count;
		return;
	    }
	    break;

	case 'F':
	    KEY( "Favour",	clan->favour,		fread_number( fp ) );
	    KEY( "Filename",	clan->filename,		fread_string_nohash( fp ) );
	    KEY( "FemaleRank1",	clan->fRank1,		fread_string( fp ) );
	    KEY( "FemaleRank2",	clan->fRank2,		fread_string( fp ) );
	    KEY( "FemaleRank3",	clan->fRank3,		fread_string( fp ) );
	    KEY( "FemaleRank4",	clan->fRank4,		fread_string( fp ) );
	    KEY( "FemaleRank5",	clan->fRank5,		fread_string( fp ) );
	    KEY( "FemaleRank6",	clan->fRank6,		fread_string( fp ) );
	    KEY( "FemaleRank7",	clan->fRank7,		fread_string( fp ) );
	    KEY( "FemaleRank1C",	clan->fRank1Count,		fread_number( fp ) );
	    KEY( "FemaleRank2C",	clan->fRank2Count,		fread_number( fp ) );
	    KEY( "FemaleRank3C",	clan->fRank3Count,		fread_number( fp ) );
	    KEY( "FemaleRank4C",	clan->fRank4Count,		fread_number( fp ) );
	    KEY( "FemaleRank5C",	clan->fRank5Count,		fread_number( fp ) );
	    KEY( "FemaleRank6C",	clan->fRank6Count,		fread_number( fp ) );
	    KEY( "FemaleRank7C",	clan->fRank7Count,		fread_number( fp ) );
	    break;

	case 'G':
	    KEY( "GuardOne",	clan->guard1,		fread_number( fp ) );
	    KEY( "GuardTwo",	clan->guard2,		fread_number( fp ) );
	    break;

	case 'I':
	    KEY( "IllegalPK",	clan->illegal_pk,	fread_number( fp ) );
	    break;

	case 'J':
	    KEY( "Jail",	clan->clanJail,	fread_number( fp ) );
	    break;

	case 'L':
	    KEY( "Leader1",	clan->leader1,		fread_string( fp ) );
	    KEY( "Leader2",	clan->leader2,		fread_string( fp ) );
	    KEY( "Leader3",	clan->leader3,		fread_string( fp ) );
	    KEY( "Leader4",	clan->leader4,		fread_string( fp ) );
	    KEY( "Leader5",	clan->leader5,		fread_string( fp ) );
	    KEY( "Leader6",	clan->leader6,		fread_string( fp ) );
	    break;

	case 'M':
	    KEY( "MDeaths",	clan->mdeaths,		fread_number( fp ) );
	    KEY( "Members",	clan->members,		fread_number( fp ) );
	    KEY( "MemLimit",	clan->mem_limit,	fread_number( fp ) );
	    KEY( "MKills",	clan->mkills,		fread_number( fp ) );
	    KEY( "Motto",	clan->motto,		fread_string( fp ) );
	    KEY( "MaleRank1",	clan->mRank1,		fread_string( fp ) );
	    KEY( "MaleRank2",	clan->mRank2,		fread_string( fp ) );
	    KEY( "MaleRank3",	clan->mRank3,		fread_string( fp ) );
	    KEY( "MaleRank4",	clan->mRank4,		fread_string( fp ) );
	    KEY( "MaleRank5",	clan->mRank5,		fread_string( fp ) );
	    KEY( "MaleRank6",	clan->mRank6,		fread_string( fp ) );
	    KEY( "MaleRank7",	clan->mRank7,		fread_string( fp ) );
	    KEY( "MaleRank1C",	clan->mRank1Count,		fread_number( fp ) );
	    KEY( "MaleRank2C",	clan->mRank2Count,		fread_number( fp ) );
	    KEY( "MaleRank3C",	clan->mRank3Count,		fread_number( fp ) );
	    KEY( "MaleRank4C",	clan->mRank4Count,		fread_number( fp ) );
	    KEY( "MaleRank5C",	clan->mRank5Count,		fread_number( fp ) );
	    KEY( "MaleRank6C",	clan->mRank6Count,		fread_number( fp ) );
	    KEY( "MaleRank7C",	clan->mRank7Count,		fread_number( fp ) );
	    KEY( "Morgue",	clan->clanMorgue,		fread_number( fp ) );
	    break;

	case 'N':
	    KEY( "Name",	clan->name,		fread_string( fp ) );
	    break;
	
	case 'O':
	    KEY( "Owner",       clan->owner,          fread_string( fp ) );
	    break;

	case 'P':
	    KEY( "PDeaths",	clan->pdeaths[6],	fread_number( fp ) );
	    KEY( "PKills",	clan->pkills[6],	fread_number( fp ) );
	    /* Addition of New Ranges */
	    if ( !str_cmp ( word, "PDeathRange" ) )
	    {
		fMatch = TRUE;
		fread_number( fp );
		fread_number( fp );
		fread_number( fp );
		fread_number( fp );
		fread_number( fp );
		fread_number( fp );
		fread_number( fp );
            }
	    if ( !str_cmp ( word, "PDeathRangeNew" ) )
            {
		fMatch = TRUE;
		clan->pdeaths[0] = fread_number( fp );
		clan->pdeaths[1] = fread_number( fp );
		clan->pdeaths[2] = fread_number( fp );
		clan->pdeaths[3] = fread_number( fp );
		clan->pdeaths[4] = fread_number( fp );
		clan->pdeaths[5] = fread_number( fp );
		clan->pdeaths[6] = fread_number( fp );
	    }
	    if ( !str_cmp ( word, "PKillRangeNew" ) )
            {
		fMatch = TRUE;
		clan->pkills[0] = fread_number( fp );
		clan->pkills[1] = fread_number( fp );
		clan->pkills[2] = fread_number( fp );
		clan->pkills[3] = fread_number( fp );
		clan->pkills[4] = fread_number( fp );
		clan->pkills[5] = fread_number( fp );
		clan->pkills[6] = fread_number( fp );
	    }
	    if ( !str_cmp ( word, "PKillRange" ) )
	    {
		fMatch = TRUE;
		fread_number( fp );
		fread_number( fp );
		fread_number( fp );
		fread_number( fp );
		fread_number( fp );
		fread_number( fp );
		fread_number( fp );
            }
	    break;

	case 'R':
	    KEY( "Recall",	clan->recall,		fread_number( fp ) );
	    break;

	case 'S':
	    KEY( "ShortName",	clan->short_name,		fread_string( fp ) );
	    KEY( "Score",	clan->score,		fread_number( fp ) );
	    if ( !str_cmp ( word, "SparLossRangeNew" ) )
            {
		fMatch = TRUE;
		clan->spar_loss[0] = fread_number( fp );
		clan->spar_loss[1] = fread_number( fp );
		clan->spar_loss[2] = fread_number( fp );
		clan->spar_loss[3] = fread_number( fp );
		clan->spar_loss[4] = fread_number( fp );
		clan->spar_loss[5] = fread_number( fp );
		clan->spar_loss[6] = fread_number( fp );
	    }
	    if ( !str_cmp ( word, "SparWinsRangeNew" ) )
            {
		fMatch = TRUE;
		clan->spar_wins[0] = fread_number( fp );
		clan->spar_wins[1] = fread_number( fp );
		clan->spar_wins[2] = fread_number( fp );
		clan->spar_wins[3] = fread_number( fp );
		clan->spar_wins[4] = fread_number( fp );
		clan->spar_wins[5] = fread_number( fp );
		clan->spar_wins[6] = fread_number( fp );
	    }
	    KEY( "Strikes",	clan->strikes,		fread_number( fp ) );
	    KEY( "Storeroom",	clan->storeroom,	fread_number( fp ) );
	    break;

	case 'T':
	    KEY( "TaxRate",	clan->tax,	fread_number( fp ) );
	    KEY( "Type",	clan->clan_type,	fread_number( fp ) );
	    break;

	case 'U':
	    KEY( "URL",	clan->clanWebSite,		fread_string( fp ) );
	    break;
	}

	if ( !fMatch )
	{
	    sprintf( buf, "Fread_clan: no match: %s", word );
	    bug( buf, 0 );
	}
    }

    /* Why the FUCK was this put down here?!
    clan->members = clan->fRank1Count + clan->fRank2Count + clan->fRank3Count + clan->fRank4Count;
    clan->members+= clan->fRank5Count + clan->fRank6Count + clan->fRank7Count;
    clan->members+= clan->mRank1Count + clan->mRank2Count + clan->mRank3Count + clan->mRank4Count;
    clan->members+= clan->mRank5Count + clan->mRank6Count + clan->mRank7Count;
    */
}

/*
 * Read in actual council data.
 */

#if defined(KEY)
#undef KEY
#endif

#define KEY( literal, field, value )					\
				if ( !str_cmp( word, literal ) )	\
				{					\
				    field  = value;			\
				    fMatch = TRUE;			\
				    break;				\
				}

void fread_council( COUNCIL_DATA *council, FILE *fp )
{
    char buf[MAX_STRING_LENGTH];
    char *word;
    bool fMatch;

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

	case 'B':
	    KEY( "Board",	council->board,		fread_number( fp ) );
	    break;

	case 'D':
	    KEY( "Description",	council->description,	fread_string( fp ) );
	    break;

	case 'E':
	    if ( !str_cmp( word, "End" ) )
	    {
		if (!council->name)
		  council->name		= STRALLOC( "" );
		if (!council->description)
		  council->description 	= STRALLOC( "" );
		if (!council->powers)
		  council->powers	= STRALLOC( "" );
		return;
	    }
	    break;

	case 'F':
	    KEY( "Filename",	council->filename,	fread_string_nohash( fp ) );
  	    break;

	case 'H':
	    KEY( "Head", 	council->head, 		fread_string( fp ) );
            KEY ("Head2", 	council->head2, 	fread_string( fp ) );
	    break;

	case 'M':
	    KEY( "Members",	council->members,	fread_number( fp ) );
	    KEY( "Meeting",   	council->meeting, 	fread_number( fp ) );
	    break;

	case 'N':
	    KEY( "Name",	council->name,		fread_string( fp ) );
	    break;

	case 'P':
	    KEY( "Powers",	council->powers,	fread_string( fp ) );
	    break;
	}

	if ( !fMatch )
	{
	    sprintf( buf, "Fread_council: no match: %s", word );
	    bug( buf, 0 );
	}
    }
}


/*
 * Load a clan file
 */

bool load_clan_file( char *clanfile )
{
    char filename[256];
    CLAN_DATA *clan;
    FILE *fp;
    bool found;

    CREATE( clan, CLAN_DATA, 1 );

    /* Make sure we default these to 0 --Shaddai */
    clan->pkills[0] = 0;
    clan->pkills[1] = 0;
    clan->pkills[2] = 0;
    clan->pkills[3] = 0;
    clan->pkills[4] = 0;
    clan->pkills[5] = 0;
    clan->pkills[6] = 0;
    clan->pdeaths[0]= 0;
    clan->pdeaths[1]= 0;
    clan->pdeaths[2]= 0;
    clan->pdeaths[3]= 0;
    clan->pdeaths[4]= 0;
    clan->pdeaths[5]= 0;
    clan->pdeaths[6]= 0;
    clan->spar_wins[0] = 0;
    clan->spar_wins[1] = 0;
    clan->spar_wins[2] = 0;
    clan->spar_wins[3] = 0;
    clan->spar_wins[4] = 0;
    clan->spar_wins[5] = 0;
    clan->spar_wins[6] = 0;
    clan->spar_loss[0] = 0;
    clan->spar_loss[1] = 0;
    clan->spar_loss[2] = 0;
    clan->spar_loss[3] = 0;
    clan->spar_loss[4] = 0;
    clan->spar_loss[5] = 0;
    clan->spar_loss[6] = 0;

    clan->bank = 0;

    found = FALSE;
    sprintf( filename, "%s%s", CLAN_DIR, clanfile );

    if ( ( fp = fopen( filename, "r" ) ) != NULL )
    {

	found = TRUE;
	for ( ; ; )
	{
	    char letter;
	    char *word;

	    letter = fread_letter( fp );
	    if ( letter == '*' )
	    {
		fread_to_eol( fp );
		continue;
	    }

	    if ( letter != '#' )
	    {
		bug( "Load_clan_file: # not found.", 0 );
		break;
	    }

	    word = fread_word( fp );
	    if ( !str_cmp( word, "CLAN"	) )
	    {
	    	fread_clan( clan, fp );
	    	break;
	    }
	    else
	    if ( !str_cmp( word, "END"	) )
	        break;
	    else
	    {
		char buf[MAX_STRING_LENGTH];

		sprintf( buf, "Load_clan_file: bad section: %s.", word );
		bug( buf, 0 );
		break;
	    }
	}
	fclose( fp );
    }

    if ( found )
    {
	ROOM_INDEX_DATA *storeroom;

	LINK( clan, first_clan, last_clan, next, prev );

         if( !load_member_list( clan->filename ) )
         {
             MEMBER_LIST *members_list;

             log_string( "No memberlist found, creating new list" );
             CREATE( members_list, MEMBER_LIST, 1 );
             members_list->name = STRALLOC( clan->name );
             LINK( members_list, first_member_list, last_member_list, next, prev );
             save_member_list( members_list );
         }

	if ( clan->storeroom == 0
	|| (storeroom = get_room_index( clan->storeroom )) == NULL )
	{
	    log_string( "Storeroom not found" );
	    return found;
	}

	sprintf( filename, "%s%s.vault", CLAN_DIR, clan->filename );
	if ( ( fp = fopen( filename, "r" ) ) != NULL )
	{
	    int iNest;
	    bool found;
	    OBJ_DATA *tobj, *tobj_next;

	    log_string( "Loading clan storage room" );
	    rset_supermob(storeroom);
	    for ( iNest = 0; iNest < MAX_NEST; iNest++ )
		rgObjNest[iNest] = NULL;

	    found = TRUE;
	    for ( ; ; )
	    {
		char letter;
		char *word;

		letter = fread_letter( fp );
		if ( letter == '*' )
		{
		    fread_to_eol( fp );
		    continue;
		}

		if ( letter != '#' )
		{
		    bug( "Load_clan_vault: # not found.", 0 );
		    bug( clan->name, 0 );
		    break;
		}

		word = fread_word( fp );
		if ( !str_cmp( word, "OBJECT" ) )	/* Objects	*/
		  fread_obj  ( supermob, fp, OS_CARRY );
		else
		if ( !str_cmp( word, "END"    ) )	/* Done		*/
		  break;
		else
		{
		    bug( "Load_clan_vault: bad section.", 0 );
		    bug( clan->name, 0 );
		    break;
		}
	    }
	    fclose( fp );
	    for ( tobj = supermob->first_carrying; tobj; tobj = tobj_next )
	    {
		tobj_next = tobj->next_content;
		obj_from_char( tobj );
		obj_to_room( tobj, storeroom );
	    }
	    release_supermob();
	}
	else
	    log_string( "Cannot open clan vault" );
    }
    else
      DISPOSE( clan );


    return found;
}

/*
 * Load a council file
 */

bool load_council_file( char *councilfile )
{
    char filename[256];
    COUNCIL_DATA *council;
    FILE *fp;
    bool found;

    CREATE( council, COUNCIL_DATA, 1 );

    found = FALSE;
    sprintf( filename, "%s%s", COUNCIL_DIR, councilfile );

    if ( ( fp = fopen( filename, "r" ) ) != NULL )
    {

	found = TRUE;
	for ( ; ; )
	{
	    char letter;
	    char *word;

	    letter = fread_letter( fp );
	    if ( letter == '*' )
	    {
		fread_to_eol( fp );
		continue;
	    }

	    if ( letter != '#' )
	    {
		bug( "Load_council_file: # not found.", 0 );
		break;
	    }

	    word = fread_word( fp );
	    if ( !str_cmp( word, "COUNCIL"	) )
	    {
	    	fread_council( council, fp );
	    	break;
	    }
	    else
	    if ( !str_cmp( word, "END"	) )
	        break;
	    else
	    {
		bug( "Load_council_file: bad section.", 0 );
		break;
	    }
	}
	fclose( fp );
    }

    if ( found )
      LINK( council, first_council, last_council, next, prev );

    else
      DISPOSE( council );

    return found;
}

/*
 * Load in all the clan files.
 */
void load_clans( )
{
    FILE *fpList;
    char *filename;
    char clanlist[256];
    char buf[MAX_STRING_LENGTH];


    first_clan	= NULL;
    last_clan	= NULL;

    log_string( "Loading clans..." );

    sprintf( clanlist, "%s%s", CLAN_DIR, CLAN_LIST );
    fclose( fpReserve );
    if ( ( fpList = fopen( clanlist, "r" ) ) == NULL )
    {
	perror( clanlist );
	exit( 1 );
    }

    for ( ; ; )
    {
	filename = feof( fpList ) ? "$" : fread_word( fpList );
	log_string( filename );
	if ( filename[0] == '$' )
	  break;

	if ( !load_clan_file( filename ) )
	{
	  sprintf( buf, "Cannot load clan file: %s", filename );
	  bug( buf, 0 );
	}
    }

    load_alliance();

    fclose( fpList );
    log_string(" Done clans " );
    fpReserve = fopen( NULL_FILE, "r" );
    return;
}

/*
 * Load in all the council files.
 */
void load_councils( )
{
    FILE *fpList;
    char *filename;
    char councillist[256];
    char buf[MAX_STRING_LENGTH];


    first_council	= NULL;
    last_council	= NULL;

    log_string( "Loading councils..." );

    sprintf( councillist, "%s%s", COUNCIL_DIR, COUNCIL_LIST );
    fclose( fpReserve );
    if ( ( fpList = fopen( councillist, "r" ) ) == NULL )
    {
	perror( councillist );
	exit( 1 );
    }

    for ( ; ; )
    {
	filename = feof( fpList ) ? "$" : fread_word( fpList );
	log_string( filename );
	if ( filename[0] == '$' )
	  break;

	if ( !load_council_file( filename ) )
	{
	  sprintf( buf, "Cannot load council file: %s", filename );
	  bug( buf, 0 );
	}
    }
    fclose( fpList );
    log_string(" Done councils " );
    fpReserve = fopen( NULL_FILE, "r" );
    return;
}

void do_savealliances( CHAR_DATA *ch, char *argument )
{
    if( ch->level < 63 )
    {
	ch_printf(ch,"Access denied.\n\r");
	return;
    }
    save_alliance( );
    ch_printf(ch,"Done.\n\r");
    return;
}

void do_make( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_INDEX_DATA *pObjIndex;
    OBJ_DATA *obj;
    CLAN_DATA *clan;
    int level;

    if ( IS_NPC( ch ) || !ch->pcdata->clan )
    {
	send_to_char( "Huh?\n\r", ch );
	return;
    }

    clan = ch->pcdata->clan;

    if (!is_leader(ch) && !is_deity(ch))
    {
	send_to_char( "Huh?\n\r", ch );
	return;
    }

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Make what?\n\r", ch );
	return;
    }

    pObjIndex = get_obj_index( clan->clanobj1 );
    level = 40;

    if ( !pObjIndex || !is_name( arg, pObjIndex->name ) )
    {
      pObjIndex = get_obj_index( clan->clanobj2 );
      level = 45;
    }
    if ( !pObjIndex || !is_name( arg, pObjIndex->name ) )
    {
      pObjIndex = get_obj_index( clan->clanobj3 );
      level = 50;
    }
    if ( !pObjIndex || !is_name( arg, pObjIndex->name ) )
    {
      pObjIndex = get_obj_index( clan->clanobj4 );
      level = 35;
    }
    if ( !pObjIndex || !is_name( arg, pObjIndex->name ) )
    {
      pObjIndex = get_obj_index( clan->clanobj5 );
      level = 1;
    }

    if ( !pObjIndex || !is_name( arg, pObjIndex->name ) )
    {
	send_to_char( "You don't know how to make that.\n\r", ch );
	return;
    }

    obj = create_object( pObjIndex, level );
    xSET_BIT( obj->extra_flags, ITEM_CLANOBJECT );
    if ( CAN_WEAR(obj, ITEM_TAKE) )
      obj = obj_to_char( obj, ch );
    else
      obj = obj_to_room( obj, ch->in_room );
    act( AT_MAGIC, "$n makes $p!", ch, obj, NULL, TO_ROOM );
    act( AT_MAGIC, "You make $p!", ch, obj, NULL, TO_CHAR );
    return;
}

void do_induct( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    CLAN_DATA *clan;

    if ( IS_NPC( ch ) || !ch->pcdata->clan )
    {
	send_to_char( "Huh?\n\r", ch );
	return;
    }

    clan = ch->pcdata->clan;

    if ( (ch->pcdata && ch->pcdata->bestowments
    &&    is_name("induct", ch->pcdata->bestowments))
	|| is_deity(ch) || is_leader(ch))
	;
    else
    {
	send_to_char( "Huh?\n\r", ch );
	return;
    }

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Induct whom?\n\r", ch );
	return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL )
    {
	send_to_char( "That player is not here.\n\r", ch);
	return;
    }

    if ( IS_NPC(victim) )
    {
	send_to_char( "Not on NPC's.\n\r", ch );
	return;
    }

    if ( IS_IMMORTAL(victim) )
    {
        send_to_char( "Not on imms.\n\r", ch );
        return;
    }

    if ( clan->clan_type == CLAN_GUILD )
    {
	if ( victim->class != clan->class)
	{
	    send_to_char( "This player's will is not in accordance with your guild.\n\r", ch);
            return;
	}
    }
    else
    {
	if ( victim->exp < 5000 )
	{
	    send_to_char( "This player is not worthy of joining yet (5,000 PL min).\n\r", ch );
	    return;
	}

    }

	if (!xIS_SET(victim->act, PLR_CAN_CHAT))
	{
	    send_to_char( "That player needs their bio authorized before they can join.\n\r", ch );
	    return;
	}

    if ( victim->pcdata->clan )
    {
      if ( victim->pcdata->clan->clan_type == CLAN_ORDER )
      {
	if ( victim->pcdata->clan == clan )
	  send_to_char( "This player already belongs to your order!\n\r", ch );
	else
	  send_to_char( "This player already belongs to an order!\n\r", ch );
	return;
      }
      else
      if ( victim->pcdata->clan->clan_type == CLAN_GUILD )
      {
	if ( victim->pcdata->clan == clan )
	  send_to_char( "This player already belongs to your guild!\n\r", ch );
	else
	  send_to_char( "This player already belongs to an guild!\n\r", ch );
	return;
      }
      else
      {
	if ( victim->pcdata->clan == clan )
	  send_to_char( "This player already belongs to your clan!\n\r", ch );
	else
	  send_to_char( "This player already belongs to a clan!\n\r", ch );
	return;
      }
    }
    if ( clan->mem_limit && clan->members >= clan->mem_limit )
    {
    	send_to_char("Your clan is too big to induct anymore players.\n\r",ch);
	return;
    }

    /* clan->members++; */
    if ( clan->clan_type != CLAN_ORDER && clan->clan_type != CLAN_GUILD )
      SET_BIT(victim->speaks, LANG_CLAN);

    if ( clan->clan_type != CLAN_NOKILL && clan->clan_type != CLAN_ORDER
    &&   clan->clan_type != CLAN_GUILD )
      SET_BIT( victim->pcdata->flags, PCFLAG_DEADLY );

    if ( clan->clan_type != CLAN_GUILD && clan->clan_type != CLAN_ORDER
    &&   clan->clan_type != CLAN_NOKILL )
    {
	int sn;

	for ( sn = 0; sn < top_sn; sn++ )
	{
	    if (skill_table[sn]->guild == clan->class &&
		skill_table[sn]->name != NULL )
	    {
		victim->pcdata->learned[sn] = GET_ADEPT(victim, sn);
		ch_printf( victim, "%s instructs you in the ways of %s.\n\r", ch->name, skill_table[sn]->name);
	    }
	}
    }

    victim->pcdata->clanRank = 7;
    victim->pcdata->clanZeniDonated = 0;
    victim->pcdata->clanZeniClanTax = 0;
    victim->pcdata->clanItemsDonated = 0;
    if (victim->sex == SEX_FEMALE)
    	clan->fRank7Count++;
    else
    	clan->mRank7Count++;

    victim->pcdata->clan = clan;
    STRFREE(victim->pcdata->clan_name);
    victim->pcdata->clan_name = QUICKLINK( clan->name );

      
    /*clan->clanPL += victim->exp;*/

	if (xIS_SET(victim->act, PLR_OUTCAST))
		xREMOVE_BIT(victim->act, PLR_OUTCAST);

    update_member( victim );
    act( AT_MAGIC, "You recruit $N into $t", ch, clan->name, victim, TO_CHAR );
    act( AT_MAGIC, "$n recruits $N into $t", ch, clan->name, victim, TO_NOTVICT );
    act( AT_MAGIC, "$n recruits you into $t", ch, clan->name, victim, TO_VICT );
    clan->members = clan->fRank1Count + clan->fRank2Count + clan->fRank3Count + clan->fRank4Count;
    clan->members+= clan->fRank5Count + clan->fRank6Count + clan->fRank7Count;
    clan->members+= clan->mRank1Count + clan->mRank2Count + clan->mRank3Count + clan->mRank4Count;
    clan->members+= clan->mRank5Count + clan->mRank6Count + clan->mRank7Count;
    save_char_obj( victim );
    save_clan( clan );
    return;
}

void do_council_induct( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    COUNCIL_DATA *council;

    if ( IS_NPC( ch ) || !ch->pcdata->council )
    {
	send_to_char( "Huh?\n\r", ch );
	return;
    }

    council = ch->pcdata->council;

  if ((council->head == NULL || str_cmp (ch->name, council->head))
      && ( council->head2 == NULL || str_cmp ( ch->name, council->head2 ))
      && str_cmp (council->name, "mortal council"))
    {
	send_to_char( "Huh?\n\r", ch );
	return;
    }

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Induct whom into your council?\n\r", ch );
	return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL )
    {
	send_to_char( "That player is not here.\n\r", ch);
	return;
    }

    if ( IS_NPC(victim) )
    {
	send_to_char( "Not on NPC's.\n\r", ch );
	return;
    }

/*    if ( victim->level < 51 )
    {
	send_to_char( "This player is not worthy of joining any council yet.\n\r", ch );
	return;
    }
*/
    if ( victim->pcdata->council )
    {
	send_to_char( "This player already belongs to a council!\n\r", ch );
	return;
    }

    council->members++;
    victim->pcdata->council = council;
    STRFREE(victim->pcdata->council_name);
    victim->pcdata->council_name = QUICKLINK( council->name );
    act( AT_MAGIC, "You induct $N into $t", ch, council->name, victim, TO_CHAR );
    act( AT_MAGIC, "$n inducts $N into $t", ch, council->name, victim, TO_ROOM );
    act( AT_MAGIC, "$n inducts you into $t", ch, council->name, victim, TO_VICT );
    save_char_obj( victim );
    save_council( council );
    return;
}

void do_outcast( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    CLAN_DATA *clan;
    char buf[MAX_STRING_LENGTH];

    if ( IS_NPC( ch ) || !ch->pcdata->clan )
    {
	send_to_char( "Huh?\n\r", ch );
	return;
    }

    clan = ch->pcdata->clan;

    if ( (ch->pcdata && ch->pcdata->bestowments
    &&    is_name("outcast", ch->pcdata->bestowments))
	|| is_deity(ch) || is_leader(ch))
	;
    else
    {
	send_to_char( "Huh?\n\r", ch );
	return;
    }


    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Outcast whom?\n\r", ch );
	return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL )
    {
	send_to_char( "That player is not here.\n\r", ch);
	return;
    }

    if ( IS_NPC(victim) )
    {
	send_to_char( "Not on NPC's.\n\r", ch );
	return;
    }

    if ( IS_IMMORTAL(victim) )
    {
        send_to_char( "Not on imms.\n\r", ch );
        return;
    }

    if ( victim == ch )
    {
	if ( ch->pcdata->clan->clan_type == CLAN_ORDER )
	{
	    send_to_char( "Kick yourself out of your own order?\n\r", ch );
	    return;
	}
	else
	if ( ch->pcdata->clan->clan_type == CLAN_GUILD )
	{
	    send_to_char( "Kick yourself out of your own guild?\n\r", ch );
	    return;
	}
	else
	{
	    send_to_char( "Kick yourself out of your own clan?\n\r", ch );
	    return;
	}
    }

    if ( victim->pcdata->clan != ch->pcdata->clan )
    {
	if ( ch->pcdata->clan->clan_type == CLAN_ORDER )
	{
	    send_to_char( "This player does not belong to your order!\n\r", ch );
	    return;
	}
	else
	if ( ch->pcdata->clan->clan_type == CLAN_GUILD )
	{
	    send_to_char( "This player does not belong to your guild!\n\r", ch );
	    return;
	}
	else
	{
	    send_to_char( "This player does not belong to your clan!\n\r", ch );
	    return;
	}
    }

    if (is_leader(victim) || is_deity(victim))
	{
	    send_to_char( "You can't outcast that player.\n\r", ch );
	    return;
	}

    if ( clan->clan_type != CLAN_GUILD && clan->clan_type != CLAN_ORDER
    &&   clan->clan_type != CLAN_NOKILL )
    {
	int sn;

	for ( sn = 0; sn < top_sn; sn++ )
	    if ( skill_table[sn]->guild == victim->pcdata->clan->class
	    &&   skill_table[sn]->name != NULL )
	    {
		victim->pcdata->learned[sn] = 0;
		ch_printf( victim, "You forget the ways of %s.\n\r", skill_table[sn]->name);
	    }
    }

    if ( victim->speaking & LANG_CLAN )
        victim->speaking = LANG_COMMON;
    REMOVE_BIT( victim->speaks, LANG_CLAN );
    clan->members--;

		switch (victim->pcdata->clanRank)
		{
			default:
				break;
			case 7:
				if (victim->sex == SEX_FEMALE)
					clan->fRank7Count--;
				else
					clan->mRank7Count--;
				break;
			case 6:
				if (victim->sex == SEX_FEMALE)
					clan->fRank6Count--;
				else
					clan->mRank6Count--;
				break;
			case 5:
				if (victim->sex == SEX_FEMALE)
					clan->fRank5Count--;
				else
					clan->mRank5Count--;
				break;
			case 4:
				if (victim->sex == SEX_FEMALE)
					clan->fRank4Count--;
				else
					clan->mRank4Count--;
				break;
			case 3:
				if (victim->sex == SEX_FEMALE)
					clan->fRank3Count--;
				else
					clan->mRank3Count--;
				break;
			case 2:
				if (victim->sex == SEX_FEMALE)
					clan->fRank2Count--;
				else
					clan->mRank2Count--;
				break;
			case 1:
				if (victim->sex == SEX_FEMALE)
					clan->fRank1Count--;
				else
					clan->mRank1Count--;
				break;
		}
    victim->pcdata->clanRank = 0;
    victim->pcdata->clan = NULL;
    remove_member( victim );
    STRFREE(victim->pcdata->clan_name);
    victim->pcdata->clan_name = STRALLOC( "" );

    
    /*clan->clanPL -= victim->exp;*/

	xSET_BIT(victim->act, PLR_OUTCAST);
    act( AT_MAGIC, "You outcast $N from $t", ch, clan->name, victim, TO_CHAR );
    act( AT_MAGIC, "$n outcasts $N from $t", ch, clan->name, victim, TO_ROOM );
    act( AT_MAGIC, "$n outcasts you from $t", ch, clan->name, victim, TO_VICT );
    if ( clan->clan_type != CLAN_GUILD
    &&   clan->clan_type != CLAN_ORDER )
    {
	sprintf(buf, "%s has been outcast from %s!", victim->name, clan->name);
	echo_to_all(AT_MAGIC, buf, ECHOTAR_ALL);
    }

    save_char_obj( victim );	/* clan gets saved when pfile is saved */
    save_clan( clan );
    return;
}

void do_council_outcast( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    COUNCIL_DATA *council;

    if ( IS_NPC( ch ) || !ch->pcdata->council )
    {
	send_to_char( "Huh?\n\r", ch );
	return;
    }

    council = ch->pcdata->council;

  if ((council->head == NULL || str_cmp (ch->name, council->head))
      && ( council->head2 == NULL || str_cmp ( ch->name, council->head2 ))
      && str_cmp (council->name, "mortal council"))
    {
	send_to_char( "Huh?\n\r", ch );
	return;
    }

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Outcast whom from your council?\n\r", ch );
	return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL )
    {
	send_to_char( "That player is not here.\n\r", ch);
	return;
    }

    if ( IS_NPC(victim) )
    {
	send_to_char( "Not on NPC's.\n\r", ch );
	return;
    }

    if ( victim == ch )
    {
	send_to_char( "Kick yourself out of your own council?\n\r", ch );
	return;
    }

    if ( victim->pcdata->council != ch->pcdata->council )
    {
	send_to_char( "This player does not belong to your council!\n\r", ch );
	return;
    }

    --council->members;
    victim->pcdata->council = NULL;
    STRFREE(victim->pcdata->council_name);
    victim->pcdata->council_name = STRALLOC( "" );
    act( AT_MAGIC, "You outcast $N from $t", ch, council->name, victim, TO_CHAR );
    act( AT_MAGIC, "$n outcasts $N from $t", ch, council->name, victim, TO_ROOM );
    act( AT_MAGIC, "$n outcasts you from $t", ch, council->name, victim, TO_VICT );
    save_char_obj( victim );
    save_council( council );
    return;
}

bool is_owner( CHAR_DATA *ch, CLAN_DATA *clan )
{
    if( ch->pcdata->clan == clan )
    {
	if( !str_cmp(ch->name,clan->owner) )
	  return TRUE;
    }
    return FALSE;
}

void do_setclan( CHAR_DATA *ch, char *argument )
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    CLAN_DATA *clan;

    set_char_color( AT_PLAIN, ch );
    if ( IS_NPC( ch ) )
    {
	send_to_char( "Huh?\n\r", ch );
	return;
    }

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    if ( arg1[0] == '\0' )
    {
	send_to_char( "Usage: setclan <clan> <field> <#/player>\n\r", ch );
	send_to_char( "\n\rField being one of:\n\r", ch );
	send_to_char( " admin leader1-6 mrank1-7 frank1-7\n\r", ch );
	send_to_char( " members board recall storage guard1 guard2\n\r", ch );
	send_to_char( " align memlimit deathrecall morgue donate\n\r", ch );
	send_to_char( " jail skill1-3 url tax bank active\n\r", ch );
	send_to_char( " obj1 obj2 obj3 obj4 obj5\n\r", ch );
	send_to_char( " mrankc1-7 frankc1-7\n\r", ch );
	if ( get_trust( ch ) >= LEVEL_GOD )
	{
	  send_to_char( " name filename motto desc clanhq\n\r", ch );
	  send_to_char( " short favour strikes type class\n\r", ch );
	  send_to_char( " owner \n\r", ch );
	}
	if ( get_trust( ch ) >= LEVEL_IMPLEMENTOR )
	  send_to_char(" pkill1-7 pdeath1-7 clanpl\n\r", ch );
	return;
    }

    clan = get_clan( arg1 );
    if ( !clan )
    {
	send_to_char( "No such clan.\n\r", ch );
	return;
    }
	if (!str_cmp(arg2, "clanhq"))
	{
		AREA_DATA * clanHQ;
		for (clanHQ = first_area; clanHQ; clanHQ = clanHQ->next)
		{
			if (!str_cmp(clanHQ->filename, argument))
			{
				clan->clanHQ = clanHQ;
				send_to_char( "Done.\n\r", ch );
				return;
			}
		}
		send_to_char( "Area filename not found.\n\r", ch );
		return;
	}
    if ( !str_cmp( arg2, "active" ) )
    {
	clan->activeMem = atoi( argument );
	send_to_char( "Done.\n\r", ch );
	save_clan( clan );
	return;
    }
    /*if ( !str_cmp( arg2, "clanpl" ) )
    {
	clan->clanPL = atof( argument );
	send_to_char( "Done.\n\r", ch );
	save_clan( clan );
	return;
    }*/
    if ( !str_cmp( arg2, "admin" ) )
    {
	STRFREE( clan->deity );
	clan->deity = STRALLOC( argument );
	send_to_char( "Done.\n\r", ch );
	save_clan( clan );
	return;
    }
    if ( !str_cmp( arg2, "leader1" ) )
    {
	STRFREE( clan->leader1 );
	clan->leader1 = STRALLOC( argument );
	send_to_char( "Done.\n\r", ch );
	save_clan( clan );
	return;
    }
    if ( !str_cmp( arg2, "owner" ) )
    {
        STRFREE( clan->owner );
        clan->owner = STRALLOC( argument );
        send_to_char( "Done.\n\r", ch );
        save_clan( clan );
        return;
    }
    if ( !str_cmp( arg2, "leader2" ) )
    {
	STRFREE( clan->leader2 );
	clan->leader2 = STRALLOC( argument );
	send_to_char( "Done.\n\r", ch );
	save_clan( clan );
	return;
    }
    if ( !str_cmp( arg2, "leader3" ) )
    {
	STRFREE( clan->leader3 );
	clan->leader3 = STRALLOC( argument );
	send_to_char( "Done.\n\r", ch );
	save_clan( clan );
	return;
    }
    if ( !str_cmp( arg2, "leader4" ) )
    {
	STRFREE( clan->leader4 );
	clan->leader4 = STRALLOC( argument );
	send_to_char( "Done.\n\r", ch );
	save_clan( clan );
	return;
    }
    if ( !str_cmp( arg2, "leader5" ) )
    {
	STRFREE( clan->leader5 );
	clan->leader5 = STRALLOC( argument );
	send_to_char( "Done.\n\r", ch );
	save_clan( clan );
	return;
    }
    if ( !str_cmp( arg2, "leader6" ) )
    {
	STRFREE( clan->leader6 );
	clan->leader6 = STRALLOC( argument );
	send_to_char( "Done.\n\r", ch );
	save_clan( clan );
	return;
    }
    if ( !str_cmp( arg2, "mrankc1" ) )
    {
	clan->mRank1Count = atoi( argument );
	send_to_char( "Done.\n\r", ch );
	save_clan( clan );
	return;
    }
    if ( !str_cmp( arg2, "mrankc2" ) )
    {
	clan->mRank2Count = atoi( argument );
	send_to_char( "Done.\n\r", ch );
	save_clan( clan );
	return;
    }
    if ( !str_cmp( arg2, "mrankc3" ) )
    {
	clan->mRank3Count = atoi( argument );
	send_to_char( "Done.\n\r", ch );
	save_clan( clan );
	return;
    }
    if ( !str_cmp( arg2, "mrankc4" ) )
    {
	clan->mRank4Count = atoi( argument );
	send_to_char( "Done.\n\r", ch );
	save_clan( clan );
	return;
    }
    if ( !str_cmp( arg2, "mrankc5" ) )
    {
	clan->mRank5Count = atoi( argument );
	send_to_char( "Done.\n\r", ch );
	save_clan( clan );
	return;
    }
    if ( !str_cmp( arg2, "mrankc6" ) )
    {
	clan->mRank6Count = atoi( argument );
	send_to_char( "Done.\n\r", ch );
	save_clan( clan );
	return;
    }
    if ( !str_cmp( arg2, "mrankc7" ) )
    {
	clan->mRank7Count = atoi( argument );
	send_to_char( "Done.\n\r", ch );
	save_clan( clan );
	return;
    }
    if ( !str_cmp( arg2, "fRankc1" ) )
    {
	clan->fRank1Count = atoi( argument );
	send_to_char( "Done.\n\r", ch );
	save_clan( clan );
	return;
    }
    if ( !str_cmp( arg2, "fRankc2" ) )
    {
	clan->fRank2Count = atoi( argument );
	send_to_char( "Done.\n\r", ch );
	save_clan( clan );
	return;
    }
    if ( !str_cmp( arg2, "fRankc3" ) )
    {
	clan->fRank3Count = atoi( argument );
	send_to_char( "Done.\n\r", ch );
	save_clan( clan );
	return;
    }
    if ( !str_cmp( arg2, "fRankc4" ) )
    {
	clan->fRank4Count = atoi( argument );
	send_to_char( "Done.\n\r", ch );
	save_clan( clan );
	return;
    }
    if ( !str_cmp( arg2, "fRankc5" ) )
    {
	clan->fRank5Count = atoi( argument );
	send_to_char( "Done.\n\r", ch );
	save_clan( clan );
	return;
    }
    if ( !str_cmp( arg2, "fRankc6" ) )
    {
	clan->fRank6Count = atoi( argument );
	send_to_char( "Done.\n\r", ch );
	save_clan( clan );
	return;
    }
    if ( !str_cmp( arg2, "fRankc7" ) )
    {
	clan->fRank7Count = atoi( argument );
	send_to_char( "Done.\n\r", ch );
	save_clan( clan );
	return;
    }
    if ( !str_cmp( arg2, "mrank1" ) )
    {
	STRFREE( clan->mRank1 );
	clan->mRank1 = STRALLOC( argument );
	send_to_char( "Done.\n\r", ch );
	save_clan( clan );
	return;
    }
    if ( !str_cmp( arg2, "mrank2" ) )
    {
	STRFREE( clan->mRank2 );
	clan->mRank2 = STRALLOC( argument );
	send_to_char( "Done.\n\r", ch );
	save_clan( clan );
	return;
    }
    if ( !str_cmp( arg2, "mrank3" ) )
    {
	STRFREE( clan->mRank3 );
	clan->mRank3 = STRALLOC( argument );
	send_to_char( "Done.\n\r", ch );
	save_clan( clan );
	return;
    }
    if ( !str_cmp( arg2, "mrank4" ) )
    {
	STRFREE( clan->mRank4 );
	clan->mRank4 = STRALLOC( argument );
	send_to_char( "Done.\n\r", ch );
	save_clan( clan );
	return;
    }
    if ( !str_cmp( arg2, "mrank5" ) )
    {
	STRFREE( clan->mRank5 );
	clan->mRank5 = STRALLOC( argument );
	send_to_char( "Done.\n\r", ch );
	save_clan( clan );
	return;
    }
    if ( !str_cmp( arg2, "mrank6" ) )
    {
	STRFREE( clan->mRank6 );
	clan->mRank6 = STRALLOC( argument );
	send_to_char( "Done.\n\r", ch );
	save_clan( clan );
	return;
    }
    if ( !str_cmp( arg2, "mrank7" ) )
    {
	STRFREE( clan->mRank7 );
	clan->mRank7 = STRALLOC( argument );
	send_to_char( "Done.\n\r", ch );
	save_clan( clan );
	return;
    }
    if ( !str_cmp( arg2, "fRank1" ) )
    {
	STRFREE( clan->fRank1 );
	clan->fRank1 = STRALLOC( argument );
	send_to_char( "Done.\n\r", ch );
	save_clan( clan );
	return;
    }
    if ( !str_cmp( arg2, "fRank2" ) )
    {
	STRFREE( clan->fRank2 );
	clan->fRank2 = STRALLOC( argument );
	send_to_char( "Done.\n\r", ch );
	save_clan( clan );
	return;
    }
    if ( !str_cmp( arg2, "fRank3" ) )
    {
	STRFREE( clan->fRank3 );
	clan->fRank3 = STRALLOC( argument );
	send_to_char( "Done.\n\r", ch );
	save_clan( clan );
	return;
    }
    if ( !str_cmp( arg2, "fRank4" ) )
    {
	STRFREE( clan->fRank4 );
	clan->fRank4 = STRALLOC( argument );
	send_to_char( "Done.\n\r", ch );
	save_clan( clan );
	return;
    }
    if ( !str_cmp( arg2, "fRank5" ) )
    {
	STRFREE( clan->fRank5 );
	clan->fRank5 = STRALLOC( argument );
	send_to_char( "Done.\n\r", ch );
	save_clan( clan );
	return;
    }
    if ( !str_cmp( arg2, "fRank6" ) )
    {
	STRFREE( clan->fRank6 );
	clan->fRank6 = STRALLOC( argument );
	send_to_char( "Done.\n\r", ch );
	save_clan( clan );
	return;
    }
    if ( !str_cmp( arg2, "fRank7" ) )
    {
	STRFREE( clan->fRank7 );
	clan->fRank7 = STRALLOC( argument );
	send_to_char( "Done.\n\r", ch );
	save_clan( clan );
	return;
    }
    if ( !str_cmp( arg2, "badge" ) )
    {
	STRFREE( clan->badge );
	clan->badge = STRALLOC( argument );
	send_to_char( "Done.\n\r", ch );
	save_clan( clan );
	return;
    }
    if ( !str_cmp( arg2, "url" ) )
    {
	STRFREE( clan->clanWebSite );
	clan->clanWebSite = STRALLOC( argument );
	send_to_char( "Done.\n\r", ch );
	save_clan( clan );
	return;
    }
    if ( !str_cmp( arg2, "tax" ) )
    {
	clan->tax = atoi( argument );
	send_to_char( "Done.\n\r", ch );
	save_clan( clan );
	return;
    }
    if ( !str_cmp( arg2, "bank" ) )
    {
	clan->bank = atof( argument );
	send_to_char( "Done.\n\r", ch );
	save_clan( clan );
	return;
    }
    if ( !str_cmp( arg2, "board" ) )
    {
	clan->board = atoi( argument );
	send_to_char( "Done.\n\r", ch );
	save_clan( clan );
	return;
    }
    if ( !str_cmp( arg2, "memlimit") )
    {
    	clan->mem_limit = atoi( argument );
	send_to_char( "Done.\n\r", ch  );
	save_clan( clan );
	return;
    }
    if ( !str_cmp( arg2, "members" ) )
    {
	clan->members = atoi( argument );
	send_to_char( "Done.\n\r", ch );
	save_clan( clan );
	return;
    }
    if ( !str_cmp( arg2, "recall" ) )
    {
	clan->recall = atoi( argument );
	send_to_char( "Done.\n\r", ch );
	save_clan( clan );
	return;
    }
    if ( !str_cmp( arg2, "storage" ) )
    {
	clan->storeroom = atoi( argument );
	send_to_char( "Done.\n\r", ch );
	save_clan( clan );
	return;
    }
    if ( !str_cmp( arg2, "deathrecall" ) )
    {
	clan->deathRecall = atoi( argument );
	send_to_char( "Done.\n\r", ch );
	save_clan( clan );
	return;
    }
    if ( !str_cmp( arg2, "jail" ) )
    {
	clan->clanJail = atoi( argument );
	send_to_char( "Done.\n\r", ch );
	save_clan( clan );
	return;
    }
    if ( !str_cmp( arg2, "morgue" ) )
    {
	clan->clanMorgue = atoi( argument );
	send_to_char( "Done.\n\r", ch );
	save_clan( clan );
	return;
    }
    if ( !str_cmp( arg2, "donate" ) )
    {
	clan->clanDonate = atoi( argument );
	send_to_char( "Done.\n\r", ch );
	save_clan( clan );
	return;
    }
    if ( !str_cmp( arg2, "skill1" ) )
    {
	clan->clanSkill1 = atoi( argument );
	send_to_char( "Done.\n\r", ch );
	save_clan( clan );
	return;
    }
    if ( !str_cmp( arg2, "skill2" ) )
    {
	clan->clanSkill2 = atoi( argument );
	send_to_char( "Done.\n\r", ch );
	save_clan( clan );
	return;
    }
    if ( !str_cmp( arg2, "skill3" ) )
    {
	clan->clanSkill3 = atoi( argument );
	send_to_char( "Done.\n\r", ch );
	save_clan( clan );
	return;
    }
    if ( !str_cmp( arg2, "obj1" ) )
    {
	clan->clanobj1 = atoi( argument );
	send_to_char( "Done.\n\r", ch );
	save_clan( clan );
	return;
    }
    if ( !str_cmp( arg2, "obj2" ) )
    {
	clan->clanobj2 = atoi( argument );
	send_to_char( "Done.\n\r", ch );
	save_clan( clan );
	return;
    }
    if ( !str_cmp( arg2, "obj3" ) )
    {
	clan->clanobj3 = atoi( argument );
	send_to_char( "Done.\n\r", ch );
	save_clan( clan );
	return;
    }
    if ( !str_cmp( arg2, "obj4" ) )
    {
        clan->clanobj4 = atoi( argument );
        send_to_char( "Done.\n\r", ch );
        save_clan( clan );
        return;
    }
    if ( !str_cmp( arg2, "obj5" ) )
    {
        clan->clanobj5 = atoi( argument );
        send_to_char( "Done.\n\r", ch );
        save_clan( clan );
        return;
    }
    if ( !str_cmp( arg2, "guard1" ) )
    {
	clan->guard1 = atoi( argument );
	send_to_char( "Done.\n\r", ch );
	save_clan( clan );
	return;
    }
    if ( !str_cmp( arg2, "guard2" ) )
    {
	clan->guard2 = atoi( argument );
	send_to_char( "Done.\n\r", ch );
	save_clan( clan );
	return;
    }
    if ( get_trust( ch ) < LEVEL_GOD )
    {
	do_setclan( ch, "" );
	return;
    }
    if ( !str_cmp( arg2, "align" ) )
    {
	clan->alignment = atoi( argument );
	send_to_char( "Done.\n\r", ch );
	save_clan( clan );
	return;
    }
    if ( !str_cmp( arg2, "type" ) )
    {
	if ( !str_cmp( argument, "order" ) )
	  clan->clan_type = CLAN_ORDER;
	else
	if ( !str_cmp( argument, "guild" ) )
	  clan->clan_type = CLAN_GUILD;
	else
	  clan->clan_type = atoi( argument );
	send_to_char( "Done.\n\r", ch );
	save_clan( clan );
	return;
    }
    if ( !str_cmp( arg2, "class" ) )
    {
	clan->class = atoi( argument );
	send_to_char( "Done.\n\r", ch );
	save_clan( clan );
	return;
    }
    if ( !str_cmp( arg2, "name" ) )
    {
	STRFREE( clan->name );
	clan->name = STRALLOC( argument );
	send_to_char( "Done.\n\r", ch );
	save_clan( clan );
	return;
    }
    if ( !str_cmp( arg2, "short" ) )
    {
	STRFREE( clan->short_name );
	clan->short_name = STRALLOC( argument );
	send_to_char( "Done.\n\r", ch );
	save_clan( clan );
	return;
    }
    if ( !str_cmp( arg2, "filename" ) )
    {
        if ( clan->filename && clan->filename[0] != '\0' )
                DISPOSE( clan->filename );
	clan->filename = str_dup( argument );
	send_to_char( "Done.\n\r", ch );
	save_clan( clan );
	write_clan_list( );
	return;
    }
    if ( !str_cmp( arg2, "motto" ) )
    {
	STRFREE( clan->motto );
	clan->motto = STRALLOC( argument );
	send_to_char( "Done.\n\r", ch );
	save_clan( clan );
	return;
    }
    if ( !str_cmp( arg2, "desc" ) )
    {
	STRFREE( clan->description );
	clan->description = STRALLOC( argument );
	send_to_char( "Done.\n\r", ch );
	save_clan( clan );
	return;
    }
    if ( get_trust( ch ) < LEVEL_IMPLEMENTOR )
    {
        do_setclan( ch, "" );
        return;
    }

    if ( !str_prefix( "pkill", arg2) )
    {
	int temp_value;
	if ( !str_cmp( arg2, "pkill1" ) )
		temp_value = 0;
	else if ( !str_cmp( arg2, "pkill2" ) )
		temp_value = 1;
	else if ( !str_cmp( arg2, "pkill3" ) )
		temp_value = 2;
	else if ( !str_cmp( arg2, "pkill4" ) )
		temp_value = 3;
	else if ( !str_cmp( arg2, "pkill5" ) )
		temp_value = 4;
	else if ( !str_cmp( arg2, "pkill6" ) )
		temp_value = 5;
	else if ( !str_cmp( arg2, "pkill7" ) )
		temp_value = 6;
	else
	{
		do_setclan ( ch, "" );
		return;
	}
	clan->pkills[temp_value] = atoi( argument );
	send_to_char ("Ok.\n\r", ch );
	return;
	}
    if ( !str_prefix( "spar_wins", arg2) )
    {
	int temp_value;
	if ( !str_cmp( arg2, "spar_wins1" ) )
		temp_value = 0;
	else if ( !str_cmp( arg2, "spar_wins2" ) )
		temp_value = 1;
	else if ( !str_cmp( arg2, "spar_wins3" ) )
		temp_value = 2;
	else if ( !str_cmp( arg2, "spar_wins4" ) )
		temp_value = 3;
	else if ( !str_cmp( arg2, "spar_wins5" ) )
		temp_value = 4;
	else if ( !str_cmp( arg2, "spar_wins6" ) )
		temp_value = 5;
	else if ( !str_cmp( arg2, "spar_wins7" ) )
		temp_value = 6;
	else
	{
		do_setclan ( ch, "" );
		return;
	}
	clan->spar_wins[temp_value] = atoi( argument );
	send_to_char ("Ok.\n\r", ch );
	return;
	}

    if ( !str_prefix( "spar_loss", arg2) )
    {
	int temp_value;
	if ( !str_cmp( arg2, "spar_loss1" ) )
		temp_value = 0;
	else if ( !str_cmp( arg2, "spar_loss2" ) )
		temp_value = 1;
	else if ( !str_cmp( arg2, "spar_loss3" ) )
		temp_value = 2;
	else if ( !str_cmp( arg2, "spar_loss4" ) )
		temp_value = 3;
	else if ( !str_cmp( arg2, "spar_loss5" ) )
		temp_value = 4;
	else if ( !str_cmp( arg2, "spar_loss6" ) )
		temp_value = 5;
	else if ( !str_cmp( arg2, "spar_loss7" ) )
		temp_value = 6;
	else
	{
		do_setclan ( ch, "" );
		return;
	}
	clan->spar_loss[temp_value] = atoi( argument );
	send_to_char ("Ok.\n\r", ch );
	return;
    }

    if ( !str_prefix( "pdeath", arg2) )
    {
	int temp_value;
	if ( !str_cmp( arg2, "pdeath1" ) )
		temp_value = 0;
	else if ( !str_cmp( arg2, "pdeath2" ) )
		temp_value = 1;
	else if ( !str_cmp( arg2, "pdeath3" ) )
		temp_value = 2;
	else if ( !str_cmp( arg2, "pdeath4" ) )
		temp_value = 3;
	else if ( !str_cmp( arg2, "pdeath5" ) )
		temp_value = 4;
	else if ( !str_cmp( arg2, "pdeath6" ) )
		temp_value = 5;
	else if ( !str_cmp( arg2, "pdeath7" ) )
		temp_value = 6;
	else
	{
		do_setclan ( ch, "" );
		return;
	}
	clan->pdeaths[temp_value] = atoi( argument );
	send_to_char ("Ok.\n\r", ch );
	return;
    }
    do_setclan( ch, "" );
    return;

}

void do_setcouncil( CHAR_DATA *ch, char *argument )
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    COUNCIL_DATA *council;

    set_char_color( AT_PLAIN, ch );

    if ( IS_NPC( ch ) )
    {
	send_to_char( "Huh?\n\r", ch );
	return;
    }

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    if ( arg1[0] == '\0' )
    {
	send_to_char( "Usage: setcouncil <council> <field> <deity|leader|number1|number2> <player>\n\r", ch );
	send_to_char( "\n\rField being one of:\n\r", ch );
	send_to_char( " head head2 members board meeting\n\r", ch );
	if ( get_trust( ch ) >= LEVEL_GOD )
	  send_to_char( " name filename desc\n\r", ch );
        if ( get_trust( ch ) >= LEVEL_SUB_IMPLEM )
	  send_to_char( " powers\n\r", ch);
	return;
    }

    council = get_council( arg1 );
    if ( !council )
    {
	send_to_char( "No such council.\n\r", ch );
	return;
    }
    if ( !str_cmp( arg2, "head" ) )
    {
	STRFREE( council->head );
	council->head = STRALLOC( argument );
	send_to_char( "Done.\n\r", ch );
	save_council( council );
	return;
    }

  if (!str_cmp (arg2, "head2"))
    {
      if ( council->head2 != NULL )
        STRFREE (council->head2);
      if ( !str_cmp ( argument, "none" ) || !str_cmp ( argument, "clear" ) )
        council->head2 = NULL;
      else
        council->head2 = STRALLOC (argument);
      send_to_char ("Done.\n\r", ch);
      save_council (council);
      return;
    }
    if ( !str_cmp( arg2, "board" ) )
    {
	council->board = atoi( argument );
	send_to_char( "Done.\n\r", ch );
	save_council( council );
	return;
    }
    if ( !str_cmp( arg2, "members" ) )
    {
	council->members = atoi( argument );
	send_to_char( "Done.\n\r", ch );
	save_council( council );
	return;
    }
    if ( !str_cmp( arg2, "meeting" ) )
    {
	council->meeting = atoi( argument );
	send_to_char( "Done.\n\r", ch );
	save_council( council );
	return;
    }
    if ( get_trust( ch ) < LEVEL_GOD )
    {
	do_setcouncil( ch, "" );
	return;
    }
    if ( !str_cmp( arg2, "name" ) )
    {
	STRFREE( council->name );
	council->name = STRALLOC( argument );
	send_to_char( "Done.\n\r", ch );
	save_council( council );
	return;
    }
    if ( !str_cmp( arg2, "filename" ) )
    {
        if ( council->filename && council->filename[0] != '\0' )
                DISPOSE( council->filename );
	council->filename = str_dup( argument );
	send_to_char( "Done.\n\r", ch );
	save_council( council );
	write_council_list( );
	return;
    }
    if ( !str_cmp( arg2, "desc" ) )
    {
	STRFREE( council->description );
	council->description = STRALLOC( argument );
	send_to_char( "Done.\n\r", ch );
	save_council( council );
	return;
    }
    if ( get_trust( ch ) < LEVEL_SUB_IMPLEM )
    {
	do_setcouncil( ch, "" );
	return;
    }
    if ( !str_cmp( arg2, "powers" ) )
    {
	STRFREE( council->powers );
	council->powers = STRALLOC( argument );
	send_to_char( "Done.\n\r", ch );
	save_council( council );
	return;
    }

    do_setcouncil( ch, "" );
    return;
}

/*
 * Added multiple levels on pkills and pdeaths. -- Shaddai
 */

void do_showclan( CHAR_DATA *ch, char *argument )
{
    CLAN_DATA *clan;

    set_char_color( AT_PLAIN, ch );

    if ( IS_NPC( ch ) )
    {
	send_to_char( "Huh?\n\r", ch );
	return;
    }
    if ( argument[0] == '\0' )
    {
	send_to_char( "Usage: showclan <clan>\n\r", ch );
	return;
    }

    clan = get_clan( argument );
    if ( !clan )
    {
	send_to_char( "No such clan, guild or order.\n\r", ch );
	return;
    }

    ch_printf( ch, "\n\r&w%s    : &W%s\t\t&wShort Name: &W%s\t\t&wBadge: %s\n\r&wFilename : &W%s\n\r&wMotto    : &W%s\n\r",
			clan->clan_type == CLAN_ORDER ? "Order" :
		       (clan->clan_type == CLAN_GUILD ? "Guild" : "Clan "),
    			clan->name, clan->short_name,
			clan->badge ? clan->badge : "(not set)",
    			clan->filename,
    			clan->motto );
    ch_printf( ch, "&wDesc     : &W%s\n\r&wDeity    : &W%s\n\r",
    			clan->description,
    			clan->deity );
    ch_printf( ch, "&wOwner: &W%-15.15s\n\r",clan->owner);

    ch_printf( ch, "&wLeader1: &W%-15.15s  &wLeader2: &W%-15.15s  &wLeader3: &W%-15.15s",
			clan->leader1, clan->leader2, clan->leader3 );
    ch_printf( ch, "&wLeader4: &W%-15.15s  &wLeader5: &W%-15.15s  &wLeader6: &W%-15.15s",
			clan->leader4, clan->leader5, clan->leader6 );
    ch_printf( ch, "&wPKills   : &w  5k-10k :&W%-3d &w 10k-100k:&W%-3d &w100k-1m  :&W%-3d &w  1m-10m :&W%-3d\n         : &w 10m-100m:&W%-3d &w100m-1b  :&W%-3d &w    +1b  :&W%-3d\n\r",
    			clan->pkills[0], clan->pkills[1], clan->pkills[2],
    			clan->pkills[3], clan->pkills[4], clan->pkills[5],
			clan->pkills[6]);
    ch_printf( ch, "&wPDeaths  : &w  5k-10k :&W%-3d &w 10k-100k:&W%-3d &w100k-1m  :&W%-3d &w  1m-10m :&W%-3d\n         : &w 10m-100m:&W%-3d &w100m-1b  :&W%-3d &w    +1b  :&W%-3d\n\r",
    			clan->pdeaths[0], clan->pdeaths[1], clan->pdeaths[2],
    			clan->pdeaths[3], clan->pdeaths[4], clan->pdeaths[5],
			clan->pdeaths[6] );
    ch_printf( ch, "&wSpar_Wins: &w  5k-10k :&W%-3d &w 10k-100k:&W%-3d &w100k-1m  :&W%-3d &w  1m-10m :&W%-3d\n         : &w 10m-100m:&W%-3d &w100m-1b  :&W%-3d &w    +1b  :&W%-3d\n\r",
    			clan->spar_wins[0], clan->spar_wins[1], clan->spar_wins[2],
    			clan->spar_wins[3], clan->spar_wins[4], clan->spar_wins[5],
			clan->spar_wins[6]);
    ch_printf( ch, "&wSpar_Loss: &w  5k-10k :&W%-3d &w 10k-100k:&W%-3d &w100k-1m  :&W%-3d &w  1m-10m :&W%-3d\n         : &w 10m-100m:&W%-3d &w100m-1b  :&W%-3d &w    +1b  :&W%-3d\n\r",
    			clan->spar_loss[0], clan->spar_loss[1], clan->spar_loss[2],
    			clan->spar_loss[3], clan->spar_loss[4], clan->spar_loss[5],
			clan->spar_loss[6]);
    ch_printf( ch, "&wIllegalPK: &W%-6d\n\r",
			clan->illegal_pk );
    ch_printf( ch, "&wMKills   : &W%-6d   &wMDeaths: &W%-6d\n\r",
    			clan->mkills,
    			clan->mdeaths );
    ch_printf( ch, "&wScore    : &W%-6d   &wFavor  : &W%-6d   &wStrikes: &W%d\n\r",
    			clan->score,
    			clan->favour,
    			clan->strikes );
    ch_printf( ch, "&wMembers  : &W%-6d  &wMemLimit: &W%-6d   &wActive: &W%-6d   &wAlign  : &W%-6d",
    			clan->members,
    			clan->mem_limit,
    			clan->activeMem,
    			clan->alignment );
    if ( clan->clan_type == CLAN_GUILD )
	ch_printf( ch, "   &wClass  : &W%d &w(&W%s&w)",
			clan->class,
    			clan->class<MAX_PC_CLASS?class_table[clan->class]->who_name:
			"unknown" );
    send_to_char( "\n\r", ch );
    ch_printf( ch, "&wBoard    : &W%-5d    &wRecall : &W%-5d    &wStorage: &W%-5d\n\r",
			clan->board,
			clan->recall,
			clan->storeroom );
    ch_printf( ch, "&wDeathRecall:  &W%-5d    &wMorgue : &W%-5d    &wDonate: &W%-5d\n\r",
			clan->board,
			clan->recall,
			clan->storeroom );
	ch_printf(ch, "&wJail: &W%-5d  &wTax: &W%-5d", clan->clanJail, clan->tax);
	ch_printf(ch, "&wBank: &W%s", num_punct_ld(clan->bank));
    ch_printf( ch, "&wGuard1   : &W%-5d    &wGuard2 : &W%-5d\n\r",
 			clan->guard1,
			clan->guard2 );
    ch_printf( ch, "&wObj1( &W%d &w)  Obj2( &W%d &w)  Obj3( &W%d &w)  Obj4( &W%d &w)  Obj5( &W%d &w)\n\r",
    			clan->clanobj1,
    			clan->clanobj2,
    			clan->clanobj3,
			clan->clanobj4,
			clan->clanobj5 );
    ch_printf( ch, "&wSkill1( &W%d &w)  Skill2( &W%d &w)  Skill3( &W%d &w)\n\r",
    			clan->clanSkill1,
    			clan->clanSkill2,
    			clan->clanSkill3);
    /*ch_printf(ch, "&wTotal Clan Power Level: &W%s", num_punct_ld(clan->clanPL));*/
	ch_printf(ch, "        Male Rankings               Female Rankings\n\r");
	ch_printf(ch, "  [ %-20.20s ]     [ %-20.20s ]\n\r", clan->mRank1, clan->fRank1);
	ch_printf(ch, "  [ %-20.20s ]     [ %-20.20s ]\n\r", clan->mRank2, clan->fRank2);
	ch_printf(ch, "  [ %-20.20s ]     [ %-20.20s ]\n\r", clan->mRank3, clan->fRank3);
	ch_printf(ch, "  [ %-20.20s ]     [ %-20.20s ]\n\r", clan->mRank4, clan->fRank4);
	ch_printf(ch, "  [ %-20.20s ]     [ %-20.20s ]\n\r", clan->mRank5, clan->fRank5);
	ch_printf(ch, "  [ %-20.20s ]     [ %-20.20s ]\n\r", clan->mRank6, clan->fRank6);
	ch_printf(ch, "  [ %-20.20s ]     [ %-20.20s ]\n\r", clan->mRank7, clan->fRank7);

	ch_printf(ch, "show clan alliances here\n\r");

    return;
}

void do_showcouncil( CHAR_DATA *ch, char *argument )
{
    COUNCIL_DATA *council;

    set_char_color( AT_PLAIN, ch );

    if ( IS_NPC( ch ) )
    {
	send_to_char( "Huh?\n\r", ch );
	return;
    }
    if ( argument[0] == '\0' )
    {
	send_to_char( "Usage: showcouncil <council>\n\r", ch );
	return;
    }

    council = get_council( argument );
    if ( !council )
    {
	send_to_char( "No such council.\n\r", ch );
	return;
    }

    ch_printf_color( ch, "\n\r&wCouncil :  &W%s\n\r&wFilename:  &W%s\n\r",
    			council->name,
    			council->filename );
  ch_printf_color (ch, "&wHead:      &W%s\n\r", council->head );
  ch_printf_color (ch, "&wHead2:     &W%s\n\r", council->head2 );
  ch_printf_color (ch, "&wMembers:   &W%-d\n\r", council->members );
    ch_printf_color( ch, "&wBoard:     &W%-5d\n\r&wMeeting:   &W%-5d\n\r&wPowers:    &W%s\n\r",
    			council->board,
    			council->meeting,
			council->powers );
    ch_printf_color( ch, "&wDescription:\n\r&W%s\n\r", council->description );
    return;
}

void do_makeclan( CHAR_DATA *ch, char *argument )
{
    CLAN_DATA *clan;
    CLAN_DATA *aclan;
    ALLIANCE_DATA *alliance;

    set_char_color( AT_IMMORT, ch );

    if ( !argument || argument[0] == '\0' )
    {
	send_to_char( "Usage: makeclan <clan name>\n\r", ch );
	return;
    }

    set_char_color( AT_PLAIN, ch );
    clan = get_clan( argument );
    if ( clan )
     {
	send_to_char( "There is already a clan with that name.\n\r", ch );
	return;
     }

    CREATE( clan, CLAN_DATA, 1 );
    LINK( clan, first_clan, last_clan, next, prev );

    clan->name		= STRALLOC( argument );
    clan->short_name= STRALLOC( "None" );
    
    /*clan->filename	= str_dup( "" );*/
    clan->motto		= STRALLOC( "" );
    clan->description	= STRALLOC( "" );
    clan->bank		= 0;
    clan->deity		= STRALLOC( "None" );
    clan->owner		= STRALLOC( "None" );
    clan->leader1	= STRALLOC( "None" );
    clan->leader2	= STRALLOC( "None" );
    clan->leader3	= STRALLOC( "None" );
    clan->leader4	= STRALLOC( "None" );
    clan->leader5	= STRALLOC( "None" );
    clan->leader6	= STRALLOC( "None" );
    clan->badge		= STRALLOC( "" );
    clan->mRank1	= STRALLOC( "Leader" );
    clan->mRank2	= STRALLOC( "Rank2" );
    clan->mRank3	= STRALLOC( "Rank3" );
    clan->mRank4	= STRALLOC( "Rank4" );
    clan->mRank5	= STRALLOC( "Rank5" );
    clan->mRank6	= STRALLOC( "Rank6" );
    clan->mRank7	= STRALLOC( "Rank7" );
    clan->fRank1	= STRALLOC( "Leader" );
    clan->fRank2	= STRALLOC( "Rank2" );
    clan->fRank3	= STRALLOC( "Rank3" );
    clan->fRank4	= STRALLOC( "Rank4" );
    clan->fRank5	= STRALLOC( "Rank5" );
    clan->fRank6	= STRALLOC( "Rank6" );
    clan->fRank7	= STRALLOC( "Rank7" );
    clan->clanWebSite	= STRALLOC( "" );

    for(aclan = first_clan; aclan; aclan = aclan->next)
    {
    	if (aclan == clan)
    		continue;

		CREATE( alliance, ALLIANCE_DATA, 1 );
		alliance->clan = clan;
		alliance->vclan = aclan;
		alliance->status = 0;
		alliance->vclanStatus = 0;
		alliance->votes = 0;
		alliance->leader1Vote = STRALLOC( "None" );
		alliance->leader2Vote = STRALLOC( "None" );
		LINK( alliance, first_alliance, last_alliance, next, prev );
	}
	newClanAlliance(clan);
	
	save_alliance();
	return;
}

void do_makecouncil( CHAR_DATA *ch, char *argument )
{
    char filename[256];
    COUNCIL_DATA *council;
    bool found;

    set_char_color( AT_IMMORT, ch );

    if ( !argument || argument[0] == '\0' )
    {
	send_to_char( "Usage: makecouncil <council name>\n\r", ch );
	return;
    }

    found = FALSE;
    sprintf( filename, "%s%s", COUNCIL_DIR, strlower(argument) );

    CREATE( council, COUNCIL_DATA, 1 );
    LINK( council, first_council, last_council, next, prev );
    council->name		= STRALLOC( argument );
    council->head		= STRALLOC( "" );
    council->head2 		= NULL;
    council->powers		= STRALLOC( "" );
}

void do_claninfo( CHAR_DATA *ch, char *argument )
{
	CLAN_DATA *clan;
	char buf[MAX_STRING_LENGTH];

	if (IS_NPC(ch))
		return;

	if (argument[0] == '\0')
    {
        pager_printf(ch, "Syntax: claninfo (clan short name)\n\r" );
        return;
    }


	clan = get_clan(argument);

    if (!clan)
    {
        pager_printf(ch, "No such clan.\n\r" );
        return;
    }

	switch (clan->alignment)
	{
		default:
			sprintf(buf, "&WNeutral");
			break;
		case CLANALIGN_GOOD:
			sprintf(buf, "   &CGood");
			break;
		case CLANALIGN_EVIL:
			sprintf(buf, "   &REvil");
			break;
	}

	pager_printf(ch, "&cClan Name : &W[ &R%s &W]\n\r", clan->name);
	pager_printf(ch, "&cShort Name: &W[ &R%s &W]\n\r", clan->short_name);
	pager_printf(ch, "&g-----------------------------------------------------------------\n\r");
	pager_printf(ch, "&cAdmin  : &W[ &R%-29s &W]  &cMembers : &W[ &R%7d &W]\n\r", clan->deity, clan->members);
	pager_printf(ch, "&cOwner  : &W[ &R%-29s &W]\n\r", clan->owner );
	pager_printf(ch, "&cLeaders: &W[ &R%-12s &W] [ &R%-12s &W]  &cTax Rate: &W[ &R%6d%% &W]\n\r", clan->leader1, clan->leader4, clan->tax);
	pager_printf(ch, "         &W[ &R%-12s &W] [ &R%-12s &W]  &cAlign   : &W[ %7s &W]\n\r", clan->leader2, clan->leader5, buf);
	/*pager_printf(ch, "         &W[ &R%-12s &W] [ &R%-12s &W]  &cClan PL : &W[ &R%7s &W]\n\r", clan->leader3, clan->leader6, abbNumLD(clan->clanPL));*/
	pager_printf(ch, "         &W[ &R%-12s &W] [ &R%-12s &W]\n\r", clan->leader3, clan->leader6 );
	pager_printf(ch, "&g-----------------------------------------------------------------\n\r");
	pager_printf(ch, "&c          Male Rankings                       Female Rankings\n\r");
	if (IS_IMMORTAL(ch) || ch->pcdata->clan == clan)
	{
		pager_printf(ch, "&c(%3d&c)&W[ &R%-20s &W]       &c(%3d&c)&W[ &R%-20s &W]\n\r", clan->mRank1Count, clan->mRank1, clan->fRank1Count, clan->fRank1);
		pager_printf(ch, "&c(%3d&c)&W[ &R%-20s &W]       &c(%3d&c)&W[ &R%-20s &W]\n\r", clan->mRank2Count, clan->mRank2, clan->fRank2Count, clan->fRank2);
		pager_printf(ch, "&c(%3d&c)&W[ &R%-20s &W]       &c(%3d&c)&W[ &R%-20s &W]\n\r", clan->mRank3Count, clan->mRank3, clan->fRank3Count, clan->fRank3);
		pager_printf(ch, "&c(%3d&c)&W[ &R%-20s &W]       &c(%3d&c)&W[ &R%-20s &W]\n\r", clan->mRank4Count, clan->mRank4, clan->fRank4Count, clan->fRank4);
		pager_printf(ch, "&c(%3d&c)&W[ &R%-20s &W]       &c(%3d&c)&W[ &R%-20s &W]\n\r", clan->mRank5Count, clan->mRank5, clan->fRank5Count, clan->fRank5);
		pager_printf(ch, "&c(%3d&c)&W[ &R%-20s &W]       &c(%3d&c)&W[ &R%-20s &W]\n\r", clan->mRank6Count, clan->mRank6, clan->fRank6Count, clan->fRank6);
		pager_printf(ch, "&c(%3d&c)&W[ &R%-20s &W]       &c(%3d&c)&W[ &R%-20s &W]\n\r", clan->mRank7Count, clan->mRank7, clan->fRank7Count, clan->fRank7);
	}
	else
	{
		pager_printf(ch, "     &W[ &R%-20s &W]            [ &R%-20s &W]\n\r", clan->mRank1, clan->fRank1);
		pager_printf(ch, "     &W[ &R%-20s &W]            [ &R%-20s &W]\n\r", clan->mRank2, clan->fRank2);
		pager_printf(ch, "     &W[ &R%-20s &W]            [ &R%-20s &W]\n\r", clan->mRank3, clan->fRank3);
		pager_printf(ch, "     &W[ &R%-20s &W]            [ &R%-20s &W]\n\r", clan->mRank4, clan->fRank4);
		pager_printf(ch, "     &W[ &R%-20s &W]            [ &R%-20s &W]\n\r", clan->mRank5, clan->fRank5);
		pager_printf(ch, "     &W[ &R%-20s &W]            [ &R%-20s &W]\n\r", clan->mRank6, clan->fRank6);
		pager_printf(ch, "     &W[ &R%-20s &W]            [ &R%-20s &W]\n\r", clan->mRank7, clan->fRank7);
	}
	pager_printf(ch, "&g-----------------------------------------------------------------\n\r");
	pager_printf(ch, "&cClan Recall?       : &W[%s&W]     &cClan Donate Room? : &W[%s&W]\n\r", clan->recall ? "&RY" : "&rN", clan->clanDonate ? "&RY" : "&rN");
	pager_printf(ch, "&cClan Death Recall? : &W[%s&W]     &cClan Jail?        : &W[%s&W]\n\r", clan->deathRecall ? "&RY" : "&rN", clan->clanJail ? "&RY" : "&rN");
	pager_printf(ch, "&cClan Morgue?       : &W[%s&W]     &cClan Skill?       : &W[&rNone&W]\n\r", clan->clanMorgue ? "&RY" : "&rN");
	if (IS_IMMORTAL(ch) || ch->pcdata->clan == clan)
		pager_printf(ch, "&cClan Bank Account  : &W[ &R%s &W] &czeni\n\r", num_punct_ld(clan->bank));
	pager_printf(ch, "&g-----------------------------------------------------------------\n\r");
	send_to_pager_color ( "&wVictories (&zsorted by power level&w):&w\n\r", ch );
	send_to_pager_color ( "&w      Player Kills           Sparing&w\n\r", ch );
	pager_printf_color( ch, "    &w5k-10k...  &r[&R%4d&r]    &w5k-10k...  &r[&R%4d&r]\n\r    &w10k-100k...&r[&R%4d&r]    &w10k-100k...&r[&R%4d&r]\n\r    &w100k-1m... &r[&R%4d&r]    &w100k-1m... &r[&R%4d&r]\n\r    &w1m-10m...  &r[&R%4d&r]    &w1m-10m...  &r[&R%4d&r]\n\r",
		clan->pkills[0], clan->spar_wins[0],
		clan->pkills[1], clan->spar_wins[1],
		clan->pkills[2], clan->spar_wins[2],
		clan->pkills[3], clan->spar_wins[3] );
	pager_printf_color( ch, "    &w10m-100m...&r[&R%4d&r]    &w10m-100m...&r[&R%4d&r]\n\r    &w100m-1b... &r[&R%4d&r]    &w100m-1b... &r[&R%4d&r]\n\r    &w+1b...     &r[&R%4d&r]    &w+1b...     &r[&R%4d&r]\n\r",
		clan->pkills[4], clan->spar_wins[4],
		clan->pkills[5], clan->spar_wins[5],
		clan->pkills[6], clan->spar_wins[6] );
	send_to_pager_color("&w    --------------------------------------\n\r", ch);
	pager_printf_color( ch, "&C    &WTOTAL...   &r[&R%4d&r]&C    &WTOTAL...   &r[&R%4d&r]&w\n\r",
		(clan->pkills[0]+clan->pkills[1]+clan->pkills[2]+clan->pkills[3]+clan->pkills[4]+clan->pkills[5]+clan->pkills[6]),
		(clan->spar_wins[0]+clan->spar_wins[1]+clan->spar_wins[2]+clan->spar_wins[3]+clan->spar_wins[4]+clan->spar_wins[5]+clan->spar_wins[6]) );
	pager_printf(ch, "&g------------------------------------------------------------------------\n\r");
	pager_printf(ch, "&cWeb Page    : &W[ &w%s &W]\n\r", clan->clanWebSite);
	pager_printf(ch, "&cMotto       : &W\"&w%s&W\"\n\r", clan->motto);
	pager_printf(ch, "&cDescription : \n\r &w%s\n\r", clan->description);
	pager_printf(ch, "&g------------------------------------------------------------------------&w\n\r");
	show_alliances(ch, clan);
	return;
}

void do_clans( CHAR_DATA *ch, char *argument )
{
    CLAN_DATA *clan;
    int count = 0;

	pager_printf(ch, "\n\r&RClan name                                      Short    Members  Active\n\r", ch );
	/*pager_printf(ch,"&r------------------------------------------- ----------- -------  ------ ------\n\r", ch );*/
	pager_printf(ch,"&r------------------------------------------- ----------- -------  ------\n\r", ch );
	for ( clan = first_clan; clan; clan = clan->next )
	{
		pager_printf(ch, "&c%-43s &w%-11s &g%7d  &c%6d\n\r",
			clan->name, clan->short_name, clan->members, clan->activeMem/*, abbNumLD(clan->clanPL)*/);
		count++;
	}
	if ( !count )
		send_to_pager_color ( "&wThere are no Clans currently formed.\n\r", ch );
	else
	{
		send_to_pager_color ( "&r------------------------------------------------------------------------------&c\n\r",ch);
		pager_printf(ch, "For more information on each clan, use : 'Claninfo (short name)'\n\r");
		pager_printf(ch, "Default inactive days is 30. Use 'Clans <days>' change. [NOT FUNCTIONAL]\n\r");
		pager_printf(ch, "For clan membership listings, see 'Help Roster' [NOT FUNCTIONAL]\n\r");
	}

	return;

/*
    clan = get_clan( argument );
    if ( !clan || clan->clan_type == CLAN_GUILD || clan->clan_type == CLAN_ORDER )
    {
        set_char_color( AT_BLOOD, ch );
        send_to_pager_color( "No such clan.\n\r", ch );
        return;
    }
    pager_printf_color( ch, "&r\n\r%s, '&R%s&r'\n\r\n\r", clan->name, clan->motto );
    send_to_pager_color ( "&wVictories (&zsorted by power level&w):&w\n\r", ch );
    send_to_pager_color ( "&w      Player Kills           Sparing&w\n\r", ch );

	pager_printf_color( ch, "    &w5k-10k...  &r[&R%4d&r]    &w5k-10k...  &r[&R%4d&r]\n\r    &w10k-100k...&r[&R%4d&r]    &w10k-100k...&r[&R%4d&r]\n\r    &w100k-1m... &r[&R%4d&r]    &w100k-1m... &r[&R%4d&r]\n\r    &w1m-10m...  &r[&R%4d&r]    &w1m-10m...  &r[&R%4d&r]\n\r",
	clan->pkills[0], clan->spar_wins[0],
	clan->pkills[1], clan->spar_wins[1],
	clan->pkills[2], clan->spar_wins[2],
	clan->pkills[3], clan->spar_wins[3] );
    pager_printf_color( ch, "    &w10m-100m...&r[&R%4d&r]    &w10m-100m...&r[&R%4d&r]\n\r    &w100m-1b... &r[&R%4d&r]    &w100m-1b... &r[&R%4d&r]\n\r    &w+1b...     &r[&R%4d&r]    &w+1b...     &r[&R%4d&r]\n\r",
	clan->pkills[4], clan->spar_wins[4],
	clan->pkills[5], clan->spar_wins[5],
	clan->pkills[6], clan->spar_wins[6] );
	send_to_pager_color("&w    --------------------------------------\n\r", ch);
    pager_printf_color( ch, "&C    &WTOTAL...   &r[&R%4d&r]&C    &WTOTAL...   &r[&R%4d&r]\n\r",
	(clan->pkills[0]+clan->pkills[1]+clan->pkills[2]+clan->pkills[3]+clan->pkills[4]+clan->pkills[5]+clan->pkills[6]),
	(clan->spar_wins[0]+clan->spar_wins[1]+clan->spar_wins[2]+clan->spar_wins[3]+clan->spar_wins[4]+clan->spar_wins[5]+clan->spar_wins[6]) );



    pager_printf_color( ch, "&wClan Leader:  &W%s\n\r&wNumber One :  &W%s\n\r&wNumber Two :  &W%s\n\r&wClan Admin :  &W%s\n\r",
                        clan->leader1,
                        clan->leader2,
                        clan->leader3,
			clan->deity );
    if ( !str_cmp( ch->name, clan->deity   )
    ||   !str_cmp( ch->name, clan->leader1  )
    ||   !str_cmp( ch->name, clan->leader2 )
    ||   !str_cmp( ch->name, clan->leader3 ) )
	pager_printf_color( ch, "&wMembers    :  &W%d\n\r",
			clan->members );
    set_char_color( AT_BLOOD, ch );
    pager_printf_color( ch, "\n\rDescription:&R  %s&D\n\r", clan->description );
    return;
*/
}

void do_orders( CHAR_DATA *ch, char *argument )
{
    CLAN_DATA *order;
    int count = 0;

    if ( argument[0] == '\0' )
    {
        set_char_color( AT_DGREEN, ch );
        send_to_char( "\n\rOrder            Deity          Leader           Mkills      Mdeaths\n\r____________________________________________________________________\n\r\n\r", ch );
	set_char_color( AT_GREEN, ch );
        for ( order = first_clan; order; order = order->next )
        if ( order->clan_type == CLAN_ORDER )
        {
            ch_printf( ch, "%-16s %-14s %-14s   %-7d       %5d\n\r",
	      order->name, order->deity, order->leader1, order->mkills, order->mdeaths );
            count++;
	}
        set_char_color( AT_DGREEN, ch );
	if ( !count )
	  send_to_char( "There are no Orders currently formed.\n\r", ch );
	else
	  send_to_char( "____________________________________________________________________\n\r\n\rUse 'orders <order>' for more detailed information.\n\r", ch );
	return;
    }

    order = get_clan( argument );
    if ( !order || order->clan_type != CLAN_ORDER )
    {
        set_char_color( AT_DGREEN, ch );
        send_to_char( "No such Order.\n\r", ch );
        return;
    }

    set_char_color( AT_DGREEN, ch );
    ch_printf( ch, "\n\rOrder of %s\n\r'%s'\n\r\n\r", order->name, order->motto );
    set_char_color( AT_GREEN, ch );
    ch_printf( ch, "Deity      :  %s\n\rLeader     :  %s\n\rNumber One :  %s\n\rNumber Two :  %s\n\r",
                        order->deity,
                        order->leader1,
                        order->leader2,
                        order->leader3 );
    if ( !str_cmp( ch->name, order->deity   )
    ||   !str_cmp( ch->name, order->leader1  )
    ||   !str_cmp( ch->name, order->leader2 )
    ||   !str_cmp( ch->name, order->leader3 ) )
        ch_printf( ch, "Members    :  %d\n\r",
			order->members );
    set_char_color( AT_DGREEN, ch );
    ch_printf( ch, "\n\rDescription:\n\r%s\n\r", order->description );
    return;
}

void do_councils( CHAR_DATA *ch, char *argument)
{
    COUNCIL_DATA *council;

    set_char_color( AT_CYAN, ch );
    if ( !first_council )
    {
	send_to_char( "There are no councils currently formed.\n\r", ch );
	return;
    }
    if ( argument[0] == '\0' )
    {
        send_to_char_color ("\n\r&cTitle                    Head\n\r", ch);
      for (council = first_council; council; council = council->next)
      {
        if ( council->head2 != NULL )
           ch_printf_color (ch, "&w%-24s %s and %s\n\r",  council->name,
                council->head, council->head2 );
        else
           ch_printf_color (ch, "&w%-24s %-14s\n\r", council->name, council->head);
      }
      send_to_char_color( "&cUse 'councils <name of council>' for more detailed information.\n\r", ch );
      return;
    }
    council = get_council( argument );
    if ( !council )
    {
      send_to_char_color( "&cNo such council exists...\n\r", ch );
      return;
    }
    ch_printf_color( ch, "&c\n\r%s\n\r", council->name );
  if ( council->head2 == NULL )
        ch_printf_color (ch, "&cHead:     &w%s\n\r&cMembers:  &w%d\n\r",
        council->head, council->members );
  else
        ch_printf_color (ch, "&cCo-Heads:     &w%s &cand &w%s\n\r&cMembers:  &w%d\n\r",
           council->head, council->head2, council->members );
    ch_printf_color( ch, "&cDescription:\n\r&w%s\n\r",
        council->description );
    return;
}

void do_guilds( CHAR_DATA *ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    CLAN_DATA *guild;
    int count = 0;

    if ( argument[0] == '\0' )
    {
        set_char_color( AT_HUNGRY, ch );
        send_to_char( "\n\rGuild                  Leader             Mkills      Mdeaths\n\r_____________________________________________________________\n\r\n\r", ch );
	set_char_color( AT_YELLOW, ch );
        for ( guild = first_clan; guild; guild = guild->next )
        if ( guild->clan_type == CLAN_GUILD )
	{
	    ++count;
	    ch_printf( ch, "%-20s   %-14s     %-6d       %6d\n\r", guild->name, guild->leader1, guild->mkills, guild->mdeaths );
	}
        set_char_color( AT_HUNGRY, ch );
        if ( !count )
          send_to_char( "There are no Guilds currently formed.\n\r", ch );
        else
          send_to_char( "_____________________________________________________________\n\r\n\rUse guilds <class>' for specifics. (ex:  guilds thieves)\n\r", ch );
	return;
    }

    sprintf( buf, "guild of %s", argument );
    guild = get_clan( buf );
    if ( !guild || guild->clan_type != CLAN_GUILD )
    {
        set_char_color( AT_HUNGRY, ch );
        send_to_char( "No such Guild.\n\r", ch );
        return;
    }
    set_char_color( AT_HUNGRY, ch );
    ch_printf( ch, "\n\r%s\n\r", guild->name );
    set_char_color( AT_YELLOW, ch );
    ch_printf( ch, "Leader:    %s\n\rNumber 1:  %s\n\rNumber 2:  %s\n\rMotto:     %s\n\r",
                        guild->leader1,
                        guild->leader2,
                        guild->leader3,
			guild->motto );
    if ( !str_cmp( ch->name, guild->deity   )
    ||   !str_cmp( ch->name, guild->leader1  )
    ||   !str_cmp( ch->name, guild->leader2 )
    ||   !str_cmp( ch->name, guild->leader3 ) )
        ch_printf( ch, "Members:   %d\n\r",
			guild->members );
    set_char_color( AT_HUNGRY, ch );
    ch_printf( ch, "Guild Description:\n\r%s\n\r", guild->description );
    return;
}

void do_victories( CHAR_DATA *ch, char *argument )
{
    char filename[256];

    if ( IS_NPC( ch ) || !ch->pcdata->clan ) {
        send_to_char( "Huh?\n\r", ch );
        return;
    }
    if ( ch->pcdata->clan->clan_type != CLAN_ORDER
    &&   ch->pcdata->clan->clan_type != CLAN_GUILD ) {
        sprintf( filename, "%s%s.record",
          CLAN_DIR, ch->pcdata->clan->short_name );
	set_pager_color( AT_PURPLE, ch );
	if ( !str_cmp( ch->name, ch->pcdata->clan->leader1 )
	&&   !str_cmp( argument, "clean" ) ) {
	  FILE *fp = fopen( filename, "w" );
	  if ( fp )
	    fclose( fp );
	  send_to_pager( "\n\rVictories ledger has been cleared.\n\r", ch );
	  return;
	} else {
	  send_to_pager( "\n\rLVL  Character       LVL  Character\n\r", ch );
	  show_file( ch, filename );
	  return;
	}
    }
    else
    {
        send_to_char( "Huh?\n\r", ch );
        return;
    }
}


void do_shove( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    int exit_dir;
    EXIT_DATA *pexit;
    CHAR_DATA *victim;
    bool nogo;
    ROOM_INDEX_DATA *to_room;
    int chance = 0;

    argument = one_argument( argument, arg );
    argument = one_argument( argument, arg2 );

    if ( IS_NPC(ch)
    || !IS_SET( ch->pcdata->flags, PCFLAG_DEADLY ) )
    {
	send_to_char("Only deadly characters can shove.\n\r", ch);
	return;
    }

    if  ( get_timer(ch, TIMER_PKILLED) > 0)
    {
	send_to_char("You can't shove a player right now.\n\r", ch);
	return;
    }

    if ( arg[0] == '\0' )
    {
	send_to_char( "Shove whom?\n\r", ch);
	return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL )
    {
	send_to_char( "They aren't here.\n\r", ch);
	return;
    }

    if (victim == ch)
    {
	send_to_char("You shove yourself around, to no avail.\n\r", ch);
	return;
    }
    if ( IS_NPC(victim)
    || !IS_SET( victim->pcdata->flags, PCFLAG_DEADLY ) )
    {
	send_to_char("You can only shove deadly characters.\n\r", ch);
	return;
    }

    if ( ch->level - victim->level > 5
    ||   victim->level - ch->level > 5 )
    {
	send_to_char("There is too great an experience difference for you to even bother.\n\r", ch);
	return;
    }

    if  ( get_timer(victim, TIMER_PKILLED) > 0)
    {
	send_to_char("You can't shove that player right now.\n\r", ch);
	return;
    }

    if ( (victim->position) != POS_STANDING )
    {
	act( AT_PLAIN, "$N isn't standing up.", ch, NULL, victim, TO_CHAR );
	return;
    }

    if ( arg2[0] == '\0' )
    {
	send_to_char( "Shove them in which direction?\n\r", ch);
	return;
    }

    exit_dir = get_dir( arg2 );
    if ( xIS_SET(victim->in_room->room_flags, ROOM_SAFE))
    {
	send_to_char("That character cannot be shoved right now.\n\r", ch);
	return;
    }
    victim->position = POS_SHOVE;
    nogo = FALSE;
    if ((pexit = get_exit(ch->in_room, exit_dir)) == NULL )
      nogo = TRUE;
    else
    if ( IS_SET(pexit->exit_info, EX_CLOSED)
    && (!IS_AFFECTED(victim, AFF_PASS_DOOR)
    ||   IS_SET(pexit->exit_info, EX_NOPASSDOOR)) )
      nogo = TRUE;
    if ( nogo )
    {
	send_to_char( "There's no exit in that direction.\n\r", ch );
        victim->position = POS_STANDING;
	return;
    }
    to_room = pexit->to_room;
    if (xIS_SET(to_room->room_flags, ROOM_DEATH))
    {
      send_to_char("You cannot shove someone into a death trap.\n\r", ch);
      victim->position = POS_STANDING;
      return;
    }

    if (ch->in_room->area != to_room->area
    &&  !in_hard_range( victim, to_room->area ) )
    {
      send_to_char("That character cannot enter that area.\n\r", ch);
      victim->position = POS_STANDING;
      return;
    }

/* Check for class, assign percentage based on that. */

/* Add 3 points to chance for every str point above 15, subtract for
below 15 */

chance += (get_curr_str(ch) - 10);

chance += (get_attmod(ch, victim));


/* Debugging purposes - show percentage for testing */

/* sprintf(buf, "Shove percentage of %s = %d", ch->name, chance);
act( AT_ACTION, buf, ch, NULL, NULL, TO_ROOM );
*/

if (chance < number_percent( ))
{
  send_to_char("You failed.\n\r", ch);
  victim->position = POS_STANDING;
  return;
}
    act( AT_ACTION, "You shove $M.", ch, NULL, victim, TO_CHAR );
    act( AT_ACTION, "$n shoves you.", ch, NULL, victim, TO_VICT );
    move_char( victim, get_exit(ch->in_room,exit_dir), 0);
    if ( !char_died(victim) )
      victim->position = POS_STANDING;
    WAIT_STATE(ch, 12);
    /* Remove protection from shove/drag if char shoves -- Blodkai */
    if ( xIS_SET(ch->in_room->room_flags, ROOM_SAFE)
    &&   get_timer(ch, TIMER_SHOVEDRAG) <= 0 )
      add_timer( ch, TIMER_SHOVEDRAG, 10, NULL, 0 );
}

void do_drag( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    int exit_dir;
    CHAR_DATA *victim;
    EXIT_DATA *pexit;
    ROOM_INDEX_DATA *to_room;
    bool nogo;
    int chance = 0;

    argument = one_argument( argument, arg );
    argument = one_argument( argument, arg2 );

    if ( IS_NPC(ch) )
    /* || !IS_SET( ch->pcdata->flags, PCFLAG_DEADLY ) )  */
    {
	send_to_char("Only characters can drag.\n\r", ch);
	return;
    }

    if  ( get_timer(ch, TIMER_PKILLED) > 0)
    {
	send_to_char("You can't drag a player right now.\n\r", ch);
	return;
    }

    if ( arg[0] == '\0' )
    {
	send_to_char( "Drag whom?\n\r", ch);
	return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL )
    {
	send_to_char( "They aren't here.\n\r", ch);
	return;
    }

    if ( victim == ch )
    {
	send_to_char("You take yourself by the scruff of your neck, but go nowhere.\n\r", ch);
	return;
    }

    if ( IS_NPC(victim)  )
         /* || !IS_SET( victim->pcdata->flags, PCFLAG_DEADLY ) ) */
    {
	send_to_char("You can only drag characters.\n\r", ch);
	return;
    }

    if ( !xIS_SET( victim->act, PLR_SHOVEDRAG)
    &&   !IS_SET( victim->pcdata->flags, PCFLAG_DEADLY) )
    {
	send_to_char("That character doesn't seem to appreciate your attentions.\n\r", ch);
	return;
    }

    if  ( get_timer(victim, TIMER_PKILLED) > 0)
    {
	send_to_char("You can't drag that player right now.\n\r", ch);
	return;
    }

    if ( victim->fighting )
    {
        send_to_char( "You try, but can't get close enough.\n\r", ch);
        return;
    }

    if( !IS_SET(ch->pcdata->flags, PCFLAG_DEADLY ) && IS_SET( victim->pcdata->flags, PCFLAG_DEADLY ) ){
	send_to_char("You cannot drag a deadly character.\n\r", ch);
	return;
    }

    if ( !IS_SET(victim->pcdata->flags, PCFLAG_DEADLY) && victim->position > 3 )
    {
	send_to_char("They don't seem to need your assistance.\n\r", ch );
	return;
    }

    if ( arg2[0] == '\0' )
    {
	send_to_char( "Drag them in which direction?\n\r", ch);
	return;
    }

    exit_dir = get_dir( arg2 );

    if ( xIS_SET(victim->in_room->room_flags, ROOM_SAFE))
    {
	send_to_char("That character cannot be dragged right now.\n\r", ch);
	return;
    }

    nogo = FALSE;
    if ((pexit = get_exit(ch->in_room, exit_dir)) == NULL )
      nogo = TRUE;
    else
    if ( IS_SET(pexit->exit_info, EX_CLOSED)
    && (!IS_AFFECTED(victim, AFF_PASS_DOOR)
    ||   IS_SET(pexit->exit_info, EX_NOPASSDOOR)) )
      nogo = TRUE;
    if ( nogo )
    {
	send_to_char( "There's no exit in that direction.\n\r", ch );
	return;
    }

    to_room = pexit->to_room;
    if (xIS_SET(to_room->room_flags, ROOM_DEATH))
    {
      send_to_char("You cannot drag someone into a death trap.\n\r", ch);
      return;
    }

    if (ch->in_room->area != to_room->area
    && !in_hard_range( victim, to_room->area ) )
    {
      send_to_char("That character cannot enter that area.\n\r", ch);
      victim->position = POS_STANDING;
      return;
    }

/* Check for class, assign percentage based on that. */

/* Add 3 points to chance for every str point above 15, subtract for
below 15 */

chance += (get_curr_str(ch) - 10);

chance += (get_attmod(ch, victim));

/*
sprintf(buf, "Drag percentage of %s = %d", ch->name, chance);
act( AT_ACTION, buf, ch, NULL, NULL, TO_ROOM );
*/
if (chance < number_percent( ))
{
  send_to_char("You failed.\n\r", ch);
  victim->position = POS_STANDING;
  return;
}
    if ( victim->position < POS_STANDING )
    {
	sh_int temp;

	temp = victim->position;
	victim->position = POS_DRAG;
	act( AT_ACTION, "You drag $M into the next room.", ch, NULL, victim, TO_CHAR );
	act( AT_ACTION, "$n grabs your hair and drags you.", ch, NULL, victim, TO_VICT );
	move_char( victim, get_exit(ch->in_room,exit_dir), 0);
	if ( !char_died(victim) )
	  victim->position = temp;
/* Move ch to the room too.. they are doing dragging - Scryn */
	move_char( ch, get_exit(ch->in_room,exit_dir), 0);
	WAIT_STATE(ch, 12);
	return;
    }
    send_to_char("You cannot do that to someone who is standing.\n\r", ch);
    return;
}

void do_promote( CHAR_DATA *ch, char *argument )
{
/*
	CHAR_DATA *victim;
	CLAN_DATA *clan;
    char arg[MAX_INPUT_LENGTH];

    if ( IS_NPC( ch ) || !ch->pcdata->clan )
    {
	send_to_char( "Huh?\n\r", ch );
	return;
    }

    clan = ch->pcdata->clan;

    if (!str_cmp( ch->name, clan->deity )
    ||   !str_cmp( ch->name, clan->leader ))
	;
    else
    {
	send_to_char( "Huh?\n\r", ch );
	return;
    }

	argument = one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Syntax: promote <name> <leader/number1/number2>\n\r", ch );
	return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL )
    {
	send_to_char( "That player is not here.\n\r", ch);
	return;
    }

    if ( IS_NPC(victim) )
    {
	send_to_char( "Not on NPC's.\n\r", ch );
	return;
    }

	if (!victim->pcdata->clan)
    {
	send_to_char( "They must already be in your clan.\n\r", ch );
	return;
    }

	clan = ch->pcdata->clan;

	if (victim->pcdata->clan != clan)
    {
	send_to_char( "They must already be in your clan.\n\r", ch );
	return;
    }

	if (!str_cmp( argument, "leader" ))
	{
		STRFREE( clan->leader );
		clan->leader = STRALLOC( victim->name );
		pager_printf_color( ch, "&w%s has been promoted to leader.\n\r", victim->name );
		pager_printf_color( victim, "&wYou have been promoted to leader.\n\r", victim->name );
		return;
    }
	if (!str_cmp( argument, "number1" ))
	{
		STRFREE( clan->number1 );
		clan->number1 = STRALLOC( victim->name );
		pager_printf_color( ch, "&w%s has been promoted to first officer.\n\r", victim->name );
		pager_printf_color( victim, "&wYou have been promoted to first officer.\n\r", victim->name );
		return;
    }
	if (!str_cmp( argument, "number2" ))
	{
		STRFREE( clan->number2 );
		clan->number2 = STRALLOC( victim->name );
		pager_printf_color( ch, "&w%s has been promoted to second officer.\n\r", victim->name );
		pager_printf_color( victim, "&wYou have been promoted to second officer.\n\r", victim->name );
		return;
    }

	send_to_char( "You that isn't a rank.\n\r", ch );
*/
	return;
}

void show_members( CHAR_DATA *ch, char *argument, char *format )
{
     MEMBER_LIST	*members_list;
     MEMBER_DATA	*member;
     CLAN_DATA	*clan;


     for( members_list = first_member_list; members_list; members_list = members_list->next )
     {
         if( !str_cmp( members_list->name, argument ) )
             break;
     }

     if( !members_list )
         return;

     clan = get_clan( argument );

     if ( !clan  )
         return;

    pager_printf( ch, "\n\r&cMembers of &W%s\n\r", clan->name );
    pager_printf( ch,"&z------------------------------------------------------------&c\n\r" );
    pager_printf( ch, "Admin: &w%s\n\r", clan->deity );
    pager_printf( ch, "Leaders:&w\n\r");
    pager_printf( ch, " %-12s  %-12s\n\r", clan->leader1, clan->leader2 );
    pager_printf( ch, " %-12s  %-12s\n\r", clan->leader3, clan->leader4 );
    pager_printf( ch, " %-12s  %-12s\n\r", clan->leader5, clan->leader6 );
    pager_printf( ch,"&z------------------------------------------------------------&c\n\r" );
    pager_printf( ch, "&cName                 Race   Kills   Deaths Since\n\r\n\r" );

     if( format)
     {
         if( !str_cmp( format, "kills" )
             || !str_cmp( format, "deaths" )
             || !str_cmp( format, "alpha" ))
         {
             MS_DATA *insert = NULL;
             MS_DATA *sort;
             MS_DATA *first_member = NULL;
             MS_DATA *last_member = NULL;

             CREATE( sort, MS_DATA, 1 );
             sort->member = members_list->first_member;
             LINK( sort, first_member, last_member, next, prev );

             for( member = members_list->first_member->next; member; member = member->next )
             {
                 insert = NULL;
                 for( sort = first_member; sort; sort = sort->next )
                 {
                     if( !str_cmp( format, "kills" ))
                     {
                         if( member->kills > sort->member->kills )
                         {
                             CREATE( insert, MS_DATA, 1 );
                             insert->member = member;
                             INSERT( insert, sort, first_member, next, prev );
                             break;
                         }
                     }
                     else if( !str_cmp( format, "deaths" ))
                     {
                         if( member->deaths > sort->member->deaths )
                         {
                             CREATE( insert, MS_DATA, 1 );
                             insert->member = member;
                             INSERT( insert, sort, first_member, next, prev );
                             break;
                         }
                     }
                     else if( !str_cmp( format, "alpha" ))
                     {
                         if( strcmp( member->name, sort->member->name ) < 0 )
                         {
                             CREATE( insert, MS_DATA, 1 );
                             insert->member = member;
                             INSERT( insert, sort, first_member, next, prev );
                             break;
                         }
                     }

                 }
                 if( insert == NULL )
                 {
                     CREATE( insert, MS_DATA, 1 );
                     insert->member = member;
                     LINK( insert, first_member, last_member, next, prev );
                 }
             }

             for( sort = first_member; sort; sort = sort->next )
                 if( !is_leader(ch))
                     pager_printf( ch, "&w%-12s %13s %6d %8d %10s\n\r",
                                   capitalize(sort->member->name ),
                                   class_table[sort->member->race]->who_name,
                                   sort->member->kills,
                                   sort->member->deaths,
                                   sort->member->since );

         }

         for( member = members_list->first_member; member; member = member->next )
             if( !str_prefix( format, member->name ) )
                 pager_printf( ch, "&w%-12s %13s %6d %8d %10s\n\r",
                               capitalize(member->name ),
                               class_table[member->race]->who_name,
                               member->kills,
                               member->deaths,
                               member->since );

     }
     else
     {

         for( member = members_list->first_member; member; member = member->next )
             if( !is_leader(ch))
                 pager_printf( ch, "&w%-12s %13s %6d  %7d %10s\n\r",
                               capitalize(member->name), class_table[member->race]->who_name,
                               member->kills, member->deaths, member->since );
     }

     pager_printf( ch,"&z------------------------------------------------------------&c\n\r" );


}

void remove_member( CHAR_DATA *ch )
{
    MEMBER_LIST	*members_list;
    MEMBER_DATA	*member;

    if( !ch->pcdata )
        return;

    for( members_list = first_member_list; members_list; members_list = members_list->next )
    {
        if( !str_cmp( members_list->name, ch->pcdata->clan_name ) )
            break;
    }

    if( !members_list )
    {
	/*bug("Remove_member: No members list",0);*/
	return;
    }

    for( member = members_list->first_member; member; member = member->next )
    {
        if( !str_cmp( member->name, ch->name ) )
        {
            UNLINK( member, members_list->first_member, members_list->last_member, next, prev );
            STRFREE( member->name );
            STRFREE( member->since );
            DISPOSE( member );
            save_member_list( members_list );
/*
			if (ch->pcdata)
			{
			ch->pcdata->clan->members--;
			save_clan( ch->pcdata->clan );
			}
*/
            break;
        }
    }
}

void do_roster( CHAR_DATA *ch, char *argument )
{
     if( IS_NPC( ch ) || !ch->pcdata->clan
         || ( !is_leader(ch) /*|| !is_deity(ch)*/) )
     {
         send_to_char( "Huh?\n\r", ch );
         return;
     }

     show_members( ch, ch->pcdata->clan->name, argument );
     return;

}
void do_members( CHAR_DATA *ch, char *argument )
{

     char arg1[MAX_STRING_LENGTH];
     argument = one_argument( argument, arg1 );

     if( argument[0] == '\0' || arg1[0] == '\0' )
     {
         send_to_char( "Do what?\n\r", ch );
         return;
     }

     if( !str_cmp( arg1, "show" ) )
     {
         if( !str_cmp( argument, "all" ) )
         {
             MEMBER_LIST *members_list;
             for( members_list = first_member_list; members_list; members_list = members_list->next )
                 show_members( ch, members_list->name, NULL );
             return;
         }

         show_members( ch, argument, NULL );
         return;
     }

     if( !str_cmp( arg1, "create" ) )
     {
         MEMBER_LIST *members_list;

         CREATE( members_list, MEMBER_LIST, 1 );
         members_list->name = STRALLOC( argument );
         LINK( members_list, first_member_list, last_member_list, next, prev );
         save_member_list( members_list );
         ch_printf( ch, "Member lists \"%s\" created.\n\r", argument );
         return;
     }

     if( !str_cmp( arg1, "delete" ) )
     {
         MEMBER_LIST *members_list;
         MEMBER_DATA *member;

         for( members_list = first_member_list; members_list; members_list = members_list->next )
             if( !str_cmp( argument, members_list->name ) )
             {
                 while( members_list->first_member )
                 {
                     member = members_list->first_member;
                     STRFREE( member->name );
                     STRFREE( member->since );
                     UNLINK( member, members_list->first_member, members_list->last_member, next, prev );
                     DISPOSE( member );
                 }

                 STRFREE( members_list->name );
                 UNLINK( members_list, first_member_list, last_member_list, next, prev );
                 DISPOSE( members_list );
                 ch_printf( ch, "Member list \"%s\" destroyed.\n\r", argument );
                 return;
             }
         send_to_char( "No such list.\n\r", ch );
         return;
     }
}

void save_member_list( MEMBER_LIST *members_list )
{
     MEMBER_DATA	*member;
     FILE		*fp;
     CLAN_DATA    *clan;
     char         buf[MAX_STRING_LENGTH];

     if( !members_list )
     {
         bug( "save_member_list: NULL members_list" );
         return;
     }

     if( ( clan = get_clan( members_list->name )) == NULL )
     {
         bug( "save_member_list: no such clan: %s", members_list->name );
         return;
     }

     sprintf( buf, "%s%s.mem", CLAN_DIR, clan->filename );

     if( ( fp = fopen( buf, "w" ) ) == NULL )
     {
         bug( "Cannot open clan.mem file for writing", 0 );
         perror( buf );
         return;
     }

     fprintf( fp, "Name          %s~\n", members_list->name );
     for( member = members_list->first_member; member; member = member->next )
         fprintf( fp, "Member        %s %s %d %d %d %d\n", member->name, member->since,
                  member->kills, member->deaths, member->level, member->race );
     fprintf( fp, "End\n\n" );
     fclose( fp );

}

bool load_member_list( char *filename )
{
     FILE *fp;
     char buf[MAX_STRING_LENGTH];
     MEMBER_LIST *members_list;
     MEMBER_DATA *member;

     sprintf( buf, "%s%s.mem", CLAN_DIR, filename );

     if( ( fp = fopen( buf, "r" ) ) == NULL )
     {
         bug( "Cannot open member list for reading", 0 );
         return FALSE;
     }

     CREATE( members_list, MEMBER_LIST, 1 );

     for( ; ; )
     {
         char *word;

         word = fread_word( fp );

         if( !str_cmp( word, "Name" ) )
         {
             members_list->name = fread_string( fp );
             continue;
         }
         else
         if( !str_cmp( word, "Member" ) )
         {
             CREATE( member, MEMBER_DATA, 1 );
             member->name = STRALLOC( fread_word( fp ) );
             member->since = STRALLOC( fread_word( fp ) );
             member->kills = fread_number( fp );
             member->deaths = fread_number( fp );
             member->level = fread_number( fp );
             member->race = fread_number( fp );
             LINK( member, members_list->first_member, members_list->last_member, next, prev );
             continue;
         }
         else
         if( !str_cmp( word, "End" ) )
         {
             LINK( members_list, first_member_list, last_member_list, next, prev );
             fclose( fp );
             return TRUE;
         }
         else
         {
             bug( "load_members_list: bad section", 0 );
             fclose( fp );
             return FALSE;
         }
     }

}

void update_member( CHAR_DATA *ch )
{
     MEMBER_LIST *members_list;
     MEMBER_DATA *member;

     if( IS_NPC( ch ) || !ch->pcdata->clan )
         return;

     for( members_list = first_member_list; members_list; members_list = members_list->next )
         if( !str_cmp( members_list->name, ch->pcdata->clan_name ) )
         {
             for( member = members_list->first_member; member; member = member->next )
                 if ( !str_cmp( member->name, ch->name ) )
                 {
                     if( ch->pcdata->clan->clan_type == CLAN_PLAIN )
                     {
                         member->kills = ch->pcdata->pkills;
                         member->deaths = ch->pcdata->pdeaths;
                     }
                     else
                     {
                         member->kills = ch->pcdata->mkills;
                         member->deaths = ch->pcdata->mdeaths;
                     }

                     member->level = ch->level;
                     return;
                 }

             if( member == NULL )
             {
                 struct tm *t = localtime(&current_time);
                 char buf[MAX_STRING_LENGTH];

                 CREATE( member, MEMBER_DATA, 1 );
                 member->name = STRALLOC( ch->name );
                 member->level = ch->level;
                 member->race = ch->race;
                 sprintf( buf, "[%02d|%02d|%04d]", t->tm_mon+1, t->tm_mday, t->tm_year+1900 );
                 member->since = STRALLOC( buf );
                 if( ch->pcdata->clan->clan_type == CLAN_PLAIN )
                 {
                     member->kills = ch->pcdata->pkills;
                     member->deaths = ch->pcdata->pdeaths;
                 }
                 else
                 {
                     member->kills = ch->pcdata->mkills;
                     member->deaths = ch->pcdata->mdeaths;
                 }
                 LINK( member, members_list->first_member, members_list->last_member, next, prev );
                 save_member_list( members_list );
        		ch->pcdata->clan->members++;
				save_clan( ch->pcdata->clan );


             }

         }
}

void do_clandeposit( CHAR_DATA *ch, char *argument )
{
	char buf[MAX_STRING_LENGTH];
	int amount = 0;

	if (IS_NPC(ch))
	{
		send_to_char( "NPC's can't deposit zeni.\n\r", ch );
		return;
	}

	if (!ch->pcdata->clan)
	{
		send_to_char( "You're not in a clan.\n\r", ch );
		return;
	}

	if (argument[0] == '\0')
	{
		send_to_char( "Deposit how much?\n\r", ch );
		return;
	}

	if (!is_number(argument))
	{
		send_to_char( "You must donate zeni.\n\r", ch );
		return;
	}

	amount = atoi(argument);

	if (amount <= 0)
	{
		send_to_char( "You must donate zeni.\n\r", ch );
		return;
	}

        if( amount > ch->gold )
        {
            send_to_char( "That's more than you have!\n\r", ch );
            return;
        }

	ch->pcdata->clan->bank += amount;
	ch->gold -= amount;
	ch->pcdata->clanZeniDonated += amount;
	save_clan(ch->pcdata->clan);
	save_char_obj(ch);
	pager_printf(ch, "You deposit %s zeni into your clans bank account.\n\r", num_punct(amount));
	sprintf(buf, "%s has deposited %s zeni into the clan bank account.\n\r", ch->name, num_punct(amount));
	echo_to_clan(ch->pcdata->clan, buf);

	return;
}

void do_clanwithdraw( CHAR_DATA *ch, char *argument )
{
        char buf[MAX_STRING_LENGTH];
        int amount = 0;

        if (IS_NPC(ch))
        {
                send_to_char( "NPC's can't withdraw zeni.\n\r", ch );
                return;
        }

        if (!ch->pcdata->clan)
        {
                send_to_char( "You're not in a clan.\n\r", ch );
                return;
        }

	if( !is_owner(ch,ch->pcdata->clan) )
	{
	  ch_printf(ch,"Only the owner may withdraw funds from the clan bank account.\n\r");
	  return;
	}

	if( IS_IMMORTAL(ch) && ch->level < 63 )
	{
	  ch_printf(ch,"Access denied.\n\r");
	  return;
	}

        if (argument[0] == '\0')
        {
                send_to_char( "Withdraw how much?\n\r", ch );
                return;
        }

        if (!is_number(argument))
        {
                send_to_char( "You must withdraw zeni only.\n\r", ch );
                return;
        }

        amount = atoi(argument);

        if (amount <= 0)
        {
                send_to_char( "You must WITHDRAW zeni.\n\r", ch );
                return;
        }

        if( amount > ch->pcdata->clan->bank )
        {
            send_to_char( "That's more than the clan has!\n\r", ch );
            return;
        }

	if( (amount + ch->gold) > 2000000000 )
        {
            send_to_char( "You can't carry that much!\n\r", ch );
            return;
        }

        ch->pcdata->clan->bank -= amount;
        ch->gold += amount;
        save_clan(ch->pcdata->clan);
	save_char_obj(ch);
        pager_printf(ch, "You withdraw %s zeni from your clans bank account.\n\r", num_punct(amount));
        sprintf(buf, "%s has withdrawn %s zeni from the clan bank account.\n\r", ch->name, num_punct(amount));
        echo_to_clan(ch->pcdata->clan, buf);

        return;
}

void do_clanadmin( CHAR_DATA *ch, char *argument )
{
	char arg[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	char arg3[MAX_INPUT_LENGTH];
	char buf[MAX_STRING_LENGTH];
	CHAR_DATA *victim;
	CLAN_DATA *clan;
	CLAN_DATA *vclan;
	ALLIANCE_DATA *alliance;
	/*ROOM_INDEX_DATA *location;*/
	bool L1 = FALSE;
	bool L2 = FALSE;
	bool L3 = FALSE;
	bool L4 = FALSE;
	bool L5 = FALSE;
	bool L6 = FALSE;
	/*int tax = 0;*/

	if (IS_NPC(ch))
	{
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	if (!ch->pcdata->clan)
	{
		send_to_char( "You're not in a clan.\n\r", ch );
		return;
	}

	if (!is_leader(ch) && !is_deity(ch))
	{
		send_to_char( "Only the leaders of your clan can do this.\n\r", ch );
		return;
	}

    argument = one_argument( argument, arg );

	if (arg[0] == '\0')
	{
		ch_printf(ch,"\n\rSyntax: clanadmin (field) (value)\n\r",ch);
		ch_printf(ch,"\n\rField being one of the following:\n\r\n\r");
		ch_printf(ch,"  www | recruit | outcast | promote | demote | taxrate\n\r");
		ch_printf(ch,"  jail | release | evict | evictrank | status | &Rowner&D\n\r" );
		ch_printf(ch,"  mrank1-7 | frank1-7 | motto | desc \n\r");
		ch_printf(ch,"\n\r&RWarning: You can only transfer ownership of the clan to another clan leader,\n\r");
		ch_printf(ch,"and if you do, you will not be able to get it back, till whenever the owner\n\rtransfers it back to you.&D\n\r");
		return;
	}

	clan = ch->pcdata->clan;

	if(!str_cmp(arg, "recruit"))
	{
		do_induct(ch, argument);
		return;
	}

	if(!str_cmp(arg, "owner"))
	{
	  if(!is_owner(ch,clan) )
	  {
	    ch_printf(ch,"Only the owner may bestow ownership of this clan to another.\n\r");
	    return;
	  }
          if ((victim = get_char_room(ch, argument)) == NULL)
          {
            send_to_char( "They are not here.\n\r", ch );
            return;
          }

	  if( is_deity(victim) )
	  {
	    ch_printf(ch,"You can't make the clan admin the owner too.\n\r");
	    return;
	  }
	  if( !is_leader(victim) )
	  {
	    ch_printf(ch,"You can only give ownership to someone who is a leader.\n\r");
	    return;
	  }
	  STRFREE(clan->owner);
	  clan->owner = STRALLOC( victim->name );
	  save_clan( clan );
	  ch_printf(ch,"You have given ownership of the clan to %s.\n\r",victim->name );
	  ch_printf(victim,"%s has given you ownership of the clan.\n\r",ch->name);
	  return;
	}

	if(!str_cmp(arg, "outcast"))
	{
		do_outcast(ch, argument);
		return;
	}

        if ( !str_cmp( arg, "mrank1" ) )
        {
	  if( !is_owner(ch,clan) )
	  {
	    ch_printf(ch,"Only the owner can change rank names.\n\r");
	    return;
	  }
          STRFREE( clan->mRank1 );
          clan->mRank1 = STRALLOC( argument );
          send_to_char( "Done.\n\r", ch );
          save_clan( clan );
          return;
        }
        if ( !str_cmp( arg, "mrank2" ) )
        {
          if( !is_owner(ch,clan) )
          {
            ch_printf(ch,"Only the owner can change rank names.\n\r");
            return;
          }
          STRFREE( clan->mRank2 );
          clan->mRank2 = STRALLOC( argument );
          send_to_char( "Done.\n\r", ch );
          save_clan( clan );
          return;
        }
        if ( !str_cmp( arg, "mrank3" ) )
        {
          if( !is_owner(ch,clan) )
          {
            ch_printf(ch,"Only the owner can change rank names.\n\r");
            return;
          }
          STRFREE( clan->mRank3 );
          clan->mRank3 = STRALLOC( argument );
          send_to_char( "Done.\n\r", ch );
          save_clan( clan );
          return;
        }
        if ( !str_cmp( arg, "mrank4" ) )
        {
          if( !is_owner(ch,clan) )
          {
            ch_printf(ch,"Only the owner can change rank names.\n\r");
            return;
          }
          STRFREE( clan->mRank4 );
          clan->mRank4 = STRALLOC( argument );
          send_to_char( "Done.\n\r", ch );
          save_clan( clan );
         return;
        }
        if ( !str_cmp( arg, "mrank5" ) )
        {
          if( !is_owner(ch,clan) )
          {
            ch_printf(ch,"Only the owner can change rank names.\n\r");
            return;
          }
          STRFREE( clan->mRank5 );
          clan->mRank5 = STRALLOC( argument );
          send_to_char( "Done.\n\r", ch );
          save_clan( clan );
          return;
        }
        if ( !str_cmp( arg, "mrank6" ) )
        {
          if( !is_owner(ch,clan) )
          {
            ch_printf(ch,"Only the owner can change rank names.\n\r");
            return;
          }
          STRFREE( clan->mRank6 );
          clan->mRank6 = STRALLOC( argument );
          send_to_char( "Done.\n\r", ch );
          save_clan( clan );
          return;
        }
        if ( !str_cmp( arg, "mrank7" ) )
        {
          if( !is_owner(ch,clan) )
          {
            ch_printf(ch,"Only the owner can change rank names.\n\r");
            return;
          }
          STRFREE( clan->mRank7 );
          clan->mRank7 = STRALLOC( argument );
          send_to_char( "Done.\n\r", ch );
          save_clan( clan );
          return;
        }
        if ( !str_cmp( arg, "fRank1" ) )
        {
          if( !is_owner(ch,clan) )
          {
            ch_printf(ch,"Only the owner can change rank names.\n\r");
            return;
          }
          STRFREE( clan->fRank1 );
          clan->fRank1 = STRALLOC( argument );
          send_to_char( "Done.\n\r", ch );
          save_clan( clan );
          return;
        }
        if ( !str_cmp( arg, "fRank2" ) )
        {
          if( !is_owner(ch,clan) )
          {
            ch_printf(ch,"Only the owner can change rank names.\n\r");
            return;
          }
          STRFREE( clan->fRank2 );
          clan->fRank2 = STRALLOC( argument );
          send_to_char( "Done.\n\r", ch );
          save_clan( clan );
          return;
        }
        if ( !str_cmp( arg, "fRank3" ) )
        {
          if( !is_owner(ch,clan) )
          {
            ch_printf(ch,"Only the owner can change rank names.\n\r");
            return;
          }
          STRFREE( clan->fRank3 );
          clan->fRank3 = STRALLOC( argument );
          send_to_char( "Done.\n\r", ch );
          save_clan( clan );
          return;
        }
        if ( !str_cmp( arg, "fRank4" ) )
        {
          if( !is_owner(ch,clan) )
          {
            ch_printf(ch,"Only the owner can change rank names.\n\r");
            return;
          }
          STRFREE( clan->fRank4 );
          clan->fRank4 = STRALLOC( argument );
          send_to_char( "Done.\n\r", ch );
          save_clan( clan );
          return;
        }
        if ( !str_cmp( arg, "fRank5" ) )
        {
          if( !is_owner(ch,clan) )
          {
            ch_printf(ch,"Only the owner can change rank names.\n\r");
            return;
          }
          STRFREE( clan->fRank5 );
          clan->fRank5 = STRALLOC( argument );
          send_to_char( "Done.\n\r", ch );
          save_clan( clan );
          return;
        }
        if ( !str_cmp( arg, "fRank6" ) )
        {
          if( !is_owner(ch,clan) )
          {
            ch_printf(ch,"Only the owner can change rank names.\n\r");
            return;
          }
          STRFREE( clan->fRank6 );
          clan->fRank6 = STRALLOC( argument );
          send_to_char( "Done.\n\r", ch );
          save_clan( clan );
          return;
	}
        if ( !str_cmp( arg, "fRank7" ) )
        {
          if( !is_owner(ch,clan) )
          {
            ch_printf(ch,"Only the owner can change rank names.\n\r");
            return;
          }
          STRFREE( clan->fRank7 );
          clan->fRank7 = STRALLOC( argument );
          send_to_char( "Done.\n\r", ch );
          save_clan( clan );
          return;
        }

        if ( !str_cmp( arg, "motto" ) )
        {
          if( !is_owner(ch,clan) )
          {
            ch_printf(ch,"Only the owner can change the clan motto.\n\r");
            return;
          }
          STRFREE( clan->motto );
          clan->motto = STRALLOC( argument );
          send_to_char( "Done.\n\r", ch );
          save_clan( clan );
          return;
        }

        if ( !str_cmp( arg, "desc" ) )
        {
          if( !is_owner(ch,clan) )
          {
            ch_printf(ch,"Only the owner can change the clan description.\n\r");
            return;
          }
          STRFREE( clan->description );
          clan->description = STRALLOC( argument );
          send_to_char( "Done.\n\r", ch );
          save_clan( clan );
          return;
        }

	if(!str_cmp(arg, "evict") || !str_cmp(arg, "evictrank"))
	{
		send_to_char( "This feature isn't implemented yet.\n\r", ch );
		return;
	}
	/*
	if(!str_cmp(arg, "jail"))
	{
		if (!clan->clanJail)
		{
			send_to_char( "Your clan hasn't built a jail yet.\n\r", ch );
			return;
		}

		if ((victim = get_char_room(ch, argument)) == NULL)
		{
			send_to_char( "They are not here.\n\r", ch );
			return;
		}

		if (IS_NPC(ch) || !ch->pcdata->clan || ch->pcdata->clan != clan)
		{
			send_to_char( "They are not a member of your clan.\n\r", ch );
			return;
		}

		if (is_deity(victim) || is_leader(victim))
		{
			send_to_char( "You can't jail that person.\n\r", ch );
			return;
		}

		location = get_room_index(clan->clanJail);
		act( AT_RED, "$n is hauled off to jail.", victim, NULL, NULL, TO_ROOM );
		char_from_room( victim );
		char_to_room( victim, location );
		save_char_obj( victim );
		act( AT_RED, "$n is tossed into the jail cell.", victim, NULL, NULL, TO_ROOM );
		act( AT_RED, "You are hauled off to jail and tossed into the jail cell.", ch, NULL, victim, TO_VICT );
		do_look( victim, "auto" );
	}
	*/
	/*
	if(!str_cmp(arg, "release"))
	{
		if (!clan->clanJail)
		{
			send_to_char( "Your clan hasn't built a jail yet.\n\r", ch );
			return;
		}

		if ((victim = get_char_world(ch, argument)) == NULL)
		{
			send_to_char( "Wait until they are online.\n\r", ch );
			return;
		}

		if (IS_NPC(ch) || !ch->pcdata->clan || ch->pcdata->clan != clan)
		{
			send_to_char( "They are not a member of your clan.\n\r", ch );
			return;
		}

		if (victim->in_room->vnum != clan->clanJail)
		{
			send_to_char( "They are not in your jail.\n\r", ch );
			return;
		}

		act( AT_RED, "$n is released from jail.", victim, NULL, NULL, TO_ROOM );
		char_from_room( victim );
		char_to_room( victim, ch->in_room );
		save_char_obj( victim );
		act( AT_RED, "$n is brought in from jail.", victim, NULL, NULL, TO_ROOM );
		act( AT_RED, "You've been released from jail.", ch, NULL, victim, TO_VICT );
		do_look( victim, "auto" );
	}
	*/
	if(!str_cmp(arg, "www"))
	{
          if( !is_owner(ch,clan) )
          {
            ch_printf(ch,"Only the owner can change the clan website address.\n\r");
            return;
          }
		STRFREE( clan->clanWebSite );
		clan->clanWebSite = STRALLOC( argument );
		send_to_char("Done.\n\r", ch);
		save_clan(clan);
		return;
	}

    /*
    if (!str_cmp( arg, "taxrate" ))
    {
		if (!is_number(argument))
		{
			send_to_char( "Tax rate must be a number between 0 and 25.\n\r", ch );
			return;
		}
		tax = atoi(argument);
		if (tax > 25 || tax < 0)
		{
			send_to_char( "Tax rate must be a number between 0 and 25.\n\r", ch );
			return;
		}
		if (clan->bank < 250000)
		{
			send_to_char( "It costs 250,000 zeni to change the tax rate.\n\r", ch );
			return;
		}
		clan->tax = tax;
		clan->bank -= 250000;
		send_to_char( "Done.\n\r", ch );
		save_clan( clan );
		return;
    }
    */
	if ( !str_cmp( arg, "promote" ) )
	{
		if ((victim = get_char_room(ch, argument)) == NULL)
		{
			send_to_char( "They are not here.\n\r", ch );
			return;
		}
		if( ch == victim )
		{
		  ch_printf(ch,"You can't promote yourself.\n\r");
		  return;
		}

		if (IS_NPC(ch) || !ch->pcdata->clan || ch->pcdata->clan != clan)
		{
			send_to_char( "They are not a member of your clan.\n\r", ch );
			return;
		}

		if (is_deity(victim))
		{
			send_to_char( "You can't promote your clans admin.\n\r", ch );
			return;
		}

		if (victim->pcdata->clanRank <= 2 && !is_leader(victim) &&
		    !is_owner(ch,ch->pcdata->clan) && !is_deity(ch) )
		{
			/*send_to_char( "You can't promote someone to a leader without contacting your clan admin.\n\r", ch );*/
			ch_printf(ch,"Only the clan admin or owner may promote others to a leader.\n\r");
			return;
		}

		if( !str_cmp(clan->leader1,"None" ))
		  L1 = TRUE;
		else
		  L1 = FALSE;
                if( !str_cmp(clan->leader2,"None" ))
                  L2 = TRUE;
                else
                  L2 = FALSE;
                if( !str_cmp(clan->leader3,"None" ))
                  L3 = TRUE;
                else
                  L3 = FALSE;
                if( !str_cmp(clan->leader4,"None" ))
                  L4 = TRUE;
                else
                  L4 = FALSE;
                if( !str_cmp(clan->leader5,"None" ))
                  L5 = TRUE;
                else
                  L5 = FALSE;
                if( !str_cmp(clan->leader6,"None" ))
                  L6 = TRUE;
                else
                  L6 = FALSE;

		switch (victim->pcdata->clanRank)
		{
			default:
				send_to_char( "Unknown rank.\n\r", ch );
				bug("clanadmin: undknown rank set for %s(%d)", victim->name, victim->pcdata->clanRank);
				return;
			case 7:
				if (victim->sex == SEX_FEMALE)
				{
					clan->fRank7Count--;
					clan->fRank6Count++;
				}
				else
				{
					clan->mRank7Count--;
					clan->mRank6Count++;
				}
				break;
			case 6:
				if (victim->sex == SEX_FEMALE)
				{
					clan->fRank6Count--;
					clan->fRank5Count++;
				}
				else
				{
					clan->mRank6Count--;
					clan->mRank5Count++;
				}
				break;
			case 5:
				if (victim->sex == SEX_FEMALE)
				{
					clan->fRank5Count--;
					clan->fRank4Count++;
				}
				else
				{
					clan->mRank5Count--;
					clan->mRank4Count++;
				}
				break;
			case 4:
				if (victim->sex == SEX_FEMALE)
				{
					clan->fRank4Count--;
					clan->fRank3Count++;
				}
				else
				{
					clan->mRank4Count--;
					clan->mRank3Count++;
				}
				break;
			case 3:
				if (victim->sex == SEX_FEMALE)
				{
					clan->fRank3Count--;
					clan->fRank2Count++;
				}
				else
				{
					clan->mRank3Count--;
					clan->mRank2Count++;
				}
				break;
			case 2:
				if( !L1 && !L2 && !L3 && !L4 && !L5 && !L6 )
				{
				  ch_printf(ch,"You can't promote any more leaders now. The leader ranks are full.\n\r");
				  return;
				}
				if( L1 )
				  clan->leader1 = STRALLOC( victim->name );
                                else if( L2 )
                                  clan->leader2 = STRALLOC( victim->name );
                                else if( L3 )
                                  clan->leader3 = STRALLOC( victim->name );
                                else if( L4 )
                                  clan->leader4 = STRALLOC( victim->name );
                                else if( L5 )
                                  clan->leader5 = STRALLOC( victim->name );
                                else if( L6 )
                                  clan->leader6 = STRALLOC( victim->name );
				else
				{
				  ch_printf(ch,"A bug has occured in clanadmin promote. Contact an imm immediately.\n\r");
				  return;
				}

				if( victim->sex == SEX_FEMALE )
				{
					clan->fRank2Count--;
					clan->fRank1Count++;
				}
				else
				{
					clan->mRank2Count--;
					clan->mRank1Count++;
				}
				break;
		}

		victim->pcdata->clanRank--;
		send_to_char( "Done.\n\r", ch );
		save_clan( clan );
		save_char_obj( victim );
		sprintf(buf, "%s has been promoted to %s", victim->name, get_clanTitle(victim));
		echo_to_clan(clan, buf);
		return;
	}

	if ( !str_cmp( arg, "demote" ) )
	{
		if ((victim = get_char_room(ch, argument)) == NULL)
		{
			send_to_char( "They are not here.\n\r", ch );
			return;
		}
                if( ch == victim )
                {
                  ch_printf(ch,"You can't demote yourself.\n\r");
                  return;
                }
		if (IS_NPC(ch) || !ch->pcdata->clan || ch->pcdata->clan != clan)
		{
			send_to_char( "They are not a member of your clan.\n\r", ch );
			return;
		}

		if (is_deity(victim))
		{
			send_to_char( "You can't demote your clans admin.\n\r", ch );
			return;
		}

		if (victim->pcdata->clanRank <= 1 && !is_owner(ch,ch->pcdata->clan))
		{
			/*send_to_char( "You can't demote a leader without contacting your clan admin.\n\r", ch );*/
			ch_printf(ch,"Only the clan admin or owner may demote leaders.\n\r");
			return;
		}

		switch (victim->pcdata->clanRank)
		{
			default:
				send_to_char( "Unknown rank.\n\r", ch );
				bug("clanadmin: undknown rank set for %s(%d)", victim->name, victim->pcdata->clanRank);
				return;
			case 6:
				if (victim->sex == SEX_FEMALE)
				{
					clan->fRank7Count++;
					clan->fRank6Count--;
				}
				else
				{
					clan->mRank7Count++;
					clan->mRank6Count--;
				}
				break;
			case 5:
				if (victim->sex == SEX_FEMALE)
				{
					clan->fRank6Count++;
					clan->fRank5Count--;
				}
				else
				{
					clan->mRank6Count++;
					clan->mRank5Count--;
				}
				break;
			case 4:
				if (victim->sex == SEX_FEMALE)
				{
					clan->fRank5Count++;
					clan->fRank4Count--;
				}
				else
				{
					clan->mRank5Count++;
					clan->mRank4Count--;
				}
				break;
			case 3:
				if (victim->sex == SEX_FEMALE)
				{
					clan->fRank4Count++;
					clan->fRank3Count--;
				}
				else
				{
					clan->mRank4Count++;
					clan->mRank3Count--;
				}
				break;
			case 2:
				if (victim->sex == SEX_FEMALE)
				{
					clan->fRank3Count++;
					clan->fRank2Count--;
				}
				else
				{
					clan->mRank3Count++;
					clan->mRank2Count--;
				}
				break;
                        case 1:
				if( !str_cmp(victim->name,clan->leader1) )
				{
				  STRFREE(clan->leader1);
				  clan->leader1 = STRALLOC( "None" );
				}
                                else if( !str_cmp(victim->name,clan->leader2) )
				{
                                  STRFREE(clan->leader2);
                                  clan->leader2 = STRALLOC( "None" );
				}
                                else if( !str_cmp(victim->name,clan->leader3) )
				{
                                  STRFREE(clan->leader3);
                                  clan->leader3 = STRALLOC( "None" );
				}
                                else if( !str_cmp(victim->name,clan->leader4) )
				{
                                  STRFREE(clan->leader4);
                                  clan->leader4 = STRALLOC( "None" );
				}
                                else if( !str_cmp(victim->name,clan->leader5) )
				{
                                  STRFREE(clan->leader5);
                                  clan->leader5 = STRALLOC( "None" );
				}
                                else if( !str_cmp(victim->name,clan->leader6) )
				{
                                  STRFREE(clan->leader6);
                                  clan->leader6 = STRALLOC( "None" );
				}
				else
                                {
                                  ch_printf(ch,"A bug has occured in clanadmin demote. Contact an imm immediately.\n\r");
                                }
                                if (victim->sex == SEX_FEMALE)
                                {
                                        clan->fRank2Count++;
                                        clan->fRank1Count--;
                                }
                                else
                                {
                                        clan->mRank2Count++;
                                        clan->mRank1Count--;
                                }
                                break;

		}

		victim->pcdata->clanRank++;
		send_to_char( "Done.\n\r", ch );
		save_clan( clan );
		save_char_obj( victim );
		sprintf(buf, "%s has been demoted to %s", victim->name, get_clanTitle(victim));
		echo_to_clan(clan, buf);
		return;
	}

	
	if(!str_cmp(arg, "status"))
	{
	    argument = one_argument( argument, arg2 );
	    argument = one_argument( argument, arg3 );

		if (arg2[0] == '\0' || arg3[0] == '\0')
		{
			send_to_char( "\n\rTo change status type 'clanadmin status [clan] [allied|friendly|neutral|hostile|atwar]'.\n\r", ch );
			show_alliances(ch, clan);
			return;
		}

		if ((vclan = get_clan(arg2)) == NULL)
		{
			send_to_char( "That's not a clan.  Type 'clans' for a list.\n\r", ch );
			return;
		}

		if (clan == vclan)
		{
			send_to_char( "Your clan already has a perfect alliance with its self.\n\r", ch );
			return;
		}

		if(!str_cmp(arg3, "neutral"))
		{
		  if (allianceStatus(clan,vclan) == ALLIANCE_NEUTRAL)
		  {
		    ch_printf(ch,"Your clan already is neutral with them!\n\r");
		    return;
		  }
		  alliance = get_alliance(clan, vclan);
		  alliance->votes = 0;
		  if( str_cmp(alliance->leader1Vote,"None") )
		  {
		    STRFREE(alliance->leader1Vote);
		    alliance->leader1Vote = STRALLOC( "None" );
		  }
		  if( str_cmp(alliance->leader2Vote,"None") )
                  {
                    STRFREE(alliance->leader2Vote);
                    alliance->leader2Vote = STRALLOC( "None" );
                  }
		  
			if (!change_alliance(clan, vclan, ALLIANCE_NEUTRAL))
			{
				/*send_to_char( "Error changing status.  Please notify your clan admin.\n\r", ch );*/
				return;
			}
			/*else
			  send_to_char( "Status changed.\n\r", ch );*/
		}
		else if(!str_cmp(arg3, "friendly"))
		{
		  if (allianceStatus(clan,vclan) == ALLIANCE_FRIENDLY)
                  {
                    ch_printf(ch,"Your clan already is friendly with them!\n\r");
                    return;
                  }
		  alliance = get_alliance(clan, vclan);
                  alliance->votes = 0;
                  if( str_cmp(alliance->leader1Vote,"None") )
                  {
                    STRFREE(alliance->leader1Vote);
                    alliance->leader1Vote = STRALLOC( "None" );
                  }
                  if( str_cmp(alliance->leader2Vote,"None") )
                  {
                    STRFREE(alliance->leader2Vote);
                    alliance->leader2Vote = STRALLOC( "None" );
                  }

			if (!change_alliance(clan, vclan, ALLIANCE_FRIENDLY))
			{
				/*send_to_char( "Error changing status.  Please notify your clan admin.\n\r", ch );*/
				return;
			}
			/*else
			  send_to_char( "Status changed.\n\r", ch );*/
		}
		else if(!str_cmp(arg3, "allied"))
		{
		  if (allianceStatus(clan,vclan) == ALLIANCE_ALLIED)
                  {
                    ch_printf(ch,"Your clan already is allied with them!\n\r");
                    return;
                  }
		  alliance = get_alliance(clan, vclan);
                  alliance->votes = 0;
                  if( str_cmp(alliance->leader1Vote,"None") )
                  {
                    STRFREE(alliance->leader1Vote);
                    alliance->leader1Vote = STRALLOC( "None" );
                  }
                  if( str_cmp(alliance->leader2Vote,"None") )
                  {
                    STRFREE(alliance->leader2Vote);
                    alliance->leader2Vote = STRALLOC( "None" );
                  }

			if (!change_alliance(clan, vclan, ALLIANCE_ALLIED))
			{
				/*send_to_char( "Error changing status.  Please notify your clan admin.\n\r", ch );*/
				return;
			}
			/*else
			  send_to_char( "Status changed.\n\r", ch );*/
			/*
			if (allianceStatus( clan, vclan) == ALLIANCE_ALLIED)
			{
				sprintf(buf, "&C%s and %s are now allies.\n\r", clan->name, vclan->name);
				echo_to_clan(clan, buf);
				echo_to_clan(vclan, buf);
			}*/
		}
		else if(!str_cmp(arg3, "hostile"))
		{
		  if (allianceStatus(clan,vclan) == ALLIANCE_HOSTILE)
                  {
                    ch_printf(ch,"Your clan already is hostile with them!\n\r");
                    return;
                  }
		  alliance = get_alliance(clan, vclan);
                  alliance->votes = 0;
                  if( str_cmp(alliance->leader1Vote,"None") )
                  {
                    STRFREE(alliance->leader1Vote);
                    alliance->leader1Vote = STRALLOC( "None" );
                  }
                  if( str_cmp(alliance->leader2Vote,"None") )
                  {
                    STRFREE(alliance->leader2Vote);
                    alliance->leader2Vote = STRALLOC( "None" );
                  }

			if (!change_alliance(clan, vclan, ALLIANCE_HOSTILE))
			{
				/*send_to_char( "Error changing status.  Please notify your clan admin.\n\r", ch );*/
				return;
			}
			/*else
			  send_to_char( "Status changed.\n\r", ch );*/
		}
		else if(!str_cmp(arg3, "atwar"))
		{

		  if (allianceStatus(clan,vclan) == ALLIANCE_ATWAR)
                  {
                    ch_printf(ch,"Your clan already is at war with them!\n\r");
                    return;
                  }

		    alliance = get_alliance(clan, vclan);
		    
		    if (alliance->votes < 2 )
		    {
			if( alliance->votes == 1 )
			{
			  if (!str_cmp(alliance->leader1Vote,ch->name) )
			  {
			    ch_printf(ch,"You can't vote twice!\n\r");
			    return;
			  }
			  STRFREE(alliance->leader2Vote);
			  
			  alliance->leader2Vote = STRALLOC(ch->name);
			  alliance->votes++;
			  sprintf(buf, "%s has voted to go to war with %s!", ch->name, vclan->name );
                          echo_to_clan(clan, buf);
			  ch_printf(ch,"You have cast the second of two votes neccessary to go to war. You may now change the alliance to AT WAR status.\n\r");
			  return;
			}
			else
			{
			  STRFREE(alliance->leader1Vote);

			  alliance->leader1Vote = STRALLOC(ch->name);
                          alliance->votes++;
			  sprintf(buf, "%s has voted to go to war with %s!", ch->name, vclan->name );
			  echo_to_clan(clan, buf);
			  ch_printf(ch,"You have cast the first of two votes neccessary to go to war.\n\r");
			  return;
			}
		    }
		    else
		    {
			if (!change_alliance(clan, vclan, ALLIANCE_ATWAR))
			{
				/*send_to_char( "Error changing status.  Please notify your clan admin.\n\r", ch );*/
				return;
			}
			/*else
			  alliance_towar(ch, clan, vclan);*/
		    }
		}
		else
		{
			send_to_char( "Choices are: allied, friendly, neutral, hostile, atwar.\n\r", ch );
			return;
		}

		return;
	}
	
	/*send_to_char( "Syntax: [www|recruit|outcast|promote|demote|taxrate|jail|release|evict|evictrank|status]\n\r", ch );*/
	do_clanadmin(ch,"");
	return;
}

void do_resetalliances( CHAR_DATA *ch, char *argument )
{
    CLAN_DATA *clan;
    CLAN_DATA *aclan;
    ALLIANCE_DATA *alliance;

    for( clan = first_clan; clan; clan=clan->next )
    {
        for(aclan = first_clan; aclan; aclan = aclan->next)
        {
            if (aclan == clan)
                continue;

            CREATE( alliance, ALLIANCE_DATA, 1 );
            alliance->clan = clan;
            alliance->vclan = aclan;
            alliance->status = 0;
            alliance->vclanStatus = 0;
            alliance->votes = 0;
            alliance->leader1Vote = STRALLOC( "" );
            alliance->leader2Vote = STRALLOC( "" );
            LINK( alliance, first_alliance, last_alliance, next, prev );
        }
    }
    save_alliance();
    return;
}

void do_clanrecall( CHAR_DATA *ch, char *argument )
{
	 CLAN_DATA *clandata = NULL;
	 char buf[MAX_STRING_LENGTH];
     
         if ( !ch->desc )
                return;

         clandata = ch->pcdata->clan;		 

	 if (!clandata)
	 {
		 send_to_char( "You don\'t have a clan.\n\r", ch);
		 return;
	 }
     if (!clandata->recall)
	{
		send_to_char( "Your clan doesn\'t have a recall point.", ch);
		return;
	}  

	if ( xIS_SET(ch->in_room->room_flags, ROOM_NO_RECALL) 
       ||  IS_AFFECTED( ch, AFF_DEAD ) )
     {
        send_to_char_color( "&C&wYou can\'t go to your clan from here.\n\r", ch );
        return;
     }
	  if ( ch->in_room->vnum == 8)
	  {
		  send_to_char("You're in hell--you can't go to your clan.\n\r",ch);
		  return;
	  }

    if( ch->pcdata->gohometimer > 0 )
    {
	ch_printf( ch, "You can't go to your clan for another %d minutes", ch->pcdata->gohometimer );
	return;
    }

     switch( ch->position )
     {
         default:
          break;
         case POS_MORTAL:
         case POS_INCAP:
         case POS_DEAD:
         case POS_STUNNED:
          send_to_char_color( "&C&wYou can\'t do that, you\'re hurt too badly.\n\r", ch );
          return;
         case POS_SITTING:
         case POS_RESTING:
         case POS_SLEEPING:
          do_wake( ch, "auto" );
          return;
         case POS_FIGHTING:
         case POS_DEFENSIVE:
         case POS_AGGRESSIVE:
         case POS_BERSERK:
         case POS_EVASIVE:
          send_to_char_color( "&C&wNo way!  You are still fighting!\n\r", ch);
          return;
     }

      sprintf( buf, "%s disappears to their clan\'s headquarters.", ch->name );
      act( AT_GREY, buf, ch, NULL, ch, TO_ROOM );


      char_from_room( ch );
      char_to_room( ch, get_room_index(clandata->recall) );

      sprintf( buf, "As a vortex of luminescent light forms, %s slides gracefully through into their clan headquarters.", ch->name );
      act( AT_GREY, buf, ch, NULL, ch, TO_ROOM );

      do_look( ch, "" );

      return;
}

