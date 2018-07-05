/****************************************************************************
 *                   ^     +----- |  / ^     ^ |     | +-\                  *
 *                  / \    |      | /  |\   /| |     | |  \                 *
 *                 /   \   +---   |<   | \ / | |     | |  |                 *
 *                /-----\  |      | \  |  v  | |     | |  /                 *
 *               /       \ |      |  \ |     | +-----+ +-/                  *
 ****************************************************************************
 * AFKMud Copyright 1997-2003 by Roger Libiez (Samson),                     *
 * Levi Beckerson (Whir), Michael Ward (Tarl), Erik Wolfe (Dwip),           *
 * Cameron Carroll (Cam), Cyberfox, Karangi, Rathian, Raine, and Adjani.    *
 * All Rights Reserved.                                                     *
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
 *                      New Name Authorization module                       *
 ****************************************************************************/

/*
 *  New name authorization system
 *  Author: Rantic (supfly@geocities.com)
 *  of FrozenMUD (empire.digiunix.net 4000)
 *
 *  Permission to use and distribute this code is granted provided
 *  this header is retained and unaltered, and the distribution
 *  package contains all the original files unmodified.
 *  If you modify this code and use/distribute modified versions
 *  you must give credit to the original author(s).
 */

#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>
#include "mud.h"

/* from comm.c, for do_name */
bool check_parse_name( char *name, bool newchar );

/* from act_wiz.c, for do_authorize */
CHAR_DATA *get_waiting_desc( CHAR_DATA *ch, char *name );

AUTH_LIST *first_auth_name;
AUTH_LIST *last_auth_name;

bool exists_player( char *name )
{
   struct stat fst;
   char buf[256];
   CHAR_DATA *victim = NULL;

   /* Stands to reason that if there ain't a name to look at, they damn well don't exist! */
   if( !name || !str_cmp( name, "" ) )
	return FALSE;

   sprintf( buf, "%s%c/%s", PLAYER_DIR, tolower( name[0] ), capitalize( name ) );

   if( stat( buf, &fst ) != -1 )
	return TRUE;
   else if( ( victim = get_char_world( supermob, name ) ) != NULL )
	return TRUE;
   return FALSE;
}

void free_auth_entry( AUTH_LIST *auth )
{
   UNLINK( auth, first_auth_name, last_auth_name, next, prev );
   STRFREE( auth->authed_by );
   STRFREE( auth->change_by );
   STRFREE( auth->name );
   DISPOSE( auth );
}

void free_all_auths( void )
{
   AUTH_LIST *auth, *auth_next;

   for( auth = first_auth_name; auth; auth = auth_next )
   {
      auth_next = auth->next;
      free_auth_entry( auth );
   }
   return;
}

void clean_auth_list( void )
{
   AUTH_LIST *auth, *nauth;

   for( auth = first_auth_name; auth; auth = nauth )
   {
      nauth = auth->next;

      if( !exists_player( auth->name ) )
         free_auth_entry( auth );
      else
      {
         time_t tdiff = 0;
         time_t curr_time = time(0);
         struct stat fst;
         char file[256], name[MAX_STRING_LENGTH];
         int MAX_AUTH_WAIT = 7;

         strcpy( name, auth->name );
         sprintf( file, "%s%c/%s", PLAYER_DIR, LOWER( auth->name[0] ), capitalize( auth->name ) );

         if( stat( file, &fst ) != -1 )
            tdiff = (curr_time - fst.st_mtime)/86400;
         else
            bug( "File %s does not exist!", file );

         if( tdiff > MAX_AUTH_WAIT )
         {
            if( unlink(file) == -1 )
               perror( "Unlink: do_auth: \"clean\"" );
            else
		{
		   sprintf( log_buf, "%s deleted for inactivity: %ld days", file, (long int)tdiff );
               log_string( log_buf );
		}
         }
      }
   }
}

