/****************************************************************************
 *                   ^     +----- |  / ^     ^ |     | +-\                  *
 *                  / \    |      | /  |\   /| |     | |  \                 *
 *                 /   \   +---   |<   | \ / | |     | |  |                 *
 *                /-----\  |      | \  |  v  | |     | |  /                 *
 *               /       \ |      |  \ |     | +-----+ +-/                  *
 ****************************************************************************
 * AFKMud Copyright 1997-2002 Alsherok. Contributors: Samson, Dwip, Whir,   *
 * Cyberfox, Karangi, Rathian, Cam, Raine, and Tarl.                        *
 *                                                                          *
 * Original SMAUG 1.4a written by Thoric (Derek Snider) with Altrag,        *
 * Blodkai, Haus, Narn, Scryn, Swordbearer, Tricops, Gorog, Rennard,        *
 * Grishnakh, Fireblade, and Nivek.                                         *
 *                                                                          *
 * Original MERC 2.1 code by Hatchet, Furey, and Kahn.                      *
 *                                                                          *
 * Original DikuMUD code by: Hans Staerfeldt, Katja Nyboe, Tom Madsen,      *
 * Michael Seifert, and Sebastian Hammer.                                   *
 ****************************************************************************
 *                        Finger and Wizinfo Module                         *
 ****************************************************************************/

/******************************************************
        Additions and changes by Edge of Acedia
              Rewritten do_finger to better
             handle info of offline players.
           E-mail: nevesfirestar2002@yahoo.com
 ******************************************************/

#include <ctype.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>
#include "mud.h"

#if defined(KEY)
#undef KEY
#endif

#define KEY( literal, field, value )					\
				if ( !str_cmp( word, literal ) )	\
				{					\
				      field = value;			\
				      fMatch = TRUE;			\
				      break;				\
				}

/* Begin wizinfo stuff - Samson 6-6-99 */

bool     check_parse_name        args( ( char *name, bool newchar ) );

WIZINFO_DATA * first_wizinfo;
WIZINFO_DATA * last_wizinfo;

/* Construct wizinfo list from god dir info - Samson 6-6-99 */
void add_to_wizinfo( char *name, WIZINFO_DATA *wiz )
{
   WIZINFO_DATA *wiz_prev;

   wiz->name = str_dup( name );
   if ( !wiz->email )
      wiz->email = str_dup( "Not Set" );

   for( wiz_prev = first_wizinfo; wiz_prev; wiz_prev = wiz_prev->next )
      if( strcasecmp( wiz_prev->name, name ) >= 0 )
         break;

   if( !wiz_prev )
      LINK( wiz, first_wizinfo, last_wizinfo, next, prev );
   else
      INSERT( wiz, wiz_prev, first_wizinfo, next, prev );

   return;
}

void clear_wizinfo( bool bootup )
{
   WIZINFO_DATA *wiz, *next;

   if( !bootup )
   {
     for ( wiz = first_wizinfo; wiz; wiz = next )
     {
        next = wiz->next;
        UNLINK( wiz, first_wizinfo, last_wizinfo, next, prev );
	  DISPOSE( wiz->name );
	  DISPOSE( wiz->email );
        DISPOSE( wiz );
     }
   }

   first_wizinfo = NULL;
   last_wizinfo = NULL;

   return;
}

void fread_info( WIZINFO_DATA *wiz, FILE *fp )
{
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

	case 'E':
	    KEY( "Email", 	wiz->email,		fread_string_nohash( fp ) );
	    if ( !str_cmp( word, "End" ) )
		return;
	    break;

	case 'I':
	    KEY( "ICQ",	wiz->icq,	fread_number( fp ) );
	    break;

	case 'L':
	    KEY( "Level", wiz->level,	fread_number( fp ) );
	    break;
	}

	if ( !fMatch )
	    fread_to_eol( fp );
    }
}

