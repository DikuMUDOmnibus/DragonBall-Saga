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
 *			         Hotboot support code                             *
 ****************************************************************************/

#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include "mud.h"

// bool write_to_descriptor( DESCRIPTOR_DATA *d, char *txt, int length );
bool write_to_descriptor( int desc, char *txt, int length );
bool write_to_descriptor_old( int desc, char *txt, int length );

#ifdef MCCP
#define TELOPT_COMPRESS 85
#define TELOPT_COMPRESS2 86
bool    compressStart   args( ( DESCRIPTOR_DATA *d, unsigned char telopt ) );
bool    compressEnd     args( ( DESCRIPTOR_DATA *d ) );
const   char    compress2_on_str_2 [] = { '\xFF', '\xFB', TELOPT_COMPRESS2, '\0' };
#endif

extern int		    control;		/* Controlling descriptor	*/

/* Used by hotboot code */
void save_areas( void )
{
   AREA_DATA   *tarea;
   char         filename[256];

   for( tarea = first_build; tarea; tarea = tarea->next )
   {
      if( !IS_SET( tarea->status, AREA_LOADED ) )
         continue;
      sprintf( filename, "%s%s", BUILD_DIR, tarea->filename );
      fold_area( tarea, filename, FALSE );
   }
   return;
}
/*
 * Check to see if people are busy doing stuff that they wouldn't
 * want a hotboot to off during it  -Goku
 */
bool isSafeToHotboot(CHAR_DATA *ch)
{
	ROOM_INDEX_DATA *tRoom;
	CHAR_DATA *vch;
	bool found = FALSE;
	bool extrafound = FALSE;
	char buf [MAX_STRING_LENGTH];

	/* Check for people in the HBTC */
	tRoom = get_room_index(ROOM_HBTC);
	/*
	if (tRoom->first_person)
	{
		send_to_char( "The HBTC is in use, hotboot aborted.\n\r", ch );
		return FALSE;
	}
	*/

	/* Fixed so that hotboot will still occur if the only person(s)
	 * in the HBTC are mobs or immortals. - Karma
	 */
	for( vch = tRoom->first_person; vch; vch = vch->next_in_room )
	{
	  if( IS_NPC(vch) )
	    continue;
	  if( !IS_NPC(vch) && !IS_IMMORTAL(vch) )
	  {
	    if( !found )
	    {
		sprintf(buf,"%s",vch->name);
		found = TRUE;
	    }
	    else
	    {
		strcat(buf," and ");
		strcat(buf,vch->name);
		extrafound = TRUE;
	    }
	    
	    //send_to_char( "The HBTC is in use, hotboot aborted.\n\r", ch );
	  }
	}
	if( found )
	{
	  if( buf[0] == '\0' )
	    bug("Hotboot hbtc check: NULL buf",0);
	  if( extrafound )
	    ch_printf(ch,"%s are in the HBTC. Hotboot aborted.\n\r",buf);
	  else
	    ch_printf(ch,"%s is in the HBTC. Hotboot aborted.\n\r",buf);
	  return FALSE;
	}

	/* we could check for other stuff as well  -Goku */

	return TRUE;
}