void write_auth_file( FILE *fpout, AUTH_LIST *list )
{
   fprintf( fpout, "Name     %s~\n", list->name );
   fprintf( fpout, "State    %d\n", list->state );
   if( list->authed_by )
      fprintf( fpout, "AuthedBy %s~\n", list->authed_by );
   if( list->change_by )
      fprintf( fpout, "Change   %s~\n", list->change_by );
   fprintf( fpout, "%s", "End\n\n" );
}

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

void fread_auth( FILE *fp )
{
   AUTH_LIST *new_auth;
   bool fMatch;
   char *word;

   CREATE( new_auth, AUTH_LIST, 1 );

   new_auth->authed_by = NULL;
   new_auth->change_by = NULL;

   for( ; ; )
   {
      word = feof( fp ) ? "End" : fread_word ( fp );
      fMatch = FALSE;
      switch( UPPER( word[0] ) )
      {
         case '*':
            fMatch = TRUE;
            fread_to_eol( fp );
            break;

         case 'A':
            KEY( "AuthedBy",	new_auth->authed_by,	fread_string( fp ) );
            break;

         case 'C':
            KEY( "Change",	new_auth->change_by,	fread_string( fp ) );
            break;

         case 'E':
            if( !str_cmp( word, "End" ) )
            {
               LINK( new_auth, first_auth_name, last_auth_name, next, prev );
               return;
            }
            break;

         case 'N':
            KEY( "Name",	new_auth->name,	fread_string( fp ) );
            break;

         case 'S':
            if( !str_cmp( word, "State" ) )
            {
               new_auth->state = fread_number( fp );
               /* Crash proofing. Can't be online when */
               /* booting up. Would suck for do_auth   */
               if( new_auth->state == AUTH_ONLINE || new_auth->state == AUTH_LINK_DEAD )
                  new_auth->state = AUTH_OFFLINE;
               fMatch = TRUE;
               break;
            }
            break;
      }
      if( !fMatch )
         bug( "Fread_auth: no match: %s", word );
   }
}

void save_auth_list( void )
{
   FILE *fpout;
   AUTH_LIST *list;

   if( !( fpout = fopen( AUTH_FILE, "w" ) ) )
   {
      bug( "%s", "Cannot open auth.dat for writing." );
      perror( AUTH_FILE );
      return;
   }

   for( list = first_auth_name; list; list = list->next )
   {
      fprintf( fpout, "%s", "#AUTH\n" );
      write_auth_file( fpout, list );
   }

   fprintf( fpout, "%s", "#END\n" );
   fclose( fpout );
   fpout = NULL;
}

void clear_auth_list( void )
{
   AUTH_LIST *auth, *nauth;

   for( auth = first_auth_name; auth; auth = nauth )
   {
      nauth = auth->next;

      if( !exists_player( auth->name ) )
         free_auth_entry( auth );
   }
   save_auth_list();
}

void load_auth_list( void )
{
   FILE *fp;
   int x;

   first_auth_name = last_auth_name = NULL;

   if( ( fp = fopen( AUTH_FILE, "r" ) ) )
   {
      x = 0;
      for( ; ; )
      {
         char letter;
         char *word;

         letter = fread_letter( fp );
         if( letter == '*' )
         {
            fread_to_eol( fp );
            continue;
         }

         if( letter != '#' )
         {
            bug( "%s", "Load_auth_list: # not found." );
            break;
         }

         word = fread_word( fp );
         if( !str_cmp( word, "AUTH" ) )
         {
            fread_auth( fp );
            continue;
         }
         else if( !str_cmp( word, "END" ) )
            break;
         else
         {
            bug( "%s", "load_auth_list: bad section." );
            continue;
         }
      }
      fclose( fp );
      fp = NULL;
   }
   else
   {
      bug( "%s", "Cannot open auth.dat" );
      return;
   }
   clear_auth_list();
}