void build_wizinfo( bool bootup )
{
   DIR *dp;
   struct dirent *dentry;
   FILE *fp;
   WIZINFO_DATA *wiz;
   char buf[MAX_STRING_LENGTH];

   clear_wizinfo( bootup ); /* Clear out the table before rebuilding a new one */

   dp = opendir( GOD_DIR );

   dentry = readdir( dp );

   while ( dentry )
   {
      /* Added by Tarl 3 Dec 02 because we are now using CVS */
      if( !str_cmp( dentry->d_name, "CVS" ) )
      {
         dentry = readdir( dp );
         continue;
      }
      if ( dentry->d_name[0] != '.' )
      {
	  sprintf( buf, "%s%s", GOD_DIR, dentry->d_name );
	  fp = fopen( buf, "r" );
	  if ( fp )
	  {
	    CREATE( wiz, WIZINFO_DATA, 1 );
          fread_info( wiz, fp );
	    add_to_wizinfo( dentry->d_name, wiz );
	    FCLOSE( fp );
	  }
      }
      dentry = readdir( dp );
   }
   closedir( dp );
   return;
}

/*
 * Wizinfo information.
 * Added by Samson on 6-6-99
 */
void do_wizinfo( CHAR_DATA *ch, char *argument )
{
   WIZINFO_DATA *wiz;
   char buf[MAX_STRING_LENGTH];

   send_to_pager( "Contact Information for the Immortals:\n\r\n\r", ch );
   send_to_pager( "Name         Email Address                     ICQ#\n\r", ch );
   send_to_pager( "------------+---------------------------------+----------\n\r", ch );

   for ( wiz = first_wizinfo; wiz; wiz = wiz->next )
   {
      sprintf( buf, "%-12s %-33s %10d", wiz->name, wiz->email, wiz->icq );
	strcat( buf, "\n\r" );
      send_to_pager( buf, ch );
   }
   return;
}

/* End wizinfo stuff - Samson 6-6-99 */