/*  Warm reboot stuff, gotta make sure to thank Erwin for this :) */
void do_hotboot( CHAR_DATA *ch, char *argument )
{
    FILE *fp;
    DESCRIPTOR_DATA *d, *de_next;
    char buf [100], buf2[100], buf3[100];
	char arg [MAX_STRING_LENGTH];
    
    one_argument(argument, arg);
    
    /* typing "hotboot force" will force a hotboot */
    if (str_cmp(arg, "force"))
    	if (!isSafeToHotboot(ch))
    		return;

    sprintf( log_buf, "Hotboot initiated by %s.", ch->name );
    log_string( log_buf );

    fp = fopen( HOTBOOT_FILE, "w" );

    if( !fp )
    {
      send_to_char( "Hotboot file not writeable, aborted.\n\r", ch );
	sprintf( log_buf, "Could not write to hotboot file: %s. Hotboot aborted.", HOTBOOT_FILE );
	log_string( log_buf );
      perror( "do_copyover:fopen" );
      return;
    }

    /* Consider changing all loaded prototype areas here, if you use OLC */
    log_string( "Saving modified area files..." );
    save_areas();

    log_string( "Saving player files and connection states...." );
    if( ch && ch->desc )
        write_to_descriptor( ch->desc->descriptor, "\e[0m", 0 );
    sprintf( buf, "\n\rThe flow of time is halted momentarily as the world is reshaped!\n\r" );

    if ( auction->item )
    {
      sprintf(buf, "Sale of %s has been stopped by mud.\n\r",
          auction->item->short_descr);
      talk_auction(buf);
      obj_to_char(auction->item, auction->seller);
      auction->item = NULL;
      if ( auction->buyer && auction->buyer != auction->seller )
      {
        auction->buyer->gold += auction->bet;
        send_to_char("Your money has been returned.\n\r", auction->buyer);
      }
    }

    /* For each playing descriptor, save its state */
    for( d = first_descriptor; d ; d = de_next )
    {
        CHAR_DATA * och = CH(d);
        de_next = d->next; /* We delete from the list , so need to save this */
        if( !d->character || d->connected < CON_PLAYING ) /* drop those logging on */
        {
            write_to_descriptor( d->descriptor, "\n\rSorry, we are rebooting. Come back in a few minutes.\n\r", 0 );
            close_socket( d, FALSE ); /* throw'em out */
        }
        else
        {
            fprintf( fp, "%d %d %d %d %d %s %s %s\r",
                     d->descriptor,
#ifdef MCCP
                     (int)d->compressing,
#else
                     0,
#endif
                     och->in_room->vnum, d->port, d->idle, och->name, d->user, d->host );
            save_char_obj( och );
            write_to_descriptor( d->descriptor, buf, 0 );
#ifdef MCCP
            compressEnd( d );
#endif
        }
    }
    fprintf( fp, "-1 0 0 0 0 x x x\r" );
    FCLOSE( fp );

    log_string( "Executing hotboot...." );
    /* Close reserve and other always-open files and release other resources */
    FCLOSE( fpReserve );
    FCLOSE( fpLOG );

#ifdef IMC
   imc_hotboot();
#endif

    /* exec - descriptors are inherited */
    sprintf( buf,  "%d", port );
    sprintf( buf2, "%d", control );

#ifdef IMC
   if( this_imcmud )
      snprintf( buf3, 100, "%d", this_imcmud->desc );
   else
      strncpy( buf3, "-1", 100 );
#else
   strncpy( buf3, "-1", 100 );
#endif

    execl( EXE_FILE, "dbsaga", buf, "hotboot", buf2, buf3, (char *)NULL );
    /* Failed - sucessful exec will not return */
    perror( "do_hotboot: execl" );

    /* Here you might want to reopen fpReserve */
    /* Since I'm a neophyte type guy, I'll assume this is a good idea and cut and past from main()  */

    if( ( fpReserve = fopen( NULL_FILE, "r" ) ) == NULL )
    {
        perror( NULL_FILE );
        exit( 1 );
    }
    if( ( fpLOG = fopen( NULL_FILE, "r" ) ) == NULL )
    {
        perror( NULL_FILE );
        exit( 1 );
    }
    log_string( "Hotboot execution failed!!" );
    send_to_char( "Hotboot FAILED!\n\r", ch );
}