int get_auth_state( CHAR_DATA *ch )
{
   AUTH_LIST *namestate;
   int state;

   state = AUTH_AUTHED;

   for( namestate = first_auth_name; namestate; namestate = namestate->next )
   {
      if( !str_cmp( namestate->name, ch->name ) )
      {
         state = namestate->state;
         break;
      }
   }
   return state;
}

AUTH_LIST *get_auth_name( char *name )
{
   AUTH_LIST *mname;

   if( last_auth_name && last_auth_name->next != NULL )
      bug( "Last_auth_name->next != NULL: %s", last_auth_name->next->name );

   for( mname = first_auth_name; mname; mname = mname->next )
   {
      if( !str_cmp( mname->name, name ) ) /* If the name is already in the list, break */
         break;
   }
   return mname;
}

void add_to_auth( CHAR_DATA *ch )
{
   AUTH_LIST *new_name;

   new_name = get_auth_name( ch->name );
   if( new_name != NULL )
      return;
   else
   {
      CREATE( new_name, AUTH_LIST, 1 );
      new_name->name = STRALLOC( ch->name );
      new_name->state = AUTH_ONLINE;        /* Just entered the game */
      LINK( new_name, first_auth_name, last_auth_name, next, prev );
      save_auth_list();
   }
}

void remove_from_auth( char *name )
{
   AUTH_LIST *old_name;

   old_name = get_auth_name( name );
   if( old_name == NULL ) /* Its not old */
      return;
   else
   {
      free_auth_entry( old_name );
      save_auth_list();
   }
}

void check_auth_state( CHAR_DATA *ch )
{
	AUTH_LIST *old_auth;
      CMDTYPE *command;
      int level = LEVEL_IMMORTAL;

      command = find_command( "authorize" );
      if ( !command )
 	  level = LEVEL_IMMORTAL;
      else
	  level = command->level;

	old_auth = get_auth_name( ch->name );
	if ( old_auth == NULL )
		return;

	if ( old_auth->state == AUTH_OFFLINE /* checking as they enter the game */
	|| old_auth->state == AUTH_LINK_DEAD )
	{
		old_auth->state = AUTH_ONLINE;
		save_auth_list();
	}
	else if ( old_auth->state == AUTH_CHANGE_NAME )
	{
		ch_printf_color(ch,
		  "&R\n\rThe MUD Administrators have found the name %s\n\r"
              "to be unacceptable. You must choose a new one.\n\r"
		  "The name you choose must be medieval and original.\n\r"
		  "No titles, descriptive words, or names close to any existing\n\r"
  		  "Immortal's name. See 'help name'.\n\r", ch->name);
	}
	else if ( old_auth->state == AUTH_AUTHED )
	{
		if ( ch->pcdata->authed_by )
		    STRFREE( ch->pcdata->authed_by );
		if( old_auth->authed_by )
		{
			ch->pcdata->authed_by = QUICKLINK( old_auth->authed_by );
			STRFREE( old_auth->authed_by );
		}
		else
			ch->pcdata->authed_by = STRALLOC( "The Code" );

	      	ch_printf_color( ch,
		  "\n\r&GThe MUD Administrators have accepted the name %s.\n\r"
		  "You are now free to roam the %s.\n\r", ch->name, sysdata.mud_name );
		REMOVE_BIT(ch->pcdata->flags, PCFLAG_UNAUTHED);
		remove_from_auth( ch->name );
		return;
	}
	return;
}