/* Finger snippet courtesy of unknown author. Installed by Samson 4-6-98 */
/* File read/write code redone using standard Smaug I/O routines - Samson 9-12-98 */
/* Data gathering now done via the pfiles, eliminated separate finger files - Samson 12-21-98 */
/* Improvements for offline players by Edge of Acedia 8-26-03 */
/* Further refined by Samson on 8-26-03 */
void do_finger( CHAR_DATA *ch, char *argument )
{
   CHAR_DATA *victim = NULL;
   CMDTYPE *command;
   ROOM_INDEX_DATA *temproom, *original = NULL;
   int level = LEVEL_IMMORTAL;
   char buf[MAX_STRING_LENGTH], fingload[MAX_INPUT_LENGTH];
   struct stat fst;
   char *laston = NULL;
   bool loaded = FALSE, skip = FALSE;

   if( IS_NPC(ch) )
   {
      send_to_char( "Mobs can't use the finger command.\n\r", ch );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Finger whom?\n\r", ch );
      return;
   }

   sprintf( buf, "0.%s", argument );

   /* If player is online, check for fingerability (yeah, I coined that one)	-Edge */
   if( ( victim = get_char_world( ch, buf ) ) != NULL )
   {
	if( IS_SET( victim->pcdata->flags, PCFLAG_PRIVACY ) && !IS_IMMORTAL(ch) )
    	{
	   ch_printf( ch, "%s has privacy enabled.\n\r", victim->name );
	   return;
    	}

    	if ( IS_IMMORTAL(victim) && !IS_IMMORTAL(ch) )
    	{
	   send_to_char( "You cannot finger an immortal.\n\r", ch );
	   return;
    	}
   }

   /* Check for offline players - Edge */
   else
   {
      DESCRIPTOR_DATA *d;

      sprintf( fingload, "%s%c/%s", PLAYER_DIR, tolower(argument[0]), capitalize( argument ) );
      /* Bug fix here provided by Senir to stop /dev/null crash */
      if( stat( fingload, &fst ) == -1 || !check_parse_name( capitalize( argument ), FALSE ) )
      {
         ch_printf( ch, "&YNo such player named '%s'.\n\r", argument );
         return;
      }

      /*laston = ctime( &fst.st_mtime );*/
      temproom = get_room_index( ROOM_VNUM_LIMBO );
      if( !temproom )
      {
         bug( "%s", "do_finger: Limbo room is not available!" );
         send_to_char( "Fatal error, report to the immortals.\n\r", ch );
         return;
      }

	CREATE( d, DESCRIPTOR_DATA, 1 );
	d->next = NULL;
	d->prev = NULL;
	d->connected = CON_GET_NAME;
	d->outsize = 2000;
	CREATE( d->outbuf, char, d->outsize );
      argument[0] = UPPER( argument[0] );

	loaded = load_char_obj( d, argument, FALSE ); /* Remove second FALSE if compiler complains */
      LINK( d->character, first_char, last_char, next, prev );
	original = d->character->in_room;
	char_to_room( d->character, temproom );
	victim = d->character; /* Hopefully this will work, if not, we're SOL */
	d->character->desc	= NULL;
	d->character		= NULL;
	DISPOSE( d->outbuf );
	DISPOSE( d );

	/* Link dead check?  Was crashing on "IP Info" line below
	 * hopefully this will fix it. -Goku 10.11.03 */
	if (!victim->desc)
		loaded = FALSE;

	if( IS_SET( victim->pcdata->flags, PCFLAG_PRIVACY ) && !IS_IMMORTAL(ch) )
    	{
	   ch_printf( ch, "%s has privacy enabled.\n\r", victim->name );
         skip = TRUE;
    	}

    	if( IS_IMMORTAL(victim) && !IS_IMMORTAL(ch) )
    	{
	   send_to_char( "You cannot finger an immortal.\n\r", ch );
         skip = TRUE;
    	}
      loaded = TRUE;
   }

   if( !skip )
   {
      send_to_char( "&w          Finger Info\n\r", ch );
      send_to_char( "          -----------\n\r", ch );
      ch_printf( ch, "&wName    : &G%-20s &wMUD Age: &G%d\n\r", victim->name, get_newage( victim ) );
      ch_printf( ch, "&wRank    : &G%-20s &wRace   : &G%s\n\r",
		get_rank(victim), capitalize( get_race(victim) ) );
      ch_printf( ch, "&wSex     : &G%-20s\n\r",
                victim->sex == SEX_MALE   ? "Male"   :
                victim->sex == SEX_FEMALE ? "Female" : "Neutral" );
      ch_printf( ch, "&wTitle   : &G%s\n\r", victim->pcdata->title );
      ch_printf( ch, "&wHomepage: &G%s\n\r", victim->pcdata->homepage != NULL ? victim->pcdata->homepage : "Not specified" );
      ch_printf( ch, "&wEmail   : &G%s\n\r", victim->pcdata->email != NULL ? victim->pcdata->email : "Not specified" );
      ch_printf( ch, "&wICQ#    : &G%d\n\r", victim->pcdata->icq );
      if( loaded )
         ch_printf( ch, "&wLast on : &G%s\n\r", ctime( &victim->pcdata->lastlogon ) );
      else
         ch_printf( ch, "&wLast on : &GCurrently Online\n\n\r", laston );
      if ( IS_IMMORTAL(ch) )
      {
	   send_to_char( "&wImmortal Information\n\r", ch );
	   send_to_char( "--------------------\n\r", ch );
//	   ch_printf( ch, "&wIP Info       : &G%s\n\r", loaded ? "Unknown" : victim->desc->host );
	   ch_printf( ch, "&wTime played   : &G%ld hours\n\r", (long int)GET_TIME_PLAYED( victim ) );
         ch_printf( ch, "&wAuthorized by : &G%s\n\r",
	      victim->pcdata->authed_by ? victim->pcdata->authed_by : ( sysdata.WAIT_FOR_AUTH ? "Not Authed" : "The Code" ) );
         ch_printf( ch, "&wPrivacy Status: &G%s\n\r", IS_SET( victim->pcdata->flags, PCFLAG_PRIVACY ) ? "Enabled" : "Disabled" );
         if( victim->level < ch->level )
         {
            /* Added by Tarl 19 Dec 02 to remove Huh? when ch not high enough to view comments. */
            command = find_command( "comment" );
            if( !command )
               level = LEVEL_IMMORTAL;
            else
               level = command->level;
            if( ch->level >= command->level )
            {
               sprintf( buf, "comment list %s", victim->name );
               interpret( ch, buf );
            }
         }
      }
		pager_printf_color (ch, "&wBio:\n\r&G%s\n\r", victim->pcdata->bio ? victim->pcdata->bio : "Not created" );
		pager_printf_color (ch, "\n\r&wDescription:\n\r&G%s\n\r", victim->description ? victim->description : "Not created" );
   }

   if( loaded )
   {
      int x, y;

      char_from_room( victim );
	char_to_room( victim, original );

      quitting_char = victim;
      /*save_char_obj( victim );*/

	if( sysdata.save_pets && victim->pcdata->pet )
	   extract_char( victim->pcdata->pet, TRUE );

      saving_char = NULL;

      /*
       * After extract_char the ch is no longer valid!
       */
      extract_char( victim, TRUE );
      for ( x = 0; x < MAX_WEAR; x++ )
	  for ( y = 0; y < MAX_LAYERS; y++ )
	    save_equipment[x][y] = NULL;
   }
   return;
}