/* Recover from a hotboot - load players */
void hotboot_recover()
{
    DESCRIPTOR_DATA *d = NULL;
    FILE *fp;
    char name[100];
    char host[MAX_STRING_LENGTH];
    char user[MAX_STRING_LENGTH];
    int desc, dcompress, room, dport, idle;
//    char *lock = NULL;
    bool fOld;

    fp = fopen( HOTBOOT_FILE, "r" );

    if (!fp) /* there are some descriptors open which will hang forever then ? */
    {
        perror( "hotboot_recover: fopen" );
        log_string( "Hotboot file not found. Exitting.\n\r" );
        exit( 1 );
    }

    unlink( HOTBOOT_FILE ); /* In case something crashes - doesn't prevent reading */
    for( ; ; )
    {
        d = NULL;

	// 4 %s is one too many.  03.29.05 --Saiyr
        fscanf( fp, "%d %d %d %d %d %s %s %s\r", // %s\r",
	    &desc,
	    &dcompress,
	    &room,
	    &dport,
	    &idle,
	    name,
	    user,
	    host );
//	    lock );

	if (desc == -1 || feof(fp) )
	{
		break;
	}
            
        /* Write something, and check if it goes error-free */
//        if( !dcompress && !write_to_descriptor( desc, "\n\rThe ether swirls in chaos.\n\r", 0 ) )
//        if( !dcompress)
//        {
//bug("desc:%d dcompress:%d - %d %d %d %s %s %s", desc,dcompress, room, dport, idle, name, user, host );
//        	if (!write_to_descriptor( desc, "\n\rThe ether swirls in chaos.\n\r", 0 ) )
//        {
//            close( desc ); /* nope */
//            continue;
//        }
//    	}
#ifdef MCCP
        if( !dcompress && !write_to_descriptor_old( desc, "\n\rThe ether swirls in chaos.\n\r", 0 ) )
#else
        if( !write_to_descriptor( desc, "\n\rThe ether swirls in chaos.\n\r", 0 ) )
#endif
        {
            close( desc ); /* nope */
            continue;
        }
        CREATE( d, DESCRIPTOR_DATA, 1 );

        d->next		= NULL;
        d->descriptor	= desc;
        d->connected	= CON_GET_NAME;
        d->outsize	= 2000;
        d->idle		= 0;
        d->lines		= 0;
        d->scrlen		= 24;
        d->newstate	= 0;
        d->prevcolor	= 0x07;
        d->ansi = TRUE;
		d->ifd		= -1;
		d->ipid		= -1;

        CREATE( d->outbuf, char, d->outsize );

        d->user = STRALLOC( user );
        d->host = STRALLOC( host );
        d->port = dport;
        d->idle = idle;
#ifdef MCCP
		/* Start MCCP back up for who ever had it enabled */
		if (dcompress)
			write_to_buffer( d, compress2_on_str_2, 0 );
//			compressStart(d, TELOPT_COMPRESS2);

#endif
        LINK( d, first_descriptor, last_descriptor, next, prev );
        d->connected = CON_COPYOVER_RECOVER; /* negative so close_socket will cut them off */

        /* Now, find the pfile */
        fOld = load_char_obj( d, name, FALSE );

       if( !fOld ) /* Player file not found?! */
        {
            write_to_descriptor( d->descriptor, "\n\rSomehow, your character was lost during hotboot. Contact the immortals ASAP.\n\r", 0 );
            close_socket( d, FALSE );
        }
        else /* ok! */
        {
           write_to_descriptor( d->descriptor, "\n\rTime resumes its normal flow.\n\r", 0 );
            d->character->in_room = get_room_index( room );
            if( !d->character->in_room )
                d->character->in_room = get_room_index( ROOM_VNUM_TEMPLE );

            /* Insert in the char_list */
            add_char( d->character );

            char_to_room( d->character, d->character->in_room );
	      act( AT_MAGIC, "A puff of ethereal smoke disipates around you!", d->character, NULL, NULL, TO_CHAR );
            act( AT_MAGIC, "$n appears in a puff of ethereal smoke!", d->character, NULL, NULL, TO_ROOM );
            d->connected = CON_PLAYING;
            if ( ++num_descriptors > sysdata.maxplayers )
	         sysdata.maxplayers = num_descriptors;
#ifdef I3
	      I3_char_login( d->character );
#endif
        }
    }
    FCLOSE( fp );
    log_string( "Hotboot recovery complete." );
    return;
}