/* new auth */
void do_authorize( CHAR_DATA *ch, char *argument )
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim = NULL;
    AUTH_LIST *auth;
    CMDTYPE *command;
    int level = LEVEL_IMMORTAL;
    bool offline, authed, changename, pending;
    int count = 0;
	DESCRIPTOR_DATA *d;

    offline = authed = changename = pending = FALSE;
    auth = NULL;

    /* Checks level of authorize command, for log messages. - Samson 10-18-98 */
    command = find_command( "authorize" );
    if ( !command )
 	level = LEVEL_IMMORTAL;
    else
	level = command->level;

    set_char_color( AT_IMMORT, ch );

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    if ( arg1[0] == '\0' )
    {
	send_to_char( "To approve a waiting character: auth <name>\n\r", ch );
	send_to_char( "To deny a waiting character:    auth <name> reject\n\r", ch );
	send_to_char( "To approve a waiting bio: auth <name> bio/chat\n\r", ch );
	send_to_char( "To deny a waiting bio:    auth <name> badbio/badchat\n\r", ch );
	send_to_char( "To ask a waiting character to change names: auth <name> change\n\r", ch );
	send_to_char( "To have the code verify the list: auth fixlist\n\r", ch );
	send_to_char( "To have the code purge inactive entries: auth clean\n\r", ch );

      set_char_color( AT_DIVIDER, ch );
      send_to_char("\n\r--- Characters awaiting name approval ---\n\r", ch);

	for ( auth = first_auth_name; auth; auth = auth->next )
	{
	 	 if ( auth->state == AUTH_CHANGE_NAME )
			changename = TRUE;
		 else if ( auth->state == AUTH_AUTHED )
			authed = TRUE;

		 if ( auth->name != NULL &&  auth->state < AUTH_CHANGE_NAME)
		 	pending = TRUE;

	}
	if ( pending )
	{
		for ( auth = first_auth_name; auth; auth = auth->next )
		{
			if ( auth->state < AUTH_CHANGE_NAME )
			{
				switch( auth->state )
				{
					default:
						strcpy( buf, "Unknown?" );
						break;
					case AUTH_LINK_DEAD:
						strcpy( buf, "Link Dead" );
						break;
					case AUTH_ONLINE:
						strcpy( buf, "Online");
						break;
					case AUTH_OFFLINE:
						strcpy( buf, "Offline" );
						break;
				}

				ch_printf( ch, "\t%s\t\t%s\n\r", auth->name, buf );
			}
		}
	}
	else
		send_to_char( "\tNone\n\r", ch );

	if ( authed )
	{
		set_char_color( AT_DIVIDER, ch );
		send_to_char("\n\rAuthorized Characters:\n\r", ch );
		send_to_char("---------------------------------------------\n\r", ch );
		for ( auth = first_auth_name; auth; auth = auth->next )
		{
			if ( auth->state == AUTH_AUTHED )
				ch_printf( ch,"Name: %s\t Approved by: %s\n\r", auth->name, auth->authed_by );
		}
	}
	if ( changename )
	{
		set_char_color( AT_DIVIDER, ch );
		send_to_char("\n\rChange Name:\n\r", ch );
		send_to_char("---------------------------------------------\n\r", ch );
		for ( auth = first_auth_name; auth; auth = auth->next )
		{
			if ( auth->state == AUTH_CHANGE_NAME )
				ch_printf( ch,"Name: %s\t Change requested by: %s\n\r", auth->name, auth->change_by );
		}
	}

      send_to_char("\n\r--- Characters awaiting bio approval ---\n\r", ch);
	for ( d = first_descriptor; d; d = d->next )
	{
		if (d->character && xIS_SET(d->character->act, PLR_REQBIO))
		{
			count++;
			ch_printf(ch, " %s", d->character->name);
		}
	}
	if (count < 1)
		send_to_char( "\tNone\n\r", ch );

 	return;
    }

	if ( !str_cmp( arg2, "bio" ) || !str_cmp(arg2, "chat" ) )
	{
		if( ( victim = get_char_world( ch, arg1 ) ) == NULL )
		{
			send_to_char( "There is nobody like that waiting for authorization.\n\r", ch );
			return;
		}

		if( xIS_SET( victim->act, PLR_REQBIO ) )
			xREMOVE_BIT(victim->act, PLR_REQBIO);

		if (xIS_SET(victim->act, PLR_CAN_CHAT))
		{
			if ( victim->pcdata->authed_by )
				STRFREE( victim->pcdata->authed_by );

			xREMOVE_BIT(victim->act, PLR_CAN_CHAT);
			sprintf( buf, "%s: bio UN-authorized", victim->name);
			to_channel( buf, CHANNEL_AUTH, "Auth", LEVEL_NEOPHYTE );

			ch_printf( ch, "You have UN-authorized %s's bio.\n\r", victim->name);

			/* Below sends a message to player when name is accepted - Brittany */
			ch_printf_color( victim,
			                 "\n\r&GThe MUD Administrators have denied you access to the.\n\r"
			                 "CHAT channel.  You can not role play until you are reauthorized. ");
			return ;
		}
		if ( victim->pcdata->authed_by )
			STRFREE( victim->pcdata->authed_by );
		victim->pcdata->authed_by = QUICKLINK( ch->name );

		xSET_BIT(victim->act, PLR_CAN_CHAT);
		sprintf( buf, "%s: bio authorized", victim->name);
		to_channel( buf, CHANNEL_AUTH, "Auth", LEVEL_NEOPHYTE );

		ch_printf( ch, "You have authorized %s's bio.\n\r", victim->name);

		/* Below sends a message to player when name is accepted - Brittany */
		ch_printf_color( victim,
		                 "\n\r&GThe MUD Administrators have accepted your bio.\n\r"
		                 "You are authorized to use the CHAT channel to role play on. ");
		return ;
	}
	if ( !str_cmp( arg2, "badbio" ) || !str_cmp(arg2, "badchat" ) )
	{
		if ( !argument || argument[0] == '\0' )
		{
			send_to_char( "Please give a reason why their bio can't be authed.\n\rAuth <player> badbio <reason>\n\r", ch );
			return ;
		}

		if( ( victim = get_char_world( ch, arg1 ) ) == NULL )
		{
			send_to_char( "There is no one like that waiting for authorization.\n\r", ch );
			return;
		}

		if( xIS_SET( victim->act, PLR_REQBIO ) )
			xREMOVE_BIT(victim->act, PLR_REQBIO);
		else
		{
			send_to_char( "There is no one like that waiting for authorization.\n\r", ch );
			return;
		}
		sprintf( buf, "%s: bio denied for the following reason: %s", victim->name, argument);
		to_channel( buf, CHANNEL_AUTH, "Auth", LEVEL_NEOPHYTE );

		ch_printf( ch, "You have denied %s's bio.\n\r", victim->name);

		/* Below sends a message to player when name is accepted - Brittany */
		ch_printf_color( victim,
		                 "\n\r&GThe MUD Administrators have found a problem with your bio\n\r"
		                 "for the following reason: %s.\n\r", argument);
		return ;
	}

    if ( !str_cmp( arg1, "fixlist" ) )
    {
	send_to_pager( "Checking authorization list...\n\r", ch );
	clear_auth_list();
	send_to_pager( "Done.\n\r", ch );
	return;
    }

   if ( !str_cmp( arg1, "clean" ) )
   {
      send_to_pager( "Cleaning authorization list...\n\r", ch );
      clean_auth_list();
      send_to_pager( "Checking authorization list...\n\r", ch );
      clear_auth_list();
      send_to_pager( "Done.\n\r", ch );
      return;
   }

    auth = get_auth_name( arg1 );
    if ( auth != NULL )
    {
	if ( auth->state == AUTH_OFFLINE || auth->state == AUTH_LINK_DEAD )
	{
	    offline = TRUE;
	    if ( arg2[0]=='\0' || !str_cmp( arg2,"accept" ) || !str_cmp( arg2, "yes" ) )
	    {
		auth->state = AUTH_AUTHED;
		auth->authed_by = QUICKLINK( ch->name );
		save_auth_list();
		sprintf( buf, "%s: authorized", auth->name);
		to_channel( buf, CHANNEL_AUTH, "Auth", level );
		ch_printf( ch, "You have authorized %s.\n\r", auth->name );
		return;
	    }
	   else if( !str_cmp( arg2, "reject" ) )
	    {
		sprintf( buf, "%s: denied authorization", auth->name );
		to_channel( buf, CHANNEL_AUTH, "Auth", level );
		ch_printf( ch, "You have denied %s.\n\r", auth->name );
	      /* Addition so that denied names get added to reserved list - Samson 10-18-98 */
		sprintf( buf, "%s add", auth->name );
		do_reserve( ch, buf ); /* Samson 7-27-98 */
            do_destroy( ch, auth->name );
		return;
	    }
	    else if( !str_cmp( arg2, "change" ) )
  	    {
		auth->state = AUTH_CHANGE_NAME;
		auth->change_by = QUICKLINK( ch->name );
		save_auth_list();
		sprintf( buf, "%s: name denied", auth->name );
		to_channel( buf, CHANNEL_AUTH, "Auth", level );
		ch_printf( ch, "You requested %s change names.\n\r", auth->name );
	      /* Addition so that requested name changes get added to reserved list - Samson 10-18-98 */
		sprintf( buf, "%s add", auth->name );
		do_reserve( ch, buf );
		return;
	    }
	    else
	    {
		send_to_char( "Invalid argument.\n\r", ch );
		return;
	    }
	}
	else
	{
	    victim = get_waiting_desc( ch, arg1 );
	    if ( victim == NULL )
		return;

	    set_char_color( AT_IMMORT, victim );
	    if ( arg2[0]=='\0' || !str_cmp( arg2,"accept" ) || !str_cmp( arg2,"yes" ))
	    {
		if ( victim->pcdata->authed_by )
		    STRFREE( victim->pcdata->authed_by );
		victim->pcdata->authed_by = QUICKLINK( ch->name );
		sprintf( buf, "%s: authorized", victim->name );
		to_channel( buf, CHANNEL_AUTH, "Auth", level );

		ch_printf( ch, "You have authorized %s.\n\r", victim->name );

		ch_printf_color( victim,
		    "\n\r&GThe MUD Administrators have accepted the name %s.\n\r"
		    "You are now free to roam the %s.\n\r", victim->name, sysdata.mud_name );
		REMOVE_BIT(victim->pcdata->flags, PCFLAG_UNAUTHED);
		remove_from_auth( victim->name );
  		return;
	    }
	   else if( !str_cmp( arg2, "reject" ) )
	    {
		send_to_char( "&RYou have been denied access.\n\r", victim );
		sprintf( buf, "%s: denied authorization", auth->name );
		to_channel( buf, CHANNEL_AUTH, "Auth", level );
		ch_printf( ch, "You have denied %s.\n\r", victim->name );
		remove_from_auth( victim->name );
	      /* Addition to add denied names to reserved list - Samson 10-18-98 */
		sprintf( buf, "%s add", auth->name );
		do_reserve( ch, buf ); /* Samson 7-27-98 */
            do_destroy( ch, victim->name );
            return;
	    }
  	    else if( !str_cmp( arg2, "change" ) )
  	    {
		auth->state = AUTH_CHANGE_NAME;
		auth->change_by = QUICKLINK( ch->name );
		save_auth_list();
		sprintf( buf, "%s: name denied", victim->name );
		to_channel( buf, CHANNEL_AUTH, "Auth", level );
  		ch_printf_color( victim,
  		    "&R\n\rThe MUD Administrators have found the name %s to be unacceptable.\n\r"
  	            "You may choose a new name when you reach the end of this area.\n\r"
  	            "The name you choose must be medieval and original.\n\r"
  	            "No titles, descriptive words, or names close to any existing\n\r"
  	            "Immortal's name. See 'help name'.\n\r", victim->name);
		ch_printf( ch, "You requested %s change names.\n\r", victim->name);
	      /* Addition to put denied name on reserved list - Samson 10-18-98 */
	      sprintf( buf, "%s add", victim->name );
		do_reserve( ch, buf );
		return;
  	    }
  	    else
  	    {
		send_to_char( "Invalid argument.\n\r", ch );
		return;
	    }
	}
    }
    else
    {
	send_to_char( "No such player pending authorization.\n\r", ch );
	return;
    }
}