/* Added a clone of homepage to let players input their email addy - Samson 4-18-98 */
void do_email( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];

    if ( IS_NPC(ch) )
	return;

    if ( argument[0] == '\0' )
    {
	if ( !ch->pcdata->email )
	  ch->pcdata->email = str_dup( "" );
	ch_printf( ch, "Your email address is: %s\n\r",	show_tilde( ch->pcdata->email ) );
	return;
    }

    if ( !str_cmp( argument, "clear" ) )
    {
	if ( ch->pcdata->email )
	  DISPOSE(ch->pcdata->email);
	ch->pcdata->email = str_dup("");

      if ( IS_IMMORTAL( ch ) );
      {
	  save_char_obj( ch );
	  build_wizinfo( FALSE );
      }

	send_to_char( "Email address cleared.\n\r", ch );
	return;
    }

    strcpy( buf, argument );

    if ( strlen(buf) > 70 )
	buf[70] = '\0';

    hide_tilde( buf );
    if ( ch->pcdata->email )
      DISPOSE(ch->pcdata->email);
    ch->pcdata->email = str_dup(buf);
    if ( IS_IMMORTAL( ch ) );
    {
	save_char_obj( ch );
	build_wizinfo( FALSE );
    }
    send_to_char( "Email address set.\n\r", ch );
}

void do_icq_number( CHAR_DATA *ch, char *argument )
{
    int icq;

    if ( IS_NPC( ch ) )
	return;

    if ( argument[0] == '\0' )
    {
	if ( !ch->pcdata->icq )
	  ch->pcdata->icq = 0;
	ch_printf( ch, "Your ICQ# is: %d\n\r", ch->pcdata->icq );
	return;
    }

    if ( !str_cmp( argument, "clear" ) )
    {
	ch->pcdata->icq = 0;

      if ( IS_IMMORTAL( ch ) );
      {
	  save_char_obj( ch );
	  build_wizinfo( FALSE );
      }

	send_to_char( "ICQ# cleared.\n\r", ch );
	return;
    }

    if ( !is_number( argument ) )
    {
	send_to_char( "You must enter numeric data.\n\r", ch );
	return;
    }

    icq = atoi( argument );

    if ( icq < 1 )
    {
	send_to_char( "Valid range is greater than 0.\n\r", ch );
	return;
    }

    ch->pcdata->icq = icq;

    if ( IS_IMMORTAL( ch ) );
    {
	save_char_obj( ch );
	build_wizinfo( FALSE );
    }

    send_to_char( "ICQ# set.\n\r", ch );
    return;
}