/* new auth */
void do_name( CHAR_DATA *ch, char *argument )
{
  char fname[256];
  struct stat fst;
  CHAR_DATA *tmp;
  AUTH_LIST *auth_name;

  auth_name = NULL;
  auth_name = get_auth_name( ch->name );
  if ( auth_name == NULL )
  {
  	send_to_char( "Huh?\n\r", ch );
	return;
  }

  argument[0] = UPPER(argument[0]);

  if (!check_parse_name(argument, TRUE))
  {
    send_to_char("Illegal name, try another.\n\r", ch);
    return;
  }

  if (!str_cmp(ch->name, argument))
  {
    send_to_char("That's already your name!\n\r", ch);
    return;
  }

  for ( tmp = first_char; tmp; tmp = tmp->next )
  {
    if (!str_cmp(argument, tmp->name))
    break;
  }

  if ( tmp )
  {
    send_to_char("That name is already taken.  Please choose another.\n\r", ch);
    return;
  }

  sprintf( fname, "%s%c/%s", PLAYER_DIR, tolower(argument[0]), capitalize( argument ) );
  if ( stat( fname, &fst ) != -1 )
  {
    send_to_char("That name is already taken.  Please choose another.\n\r", ch);
    return;
  }
  sprintf( fname, "%s%c/%s", PLAYER_DIR, tolower(ch->name[0]), capitalize(ch->name) );
  unlink(fname); /* cronel, for auth */

  STRFREE( ch->name );
  ch->name = STRALLOC( argument );
  STRFREE( ch->pcdata->filename );
  ch->pcdata->filename = STRALLOC( argument );
  send_to_char("Your name has been changed and is being submitted for approval.\n\r", ch );
  auth_name->name = STRALLOC( argument );
  auth_name->state = AUTH_ONLINE;
  if ( auth_name->change_by )
	STRFREE( auth_name->change_by );
  save_auth_list();
  return;
}

/* changed for new auth */
void do_mpapplyb( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *victim;
    CMDTYPE *command;
    int level = LEVEL_IMMORTAL;

    /* Checks to see level of authorize command.
       Makes no sense to see the auth channel if you can't auth. - Samson 12-28-98 */
    command = find_command( "authorize" );

    if ( !command )
       level = LEVEL_IMMORTAL;
    else
       level = command->level;


	if ( !IS_NPC( ch ) || ch->desc || IS_AFFECTED( ch, AFF_CHARM ))
	{
		send_to_char( "Huh?\n\r", ch );
		return;
	}

	if (argument[0] == '\0')
	{
		progbug("Mpapplyb - bad syntax", ch );
		return;
	}

  	if ( (victim = get_char_room( ch, argument ) ) == NULL )
  	{
  	  progbug("Mpapplyb - no such player in room.", ch );
  	  return;
  	}

	if ( !victim->desc )
	{
		send_to_char( "Not on linkdeads.\n\r", ch );
		return;
	}

	if ( victim->fighting )
	    stop_fighting( victim, TRUE );
	char_from_room(victim);
	char_to_room(victim, get_room_index(ROOM_VNUM_SCHOOL));
	act( AT_WHITE, "$n enters this world from within a column of blinding light!", victim, NULL, NULL, TO_ROOM );
        do_look(victim, "auto");
	if ( NOT_AUTHED( victim ) )
	{
	   sprintf( log_buf, "%s [%s@%s] New player entering the game.\n\r", victim->name, victim->desc->user,
	  	victim->desc->host );

	   to_channel( log_buf, CHANNEL_AUTH, "Auth", level );
	   ch_printf( victim, "\n\rYou are now entering the game...\n\r"
			"However, your character has not been authorized yet and can not\n\r"
			"advance past level 5 until then. Your character will be saved,\n\r"
			"but not allowed to fully indulge in the %s.\n\r", sysdata.mud_name );
	}

	return;
}