void do_homepage( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];

    if ( IS_NPC(ch) )
	return;

    if ( !argument || argument[0] == '\0' )
    {
	if ( !ch->pcdata->homepage )
	  ch->pcdata->homepage = str_dup( "" );
	ch_printf( ch, "Your homepage is: %s\n\r", show_tilde( ch->pcdata->homepage ) );
	return;
    }

    if ( !str_cmp( argument, "clear" ) )
    {
	if ( ch->pcdata->homepage )
	  DISPOSE(ch->pcdata->homepage);
	ch->pcdata->homepage = str_dup("");
	send_to_char( "Homepage cleared.\n\r", ch );
	return;
    }

    if ( strstr( argument, "://" ) )
	strcpy( buf, argument );
    else
	sprintf( buf, "http://%s", argument );
    if ( strlen(buf) > 70 )
	buf[70] = '\0';

    hide_tilde( buf );
    if ( ch->pcdata->homepage )
      DISPOSE(ch->pcdata->homepage);
    ch->pcdata->homepage = str_dup(buf);
    send_to_char( "Homepage set.\n\r", ch );
}

void do_privacy( CHAR_DATA *ch, char *argument )
{
   if ( IS_NPC( ch ) )
   {
	send_to_char( "Mobs can't use the privacy toggle.\n\r", ch );
	return;
   }

   TOGGLE_BIT( ch->pcdata->flags, PCFLAG_PRIVACY );

   if ( IS_SET( ch->pcdata->flags, PCFLAG_PRIVACY ) )
   {
	send_to_char( "Privacy flag enabled.\n\r", ch );
	return;
   }
   else
   {
	send_to_char( "Privacy flag disabled.\n\r", ch );
	return;
   }
}

/*
 * basicly a copy of do_finger but checks pfiles in the backup dir -Goku 09.29.04
 */