/* changed for new auth */
void auth_update( void )
{
    AUTH_LIST *auth;
    char buf [MAX_INPUT_LENGTH];
    CMDTYPE *command;
    DESCRIPTOR_DATA *d;
    int level = LEVEL_IMMORTAL;
    bool found_imm = FALSE;	      /* Is at least 1 immortal on? */
    bool found_hit = FALSE;         /* was at least one found? */

    command = find_command( "authorize" );
    if ( !command )
 	level = LEVEL_IMMORTAL;
    else
	level = command->level;

	strcpy( log_buf, "--- Characters awaiting approval ---\n\r" );
	for ( auth = first_auth_name; auth; auth = auth->next )
	{
		if ( auth != NULL && auth->state < AUTH_CHANGE_NAME )
		{
			found_hit = TRUE;
			sprintf( buf, "Name: %s      Status: %s\n\r", auth->name, ( auth->state == AUTH_ONLINE ) ? "Online" : "Offline" );
			strcat( log_buf, buf );
		}
	}
	if ( found_hit )
      {
        for ( d = first_descriptor; d; d=d->next )
          if ( d->connected == CON_PLAYING && d->character && IS_IMMORTAL(d->character) && d->character->level >= level )
            found_imm = TRUE;

        if ( found_imm )
	      to_channel( log_buf, CHANNEL_AUTH, "Auth", level );
	}
}

RESERVE_DATA *first_reserved;
RESERVE_DATA *last_reserved;

void save_reserved	args( ( void ) );
void sort_reserved	args( ( RESERVE_DATA *pRes ) );

/* Modified to require an "add" or "remove" argument in addition to name - Samson 10-18-98 */
void do_reserve(CHAR_DATA *ch, char *argument)
{
  char arg[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  RESERVE_DATA *res;

  set_char_color( AT_PLAIN, ch );

  argument = one_argument(argument, arg);
  argument = one_argument(argument, arg2);

  if (!*arg)
  {
    int wid = 0;

    send_to_char("To add a name: reserve <name> add\n\rTo remove a name: reserve <name> remove\n\r", ch );
    send_to_char("\n\r-- Reserved Names --\n\r", ch);
    for (res = first_reserved; res; res = res->next)
    {
      ch_printf(ch, "%c%-17s ", (*res->name == '*' ? '*' : ' '), (*res->name == '*' ? res->name+1 : res->name));
      if (++wid % 4 == 0)
        send_to_char("\n\r", ch);
    }
    if (wid % 4 != 0)
      send_to_char("\n\r", ch);
    return;
  }

  if ( !str_cmp( arg2, "remove" ) )
  {
     for (res = first_reserved; res; res = res->next)
       if (!str_cmp(arg, res->name))
       {
         UNLINK(res, first_reserved, last_reserved, next, prev);
         DISPOSE(res->name);
         DISPOSE(res);
         save_reserved();
         send_to_char("Name no longer reserved.\n\r", ch);
         return;
       }
     ch_printf( ch, "The name %s isn't on the reserved list.\n\r", arg );
     return;

  }

  if ( !str_cmp( arg2, "add" ) )
  {
     	for ( res = first_reserved; res; res = res->next )
	  if ( !str_cmp(arg, res->name) )
	  {
	     ch_printf( ch, "The name %s has already been reserved.\n\r", arg );
	     return;
	  }

      CREATE(res, RESERVE_DATA, 1);
      res->name = str_dup(arg);
      sort_reserved(res);
      save_reserved();
      send_to_char("Name reserved.\n\r", ch);
      return;
  }
  send_to_char( "Invalid argument.\n\r", ch );
}