void do_backfinger( CHAR_DATA *ch, char *argument )
{
   CHAR_DATA *victim = NULL;
   CMDTYPE *command;
   ROOM_INDEX_DATA *temproom, *original = NULL;
   int level = LEVEL_IMMORTAL;
   char buf[MAX_STRING_LENGTH], fingload[MAX_INPUT_LENGTH];
   char *laston = NULL;
   struct stat fst;
   bool loaded = FALSE, skip = FALSE;
      DESCRIPTOR_DATA *d;

   if( IS_NPC(ch) )
   {
      send_to_char( "Mobs can't use the finger command.\n\r", ch );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Back finger whom?\n\r", ch );
      return;
   }

   sprintf( buf, "0.%s", argument );


      sprintf( fingload, "%s%c/%s", BACKUP_DIR, tolower(argument[0]), capitalize( argument ) );
      /* Bug fix here provided by Senir to stop /dev/null crash */
      if( stat( fingload, &fst ) == -1 || !check_parse_name( capitalize( argument ), FALSE ) )
      {
         ch_printf( ch, "&YNo such player named '%s'.\n\r", argument );
         return;
      }

      /*laston = ctime( &fst.st_mtime );*/
      temproom = get_room_index( ROOM_VNUM_LIMBO );
      if( !temproom )
      {
         bug( "%s", "do_backfinger: Limbo room is not available!" );
         send_to_char( "Fatal error, report to the immortals.\n\r", ch );
         return;
      }

	CREATE( d, DESCRIPTOR_DATA, 1 );
	d->next = NULL;
	d->prev = NULL;
	d->connected = CON_GET_NAME;
	d->outsize = 2000;
	CREATE( d->outbuf, char, d->outsize );
      argument[0] = UPPER( argument[0] );

	loaded = load_char_obj( d, argument, FALSE ); /* Remove second FALSE if compiler complains */
      LINK( d->character, first_char, last_char, next, prev );
	original = d->character->in_room;
	char_to_room( d->character, temproom );
	victim = d->character; /* Hopefully this will work, if not, we're SOL */
	d->character->desc	= NULL;
	d->character		= NULL;
	DISPOSE( d->outbuf );
	DISPOSE( d );

	/* Link dead check?  Was crashing on "IP Info" line below
	 * hopefully this will fix it. -Goku 10.11.03 */
	if (!victim->desc)
		loaded = FALSE;

	if( IS_SET( victim->pcdata->flags, PCFLAG_PRIVACY ) && !IS_IMMORTAL(ch) )
    	{
	   ch_printf( ch, "%s has privacy enabled.\n\r", victim->name );
         skip = TRUE;
    	}

    	if( IS_IMMORTAL(victim) && !IS_IMMORTAL(ch) )
    	{
	   send_to_char( "You cannot back finger an immortal.\n\r", ch );
         skip = TRUE;
    	}
      loaded = TRUE;

   if( !skip )
   {
      send_to_char( "&w          Back Finger Info\n\r", ch );
      send_to_char( "          -----------\n\r", ch );
      ch_printf( ch, "&wName    : &G%-20s &wMUD Age: &G%d\n\r", victim->name, get_newage( victim ) );
      ch_printf( ch, "&wRank    : &G%-20s &wRace   : &G%s\n\r",
		get_rank(victim), capitalize( get_race(victim) ) );
      ch_printf( ch, "&wSex     : &G%-20s\n\r",
                victim->sex == SEX_MALE   ? "Male"   :
                victim->sex == SEX_FEMALE ? "Female" : "Neutral" );
      ch_printf( ch, "&wTitle   : &G%s\n\r", victim->pcdata->title );
      ch_printf( ch, "&wHomepage: &G%s\n\r", victim->pcdata->homepage != NULL ? victim->pcdata->homepage : "Not specified" );
      ch_printf( ch, "&wEmail   : &G%s\n\r", victim->pcdata->email != NULL ? victim->pcdata->email : "Not specified" );
      ch_printf( ch, "&wICQ#    : &G%d\n\r", victim->pcdata->icq );
      if( loaded )
         ch_printf( ch, "&wLast on : &G%s\n\r", ctime( &victim->pcdata->lastlogon ) );
      else
         ch_printf( ch, "&wLast on : &GCurrently Online\n\n\r", laston );
      if ( IS_IMMORTAL(ch) )
      {
	   send_to_char( "&wImmortal Information\n\r", ch );
	   send_to_char( "--------------------\n\r", ch );
//	   ch_printf( ch, "&wIP Info       : &G%s\n\r", loaded ? "Unknown" : victim->desc->host );
	   ch_printf( ch, "&wTime played   : &G%ld hours\n\r", (long int)GET_TIME_PLAYED( victim ) );
         ch_printf( ch, "&wAuthorized by : &G%s\n\r",
	      victim->pcdata->authed_by ? victim->pcdata->authed_by : ( sysdata.WAIT_FOR_AUTH ? "Not Authed" : "The Code" ) );
         ch_printf( ch, "&wPrivacy Status: &G%s\n\r", IS_SET( victim->pcdata->flags, PCFLAG_PRIVACY ) ? "Enabled" : "Disabled" );
         if( victim->level < ch->level )
         {
            /* Added by Tarl 19 Dec 02 to remove Huh? when ch not high enough to view comments. */
            command = find_command( "comment" );
            if( !command )
               level = LEVEL_IMMORTAL;
            else
               level = command->level;
            if( ch->level >= command->level )
            {
               sprintf( buf, "comment list %s", victim->name );
               interpret( ch, buf );
            }
         }
      }
		pager_printf_color (ch, "&wBio:\n\r&G%s\n\r", victim->pcdata->bio ? victim->pcdata->bio : "Not created" );
		pager_printf_color (ch, "\n\r&wDescription:\n\r&G%s\n\r", victim->description ? victim->description : "Not created" );
   }

   if( loaded )
   {
      int x, y;

      char_from_room( victim );
	char_to_room( victim, original );

      quitting_char = victim;
      /*save_char_obj( victim );*/

	if( sysdata.save_pets && victim->pcdata->pet )
	   extract_char( victim->pcdata->pet, TRUE );

      saving_char = NULL;

      /*
       * After extract_char the ch is no longer valid!
       */
      extract_char( victim, TRUE );
      for ( x = 0; x < MAX_WEAR; x++ )
	  for ( y = 0; y < MAX_LAYERS; y++ )
	    save_equipment[x][y] = NULL;
   }
   return;
}

void fread_pfile2( CHAR_DATA *ch, FILE *fp )
{
    char *word;
    char *name = NULL;
    sh_int level = 0;
    sh_int file_ver = 0;
    bool fMatch;
        long double exp = 0;
        char *          description = NULL;
	char *		bio = NULL;
    
    for ( ; ; )
    {
        word   = feof( fp ) ? "End" : fread_word( fp );
        fMatch = FALSE;

/*        if (StopFP)
        {
                bug("Bad Pfile detected.  Stoping proccessing of bad Pfile.");
                StopFP = FALSE;
                return;
        }
*/        switch ( UPPER(word[0]) )
        {

        case '*':
            fMatch = TRUE;
            fread_to_eol( fp );
            break;

	case 'B':
            KEY( "Bio",bio,fread_string(fp));
            break;
        case 'D':
            KEY( "Description", description,    fread_string( fp ) );
            break;

	case 'E':
                KEY( "Exp",             exp,            fread_number_ld( fp ) );
            if ( !strcmp( word, "End" ) )
		goto timecheck;
            break;

        case 'L':
            KEY( "Level",               level,  fread_number( fp ) );
            break;

        case 'N':
            KEY( "Name",                name,   fread_string( fp ) );
            break;

        case 'V':
            KEY( "Version",     file_ver,       fread_number( fp ) );
            break;
        }

        if ( !fMatch )
           fread_to_eol( fp );
    }
timecheck:
{
    send_to_char( "Name: ", ch );
    send_to_char( name, ch );
    send_to_char( "\n\rBio:\n\r", ch );
    send_to_char( bio, ch );
    send_to_char( "\n\r\n\rDesc:\n\r", ch );
    send_to_char( description, ch );
}
}

void do_finger2( CHAR_DATA *ch, char *argument )
{
   char buf[MAX_STRING_LENGTH], fingload[MAX_INPUT_LENGTH];
   FILE *fp;

   if( IS_NPC(ch) )
   {
      send_to_char( "Mobs can't use the finger command.\n\r", ch );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Finger whom?\n\r", ch );
      return;
   }

   sprintf( buf, "0.%s", argument );

   /* Check for offline players - Edge */
//   else
   {
      sprintf( fingload, "%s%c/%s", BACKUP_DIR, tolower(argument[0]), capitalize( argument ) );
      /* Bug fix here provided by Senir to stop /dev/null crash */

	if( ( fp = fopen( fingload, "r" ) ) == NULL )
	{
		send_to_char( "Error.\n\r", ch );
		return;
	}

	for ( ; ; )
        {
            char letter;
            char *word;
/*            if (StopFP)   
            {
                bug("Bad Pfile detected.  Stoping proccessing of bad Pfile.");
                StopFP = FALSE;
                return;
            }
*/            letter = fread_letter( fp );

            if ( letter != '#' )
               continue;

            word = fread_word( fp );

            if ( !str_cmp( word, "End" ) )
                break;

            if ( !str_cmp( word, "PLAYER" ) )
                fread_pfile2( ch, fp );
            else
            if ( !str_cmp( word, "END" ) )      /* Done         */
                break;
        }
        fclose( fp );          

   }
   return;
}

