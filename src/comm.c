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
 *			 Low-level communication module                                 *
 ****************************************************************************/

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <stdarg.h>
#include <crypt.h>
#include "mud.h"
#include "sha256.h"

/*
 * Socket and TCP/IP stuff.
 */
#ifdef WIN32
  #include <io.h>
  #undef EINTR
  #undef EMFILE
  #define EINTR WSAEINTR
  #define EMFILE WSAEMFILE
  #define EWOULDBLOCK WSAEWOULDBLOCK
  #define MAXHOSTNAMELEN 32

  #define  TELOPT_ECHO        '\x01'
  #define  GA                 '\xF9'
  #define  SB                 '\xFA'
  #define  WILL               '\xFB'
  #define  WONT               '\xFC'
  #define  DO                 '\xFD'
  #define  DONT               '\xFE'
  #define  IAC                '\xFF'
  void bailout(void);
  void shutdown_checkpoint (void);
#else
  #include <sys/socket.h>
  #include <netinet/in.h>
//  #include <netinet/in_systm.h>
  #include <netinet/ip.h>
  #include <arpa/inet.h>
  #include <arpa/telnet.h>
  #include <netdb.h>
  #define closesocket close
#endif

#ifdef sun
int gethostname ( char *name, int namelen );
#endif

const	char	echo_off_str	[] = { IAC, WILL, TELOPT_ECHO, '\0' };
const	char	echo_on_str	[] = { IAC, WONT, TELOPT_ECHO, '\0' };
const	char 	go_ahead_str	[] = { IAC, GA, '\0' };

#ifdef MCCP
#define TELOPT_COMPRESS 85
#define TELOPT_COMPRESS2 86
const   char    eor_on_str      [] = { IAC, WILL, TELOPT_EOR, '\0' };
const   char    compress_on_str [] = { IAC, WILL, TELOPT_COMPRESS2, '\0' };
const   char    compress2_on_str [] = { IAC, WILL, TELOPT_COMPRESS2, '\0' };

bool    compressStart   args( ( DESCRIPTOR_DATA *d, unsigned char telopt ) );
bool    compressEnd     args( ( DESCRIPTOR_DATA *d ) );
#endif

void	auth_maxdesc	args( ( int *md, fd_set *ins, fd_set *outs,
				fd_set *excs ) );
void	auth_check	args( ( fd_set *ins, fd_set *outs, fd_set *excs ) );
void	set_auth	args( ( DESCRIPTOR_DATA *d ) );
void	kill_auth	args( ( DESCRIPTOR_DATA *d ) );


//void    save_sysdata args( ( SYSTEM_DATA sys ) );

/*
 * External Functions
 */
void	shutdown_mud	args( ( char *reason ) );
void save_auth_list	args( ( void ) );


/*
 * Global variables.
 */
IMMORTAL_HOST * immortal_host_start;    /* Start of Immortal legal domains */
IMMORTAL_HOST * immortal_host_end;    /* End of Immortal legal domains */
DESCRIPTOR_DATA *   first_descriptor;	/* First descriptor		*/
DESCRIPTOR_DATA *   last_descriptor;	/* Last descriptor		*/
DESCRIPTOR_DATA *   d_next;		/* Next descriptor in loop	*/
int		    num_descriptors;
FILE *		    fpReserve;		/* Reserved file handle		*/
bool		    mud_down;		/* Shutdown			*/
bool		    service_shut_down;  /* Shutdown by operator closing down service */
bool		    wizlock;
int		    locklev;
time_t              boot_time;
HOUR_MIN_SEC  	    set_boot_time_struct;
HOUR_MIN_SEC *      set_boot_time;
struct tm *         new_boot_time;
struct tm           new_boot_struct;
char		    str_boot_time[MAX_INPUT_LENGTH];
char		    lastplayercmd[MAX_INPUT_LENGTH*2];
time_t		    current_time;	/* Time of this pulse		*/
int		    control;		/* Controlling descriptor	*/
int		    newdesc;		/* New descriptor		*/
fd_set		    in_set;		/* Set of desc's for reading	*/
fd_set		    out_set;		/* Set of desc's for writing	*/
fd_set		    exc_set;		/* Set of desc's with errors	*/
int 		    maxdesc;
char *		    alarm_section = "(unknown)";


/*
 * OS-dependent local functions.
 */
void	game_loop		args( ( ) );
int	init_socket		args( ( int port ) );
void	new_descriptor		args( ( int new_desc ) );
bool	read_from_descriptor	args( ( DESCRIPTOR_DATA *d ) );
bool	write_to_descriptor	args( ( int desc, char *txt, int length ) );


/*
 * Other local functions (OS-independent).
 */
bool	check_parse_name	args( ( char *name, bool newchar ) );
bool	check_reconnect		args( ( DESCRIPTOR_DATA *d, char *name,
				    bool fConn ) );
bool	check_playing		args( ( DESCRIPTOR_DATA *d, char *name, bool kick ) );
int	main			args( ( int argc, char **argv ) );
void	nanny			args( ( DESCRIPTOR_DATA *d, char *argument ) );
bool	flush_buffer		args( ( DESCRIPTOR_DATA *d, bool fPrompt ) );
void	read_from_buffer	args( ( DESCRIPTOR_DATA *d ) );
void	stop_idling		args( ( CHAR_DATA *ch ) );
void	free_desc		args( ( DESCRIPTOR_DATA *d ) );
void	display_prompt		args( ( DESCRIPTOR_DATA *d ) );
int	make_color_sequence	args( ( const char *col, char *buf,
					DESCRIPTOR_DATA *d ) );
void	set_pager_input		args( ( DESCRIPTOR_DATA *d,
					char *argument ) );
bool	pager_output		args( ( DESCRIPTOR_DATA *d ) );

void	mail_count		args( ( CHAR_DATA *ch ) );

void    tax_player		args( ( CHAR_DATA *ch ) );
void    mccp_interest		args( ( CHAR_DATA *ch ) );

bool	check_total_ip	args( ( DESCRIPTOR_DATA *dnew) );


int port;

char capitalizeString(char *text)
{
	int i;

	if (text[0] == '\0')
		return *text;

	text[0] = UPPER(text[0]);

	for(i=1;text[i] != '\0';i++)
	{
		text[i] = LOWER(text[i]);
	}

	return *text;
}

/*
====================
send color to a desc	-Nopey
====================
*/
void send_to_desc_color( const char *txt, DESCRIPTOR_DATA *d )
{
	char *colstr;
	const char *prevstr = txt;
	char colbuf[20];
	int ln;

	if ( !d )
	{
	  bug( "send_to_desc_color: NULL *d" );
	  return;
	}

	if ( !txt || !d->descriptor )
	  return;

	while ( (colstr = strpbrk(prevstr, "&^}")) != NULL )
	{
	  if (colstr > prevstr)
	    write_to_buffer(d, prevstr, (colstr-prevstr));
	  ln = make_color_sequence(colstr, colbuf, d);
	  if ( ln < 0 )
	  {
	    prevstr = colstr+1;
	    break;
	  }
	  else if ( ln > 0 )
	    write_to_buffer(d, colbuf, ln);
	  prevstr = colstr+2;
	}
	if ( *prevstr )
	  write_to_buffer(d, prevstr, 0);
	return;
}

#ifdef WIN32
  int mainthread( int argc, char **argv )
#else
  int main( int argc, char **argv )
#endif
{
    struct timeval now_time;
    char hostn[128];

   bool fCopyOver = FALSE;
#ifdef IMC
   int imcsocket = -1;
#endif

    DONT_UPPER			= FALSE;
    num_descriptors		= 0;
    first_descriptor		= NULL;
    last_descriptor		= NULL;
    sysdata.NO_NAME_RESOLVING	= TRUE;
    sysdata.WAIT_FOR_AUTH	= TRUE;

    /*
     * Init time.
     */
    gettimeofday( &now_time, NULL );
    current_time = (time_t) now_time.tv_sec;
/*  gettimeofday( &boot_time, NULL);   okay, so it's kludgy, sue me :) */
    boot_time = time(0);         /*  <-- I think this is what you wanted */
    strcpy( str_boot_time, ctime( &current_time ) );

    /*
     * Init boot time.
     */
    set_boot_time = &set_boot_time_struct;
    set_boot_time->manual = 0;

    new_boot_time = update_time(localtime(&current_time));
    /* Copies *new_boot_time to new_boot_struct, and then points
       new_boot_time to new_boot_struct again. -- Alty */
    new_boot_struct = *new_boot_time;
    new_boot_time = &new_boot_struct;
    if (new_boot_time->tm_hour > 3)
    	new_boot_time->tm_mday += 1;
    new_boot_time->tm_sec = 0;
    new_boot_time->tm_min = 0;
    new_boot_time->tm_hour = 4;

    /* Update new_boot_time (due to day increment) */
    new_boot_time = update_time(new_boot_time);
    new_boot_struct = *new_boot_time;
    new_boot_time = &new_boot_struct;
    /* Bug fix submitted by Gabe Yoder */
    new_boot_time_t = mktime(new_boot_time);
    reboot_check(mktime(new_boot_time));
    /* Set reboot time string for do_time */
    get_reboot_string();
    init_pfile_scan_time(); /* Pfile autocleanup initializer - Samson 5-8-99 */

    /*
     * Reserve two channels for our use.
     */
    if ( ( fpReserve = fopen( NULL_FILE, "r" ) ) == NULL )
    {
	perror( NULL_FILE );
	exit( 1 );
    }
    if ( ( fpLOG = fopen( NULL_FILE, "r" ) ) == NULL )
    {
	perror( NULL_FILE );
	exit( 1 );
    }

   /*
    * Get the port number.
    */
   port = 4000;
   if ( argc > 1 )
   {
	if ( !is_number( argv[1] ) )
	{
	    fprintf( stderr, "Usage: %s [port #]\n", argv[0] );
	    exit( 1 );
	}
	else if ( ( port = atoi( argv[1] ) ) <= 1024 )
	{
	    fprintf( stderr, "Port number must be above 1024.\n" );
	    exit( 1 );
	}

      if (argv[2] && argv[2][0])
      {
         fCopyOver = TRUE;
         control = atoi( argv[3] );
#ifdef IMC
	   imcsocket = atoi( argv[4] );
#endif
      }
      else
         fCopyOver = FALSE;
   }


    /*
     * Run the game.
     */
#ifdef WIN32
    {
	/* Initialise Windows sockets library */

	unsigned short wVersionRequested = MAKEWORD(1, 1);
	WSADATA wsadata;
	int err;

	/* Need to include library: wsock32.lib for Windows Sockets */
	err = WSAStartup(wVersionRequested, &wsadata);
	if (err)
	{
	    fprintf(stderr, "Error %i on WSAStartup\n", err);
	    exit(1);
	}

	/* standard termination signals */
	signal(SIGINT, (void *) bailout);
	signal(SIGTERM, (void *) bailout);
  }
#endif /* WIN32 */

/*	Disabled -Goku 10.14.03
    log_string("Booting Chat Robot");
    startchat(CHAT_FILE);
*/
#ifdef IMC
   /* Initialize and connect to IMC2 */
   imc_startup( FALSE, imcsocket, fCopyOver );
#endif

    log_string("Booting Database");
    boot_db( fCopyOver );
    log_string("Initializing socket");
    if (!fCopyOver) /* We have already the port if copyover'ed */
    {
	control  = init_socket( port   );
    }

    /* I don't know how well this will work on an unnamed machine as I don't
       have one handy, and the man pages are ever-so-helpful.. -- Alty */
    if (gethostname(hostn, sizeof(hostn)) < 0)
    {
      perror("main: gethostname");
      strcpy(hostn, "unresolved");
    }
    sprintf( log_buf, "%s ready at address %s on port %d.",
		sysdata.mud_name, hostn, port );
/*
    sprintf( log_buf, "Realms of Despair ready at address %s on port %d.",
	hostn, port );
*/
    log_string( log_buf );

    game_loop( );

    closesocket( control  );

#ifdef IMC
   imc_shutdown( FALSE );
#endif

#ifdef WIN32
    if (service_shut_down)
    {
	CHAR_DATA *vch;

	/* Save all characters before booting. */
	for ( vch = first_char; vch; vch = vch->next )
	    if ( !IS_NPC( vch ) )
	    {
		shutdown_checkpoint ();
		save_char_obj( vch );
	    }
	}

	/* Save MUD time */
	save_timedata();

    /* Shut down Windows sockets */

    WSACleanup();                 /* clean up */
    kill_timer();                 /* stop timer thread */
#endif


    /*
     * That's all, folks.
     */
    log_string( "Normal termination of game." );
    exit( 0 );
    return 0;
}


int init_socket( int port )
{
    char hostname[64];
    struct sockaddr_in	 sa;
    int x = 1;
    int fd;

    gethostname(hostname, sizeof(hostname));


    if ( ( fd = socket( AF_INET, SOCK_STREAM, 0 ) ) < 0 )
    {
	perror( "Init_socket: socket" );
	exit( 1 );
    }

    if ( setsockopt( fd, SOL_SOCKET, SO_REUSEADDR,
		    (void *) &x, sizeof(x) ) < 0 )
    {
	perror( "Init_socket: SO_REUSEADDR" );
	closesocket( fd );
	exit( 1 );
    }

#if defined(SO_DONTLINGER) && !defined(SYSV)
    {
	struct	linger	ld;

	ld.l_onoff  = 1;
	ld.l_linger = 1000;

	if ( setsockopt( fd, SOL_SOCKET, SO_DONTLINGER,
			(void *) &ld, sizeof(ld) ) < 0 )
	{
	    perror( "Init_socket: SO_DONTLINGER" );
	    closesocket( fd );
	    exit( 1 );
	}
    }
#endif

    memset(&sa, '\0', sizeof(sa));
    sa.sin_family   = AF_INET;
    sa.sin_port	    = htons( port );

    if ( bind( fd, (struct sockaddr *) &sa, sizeof(sa) ) == -1 )
    {
	perror( "Init_socket: bind" );
	closesocket( fd );
	exit( 1 );
    }

    if ( listen( fd, 50 ) < 0 )
    {
	perror( "Init_socket: listen" );
	closesocket( fd );
	exit( 1 );
    }

    return fd;
}

static void SegVio()
{
  CHAR_DATA *ch;

  for( ch = first_char; ch; ch = ch->next )
  {
    if( !IS_NPC( ch ) )
	save_char_obj( ch );
  }
  if( fork() == 0 )
    abort();
  exit(1);
/*  char buf[MAX_STRING_LENGTH];

  log_string( "SEGMENTATION VIOLATION" );
  log_string( lastplayercmd );
  for ( ch = first_char; ch; ch = ch->next )
  {
    sprintf( buf, "%cPC: %-20s room: %d", IS_NPC(ch) ? 'N' : ' ',
    		ch->name, ch->in_room->vnum );
    log_string( buf );
  }
  exit(0);
*/}

static void SigTerm( int signum )
{
	CHAR_DATA *vch;
	char buf[MAX_STRING_LENGTH];

	sprintf( log_buf, "&RATTENTION!! Message from game server: &YEmergency shutdown called.\a" );
	echo_to_all( AT_RED, log_buf, ECHOTAR_ALL );
	sprintf( log_buf, "Executing emergency shutdown proceedure." );
	echo_to_all( AT_YELLOW, log_buf, ECHOTAR_ALL );
	log_string( "Message from server: Executing emergency shutdown proceedure." );
	shutdown_mud( log_buf );	strcat( buf, "\n\r" );

	if ( auction->item )
		do_auction( supermob, "stop" );

	log_string( "Saving players...." );
	for ( vch = first_char; vch; vch = vch->next )
	{
	 	if ( !IS_NPC( vch ) )
		{
			save_char_obj( vch );
			sprintf( log_buf, "%s saved.", vch->name );
			log_string( log_buf );
			if( vch->desc )
			{
				write_to_descriptor( vch->desc->descriptor, buf, 0 );
				write_to_descriptor( vch->desc->descriptor, "You have been saved to disk.\n\r", 0 );
			}
		}
	}

	/*  Save morphs so can sort later. --Shaddai */
	if ( sysdata.morph_opt )
	save_morphs( );

	fflush( stderr );	/* make sure strerr is flushed */

	close( control );

	/* Extra ports disabled - Samson 12-1-98 : uncomment
	   this section if you still use them
	close( control2 );
	close( conclient);
	close( conjava  ); */

	log_string( "Emergency shutdown complete." );

	/* Using exit here instead of mud_down because the
	   thing sometimes failed to kill when asked!! */
	exit( 0 );
}


/*
 * LAG alarm!							-Thoric
 */
void caught_alarm()
{
    char buf[MAX_STRING_LENGTH];

    sprintf(buf, "ALARM CLOCK! In section %s", alarm_section );
    bug( buf );
    strcpy( buf, "Alas, the hideous malevalent entity known only as 'Lag' rises once more!\n\r" );
    echo_to_all( AT_IMMORT, buf, ECHOTAR_ALL );
    if ( newdesc )
    {
	FD_CLR( newdesc, &in_set );
	FD_CLR( newdesc, &out_set );
	FD_CLR( newdesc, &exc_set );
	log_string( "clearing newdesc" );
    }
}

bool check_bad_desc( int desc )
{
    if ( FD_ISSET( desc, &exc_set ) )
    {
	FD_CLR( desc, &in_set );
	FD_CLR( desc, &out_set );
	log_string( "Bad FD caught and disposed." );
	return TRUE;
    }
    return FALSE;
}

/*
 * Determine whether this player is to be watched  --Gorog
 */
bool chk_watch(sh_int player_level, char *player_name, char *player_site)
{
    WATCH_DATA *pw;
/*
    char buf[MAX_INPUT_LENGTH];
    sprintf( buf, "che_watch entry: plev=%d pname=%s psite=%s",
                  player_level, player_name, player_site);
    log_string(buf);
*/
    if ( !first_watch ) return FALSE;

    for ( pw = first_watch; pw; pw = pw->next )
    {
        if ( pw->target_name )
        {
           if ( !str_cmp(pw->target_name, player_name)
           &&   player_level < pw->imm_level )
                 return TRUE;
        }
        else
        if ( pw->player_site )
        {
           if ( !str_prefix(pw->player_site, player_site)
           &&   player_level < pw->imm_level )
                 return TRUE;
        }
    }
    return FALSE;
}


void accept_new( int ctrl )
{
	static struct timeval null_time;
	DESCRIPTOR_DATA *d;
	/* int maxdesc; Moved up for use with id.c as extern */

#if defined(MALLOC_DEBUG)
	if ( malloc_verify( ) != 1 )
	    abort( );
#endif

	/*
	 * Poll all active descriptors.
	 */
	FD_ZERO( &in_set  );
	FD_ZERO( &out_set );
	FD_ZERO( &exc_set );
	FD_SET( ctrl, &in_set );

	maxdesc = ctrl;
	newdesc = 0;
	for ( d = first_descriptor; d; d = d->next )
	{
	    maxdesc = UMAX( maxdesc, d->descriptor );
	    FD_SET( d->descriptor, &in_set  );
	    FD_SET( d->descriptor, &out_set );
	    FD_SET( d->descriptor, &exc_set );
	    if( d->ifd != -1 && d->ipid != -1 )
	    {
	    	maxdesc = UMAX( maxdesc, d->ifd );
	    	FD_SET( d->ifd, &in_set );
	    }
	    if ( d == last_descriptor )
	      break;
	}
	auth_maxdesc(&maxdesc, &in_set, &out_set, &exc_set);

	if ( select( maxdesc+1, &in_set, &out_set, &exc_set, &null_time ) < 0 )
	{
	    perror( "accept_new: select: poll" );
	    exit( 1 );
	}


	if ( FD_ISSET( ctrl, &exc_set ) )
	{
	    bug( "Exception raise on controlling descriptor %d", ctrl );
	    FD_CLR( ctrl, &in_set );
	    FD_CLR( ctrl, &out_set );
	}
	else
	if ( FD_ISSET( ctrl, &in_set ) )
	{
	    newdesc = ctrl;
	    new_descriptor( newdesc );
	}
}

void game_loop( )
{
    struct timeval	  last_time;
    char cmdline[MAX_INPUT_LENGTH];
    DESCRIPTOR_DATA *d;
/*  time_t	last_check = 0;  */

#ifndef WIN32
    signal( SIGPIPE, SIG_IGN );
    signal( SIGALRM, caught_alarm );
#endif

    signal( SIGSEGV, SegVio );

    signal( SIGTERM, SigTerm ); /* Catch kill signals */

    gettimeofday( &last_time, NULL );
    current_time = (time_t) last_time.tv_sec;

    /* Main loop */
    while ( !mud_down )
    {
	accept_new( control  );

	auth_check(&in_set, &out_set, &exc_set);

	/*
	 * Kick out descriptors with raised exceptions
	 * or have been idle, then check for input.
	 */
	for ( d = first_descriptor; d; d = d_next )
	{
	    if ( d == d->next )
	    {
	      bug( "descriptor_loop: loop found & fixed" );
	      d->next = NULL;
	    }
 	    d_next = d->next;

	    d->idle++;	/* make it so a descriptor can idle out */
	    if (d->idle > 2)
	    	d->prev_idle = d->idle;

	    if ( FD_ISSET( d->descriptor, &exc_set ) )
	    {
		FD_CLR( d->descriptor, &in_set  );
		FD_CLR( d->descriptor, &out_set );
		if ( d->character
		&& ( d->connected == CON_PLAYING
		||   d->connected == CON_EDITING ) )
		    save_char_obj( d->character );
		d->outtop	= 0;
		close_socket( d, TRUE );
		continue;
	    }
	    else
	    if ( (!d->character && d->idle > 480)		  /* 2 mins */
            ||   ( d->connected != CON_PLAYING && d->idle > 1200) /* 5 mins */
	    ||     d->idle > 28800 )				  /* 2 hrs  */
	    {
		write_to_descriptor( d->descriptor,
		 "Idle timeout... disconnecting.\n\r", 0 );
		d->outtop	= 0;
		close_socket( d, TRUE );
		continue;
	    }
	    else
	    {
		d->fcommand	= FALSE;

		if ( FD_ISSET( d->descriptor, &in_set ) )
		{
			if ( d->character )
			{
			  d->character->timer = 0;
			  if (d->character->pcdata && d->character->level < 51)
			  {
			  	if (d->idle >= 180)
			  	{
			  	if (d->character->pcdata->iIdle < 0)
			  		d->character->pcdata->iIdle = 0;
			  	if (d->character->pcdata->iIdle > 4)
			  		d->character->pcdata->iIdle = 0;

			  	d->character->pcdata->pIdle[d->character->pcdata->iIdle] = ( d->idle / 4 );
			  	d->character->pcdata->bot_warn[0]++;
			  	d->character->pcdata->iIdle++;
			    }
			  }
			}
			if (d->prev_idle < 2)
				d->psuppress_cmdspam = TRUE;
			else
				d->psuppress_cmdspam = FALSE;

			d->psuppress_channel = 0;
			d->prev_idle = d->idle;
			d->idle = 0;

			if ( !read_from_descriptor( d ) )
			{
			    FD_CLR( d->descriptor, &out_set );
			    if ( d->character
			    && ( d->connected == CON_PLAYING
			    ||   d->connected == CON_EDITING ) )
				save_char_obj( d->character );
			    d->outtop	= 0;
			    close_socket( d, FALSE );
			    continue;
			}
		}

	      /* check for input from the dns */
	      if( ( d->connected == CON_PLAYING || d->character != NULL ) && d->ifd != -1 && FD_ISSET( d->ifd, &in_set ) )
	         process_dns( d );

		if ( d->character && d->character->wait > 0 )
		{
			--d->character->wait;
			continue;
		}

		read_from_buffer( d );
		if ( d->incomm[0] != '\0' )
		{
			d->fcommand	= TRUE;
			stop_idling( d->character );

			strcpy( cmdline, d->incomm );
			d->incomm[0] = '\0';

			if ( d->character )
			  set_cur_char( d->character );

			if ( d->pagepoint )
			  set_pager_input(d, cmdline);
			else
			  switch( d->connected )
			  {
			   default:
 				nanny( d, cmdline );
				break;
			   case CON_PLAYING:
				interpret( d->character, cmdline );
				break;
			   case CON_EDITING:
				edit_buffer( d->character, cmdline );
				break;
/*
    case CON_NOTE_TEXT:
//        handle_con_note_text (d, argument);
			edit_buffer( d->character, cmdline );
        break;
*/
			  }
		}
	    }
	    if ( d == last_descriptor )
	      break;
	}

#ifdef IMC
	imc_loop();
#endif

	/*
	 * Autonomous game motion.
	 */
	update_handler( );

	/*
	 * Check REQUESTS pipe
	 */
        check_requests( );

	/*
	 * Output.
	 */
	for ( d = first_descriptor; d; d = d_next )
	{
	    d_next = d->next;

	    if ( ( d->fcommand || d->outtop > 0 )
	    &&   FD_ISSET(d->descriptor, &out_set) )
	    {
	        if ( d->pagepoint )
	        {
	          if ( !pager_output(d) )
	          {
	            if ( d->character
	            && ( d->connected == CON_PLAYING
	            ||   d->connected == CON_EDITING ) )
	                save_char_obj( d->character );
	            d->outtop = 0;
	            close_socket(d, FALSE);
	          }
	        }
		else if ( !flush_buffer( d, TRUE ) )
		{
		    if ( d->character
		    && ( d->connected == CON_PLAYING
		    ||   d->connected == CON_EDITING ) )
			save_char_obj( d->character );
		    d->outtop	= 0;
		    close_socket( d, FALSE );
		}
	    }
	    if ( d == last_descriptor )
	      break;
	}

	/*
	 * Synchronize to a clock.
	 * Sleep( last_time + 1/PULSE_PER_SECOND - now ).
	 * Careful here of signed versus unsigned arithmetic.
	 */
	{
	    struct timeval now_time;
	    long secDelta;
	    long usecDelta;

	    gettimeofday( &now_time, NULL );
	    usecDelta	= ((int) last_time.tv_usec) - ((int) now_time.tv_usec)
			+ 1000000 / PULSE_PER_SECOND;
	    secDelta	= ((int) last_time.tv_sec ) - ((int) now_time.tv_sec );
	    while ( usecDelta < 0 )
	    {
		usecDelta += 1000000;
		secDelta  -= 1;
	    }

	    while ( usecDelta >= 1000000 )
	    {
		usecDelta -= 1000000;
		secDelta  += 1;
	    }

	    if ( secDelta > 0 || ( secDelta == 0 && usecDelta > 0 ) )
	    {
		struct timeval stall_time;

		stall_time.tv_usec = usecDelta;
		stall_time.tv_sec  = secDelta;
#ifdef WIN32
		Sleep( (stall_time.tv_sec * 1000L) + (stall_time.tv_usec / 1000L) );
#else
		if ( select( 0, NULL, NULL, NULL, &stall_time ) < 0 && errno != EINTR )
		{
		    perror( "game_loop: select: stall" );
		    exit( 1 );
		}
#endif
	    }
	}

	gettimeofday( &last_time, NULL );
	current_time = (time_t) last_time.tv_sec;

        /* Check every 5 seconds...  (don't need it right now)
	if ( last_check+5 < current_time )
	{
	  CHECK_LINKS(first_descriptor, last_descriptor, next, prev,
	      DESCRIPTOR_DATA);
	  last_check = current_time;
	}
	*/
    }
    /*  Save morphs so can sort later. --Shaddai */
    if ( sysdata.morph_opt )
	save_morphs( );

    fflush(stderr);	/* make sure strerr is flushed */
    return;
}


void new_descriptor( int new_desc )
{
    char buf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *dnew;
    struct sockaddr_in sock;
    struct hostent *from;
    size_t desc, size;
    char bugbuf[MAX_STRING_LENGTH];
#ifdef WIN32
    unsigned long arg = 1;
#endif

    size = sizeof(sock);
    if ( check_bad_desc( new_desc ) )
    {
      set_alarm( 0 );
      return;
    }
    set_alarm( 20 );
    alarm_section = "new_descriptor::accept";
    if ( ( desc = accept( new_desc, (struct sockaddr *) &sock, &size) ) < 0 )
    {
	perror( "New_descriptor: accept" );
	sprintf(bugbuf, "[*****] BUG: New_descriptor: accept");
	log_string_plus( bugbuf, LOG_COMM, sysdata.log_level );
	set_alarm( 0 );
	return;
    }
    if ( check_bad_desc( new_desc ) )
    {
      set_alarm( 0 );
      return;
    }
#if !defined(FNDELAY)
#define FNDELAY O_NDELAY
#endif

    set_alarm( 20 );
    alarm_section = "new_descriptor: after accept";

#ifdef WIN32
    if ( ioctlsocket(desc, FIONBIO, &arg) == -1 )
#else
    if ( fcntl( desc, F_SETFL, FNDELAY ) == -1 )
#endif
    {
	perror( "New_descriptor: fcntl: FNDELAY" );
	set_alarm( 0 );
	return;
    }
    if ( check_bad_desc( new_desc ) )
      return;

    CREATE( dnew, DESCRIPTOR_DATA, 1 );
    dnew->next		= NULL;
    dnew->descriptor	= desc;
    dnew->connected	= CON_GET_NAME;
    dnew->ansi	  = TRUE; /* force ansi */
    dnew->outsize	= 2000;
    dnew->idle		= 0;
    dnew->lines		= 0;
    dnew->scrlen	= 24;
    dnew->port		= ntohs( sock.sin_port );
    dnew->user 		= STRALLOC("(unknown)");
    dnew->newstate	= 0;
    dnew->prevcolor	= 0x07;
    dnew->ifd           = -1;    /* Descriptor pipes, used for DNS resolution and such */
    dnew->ipid          = -1;

    CREATE( dnew->outbuf, char, dnew->outsize );
    strcpy( log_buf, inet_ntoa( sock.sin_addr ) );

    dnew->host = STRALLOC( log_buf );
/*
    if ( !sysdata.NO_NAME_RESOLVING )
    {
	strcpy( buf, in_dns_cache( log_buf ) );

	if( buf[0] == '\0' )
	   resolve_dns( dnew, sock.sin_addr.s_addr );
	else
	{
	   STRFREE( dnew->host2 );
	   dnew->host = STRALLOC( buf );
	}
    }
*/
//		OLD DNS CODE --Saiyr

//    else
//    {
	from = gethostbyaddr( (char *) &sock.sin_addr,
		sizeof( sock.sin_addr), AF_INET );
	dnew->host2 = STRALLOC( (char *)( from ? from->h_name : buf ) );
//    }
    if ( check_total_bans( dnew ) )
    {
          write_to_descriptor (desc,
                         "Your site has been banned from this Mud.\n\r", 0);
          free_desc (dnew);
          set_alarm (0);
          return;
     }
    /*
     * Init descriptor data.
     */

    if ( !last_descriptor && first_descriptor )
    {
	DESCRIPTOR_DATA *d;

	bug( "New_descriptor: last_desc is NULL, but first_desc is not! ...fixing" );
	for ( d = first_descriptor; d; d = d->next )
	   if ( !d->next )
		last_descriptor = d;
    }

    LINK( dnew, first_descriptor, last_descriptor, next, prev );

#ifdef MCCP
    write_to_buffer(dnew, eor_on_str, 0);
    write_to_buffer(dnew, compress2_on_str, 0);
    write_to_buffer(dnew, compress_on_str, 0);
#endif


    /*
     * Send the greeting.
     */
    {
	     extern char * help_greeting;
	if ( help_greeting[0] == '.' )
	    send_to_desc_color( help_greeting+1, dnew );
       else
	    send_to_desc_color( help_greeting , dnew );
      }
	 send_to_desc_color( "&wEnter your character's name, or type &Wnew&w: &D", dnew );

    alarm_section = "new_descriptor: set_auth";
    set_auth(dnew);
    alarm_section = "new_descriptor: after set_auth";

    if ( ++num_descriptors > sysdata.maxplayers )
	sysdata.maxplayers = num_descriptors;
    if ( sysdata.maxplayers > sysdata.alltimemax )
    {
	if ( sysdata.time_of_max )
	  DISPOSE(sysdata.time_of_max);
	sprintf(buf, "%24.24s", ctime(&current_time));
	sysdata.time_of_max = str_dup(buf);
	sysdata.alltimemax = sysdata.maxplayers;
	sprintf( log_buf, "Broke all-time maximum player record: %d", sysdata.alltimemax );
	log_string_plus( log_buf, LOG_COMM, sysdata.log_level );
	to_channel( log_buf, CHANNEL_MONITOR, "Monitor", LEVEL_IMMORTAL );
	save_sysdata( sysdata );
    }
    set_alarm(0);
    return;
}

void free_desc( DESCRIPTOR_DATA *d )
{
    kill_auth(d);
    closesocket( d->descriptor );
    DISPOSE( d->outbuf );
    STRFREE( d->user );    /* identd */
    if ( d->pagebuf )
	DISPOSE( d->pagebuf );

#ifdef MCCP
    compressEnd(d);
#endif

   DISPOSE( d );
/*    --num_descriptors;  This is called from more than close_socket -- Alty */
    return;
}

void close_socket( DESCRIPTOR_DATA *dclose, bool force )
{
    AUTH_LIST *old_auth;
    CHAR_DATA *ch;
    DESCRIPTOR_DATA *d;
    bool DoNotUnlink = FALSE;
    OBJ_DATA *o;

    if( dclose->ipid != -1 )
    {
	int status;

	kill( dclose->ipid, SIGKILL );
	waitpid( dclose->ipid, &status, 0 );
    }
    if( dclose->ifd != -1 )
    	close( dclose->ifd );

    /* flush outbuf */
    if ( !force && dclose->outtop > 0 )
	flush_buffer( dclose, FALSE );

    /* say bye to whoever's snooping this descriptor */
    if ( dclose->snoop_by )
	write_to_buffer( dclose->snoop_by,
	    "Your victim has left the game.\n\r", 0 );

    /* stop snooping everyone else */
    for ( d = first_descriptor; d; d = d->next )
	if ( d->snoop_by == dclose )
	  d->snoop_by = NULL;

    /* Check for switched people who go link-dead. -- Altrag */
    if ( dclose->original )
    {
	if ( ( ch = dclose->character ) != NULL )
	  do_return(ch, "");
	else
	{
	  bug( "Close_socket: dclose->original without character %s",
		(dclose->original->name ? dclose->original->name : "unknown") );
	  dclose->character = dclose->original;
	  dclose->original = NULL;
	}
    }

    ch = dclose->character;

    /* sanity check :( */
    if ( !dclose->prev && dclose != first_descriptor )
    {
	DESCRIPTOR_DATA *dp, *dn;
	bug( "Close_socket: %s desc:%p != first_desc:%p and desc->prev = NULL!",
		ch ? ch->name : d->host, dclose, first_descriptor );
	dp = NULL;
	for ( d = first_descriptor; d; d = dn )
	{
	   dn = d->next;
	   if ( d == dclose )
	   {
		bug( "Close_socket: %s desc:%p found, prev should be:%p, fixing.",
		    ch ? ch->name : d->host, dclose, dp );
		dclose->prev = dp;
		break;
	   }
	   dp = d;
	}
	if ( !dclose->prev )
	{
	    bug( "Close_socket: %s desc:%p could not be found!.",
		    ch ? ch->name : dclose->host, dclose );
	    DoNotUnlink = TRUE;
	}
    }
    if ( !dclose->next && dclose != last_descriptor )
    {
	DESCRIPTOR_DATA *dp, *dn;
	bug( "Close_socket: %s desc:%p != last_desc:%p and desc->next = NULL!",
		ch ? ch->name : d->host, dclose, last_descriptor );
	dn = NULL;
	for ( d = last_descriptor; d; d = dp )
	{
	   dp = d->prev;
	   if ( d == dclose )
	   {
		bug( "Close_socket: %s desc:%p found, next should be:%p, fixing.",
		    ch ? ch->name : d->host, dclose, dn );
		dclose->next = dn;
		break;
	   }
	   dn = d;
	}
	if ( !dclose->next )
	{
	    bug( "Close_socket: %s desc:%p could not be found!.",
		    ch ? ch->name : dclose->host, dclose );
	    DoNotUnlink = TRUE;
	}
    }

    if ( dclose->character )
    {
	sprintf( log_buf, "Closing link to %s.", ch->pcdata->filename );
	log_string_plus( log_buf, LOG_COMM, UMAX( sysdata.log_level, ch->level ) );
/*
	if ( ch->level < LEVEL_DEMI )
	  to_channel( log_buf, CHANNEL_MONITOR, "Monitor", ch->level );
*/
	/* Link dead auth -- Rantic */
	old_auth = get_auth_name( ch->name );
	if ( old_auth != NULL && old_auth->state == AUTH_ONLINE )
	{
		old_auth->state = AUTH_LINK_DEAD;
		save_auth_list();
	}


        if ( (dclose->connected == CON_PLAYING
             || dclose->connected == CON_EDITING)
             ||(dclose->connected >= CON_NOTE_TO
             && dclose->connected <= CON_NOTE_FINISH))
	{
            char ldbuf[MAX_STRING_LENGTH];
	    act( AT_ACTION, "$n has lost $s link.", ch, NULL, NULL, TO_CANSEE );

            sprintf( ldbuf, "%s has gone linkdead", ch->name);
	    while( (o = carrying_noquit(ch)) != NULL )
            {
              obj_from_char(o);
              obj_to_room(o, ch->in_room);
            }
            if( !IS_IMMORTAL( ch ) )
              do_info( ch, ldbuf );
            else
              do_ainfo(ch, ldbuf);

	    ch->desc = NULL;
	}
	else
	{
	    /* clear descriptor pointer to get rid of bug message in log */
	    dclose->character->desc = NULL;
	    free_char( dclose->character );
	}
    }


    if ( !DoNotUnlink )
    {
	/* make sure loop doesn't get messed up */
	if ( d_next == dclose )
	  d_next = d_next->next;
	UNLINK( dclose, first_descriptor, last_descriptor, next, prev );
    }

#ifdef MCCP
    compressEnd(dclose);
#endif


    if ( dclose->descriptor == maxdesc )
      --maxdesc;

    free_desc( dclose );
    --num_descriptors;
    return;
}


bool read_from_descriptor( DESCRIPTOR_DATA *d )
{
    int iStart, iErr;

    /* Hold horses if pending command already. */
    if ( d->incomm[0] != '\0' )
	return TRUE;

    /* Check for overflow. */
    iStart = strlen(d->inbuf);
    if ( iStart >= sizeof(d->inbuf) - 10 )
    {
	sprintf( log_buf, "%s input overflow!", d->host );
	log_string( log_buf );
	write_to_descriptor( d->descriptor,
	    "\n\r*** PUT A LID ON IT!!! ***\n\rYou cannot enter the same command more than 20 consecutive times!\n\r", 0 );
	return FALSE;
    }

    for ( ; ; )
    {
	int nRead;

	nRead = recv( d->descriptor, d->inbuf + iStart,
	    sizeof(d->inbuf) - 10 - iStart, 0 );
#ifdef WIN32
	iErr = WSAGetLastError ();
#else
	iErr = errno;
#endif
	if ( nRead > 0 )
	{
	    iStart += nRead;
	    if ( d->inbuf[iStart-1] == '\n' || d->inbuf[iStart-1] == '\r' )
		break;
	}
	else if ( nRead == 0 )
	{
	    log_string_plus( "EOF encountered on read.", LOG_COMM, sysdata.log_level );
	    return FALSE;
	}
	else if ( iErr == EWOULDBLOCK )
	    break;
	else
	{
	    perror( "Read_from_descriptor" );
	    return FALSE;
	}
    }

    d->inbuf[iStart] = '\0';
    return TRUE;
}



/*
 * Transfer one line from input buffer to input line.
 */
void read_from_buffer( DESCRIPTOR_DATA *d )
{
    int i, j, k;

#ifdef MCCP
    int iac = 0;
#endif

    /*
     * Hold horses if pending command already.
     */
    if ( d->incomm[0] != '\0' )
	return;

    /*
     * Look for at least one new line.
     */
    for ( i = 0; d->inbuf[i] != '\n' && d->inbuf[i] != '\r' && i<MAX_INBUF_SIZE;
	  i++ )
    {
	if ( d->inbuf[i] == '\0' )
	    return;
    }

    /*
     * Canonical input processing.
     */
    for ( i = 0, k = 0; d->inbuf[i] != '\n' && d->inbuf[i] != '\r'; i++ )
    {
		int z = 0;
		if( d->connected == CON_EDITING )
			z = 254;
		else
			z = 762;
//	if ( k >= 254 ) -- This was the old buffer size, new is above.
	if ( k >= z )
	{
	    write_to_descriptor( d->descriptor, "Line too long.\n\r", 0 );

	    /* skip the rest of the line */
	    /*
	    for ( ; d->inbuf[i] != '\0' || i>= MAX_INBUF_SIZE ; i++ )
	    {
		if ( d->inbuf[i] == '\n' || d->inbuf[i] == '\r' )
		    break;
	    }
	    */
	    d->inbuf[i]   = '\n';
	    d->inbuf[i+1] = '\0';
	    break;
	}

#ifdef MCCP
        if ( d->inbuf[i] == (signed char)IAC )
            iac=1;
        else if ( iac==1 && (d->inbuf[i] == (signed char)DO || d->inbuf[i] == (signed char)DONT) )
            iac=2;
        else if ( iac==2 )
        {
            iac = 0;
            if ( d->inbuf[i] == (signed char)TELOPT_COMPRESS )
            {
                if ( d->inbuf[i-1] == (signed char)DO )
                    compressStart(d, TELOPT_COMPRESS);
                else if ( d->inbuf[i-1] == (signed char)DONT )
                    compressEnd(d);
            }
            else if ( d->inbuf[i] == (signed char)TELOPT_COMPRESS2 )
            {
                if ( d->inbuf[i-1] == (signed char)DO )
                    compressStart(d, TELOPT_COMPRESS2);
                else if ( d->inbuf[i-1] == (signed char)DONT )
                    compressEnd(d);
            }
        }
        else
#endif

	if ( d->inbuf[i] == '\b' && k > 0 )
	    --k;
	else if ( isascii(d->inbuf[i]) && isprint(d->inbuf[i]) )
	    d->incomm[k++] = d->inbuf[i];
    }

    /*
     * Finish off the line.
     */
    if ( k == 0 )
	d->incomm[k++] = ' ';
    d->incomm[k] = '\0';

    /*
     * Deal with bozos with #repeat 1000 ...
     */
    if ( k > 1 || d->incomm[0] == '!' )
    {
	if ( d->incomm[0] != '!' && strcmp( d->incomm, d->inlast ) )
	{
	    d->repeat = 0;
	}
	else
	{
	    if ( ++d->repeat >= 20 )
	    {
/*		sprintf( log_buf, "%s input spamming!", d->host );
		log_string( log_buf );
*/
		write_to_descriptor( d->descriptor,
		    "\n\r*** PUT A LID ON IT!!! ***\n\rYou cannot enter the same command more than 20 consecutive times!\n\r", 0 );
		strcpy( d->incomm, "quit" );
	    }
	}
    }

    /*
     * Do '!' substitution.
     */
    if ( d->incomm[0] == '!' )
	strcpy( d->incomm, d->inlast );
    else
	strcpy( d->inlast, d->incomm );

    /*
     * Shift the input buffer.
     */
    while ( d->inbuf[i] == '\n' || d->inbuf[i] == '\r' )
	i++;
    for ( j = 0; ( d->inbuf[j] = d->inbuf[i+j] ) != '\0'; j++ )
	;
    return;
}



/*
 * Low level output function.
 */
bool flush_buffer( DESCRIPTOR_DATA *d, bool fPrompt )
{
    char buf[MAX_INPUT_LENGTH];
    char promptbuf[MAX_STRING_LENGTH];
    extern bool mud_down;

    /*
     * If buffer has more than 4K inside, spit out .5K at a time   -Thoric
     */
    if ( !mud_down && d->outtop > 4096 )
    {
	memcpy( buf, d->outbuf, 512 );
	d->outtop -= 512;
	memmove( d->outbuf, d->outbuf + 512, d->outtop );
	if ( d->snoop_by )
	{
	    char snoopbuf[MAX_INPUT_LENGTH];

	    buf[512] = '\0';
	    if ( d->character && d->character->name )
	    {
		if (d->original && d->original->name)
		    sprintf( snoopbuf, "%s (%s)", d->character->name, d->original->name );
		else
		    sprintf( snoopbuf, "%s", d->character->name);
		write_to_buffer( d->snoop_by, snoopbuf, 0);
	    }
	    write_to_buffer( d->snoop_by, "% ", 2 );
	    write_to_buffer( d->snoop_by, buf, 0 );
	}
        if ( !write_to_descriptor( d->descriptor, buf, 512 ) )
        {
	    d->outtop = 0;
	    return FALSE;
        }
        return TRUE;
    }


    /*
     * Bust a prompt.
     */
    if ( fPrompt && !mud_down && d->connected == CON_PLAYING )
    {
	CHAR_DATA *ch;

	ch = d->original ? d->original : d->character;

	if ( xIS_SET(ch->act, PLR_BLANK))
	    write_to_buffer( d, "\n\r", 2 );

	if (ch->fighting)
	{
		d->psuppress_cmdspam = FALSE;
		d->psuppress_channel = 0;
	}

	if (d->psuppress_channel >= 5)
		d->psuppress_channel = 0;

	if ( xIS_SET(ch->act, PLR_PROMPT) )
	{
		sysdata.outBytesFlag = LOGBOUTPROMPT;
	    if (!d->psuppress_cmdspam && !d->psuppress_channel)
	    {
			display_prompt(d);
			d->psuppress_cmdspam = FALSE;
	    }
	    else if (d->psuppress_channel && ch->pcdata)
	    {
			switch (ch->pcdata->normalPromptConfig)
			{
				default:
					if (is_android(ch))
						sprintf(promptbuf, "&D<D:%d E:%s T:%s> ", ch->hit, num_punct(ch->mana), abbNumLD(ch->pl));
					else
						sprintf(promptbuf, "&D<L:%d K:%s P:%s> ", ch->hit, num_punct(ch->mana), abbNumLD(ch->pl));
					break;
				case 1:
					if (is_android(ch))
						sprintf(promptbuf, "&D<D:%d E:%s T:%s> ", ch->hit, num_punct(ch->mana), abbNumLD(ch->pl));
					else
						sprintf(promptbuf, "&D<L:%d K:%s P:%s> ", ch->hit, num_punct(ch->mana), abbNumLD(ch->pl));
					break;
			}
			send_to_char( promptbuf, ch );
			if( get_true_rank( ch ) >= 11 )
			    pager_printf( ch, "(KRT:%d) ", sysdata.kaiRestoreTimer );
		}
	    else
	    	send_to_char( "&D> ", ch );
		sysdata.outBytesFlag = LOGBOUTNORM;
	}

	if (!IS_NPC(ch))

	if ( xIS_SET(ch->act, PLR_TELNET_GA) )
	    write_to_buffer( d, go_ahead_str, 0 );
    }

    /*
     * Short-circuit if nothing to write.
     */
    if ( d->outtop == 0 )
	return TRUE;

    /*
     * Snoop-o-rama.
     */
    if ( d->snoop_by )
    {
        /* without check, 'force mortal quit' while snooped caused crash, -h */
	if ( d->character && d->character->name )
	{
	    /* Show original snooped names. -- Altrag */
	    if ( d->original && d->original->name )
		sprintf( buf, "%s (%s)", d->character->name, d->original->name );
	    else
		sprintf( buf, "%s", d->character->name);
	    write_to_buffer( d->snoop_by, buf, 0);
	}
	write_to_buffer( d->snoop_by, "% ", 2 );
	write_to_buffer( d->snoop_by, d->outbuf, d->outtop );
    }

    /*
     * OS-dependent output.
     */
    if ( !write_to_descriptor( d->descriptor, d->outbuf, d->outtop ) )
    {
	d->outtop = 0;
	return FALSE;
    }
    else
    {
	d->outtop = 0;
	return TRUE;
    }
}



/*
 * Append onto an output buffer.
 */
void write_to_buffer( DESCRIPTOR_DATA *d, const char *txt, int length )
{
    if ( !d )
    {
	bug( "Write_to_buffer: NULL descriptor" );
	return;
    }

    /*
     * Normally a bug... but can happen if loadup is used.
     */
    if ( !d->outbuf )
    	return;

    /*
     * Find length in case caller didn't.
     */
    if ( length <= 0 )
	length = strlen(txt);

/* Uncomment if debugging or something
    if ( length != strlen(txt) )
    {
	bug( "Write_to_buffer: length(%d) != strlen(txt)!", length );
	length = strlen(txt);
    }
*/

    /*
     * Initial \n\r if needed.
     */
    if ( d->outtop == 0 && !d->fcommand )
    {
	d->outbuf[0]	= '\n';
	d->outbuf[1]	= '\r';
	d->outtop	= 2;
    }

    /*
     * Expand the buffer as needed.
     */
    while ( d->outtop + length >= d->outsize )
    {
        if (d->outsize > 32000)
	{
	    /* empty buffer */
	    d->outtop = 0;
	    bug("Buffer overflow. Closing (%s).", d->character ? d->character->name : "???" );
	    close_socket(d, TRUE);
	    return;
 	}
	d->outsize *= 2;
	RECREATE( d->outbuf, char, d->outsize );
    }

    /*
     * Copy.
     */
    strncpy( d->outbuf + d->outtop, txt, length );
    d->outtop += length;
    d->outbuf[d->outtop] = '\0';
    return;
}

#ifdef MCCP
#define COMPRESS_BUF_SIZE 1024

bool write_to_descriptor( int desc, char *txt, int length )
{
    DESCRIPTOR_DATA *d;
    int     iStart = 0;
    int     nWrite = 0;
    int     nBlock;
    int     len;

    if (length <= 0)
        length = strlen(txt);

    for (d = first_descriptor; d; d = d->next)
        if (d->descriptor == desc)
            break;

    if (d && d->descriptor != desc)
        d = NULL;


    if (d && d->out_compress)
    {
        d->out_compress->next_in = (unsigned char *)txt;
        d->out_compress->avail_in = length;

        while (d->out_compress->avail_in)
        {
            d->out_compress->avail_out = COMPRESS_BUF_SIZE - (d->out_compress->next_out - d->out_compress_buf);

            if (d->out_compress->avail_out)
            {
                int status = deflate(d->out_compress, Z_SYNC_FLUSH);

                if (status != Z_OK)
                    return FALSE;
            }

            len = d->out_compress->next_out - d->out_compress_buf;
            if (len > 0)
            {
                for (iStart = 0; iStart < len; iStart += nWrite)
                {
                    nBlock = UMIN (len - iStart, 4096);
                    if ((nWrite = write(d->descriptor, d->out_compress_buf + iStart, nBlock)) < 0)
                    {
                        perror( "Write_to_descriptor: compressed" );
                        return FALSE;
                    }

                    if (!nWrite)
                        break;
                }

                if (!iStart)
                    break;

                if (iStart < len)
                    memmove(d->out_compress_buf, d->out_compress_buf+iStart, len - iStart);

                d->out_compress->next_out = d->out_compress_buf + len - iStart;
            }
        }
        return TRUE;
    }

    for (iStart = 0; iStart < length; iStart += nWrite)
    {
        nBlock = UMIN (length - iStart, 4096);
        if ((nWrite = write(desc, txt + iStart, nBlock)) < 0)
        {
            perror( "Write_to_descriptor" );
            return FALSE;
        }
    }

    return TRUE;
}
#else

/*
 * Lowest level output function.
 * Write a block of text to the file descriptor.
 * If this gives errors on very long blocks (like 'ofind all'),
 *   try lowering the max block size.
 */
bool write_to_descriptor( int desc, char *txt, int length )
{
    int iStart;
    int nWrite;
    int nBlock;

    if ( length <= 0 )
	length = strlen(txt);

    for ( iStart = 0; iStart < length; iStart += nWrite )
    {
	nBlock = UMIN( length - iStart, 4096 );
	if ( ( nWrite = send( desc, txt + iStart, nBlock, 0 ) ) < 0 )
	    { perror( "Write_to_descriptor" ); return FALSE; }
    }

    return TRUE;
}

#endif

/* Added to try to fix hotboot issue */
bool write_to_descriptor_old( int desc, char *txt, int length )
{
    int iStart;
    int nWrite;
    int nBlock;

    if ( length <= 0 )
	length = strlen(txt);

    for ( iStart = 0; iStart < length; iStart += nWrite )
    {
	nBlock = UMIN( length - iStart, 4096 );
	if ( ( nWrite = send( desc, txt + iStart, nBlock, 0 ) ) < 0 )
	    { perror( "Write_to_descriptor" ); return FALSE; }
    }

    return TRUE;
}
/* End added */

void show_title( DESCRIPTOR_DATA *d )
{
    CHAR_DATA *ch;

    ch = d->character;

    if ( !IS_SET( ch->pcdata->flags, PCFLAG_NOINTRO ) )
    {
	if (xIS_SET(ch->act, PLR_RIP))
	  send_rip_title(ch);
	else
	if (xIS_SET(ch->act, PLR_ANSI))
	  send_ansi_title(ch);
	else
	  send_ascii_title(ch);
    }
    else
    {
      write_to_buffer( d, "Press enter...\n\r", 0 );
    }
    d->connected = CON_PRESS_ENTER;
}

/*
 * Deal with sockets that haven't logged in yet.
 */
void nanny( DESCRIPTOR_DATA *d, char *argument )
{
/*	extern int lang_array[];
	extern char *lang_names[];*/
    char buf[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    char buf3[MAX_STRING_LENGTH];
    char buf4[MAX_STRING_LENGTH];
    char arg[MAX_STRING_LENGTH];
    CHAR_DATA *ch;
    char *pwdnew;
    char *p;
	int b = 0;
    int iClass;
//    int iRace;
    bool fOld, chk;
    NOTE_DATA *catchup_notes;
    int i = 0;
// these were making compile warnings making it look ugly :)  -Goku
//    FILE *ipFp;
//    char ipBuf[MAX_STRING_LENGTH];

    if( d->connected != CON_NOTE_TEXT )
    {
        while ( isspace(*argument) )
            argument++;
    }

    ch = d->character;

    switch ( d->connected )
    {

    default:
	bug( "Nanny: bad d->connected %d.", d->connected );
	close_socket( d, TRUE );
	return;

    case CON_GET_NAME:
	if ( argument[0] == '\0' )
	{
	    close_socket( d, FALSE );
	    return;
	}

//	argument[0] = UPPER(argument[0]);
	*argument = capitalizeString(argument);

	/* Old players can keep their characters. -- Alty */
	if ( !check_parse_name( argument, (d->newstate != 0) ) )
	{
	    send_to_desc_color( "&wIllegal name, try another.\n\rName: &D", d );
	    return;
	}

	bool blocked = FALSE; // Added for more convenient disabling
			      // of newbie creation. -Karma
        if ( !str_cmp( argument, "New" ) && !blocked )
	{
	    if (d->newstate == 0)
	    {
              /* New player */
              /* Don't allow new players if DENY_NEW_PLAYERS is true */
      	      if (sysdata.DENY_NEW_PLAYERS == TRUE)
      	      {
       		sprintf( buf, "The mud is currently preparing for a reboot.\n\r" );
      		send_to_desc_color( buf, d );
			sprintf( buf, "New players are not accepted during this time.\n\r" );
      		send_to_desc_color( buf, d );
      		sprintf( buf, "Please try again in a few minutes.\n\r" );
      		send_to_desc_color( buf, d );
			close_socket( d, FALSE );
              }
              sprintf( buf, "\n\r&gChoosing a name is one of the most important parts of this game...\n\r"
              			"Make sure to pick a name appropriate to the character you are going\n\r"
               			"to role play, and be sure that it fits into the DragonBall Z world.\n\r"
               			"Please type '&WHELP&g' to read what restirictions we have for naming your\n\r"
               			"character.\n\r\n\r&wPlease choose a name for your character: &D");
              send_to_desc_color( buf, d );
	      d->newstate++;
	      d->connected = CON_GET_NAME;
	      return;
	    }
	    else
   	    {
	      send_to_desc_color("&wIllegal name, try another.\n\rName: &D", d);
	      return;
	    }
	}

    if ( !str_cmp( argument, "help" ) )
    {
		HELP_DATA *pHelp;

	    for ( pHelp = first_help; pHelp; pHelp = pHelp->next )
	    {
	    	if (!str_cmp( pHelp->keyword, "dbznames" ))
	    		break;
		}
		if (!pHelp)
		{
			send_to_desc_color( "No help on that word.\n\rName: ", d);
			return;
		}
		send_to_desc_color("\n\r", d);
		send_to_desc_color(pHelp->text, d);
		send_to_desc_color("\n\r\n\r&wName: ", d);
		return;
	}

	if ( check_playing( d, argument, FALSE ) == BERR )
	{
	    write_to_buffer( d, "Name: ", 0 );
	    return;
	}

	fOld = load_char_obj( d, argument, TRUE );
	if ( !d->character )
	{
	    sprintf( log_buf, "Bad player file %s@%s.", argument, d->host );
	    log_string( log_buf );
	    send_to_desc_color( "Your playerfile is corrupt...Please notify GokuDBS@hotmail.com.\n\r", d );
	    close_socket( d, FALSE );
	    return;
	}
	ch   = d->character;
      if ( check_bans( ch, BAN_SITE ) )
      {
              send_to_desc_color ("Your site has been banned from this Mud.\n\r", d);
              close_socket (d, FALSE);
              return;
      }

      if ( fOld ) {
      if ( check_bans( ch, BAN_CLASS ) )
      {
              send_to_desc_color ("Your class has been banned from this Mud.\n\r", d);
              close_socket (d, FALSE);
              return;
      }
      if ( check_bans( ch, BAN_RACE ) )
      {
              send_to_desc_color ("Your race has been banned from this Mud.\n\r", d);
              close_socket (d, FALSE);
              return;
      }
      }

	if ( xIS_SET(ch->act, PLR_DENY) )
	{
	    sprintf( log_buf, "Denying access to %s@%s.", argument, d->host );
	    log_string_plus( log_buf, LOG_COMM, sysdata.log_level );
	    if (d->newstate != 0)
	    {
              send_to_desc_color( "That name is already taken.  Please choose another: ", d );
	      d->connected = CON_GET_NAME;
	      d->character->desc = NULL;
	      free_char( d->character ); /* Big Memory Leak before --Shaddai */
	      d->character = NULL;
	      return;
	    }
	    send_to_desc_color( "You are denied access.\n\r", d );
	    close_socket( d, FALSE );
	    return;
	}
      /*
       *  Make sure the immortal host is from the correct place.
       *  Shaddai
       */

    if ( check_total_ip( d ) )
    {
          send_to_desc_color (
                         "Your maximum amount of players you can have online has been reached.\n\r", d);
          close_socket( d, FALSE );
          return;
     }

      if ( IS_IMMORTAL(ch) && sysdata.check_imm_host
           && !check_immortal_domain( ch , d->host) )
        {
        sprintf (log_buf, "%s's char being hacked from %s.", argument, d->host);
        log_string_plus (log_buf, LOG_COMM, sysdata.log_level);
        sprintf (log_buf, "&R%s's char being hacked from %s.", argument, d->host);
        ch->level = 51;
        do_ainfo(ch, log_buf);
        send_to_desc_color ("This hacking attempt has been logged.\n\r", d);
        close_socket (d, FALSE);
        return;
        }


	chk = check_reconnect( d, argument, FALSE );
	if ( chk == BERR )
	  return;

	if ( chk )
	{
	    fOld = TRUE;
	}
	else
	{
//	    if ( wizlock && !IS_IMMORTAL(ch) )
	    if( wizlock && ch->level < locklev )
	    {
		send_to_desc_color( "The game is wizlocked.  Only immortals can connect now.\n\r", d );
		send_to_desc_color( "Please try back later.\n\r", d );
		close_socket( d, FALSE );
		return;
	    }
	}

	if ( fOld )
	{
	    if (d->newstate != 0)
	    {
	      send_to_desc_color( "&wThat name is already taken.  Please choose another: &D", d );
	      d->connected = CON_GET_NAME;
	      d->character->desc = NULL;
	      free_char( d->character ); /* Big Memory Leak before --Shaddai */
	      d->character = NULL;
	      return;
	    }
	    /* Old player */
	    send_to_desc_color( "&wPassword: &D", d );
	    write_to_buffer( d, echo_off_str, 0 );
	    d->connected = CON_GET_OLD_PASSWORD;
	    return;
	}
	else
	{
	    /*if ( !check_parse_name( argument ) )
	    {
		write_to_buffer( d, "Illegal name, try another.\n\rName: ", 0 );
		return;
	    }*/

	    if (d->newstate == 0)
	    {
	      /* No such player */
	      send_to_desc_color( "\n\r&wNo such player exists.\n\rPlease check your spelling, or type new to start a new player.\n\r\n\rName: &D", d );
	      d->connected = CON_GET_NAME;
	      d->character->desc = NULL;
	      free_char( d->character ); /* Big Memory Leak before --Shaddai */
	      d->character = NULL;
	      return;
	    }

            sprintf( buf, "&wDid I get that right, %s (&WY&w/&WN&w)? &D", argument );
            send_to_desc_color( buf, d );
            d->connected = CON_CONFIRM_NEW_NAME;
	    return;
	}
	break;

    case CON_GET_OLD_PASSWORD:
      write_to_buffer( d, "\n\r", 2 );

      if( str_cmp( sha256_crypt( argument ), ch->pcdata->pwd ) )
      {
         write_to_buffer( d, "Wrong password, disconnecting.\n\r", 0 );
         /* clear descriptor pointer to get rid of bug message in log */
         d->character->desc = NULL;
         close_socket( d, FALSE );
         return;
      }

      write_to_buffer( d, echo_on_str, 0 );

      if( check_playing( d, ch->pcdata->filename, TRUE ) )
         return;

      chk = check_reconnect( d, ch->pcdata->filename, TRUE );
      if( chk == BERR )
      {
         if( d->character && d->character->desc )
            d->character->desc = NULL;
         close_socket( d, FALSE );
         return;
      }
      if( chk == TRUE )
         return;

      strncpy( buf, ch->pcdata->filename, MAX_STRING_LENGTH );
      d->character->desc = NULL;
      free_char( d->character );
      d->character = NULL;
      fOld = load_char_obj( d, buf, FALSE );
      ch = d->character;
      if( ch->position > POS_SITTING && ch->position < POS_STANDING )
         ch->position = POS_STANDING;

      sprintf( log_buf, "%s@%s(%s) has connected.", ch->pcdata->filename, d->host, d->user );
      log_string_plus( log_buf, LOG_COMM, sysdata.log_level );

	if (ch->level == 2)
	{
		xSET_BIT(ch->deaf, CHANNEL_FOS);
		ch->level = 1;
	}
	sprintf( buf3, "%s has logged on", ch->name);
	if (!IS_IMMORTAL(ch))
		do_info(ch, buf3);
	else
		do_ainfo(ch, buf3);

        if ( !IS_IMMORTAL( ch ) && IS_AFFECTED(ch, AFF_DEAD) )
        {
          sprintf( buf4, "%s has a halo", ch->name );
	  log_string_plus( buf4, LOG_HIGH, LEVEL_LESSER );
        }
	/* player data update checks */

	pager_printf(ch, "Checking for player data updates...\n\r");

	if( ch->pcdata->upgradeL > CURRENT_UPGRADE_LEVEL )
	  ch->pcdata->upgradeL = CURRENT_UPGRADE_LEVEL - 1;

	if (upgrade_player(ch))
		pager_printf(ch, "Updated player data successfully.\n\r");
	else
		pager_printf(ch, "No updates to make.\n\r");

	adjust_hiscore( "pkill", ch, ch->pcdata->pkills ); /* cronel hiscore */
	adjust_hiscore( "sparwins", ch, ch->pcdata->spar_wins );
	adjust_hiscore( "sparloss", ch, ch->pcdata->spar_loss );
	adjust_hiscore( "mkills", ch, ch->pcdata->mkills );
	adjust_hiscore( "deaths", ch, (ch->pcdata->pdeaths + ch->pcdata->mdeaths) );
	update_plHiscore(ch);
	adjust_hiscore( "played", ch, ((get_age(ch) - 4) * 2) );
	adjust_hiscore( "zeni", ch, ch->gold );
	adjust_hiscore("bounty", ch, ch->pcdata->bkills);
	update_member(ch);

	if ( ch->level < LEVEL_DEMI )
	{
	  /*to_channel( log_buf, CHANNEL_MONITOR, "Monitor", ch->level );*/
	  log_string_plus( log_buf, LOG_COMM, sysdata.log_level );
	}
	else
	  log_string_plus( log_buf, LOG_COMM, ch->level );
	show_title(d);
	break;

    case CON_CONFIRM_NEW_NAME:
	switch ( *argument )
	{
	case 'y': case 'Y':
	    sprintf( buf, "\n\r&wMake sure to use a password that won't be easily guessed by someone else."
	    		  "\n\rPick a good password for %s: %s&D",
		ch->name, echo_off_str );
	    send_to_desc_color( buf, d );
       	xSET_BIT(ch->act, PLR_ANSI);
	    d->connected = CON_GET_NEW_PASSWORD;
	    break;

	case 'n': case 'N':
	    send_to_desc_color( "&wOk, what IS it, then? &D", d );
	    /* clear descriptor pointer to get rid of bug message in log */
	    d->character->desc = NULL;
	    free_char( d->character );
	    d->character = NULL;
	    d->connected = CON_GET_NAME;
	    break;

	default:
	    send_to_desc_color( "&wPlease type &WY&wes or &WN&wo. &D", d );
	    break;
	}
	break;

    case CON_GET_NEW_PASSWORD:
	send_to_desc_color( "\n\r", d );

	if ( strlen(argument) < 5 )
	{
	    send_to_desc_color("&wPassword must be at least five characters long.\n\rPassword: &D", d );
	    return;
	}

	pwdnew = sha256_crypt( argument );   /* SHA256 Encryption */
	for ( p = pwdnew; *p != '\0'; p++ )
	{
	    if ( *p == '~' )
	    {
		send_to_desc_color("&wNew password not acceptable, try again.\n\rPassword: &D", d );
		return;
	    }
	}

	DISPOSE( ch->pcdata->pwd );
	ch->pcdata->pwd	= str_dup( pwdnew );
	send_to_desc_color( "\n\r&wPlease retype the password to confirm: &D", d );
	d->connected = CON_CONFIRM_NEW_PASSWORD;
	break;

    case CON_CONFIRM_NEW_PASSWORD:
	send_to_desc_color( "\n\r", d );

         if( str_cmp( sha256_crypt( argument ), ch->pcdata->pwd ) )
         {
            write_to_buffer( d, "Passwords don't match.\n\rRetype password: ", 0 );
            d->connected = CON_GET_NEW_PASSWORD;
            return;
         }

	write_to_buffer( d, echo_on_str, 0 );
	send_to_desc_color( "\n\r&wWhat do you want your last name to be? [press enter for none] &D\n\r", d );
	d->connected = CON_GET_LAST_NAME;
	break;

	case CON_GET_LAST_NAME:
	if ( argument[0] == '\0' )
	{
	write_to_buffer( d, echo_on_str, 0 );
	send_to_desc_color( "\n\rDo you wish to be a &RHARDCORE&w character? (&WY&w/&WN&w)\n\rType &WHELP&w for more information.", d );
	d->connected = CON_GET_HC;
	return;
	}

	argument[0] = UPPER(argument[0]);
	*argument = capitalizeString(argument);
	/* Old players can keep their characters. -- Alty */
	if ( !check_parse_name( argument, TRUE ) )
	{
	    send_to_desc_color( "&wIllegal name, try another.\n\rLast name: &D", d );
	    return;
	}

    sprintf( buf, "&wDid I get that right, %s (&WY&w/&WN&w)? &D", argument );
    send_to_desc_color( buf, d );
	DISPOSE(ch->pcdata->last_name);
	ch->pcdata->last_name = str_dup( "" );
	buf[0] = ' ';
	strcpy( buf+1, argument );
    ch->pcdata->last_name = strdup(buf);
    d->connected = CON_CONFIRM_LAST_NAME;
	return;
	break;

	case CON_CONFIRM_LAST_NAME:
	switch ( *argument )
	{
	case 'y': case 'Y':
	write_to_buffer( d, echo_on_str, 0 );
	send_to_desc_color( "\n\rDo you wish to be a &RHARDCORE&w character? (&WY&w/&WN&w)\n\rType &WHELP&w for more information.", d );
	d->connected = CON_GET_HC;
	    break;

	case 'n': case 'N':
	    send_to_desc_color( "&wOk, what IS it, then? &D", d );
	    d->connected = CON_GET_LAST_NAME;
	    break;

	default:
	    send_to_desc_color( "&wPlease type &WY&wes or &WN&wo. &D", d );
	    break;
	}
	break;


	case CON_GET_HC:
	if (!str_cmp(argument, "help"))
	{
		HELP_DATA *pHelp;

	    for ( pHelp = first_help; pHelp; pHelp = pHelp->next )
	    {
	    	if (!str_cmp( pHelp->keyword, "HC HARDCORE UNKNOWN" ))
	    		break;
		}
		if (!pHelp)
		{
			send_to_desc_color( "No help on that word.\n\rDo you wish to be a &RHARDCORE&w character? (&WY&w/&WN&w): ", d);
			return;
		}
		send_to_desc_color("\n\r", d);
		send_to_desc_color(pHelp->text, d);
		send_to_desc_color( "&wDo you wish to be a &RHARDCORE&w character? (&WY&w/&WN&w): ", d);
		return;
	}
	switch ( *argument )
	{
	case 'y': case 'Y':
	    sprintf( buf, "\n\r&wOkay, you are now &RHARDCORE&w!&D" );
	    send_to_desc_color( buf, d );
	    xSET_BIT(ch->act, PLR_HC);
	write_to_buffer( d, echo_on_str, 0 );
	break;

	case 'n': case 'N':
	    send_to_desc_color( "\n\r&wOkay, you are a normal character.&D", d );
	write_to_buffer( d, echo_on_str, 0 );
	    break;

	default:
	    send_to_desc_color( "&wPlease type Yes or No. &D", d );
	    return;
	}
	send_to_desc_color( "\n\r&wWhat is your sex (&CM&w/&PF&w/&WN&w)? &D", d );
	d->connected = CON_GET_NEW_SEX;
	    break;

    case CON_GET_NEW_SEX:
	switch ( argument[0] )
	{
	case 'm': case 'M': ch->sex = SEX_MALE;    break;
	case 'f': case 'F': ch->sex = SEX_FEMALE;  break;
	case 'n': case 'N': ch->sex = SEX_NEUTRAL; break;
	default:
	    send_to_desc_color( "&wThat's not a sex.\n\rWhat IS your sex? &D", d );
	    return;
	}

	/* Added for when I need to close down char creation to do
	   work on this section. Leaving the commented code in for
	   future use, if the need arises. -Karma
	if(!(!str_cmp(d->character->name,"lemming")) )
	{
	  send_to_desc_color( "Character creation is currently disabled.\n\r",d);
	  return;
	}
	*/

	send_to_desc_color( "\n\r&wThe following Races are Available to You:&D\n\r", d );
	send_to_desc_color( "&c==============================================================================&D",d);
	buf[0] = '\0';

	/*
	 * Take this out SHADDAI
	 */
	i=0;
	/*for ( iClass = 0; iClass < MAX_PC_CLASS; iClass++ )*/
	send_to_desc_color( "\n\r", d );
	//for ( iClass = 0; iClass < 9; iClass++ )
	for ( iClass = 0; iClass < 31; iClass++ )
	{
	    if( iClass == 4 )
	    {
		//i++;
		continue;
	    }
	    if( iClass > 8 && iClass < 28 )
                continue;
	    //char letters[11] = "abcdefghij";
	    char letters[14] = "abcdefghijklmn";
	    if ( class_table[iClass]->who_name &&
	    	 class_table[iClass]->who_name[0] != '\0' )
	    {
	    sprintf( buf, "&w   (&W%2d&w)  &c%-12s&w  ('&R%c&w' for help)&D\n\r",
	    	i, class_table[iClass]->who_name, letters[i]);
			send_to_desc_color(buf, d);
	    i++;
	    }
	}
	send_to_desc_color( "&c==============================================================================&D",d);
	sprintf( buf, "\n\r&wChoose the number of your race: &D" );
	send_to_desc_color( buf, d );
	d->connected = CON_GET_NEW_CLASS;
	break;

    case CON_GET_NEW_CLASS:
	argument = one_argument(argument, arg);
		if (is_number(arg))
		{
			i = atoi(arg);
			int c = 0;
			if( i == 0 )
                          c = 0; // saian
			if( i == 1 )
                          c = 1; // human
			if( i == 2 )
                          c = 2; // halfbreed
			if( i == 3 )
                          c = 3; // namek
			if( i == 4 )
                          c = 5; // icer
                        if( i == 5 )
                          c = 6; // bio
                        if( i == 6 )
                          c = 7; // kaio
                        if( i == 7 )
                          c = 8; // demon
                        if( i == 8 )
                          c = 28; // android-h
                        if( i == 9 )
                          c = 29; // android-e
                        if( i == 10 )
                          c = 30; // android-fm
			/*for ( iClass = 0; iClass < MAX_PC_CLASS; iClass++ )*/
			//for ( iClass = 0; iClass < 9; iClass++ )
			for ( iClass = 0; iClass < 31; iClass++ )
			{
				if( iClass > 8 && iClass < 28 )
				  continue;
				if ( class_table[iClass]->who_name &&
				     class_table[iClass]->who_name[0] != '\0' )
				{
					//if ( i == iClass )
					if( c == iClass )
					{
						ch->class =  iClass;
						ch->race  =  iClass;
						break;
					}
				}
			}
		}
		else
		{
			//char letters[11] = "abcdefghij";
			char letters[14] = "abcdefghijklmn";
			for (i=0;i<14;i++)
			{
				if (arg[0] == letters[i] )
				{
				  int c = i;
				  if( i == 0 )
                          	    c = 0; // saian
                        	  if( i == 1 )
                          	    c = 1; // human
                        	  if( i == 2 )
                          	    c = 2; // halfbreed
                       		  if( i == 3 )
                          	    c = 3; // namek
                                  if( i == 4 )
                                    c = 5; // icer
                                  if( i == 5 )
                                    c = 6; // bio
                                  if( i == 6 )
                                    c = 7; // kaio
                                  if( i == 7 )
                                    c = 8; // demon
                                  if( i == 8 )
                                    c = 28; // android-h
                                  if( i == 9 )
                                    c = 29; // android-e
                                  if( i == 10 )
                                    c = 30; // android-fm
				  if(!str_cmp(class_table[c]->who_name,"android-h") )
				    sprintf(buf, "androidh");
				  else if(!str_cmp(class_table[c]->who_name,"android-e") )
				    sprintf(buf, "androide");
				  else if(!str_cmp(class_table[c]->who_name,"android-fm") )
				    sprintf(buf, "androidfm");
				  else
				    sprintf(buf, "%s", class_table[c]->who_name);
				  do_help(ch, buf);
				  return;
				}
			}
			i=0;
	send_to_desc_color( "\n\r&c==============================================================================&D",d);
			/*for ( iClass = 0; iClass < MAX_PC_CLASS; iClass++ )*/
			//for ( iClass = 0; iClass < 9; iClass++ )
			for ( iClass = 0; iClass < 31; iClass++ )
			{
			    if( iClass == 4 )
			    {
				//i++;
				continue;
			    }
			    if( iClass > 8 && iClass < 28 )
                                continue;
			    char letters[14] = "abcdefghijklmn";
			    if ( class_table[iClass]->who_name &&
			    	 class_table[iClass]->who_name[0] != '\0' )
			    {
			    sprintf( buf, "\n\r&w   (&W%2d&w)  &c%-12s&w  ('&R%c&w' for help)&D",
				i,
			    	class_table[iClass]->who_name, letters[i]);
					send_to_desc_color(buf, d);
			    i++;
			    }
			}
	send_to_desc_color( "\n\r&c==============================================================================&D",d);
			sprintf( buf, "\n\r&wChoose the number of your race: &D" );
			send_to_desc_color( buf, d );
			return;
		}

	/*if ( iClass == MAX_PC_CLASS */
	if( iClass != 28 && iClass != 29 && iClass != 30 )
	  if ( iClass > 8
	  ||  !class_table[iClass]->who_name
	  || class_table[iClass]->who_name[0] == '\0'
	  || !str_cmp(class_table[iClass]->who_name,"unused"))
	  {
	    send_to_desc_color( "&wThat's not a race.\n\rWhat IS your race? &D",d);
	    return;
	  }

        if ( check_bans( ch, BAN_CLASS ) )
        {
         send_to_desc_color (
             "&wThat race is not currently avaiable.\n\rWhat IS your race? &D",d);
         return;
        }

if (ch->race == 3 || ch->race == 5 || ch->race == 6)
{
ch->pcdata->haircolor = 24;
ch->pcdata->orignalhaircolor = 24;
}
else
{
send_to_desc_color( "\n\r&wPlease choose a hair color from the following list:&D\n\r", d );
		buf[0] = '\0';

		for ( iClass = 0; iClass < (MAX_HAIR - 2); iClass++ )
		{
			sprintf( buf2, "&w[&W%2d&w] &g%-18.18s  ", iClass, hair_color[iClass]);
			b++;
			strcat(buf,buf2);
			if ( (b % 3) == 0 )
				strcat( buf, "&D\n\r" );
		}
		strcat( buf, "\n\r: " );
		strcat( buf, "\r\r" );
		send_to_desc_color( buf, d );
	d->connected = CON_GET_NEW_HAIR;
	break;
}
	case CON_GET_NEW_HAIR:
		argument = one_argument(argument, arg);
if (ch->race != 3 && ch->race != 5 && ch->race != 6)
{
		if (!str_cmp( arg, "help") )
		{
			do_help(ch, argument);
			send_to_desc_color("&wPlease choose a hair color: &D", d);
			return;
		}
		for ( iClass = 0; iClass < (MAX_HAIR - 2); iClass++ )
		{
			if ( toupper(arg[0]) == toupper(hair_color[iClass][0])
				&&   !str_prefix( arg, hair_color[iClass] ) )
			{
				ch->pcdata->haircolor = iClass;
                                ch->pcdata->orignalhaircolor = iClass;
				break;
			}
			if (is_number(arg) && atoi(arg) == iClass)
			{
				ch->pcdata->haircolor = iClass;
                                ch->pcdata->orignalhaircolor = iClass;
				break;
			}
		}
		if ( iClass == (MAX_HAIR - 2) || !hair_color[iClass] || hair_color[iClass][0] == '\0' )
		{
			send_to_desc_color( "&wThat's not a hair color.\n\rWhat IS it going to be? &D", d );
			return;
		}
}
if (ch->race == 3 || ch->race == 6)
{
		send_to_desc_color("\n\r&wPlease choose a main body color from the following list:&D\n\r", d );
	buf[0] = '\0';
	buf2[0] = '\0';
	b = 0;
	for ( iClass = (MAX_COMPLEXION - 17); iClass < (MAX_COMPLEXION - 14); iClass++ )
	{
		sprintf( buf2, "&w[&W%2d&W] &g%-15s&D", iClass, complexion[iClass]);
		b++;
		strcat(buf,buf2);
		  if ( (b % 4) == 0 )
			  strcat( buf, "\n\r" );
	}
	strcat( buf, "\n\r: " );
	strcat( buf, "\r\r\r\r" );
	send_to_desc_color( buf, d );
	d->connected = CON_GET_NEW_COMPLEXION;
	break;
}

else if (ch->race == 5)
{
		send_to_desc_color( "\n\r&wPlease choose a main body color from the following list:&D\n\r", d );
	buf[0] = '\0';
	buf2[0] = '\0';
	b = 0;
	for ( iClass = (MAX_COMPLEXION - 14); iClass < (MAX_COMPLEXION); iClass++ )
	{
		sprintf( buf2, "&w[&W%2d&w] &g%-15s&D", iClass, complexion[iClass]);
		b++;
		strcat(buf,buf2);
		  if ( (b % 4) == 0 )
			  strcat( buf, "\n\r" );
	}
	strcat( buf, "\n\r: " );
	strcat( buf, "\r\r\r\r" );
	send_to_desc_color( buf, d );
	d->connected = CON_GET_NEW_COMPLEXION;
	break;
}
else
{
		send_to_desc_color( "\n\r&wPlease choose a complexion from the following list:&D\n\r", d );
	buf[0] = '\0';
	buf2[0] = '\0';
	b = 0;
	for ( iClass = 0; iClass < (MAX_COMPLEXION - 17); iClass++ )
	{
		sprintf( buf2, "&w[&W%2d&w] &g%-15s&D", iClass, complexion[iClass]);
		b++;
		strcat(buf,buf2);
		  if ( (b % 4) == 0 )
			  strcat( buf, "\n\r" );
	}
	strcat( buf, "\n\r: " );
	strcat( buf, "\r\r\r\r" );
	send_to_desc_color( buf, d );
	d->connected = CON_GET_NEW_COMPLEXION;
	break;
}
	case CON_GET_NEW_COMPLEXION:

		argument = one_argument(argument, arg);
if (ch->race == 5)
{
		if (!str_cmp( arg, "help") )
		{
			do_help(ch, argument);
			send_to_desc_color( "&wPlease choose a main body color: &D", d);
			return;
		}
	for ( iClass = (MAX_COMPLEXION - 14); iClass < (MAX_COMPLEXION); iClass++ )
		{
			if ( toupper(arg[0]) == toupper(complexion[iClass][0])
				&&   !str_prefix( arg, complexion[iClass] ) )
			{
				ch->pcdata->complexion = iClass;
				break;
			}
			if (is_number(arg) && atoi(arg) == iClass)
			{
				ch->pcdata->complexion = iClass;
				break;
			}
		}
		if ( iClass == (MAX_COMPLEXION) || !complexion[iClass] || complexion[iClass][0] == '\0' )
		{
			send_to_desc_color("&wThat's not a choice.\n\rWhat IS it going to be? &D", d );
			return;
		}
}
else if (ch->race == 3 || ch->race == 6)
{
		if (!str_cmp( arg, "help") )
		{
			do_help(ch, argument);
			send_to_desc_color( "&wPlease choose a main body color: &D", d);
			return;
		}
	for ( iClass = (MAX_COMPLEXION - 17); iClass < (MAX_COMPLEXION - 13); iClass++ )
		{
			if ( toupper(arg[0]) == toupper(complexion[iClass][0])
				&&   !str_prefix( arg, complexion[iClass] ) )
			{
				ch->pcdata->complexion = iClass;
				break;
			}
			if (is_number(arg) && atoi(arg) == iClass)
			{
				ch->pcdata->complexion = iClass;
				break;
			}
		}
		if ( iClass == (MAX_COMPLEXION - 14) || !complexion[iClass] || complexion[iClass][0] == '\0' )
		{
			send_to_desc_color( "&wThat's not a choice.\n\rWhat IS it going to be? &D", d );
			return;
		}
}
else
{
		if (!str_cmp( arg, "help") )
		{
			do_help(ch, argument);
			send_to_desc_color( "&wPlease choose complexion: &D", d);
			return;
		}
	for ( iClass = 0; iClass < (MAX_COMPLEXION - 17); iClass++ )
		{
			if ( toupper(arg[0]) == toupper(complexion[iClass][0])
				&&   !str_prefix( arg, complexion[iClass] ) )
			{
				ch->pcdata->complexion = iClass;
				break;
			}
			if (is_number(arg) && atoi(arg) == iClass)
			{
				ch->pcdata->complexion = iClass;
				break;
			}
		}
		if ( iClass == (MAX_COMPLEXION - 17) || !complexion[iClass] || complexion[iClass][0] == '\0' )
		{
			send_to_desc_color("&wThat's not a complexion.\n\rWhat IS it going to be? &D", d );
			return;
		}
}

/* ------------------------------------------------------------------------------------- */
if (ch->race == 5)
{
		send_to_desc_color("\n\r&wPlease choose a secondary body color from the following list:&D\n\r", d );
	buf[0] = '\0';
	buf2[0] = '\0';
	b = 0;
	for ( iClass = 0; iClass < (MAX_SECONDARYCOLOR - 1); iClass++ )
	{
		sprintf( buf2, "&w[&W%2d&w] &g%-15s&D", iClass, secondary_color[iClass]);
		b++;
		strcat(buf,buf2);
		  if ( (b % 4) == 0 )
			  strcat( buf, "\n\r" );
	}
	strcat( buf, "\n\r: " );
	strcat( buf, "\r\r\r\r" );
	send_to_desc_color( buf, d );
	d->connected = CON_GET_NEW_SECONDARYCOLOR;
	break;

	case CON_GET_NEW_SECONDARYCOLOR:
	/* Black, Brown, Red, Blonde, Strawberry Blonde, Argent, Golden Blonde, Platinum Blonde, Light Brown*/
		argument = one_argument(argument, arg);
		if (!str_cmp( arg, "help") )
		{
			do_help(ch, argument);
			send_to_desc_color( "&wPlease choose a secondary body color: &D", d);
			return;
		}
		for ( iClass = 0; iClass < (MAX_SECONDARYCOLOR - 1); iClass++ )
		{
			if ( toupper(arg[0]) == toupper(secondary_color[iClass][0])
				&&   !str_prefix( arg, secondary_color[iClass] ) )
			{
				ch->pcdata->secondarycolor = iClass;
				break;
			}
			if (is_number(arg) && atoi(arg) == iClass)
			{
				ch->pcdata->secondarycolor = iClass;
				break;
			}
		}
		if ( iClass == (MAX_SECONDARYCOLOR - 1) || !secondary_color[iClass] || secondary_color[iClass][0] == '\0' )
		{
			send_to_desc_color( "&wThat's not a choice.\n\rWhat IS it going to be? &D", d );
			return;
		}
}
/* ------------------------------------------------------------------------------------- */

		send_to_desc_color( "\n\r&wPlease choose a eye color from the following list:&D\n\r", d );
	buf[0] = '\0';
	buf2[0] = '\0';
	b = 0;

	for ( iClass = 0; iClass < (MAX_EYE - 3); iClass++ )
	{
		sprintf( buf2, "&w[&W%2d&w] &g%-15s&D", iClass, eye_color[iClass]);
		b++;
		strcat(buf,buf2);
		  if ( (b % 4) == 0 )
			  strcat( buf, "\n\r" );
	}
	strcat( buf, "\n\r: " );
	strcat( buf, "\r\r\r\r" );
	send_to_desc_color(buf, d );
	d->connected = CON_GET_NEW_EYE;

	break;
	case CON_GET_NEW_EYE:
	/* Black, Brown, Red, Blonde, Strawberry Blonde, Argent, Golden Blonde, Platinum Blonde, Light Brown*/
		argument = one_argument(argument, arg);
		if (!str_cmp( arg, "help") )
		{
			do_help(ch, argument);
			send_to_desc_color( "&wPlease choose a hair color: &D", d);
			return;
		}
		for ( iClass = 0; iClass < (MAX_EYE - 3); iClass++ )
		{
			if ( toupper(arg[0]) == toupper(eye_color[iClass][0])
				&&   !str_prefix( arg, eye_color[iClass] ) )
			{
				ch->pcdata->eyes = iClass;
                                ch->pcdata->orignaleyes = iClass;
				break;
			}
			if (is_number(arg) && atoi(arg) == iClass)
			{
				ch->pcdata->eyes = iClass;
                                ch->pcdata->orignaleyes = iClass;
				break;
			}
		}
		if ( iClass == (MAX_EYE - 3) || !eye_color[iClass] || eye_color[iClass][0] == '\0' )
		{
			send_to_desc_color( "&wThat's not a eye color.\n\rWhat IS it going to be? &D", d );
			return;
		}
/* ------------------------------------------------------------------------------------- */
		send_to_desc_color( "\n\r&wPlease choose a build type from the following list:&D\n\r", d );
	buf[0] = '\0';
	buf2[0] = '\0';
	b = 0;

	for ( iClass = 0; iClass < (MAX_BUILD); iClass++ )
	{
		sprintf( buf2, "&w[&W%2d&w] &g%-15s&D", iClass, build_type[iClass]);
		b++;
		strcat(buf,buf2);
		  if ( (b % 4) == 0 )
			  strcat( buf, "\n\r" );
	}
	strcat( buf, "\n\r: " );
	strcat( buf, "\r\r\r\r" );
	send_to_desc_color( buf, d );
	d->connected = CON_GET_NEW_BUILD;

	break;
	case CON_GET_NEW_BUILD:
	/* Black, Brown, Red, Blonde, Strawberry Blonde, Argent, Golden Blonde, Platinum Blonde, Light Brown*/
		argument = one_argument(argument, arg);
		if (!str_cmp( arg, "help") )
		{
			do_help(ch, argument);
			send_to_desc_color( "&wPlease choose a build type: &D", d);
			return;
		}
		for ( iClass = 0; iClass < (MAX_BUILD); iClass++ )
		{
			if ( toupper(arg[0]) == toupper(build_type[iClass][0])
				&&   !str_prefix( arg, build_type[iClass] ) )
			{
				ch->pcdata->build = iClass;
				break;
			}
			if (is_number(arg) && atoi(arg) == iClass)
			{
				ch->pcdata->build = iClass;
				break;
			}
		}
		if ( iClass == (MAX_BUILD) || !build_type[iClass] || build_type[iClass][0] == '\0' )
		{
			send_to_desc_color( "&wThat's not a build type.\n\rWhat IS it going to be? &D", d );
			return;
		}



/*	send_to_desc_color( "\n\rWould you like RIP, ANSI or no graphic/color support, (R/A/N)? ", d );
	d->connected = CON_GET_WANT_RIPANSI;
        break;

    case CON_GET_WANT_RIPANSI:
	switch ( argument[0] )
	{
	case 'r': case 'R':
	    xSET_BIT(ch->act,PLR_RIP);
	    xSET_BIT(ch->act,PLR_ANSI);
	    break;
	case 'a': case 'A': xSET_BIT(ch->act,PLR_ANSI);  break;
	case 'n': case 'N': break;
	default:
	    write_to_buffer( d, "Invalid selection.\n\rRIP, ANSI or NONE? ", 0 );
	    return;
	}
*/
	sprintf( log_buf, "%s@%s new %s %s.", ch->name, d->host,
		race_table[ch->race]->race_name,
		class_table[ch->class]->who_name );
	log_string_plus( log_buf, LOG_COMM, sysdata.log_level);
	to_channel( log_buf, CHANNEL_MONITOR, "Monitor", LEVEL_IMMORTAL );
	send_to_desc_color( "&wPress [&RENTER&w] &D", d );
	show_title(d);
	ch->level = 0;
	ch->position = POS_STANDING;
	d->connected = CON_PRESS_ENTER;
	set_pager_color( AT_PLAIN, ch );
	adjust_hiscore( "pkill", ch, ch->pcdata->pkills ); /* cronel hiscore */
	adjust_hiscore( "sparwins", ch, ch->pcdata->spar_wins );
	adjust_hiscore( "sparloss", ch, ch->pcdata->spar_loss );
	adjust_hiscore( "mkills", ch, ch->pcdata->mkills );
	adjust_hiscore( "deaths", ch, (ch->pcdata->pdeaths + ch->pcdata->mdeaths) );
	update_plHiscore(ch);
	adjust_hiscore( "played", ch, ((get_age(ch) - 4) * 2) );
	adjust_hiscore( "zeni", ch, ch->gold );
	return;
	break;

    case CON_PRESS_ENTER:
        if ( chk_watch(get_trust(ch), ch->name, d->host) ) /*  --Gorog */
           SET_BIT( ch->pcdata->flags, PCFLAG_WATCH );
        else
           REMOVE_BIT( ch->pcdata->flags, PCFLAG_WATCH );

	if ( ch->position == POS_MOUNTED )
		ch->position = POS_STANDING;

	set_pager_color( AT_PLAIN, ch );
	if ( xIS_SET(ch->act, PLR_RIP) )
	  send_rip_screen(ch);
	if ( xIS_SET(ch->act, PLR_ANSI) )
	  send_to_pager( "\033[2J", ch );
	else
	  send_to_pager( "\014", ch );
	if ( IS_IMMORTAL(ch) )
	  do_help( ch, "imotd" );
	if ( ch->level == 50)
	  do_help( ch, "amotd" );
	if ( ch->level < 50 && ch->level > 0 )
	  do_help( ch, "motd" );
	if ( ch->level == 0 )
	  do_help( ch, "nmotd" );
	send_to_pager( "\n\rPress [ENTER] ", ch );
        d->connected = CON_READ_MOTD;
        break;

    case CON_READ_MOTD:
	{
	  char motdbuf[MAX_STRING_LENGTH];

	  sprintf( motdbuf, "\n\rWelcome to %s...\n\r", sysdata.mud_name);
	  send_to_desc_color( motdbuf, d );
	}
	add_char( ch );
	d->connected	= CON_PLAYING;
      /* hopefully clear up some ansi changing issues	  -Nopey */
	      set_char_color( AT_DGREEN, ch );
	if( !xIS_SET(ch->act, PLR_ANSI) && d->ansi == TRUE )
	    d->ansi = FALSE;
	else if( xIS_SET(ch->act, PLR_ANSI) && d->ansi == FALSE)
	    d->ansi = TRUE;

	if ( ch->level == 0 )
	{
	    OBJ_DATA *obj;
	    int iLang;

		ch->pcdata->upgradeL = CURRENT_UPGRADE_LEVEL;

	    ch->pcdata->clan_name = STRALLOC( "" );
	    ch->pcdata->clan	  = NULL;
	    switch ( class_table[ch->class]->attr_prime )
	    {
	    case APPLY_STR: ch->perm_str = 10; break;
	    case APPLY_INT: ch->perm_int = 10; break;
	    case APPLY_DEX: ch->perm_dex = 10; break;
	    case APPLY_CON: ch->perm_con = 10; break;
	    case APPLY_LCK: ch->perm_lck = 10; break;
	    }

	    ch->perm_str	 += race_table[ch->race]->str_plus;
	    ch->perm_int	 += race_table[ch->race]->int_plus;
	    ch->perm_dex	 += race_table[ch->race]->dex_plus;
	    ch->perm_con	 += race_table[ch->race]->con_plus;
	    ch->affected_by	  = race_table[ch->race]->affected;
	    ch->perm_lck	 = number_range(0, 30);

		ch->pcdata->permTstr = ch->perm_str;
		ch->pcdata->permTspd = ch->perm_dex;
		ch->pcdata->permTint = ch->perm_int;
		ch->pcdata->permTcon = ch->perm_con;

        ch->armor		 += race_table[ch->race]->ac_plus;
        ch->alignment	 += race_table[ch->race]->alignment;
        ch->attacks              = race_table[ch->race]->attacks;
        ch->defenses             = race_table[ch->race]->defenses;
	    ch->saving_poison_death  	= race_table[ch->race]->saving_poison_death;
	    ch->saving_wand  		= race_table[ch->race]->saving_wand;
	    ch->saving_para_petri  	= race_table[ch->race]->saving_para_petri;
	    ch->saving_breath  		= race_table[ch->race]->saving_breath;
	    ch->saving_spell_staff	= race_table[ch->race]->saving_spell_staff;

	    ch->height = number_range(race_table[ch->race]->height *.9, race_table[ch->race]->height *1.1);
	    ch->weight = number_range(race_table[ch->race]->weight *.9, race_table[ch->race]->weight *1.1);

	    if ( (iLang = skill_lookup( "common" )) < 0 )
	    	bug( "Nanny: cannot find common language." );
	    else
	    	ch->pcdata->learned[iLang] = 100;

	    for ( iLang = 0; lang_array[iLang] != LANG_UNKNOWN; iLang++ )
	    	if ( lang_array[iLang] == race_table[ch->race]->language )
	    		break;
	    if ( lang_array[iLang] == LANG_UNKNOWN )
		;
	    	/*bug( "Nanny: invalid racial language." );*/
	    else
	    {
	    	if ( (iLang = skill_lookup( lang_names[iLang] )) < 0 )
	    		bug( "Nanny: cannot find racial language." );
	    	else
	    		ch->pcdata->learned[iLang] = 100;
	    }

            /* ch->resist           += race_table[ch->race]->resist;    drats */
            /* ch->susceptible     += race_table[ch->race]->suscept;    drats */

	    name_stamp_stats( ch );

	    ch->level	= 1;
	    ch->exp	= 100;
		ch->pl	= 100;
		ch->heart_pl	= 100;
            ch->max_hit    += race_table[ch->race]->hit;
            ch->max_mana   += race_table[ch->race]->mana;
	    	ch->max_move    = 100;
	    ch->hit	= UMAX(1,ch->max_hit);
	    ch->mana	= UMAX(1,ch->max_mana);
	    ch->move	= UMAX(1,ch->max_move);
	    ch->train = 5;
	    ch->max_train = 1;
		ch->pcdata->xTrain = 0;
		ch->pcdata->total_xTrain = 0;
	    ch->practice = 0;
	    ch->max_prac = 0;
	    ch->max_energy = 1;
		ch->pcdata->admintalk = 0;
		ch->pcdata->age = 18;
		ch->pcdata->sparcount = 0;
		if (is_saiyan(ch) || is_hb(ch) || is_icer(ch) || is_bio(ch))
			ch->pcdata->tail = TRUE;
		else
			ch->pcdata->tail = FALSE;
	    if (is_android(ch))
	    	ch->pcdata->natural_ac_max = 500;
	    if (is_bio(ch))
	    	ch->pcdata->absorb_pl_mod = 6;

	    /* To make it so that saiyans and halfies start out
	       with the Oozaru mouth cannon. -Karma
	     */
	    if( is_saiyan(ch) || is_hb(ch) )
		ch->pcdata->learned[gsn_monkey_gun] = 95;

	    sprintf( buf, "the %s",
		title_table [ch->class] [ch->level]
		[ch->sex == SEX_FEMALE ? 1 : 0] );
	    set_title( ch, buf );
		ch->pcdata->creation_date = current_time;

            /* Added by Narn.  Start new characters with autoexit and autgold
               already turned on.  Very few people don't use those. */
	    xSET_BIT( ch->act, PLR_AUTOGOLD );
	    xSET_BIT( ch->act, PLR_AUTOEXIT );
		xSET_BIT( ch->act, PLR_AUTO_COMPASS );
	    xSET_BIT( ch->act, PLR_SPAR );
	     SET_BIT( ch->pcdata->flags, PCFLAG_DEADLY);
		xSET_BIT(ch->deaf, CHANNEL_FOS);


    	/* Don't display old notes as 'unread' except for the announcment board */
        for (i = 1; i < MAX_BOARD; i++)
        {
		    for (catchup_notes = ch->pcdata->board->note_first; catchup_notes && catchup_notes->next; catchup_notes = catchup_notes->next);

		    if (!catchup_notes)
    	    ;
		    else
		    {
	        ch->pcdata->last_note[i] = catchup_notes->date_stamp;
		    }
	    }

            /* Added by Brittany, Nov 24/96.  The object is the adventurer's guide
               to the realms of despair, part of Academy.are. */
            {
            OBJ_INDEX_DATA *obj_ind = get_obj_index( 10333 );
            if ( obj_ind != NULL )
            {
              obj = create_object( obj_ind, 0 );
              obj_to_char( obj, ch );
              equip_char( ch, obj, WEAR_HOLD );
            }
            }
	    if (!sysdata.WAIT_FOR_AUTH)
	      char_to_room( ch, get_room_index( ROOM_VNUM_SCHOOL ) );
	    else
	    {
	      char_to_room( ch, get_room_index( ROOM_AUTH_START ) );
	      ch->pcdata->auth_state = 1;
	      SET_BIT(ch->pcdata->flags, PCFLAG_UNAUTHED);
	      add_to_auth( ch ); /* new auth */
	    }
	    /* Display_prompt interprets blank as default */
	    ch->pcdata->prompt = STRALLOC("");

            /* Attempt to make "stealing" easier to catch */
            // Put in code to write Created IP to file
//            sprintf( ipBuf, "%s%s", PLAYFROM_DIR, capitalize( ch->name ) );
//            if( !( ipFp = fopen( ipBuf, "a" ) ) )
//            {
//              bug( "Write playfrom: Cannot open file.", 0 );
//              perror( ipBuf );
//            }
//            else
//            {
//              fprintf( ipFp, "CREATED: %s\n\r", ch->desc->host );
//              fclose( ipFp );
//            }
	}
	else
	if ( !IS_IMMORTAL(ch) && ch->pcdata->release_date > 0 &&
		ch->pcdata->release_date > current_time )
	{
	    if ( ch->in_room->vnum == 6
	    ||   ch->in_room->vnum == 8
	    ||   ch->in_room->vnum == 1206 )
		char_to_room( ch, ch->in_room );
	    else
	      char_to_room( ch, get_room_index(8) );
	}
        else
        if( !IS_IMMORTAL( ch ) && ch->in_room
         && ch->in_room->vnum == 2060 )
        {
          act( AT_GREEN, "A Strange Force rips you from the Hyperbolic Time Chamber.", ch, NULL, NULL, TO_CHAR );
          char_to_room( ch, get_room_index( 2059 ) );
        }
	else
	if ( ch->in_room && ( IS_IMMORTAL( ch )
             || !xIS_SET( ch->in_room->room_flags, ROOM_PROTOTYPE ) ) )
	{
	    char_to_room( ch, ch->in_room );
	}
	else
	if ( IS_IMMORTAL(ch) )
	{
	    char_to_room( ch, get_room_index( ROOM_VNUM_CHAT ) );
	}
	else
	{
	    char_to_room( ch, get_room_index( ROOM_VNUM_TEMPLE ) );
	}

        /* Attempt to make "stealing" easier to catch */
        // Put in code to write Connected IP to file
//        sprintf( ipBuf, "%s%s", PLAYFROM_DIR, capitalize( ch->name ) );
//        if( !( ipFp = fopen( ipBuf, "a" ) ) )
//        {
//          bug( "Write playfrom: Cannot open file.", 0 );
//          perror( ipBuf );
//        }
//        else
//        {
//          fprintf( ipFp, "%s\n\r", ch->desc->host );
//          fclose( ipFp );
//        }

/*
 *    if ( get_timer( ch, TIMER_SHOVEDRAG ) > 0 )
 *        remove_timer( ch, TIMER_SHOVEDRAG );
 *
 *    if ( get_timer( ch, TIMER_PKILLED ) > 0 )
 *	remove_timer( ch, TIMER_PKILLED );
 */

    act( AT_ACTION, "$n has entered the game.", ch, NULL, NULL, TO_CANSEE );
    if ( ch->pcdata->pet )
    {
           act( AT_ACTION, "$n returns to $s master from the Void.",
                      ch->pcdata->pet, NULL, ch, TO_NOTVICT );
           act( AT_ACTION, "$N returns with you to the realms.",
                        ch, NULL, ch->pcdata->pet, TO_CHAR );
    }
    ch->tmystic = 0;
    ch->mysticlearn = 0;
    ch->teaching = NULL;
    if( is_kaio(ch) && ch->alignment < 0 )
	ch->alignment = 0;
    if( is_demon(ch) && ch->alignment > 0 )
        ch->alignment = 0;

    remove_member(ch);
    if( ch->pcdata->clan )
	update_member(ch);

    /* For the logon pl tracker */
    ch->logon_start = ch->exp;


/* -Temporarily removed due to this possibly being bugged -Karma
    if( ch->kairank > 0 )
    {
        int a = 0;
        bool found = FALSE;
        for( a = 0; a < 6; a++ )
          if( !str_cmp(kaioshin[a],ch->name) )
            found = TRUE;
	if( !found )
        {
          bug("%s has a kaio rank but is not on the global list. Removing rank.",ch->name);
          ch_printf(ch,"&RYou have a kaio rank, but are not on the global list. Removing kaio rank.\n\r");
          ch->kairank = 0;
        }
	found = FALSE;
    }

    if( ch->demonrank > 0 )
    {
	int a = 0;
	bool found = FALSE;
	for( a = 0; a < 6; a++ )
	  if( !str_cmp(greaterdemon[a],ch->name) )
	    found = TRUE;
	if( !found )
	  for( a = 0; a < 3; a++ )
            if( !str_cmp(demonwarlord[a],ch->name) )
              found = TRUE;
	if( !found )
	  if( !str_cmp(demonking,ch->name) )
	    found = TRUE;
	if( !found )
	{
	  bug("%s has demon rank but is not on the global list. Removing rank.",ch->name);
	  ch_printf(ch,"&RYou have a demon rank, but are not on the global list. Removing demon rank.\n\r");
	  ch->demonrank = 0;
	}
	found = FALSE;
    }
*/

    do_global_boards( ch, "" );


    ch->dodge = FALSE;
    ch->block = FALSE;
    ch->ki_dodge = FALSE;
    ch->ki_cancel = FALSE;
    ch->ki_deflect = FALSE;

    do_look( ch, "auto" );
    tax_player(ch);  /* Here we go, let's tax players to lower the gold
			pool -- TRI */
	mccp_interest(ch);
    mail_count(ch);
    check_loginmsg(ch);

    check_auth_state( ch ); /* new auth */
    if ( !ch->was_in_room && ch->in_room == get_room_index( ROOM_VNUM_TEMPLE ))
      	ch->was_in_room = get_room_index( ROOM_VNUM_TEMPLE );
    else if ( ch->was_in_room == get_room_index( ROOM_VNUM_TEMPLE ))
        ch->was_in_room = get_room_index( ROOM_VNUM_TEMPLE );
    else if ( !ch->was_in_room )
    	ch->was_in_room = ch->in_room;

    break;

    case CON_NOTE_TO:
        handle_con_note_to (d, argument);
        break;

    case CON_NOTE_SUBJECT:
        handle_con_note_subject (d, argument);
        break; /* subject */

    case CON_NOTE_EXPIRE:
        handle_con_note_expire (d, argument);
        break;

    case CON_NOTE_TEXT:
        handle_con_note_text (d, argument);
        break;
    case CON_NOTE_FINISH:
        handle_con_note_finish (d, argument);
        break;

    }

    return;
}

bool is_reserved_name( char *name )
{
  RESERVE_DATA *res;

  for (res = first_reserved; res; res = res->next)
    if ((*res->name == '*' && !str_infix(res->name+1, name)) ||
        !str_cmp(res->name, name))
      return TRUE;
  return FALSE;
}


/*
 * Parse a name for acceptability.
 */
bool check_parse_name( char *name, bool newchar )
{
 /*
  * Names checking should really only be done on new characters, otherwise
  * we could end up with people who can't access their characters.  Would
  * have also provided for that new area havoc mentioned below, while still
  * disallowing current area mobnames.  I personally think that if we can
  * have more than one mob with the same keyword, then may as well have
  * players too though, so I don't mind that removal.  -- Alty
  */

     if ( is_reserved_name(name) && newchar )
	return FALSE;

     /*
      * Outdated stuff -- Alty
      */
/*     if ( is_name( name, "all auto immortal self someone god supreme demigod dog guard cityguard cat cornholio spock hicaine hithoric death ass fuck shit piss crap quit" ) )
       return FALSE;*/

    /*
     * Length restrictions.
     */
    if ( strlen(name) <  3 )
	return FALSE;

    if ( strlen(name) > 12 )
	return FALSE;

    /*
     * Alphanumerics only.
     * Lock out IllIll twits.
     */
    {
	char *pc;
	bool fIll;

	fIll = TRUE;
	for ( pc = name; *pc != '\0'; pc++ )
	{
	    if ( !isalpha(*pc) )
		return FALSE;
	    if ( LOWER(*pc) != 'i' && LOWER(*pc) != 'l' )
		fIll = FALSE;
	}

	if ( fIll )
	    return FALSE;
    }

    /*
     * Code that followed here used to prevent players from naming
     * themselves after mobs... this caused much havoc when new areas
     * would go in...
     */

    return TRUE;
}



/*
 * Look for link-dead player to reconnect.
 */
bool check_reconnect( DESCRIPTOR_DATA *d, char *name, bool fConn )
{
    CHAR_DATA *ch;

    for ( ch = first_char; ch; ch = ch->next )
    {
	if ( !IS_NPC(ch)
	&& ( !fConn || !ch->desc )
	&&    ch->pcdata->filename
	&&   !str_cmp( name, ch->pcdata->filename ) )
	{
	    if ( fConn && ch->switched )
	    {
	      write_to_buffer( d, "Already playing.\n\rName: ", 0 );
	      d->connected = CON_GET_NAME;
	      if ( d->character )
	      {
		 /* clear descriptor pointer to get rid of bug message in log */
		 d->character->desc = NULL;
		 free_char( d->character );
		 d->character = NULL;
	      }
	      return BERR;
	    }
	    if ( fConn == FALSE )
	    {
		DISPOSE( d->character->pcdata->pwd );
		d->character->pcdata->pwd = str_dup( ch->pcdata->pwd );
	    }
	    else
	    {
		/* clear descriptor pointer to get rid of bug message in log */
		d->character->desc = NULL;
		free_char( d->character );
		d->character = ch;
		ch->desc	 = d;
		ch->timer	 = 0;
		send_to_char( "Reconnecting.\n\r", ch );
		do_look( ch, "auto" );
		act( AT_ACTION, "$n has reconnected.", ch, NULL, NULL, TO_CANSEE );
		sprintf( log_buf, "%s@%s(%s) reconnected.",
				ch->pcdata->filename, d->host, d->user );
		log_string_plus( log_buf, LOG_COMM, UMAX( sysdata.log_level, ch->level ) );
/*
		if ( ch->level < LEVEL_SAVIOR )
		  to_channel( log_buf, CHANNEL_MONITOR, "Monitor", ch->level );
*/
		d->connected = CON_PLAYING;
		check_auth_state( ch ); /* Link dead support -- Rantic */
                /* Inform the character of a note in progress and the possbility of continuation! */
                if (ch->pcdata->in_progress)
                    send_to_char ("You have a note in progress. Type \"note write\" to continue it.\n\r",ch);

            }
	    return TRUE;
	}
    }

    return FALSE;
}



/*
 * Check if already playing.
 */
bool check_playing( DESCRIPTOR_DATA *d, char *name, bool kick )
{
    CHAR_DATA *ch;

    DESCRIPTOR_DATA *dold;
    int	cstate;

    for ( dold = first_descriptor; dold; dold = dold->next )
    {
	if ( dold != d
	&& (  dold->character || dold->original )
	&&   !str_cmp( name, dold->original
		 ? dold->original->pcdata->filename :
		 dold->character->pcdata->filename ) )
	{
	    cstate = dold->connected;
	    ch = dold->original ? dold->original : dold->character;
	    if ( !ch->name
	    || ( cstate != CON_PLAYING && cstate != CON_EDITING ))
	    {
		write_to_buffer( d, "Already connected - try again.\n\r", 0 );
		sprintf( log_buf, "%s already connected.",
				ch->pcdata->filename );
		log_string_plus( log_buf, LOG_COMM, sysdata.log_level );
		return BERR;
	    }
	    if ( !kick )
	      return TRUE;
	    write_to_buffer( d, "Already playing... Kicking off old connection.\n\r", 0 );
	    write_to_buffer( dold, "Kicking off old connection... bye!\n\r", 0 );
	    close_socket( dold, FALSE );
	    /* clear descriptor pointer to get rid of bug message in log */
	    d->character->desc = NULL;
	    free_char( d->character );
	    d->character = ch;
	    ch->desc	 = d;
	    ch->timer	 = 0;
	    if ( ch->switched )
	      do_return( ch->switched, "" );
	    ch->switched = NULL;
	    send_to_char( "Reconnecting.\n\r", ch );
	    do_look( ch, "auto" );
	    act( AT_ACTION, "$n has reconnected, kicking off old link.",
	         ch, NULL, NULL, TO_CANSEE );
	    sprintf( log_buf, "%s@%s reconnected, kicking off old link.",
	             ch->pcdata->filename, d->host );
	    log_string_plus( log_buf, LOG_COMM, UMAX( sysdata.log_level, ch->level ) );
/*
	    if ( ch->level < LEVEL_SAVIOR )
	      to_channel( log_buf, CHANNEL_MONITOR, "Monitor", ch->level );
*/
	    d->connected = cstate;
	    return TRUE;
	}
    }

    return FALSE;
}



void stop_idling( CHAR_DATA *ch )
{
    ROOM_INDEX_DATA *was_in_room;

    /*
    if ( !ch
    ||   !ch->desc
    ||    ch->desc->connected != CON_PLAYING
    ||   !ch->was_in_room
    ||    ch->in_room != get_room_index( ROOM_VNUM_LIMBO ) )
	return;
	*/

    if ( !ch
    ||   !ch->desc
    ||   ch->desc->connected != CON_PLAYING
    ||   !IS_IDLE(ch) )
    	return;

    /*
    if ( IS_IMMORTAL(ch) )
    	return;
    */

    ch->timer = 0;
    was_in_room = ch->was_in_room;
    char_from_room( ch );
    char_to_room( ch, was_in_room );
    ch->was_in_room = ch->in_room;
    /*
    ch->was_in_room	= NULL;
    */
    REMOVE_BIT(ch->pcdata->flags, PCFLAG_IDLE );
    act( AT_ACTION, "$n has returned from the void.", ch, NULL, NULL, TO_ROOM );
    return;
}



/*
 * Write to one char. (update for color -Goku)
 */
void send_to_char( const char *txt, CHAR_DATA *ch )
{
    if ( !ch )
    {
      bug( "Send_to_char: NULL *ch" );
      return;
    }
   send_to_char_color( txt, ch );
   return;
}

/*
 * Same as above, but converts &color codes to ANSI sequences..
 */
void send_to_char_color( const char *txt, CHAR_DATA *ch )
{
  DESCRIPTOR_DATA *d;
  char *colstr;
  const char *prevstr = txt;
  char colbuf[20];
  int ln;

  if ( !ch )
  {
    bug( "Send_to_char_color: NULL *ch" );
    return;
  }
  if ( !txt || !ch->desc )
    return;
  d = ch->desc;

	switch (sysdata.outBytesFlag)
    {
		default:
			sysdata.outBytesOther += strlen(txt);
			break;

		case LOGBOUTCHANNEL:
			sysdata.outBytesChannel += strlen(txt);
			break;

		case LOGBOUTCOMBAT:
			sysdata.outBytesCombat += strlen(txt);
			break;

		case LOGBOUTMOVEMENT:
			sysdata.outBytesMovement += strlen(txt);
			break;

		case LOGBOUTINFORMATION:
			sysdata.outBytesInformation += strlen(txt);
			break;

		case LOGBOUTPROMPT:
			sysdata.outBytesPrompt += strlen(txt);
			break;

		case LOGBOUTINFOCHANNEL:
			sysdata.outBytesInfoChannel += strlen(txt);
			break;
	}
  /* Clear out old color stuff */
/*  make_color_sequence(NULL, NULL, NULL);*/
  while ( (colstr = strpbrk(prevstr, "&^}")) != NULL )
  {
    if (colstr > prevstr)
      write_to_buffer(d, prevstr, (colstr-prevstr));
    ln = make_color_sequence(colstr, colbuf, d);
    if ( ln < 0 )
    {
      prevstr = colstr+1;
      break;
    }
    else if ( ln > 0 )
      write_to_buffer(d, colbuf, ln);
    prevstr = colstr+2;
  }
  if ( *prevstr )
    write_to_buffer(d, prevstr, 0);
  return;
}

void write_to_pager( DESCRIPTOR_DATA *d, const char *txt, int length )
{
  int pageroffset;	/* Pager fix by thoric */

  if ( length <= 0 )
    length = strlen(txt);
  if ( length == 0 )
    return;
  if ( !d->pagebuf )
  {
    d->pagesize = MAX_STRING_LENGTH;
    CREATE( d->pagebuf, char, d->pagesize );
  }
  if ( !d->pagepoint )
  {
    d->pagepoint = d->pagebuf;
    d->pagetop = 0;
    d->pagecmd = '\0';
  }
  if ( d->pagetop == 0 && !d->fcommand )
  {
    d->pagebuf[0] = '\n';
    d->pagebuf[1] = '\r';
    d->pagetop = 2;
  }
  pageroffset = d->pagepoint - d->pagebuf;	/* pager fix (goofup fixed 08/21/97) */
  while ( d->pagetop + length >= d->pagesize )
  {
    if ( d->pagesize > 32000 )
    {
      bug( "Pager overflow.  Ignoring.\n\r" );
      d->pagetop = 0;
      d->pagepoint = NULL;
      DISPOSE(d->pagebuf);
      d->pagesize = MAX_STRING_LENGTH;
      return;
    }
    d->pagesize *= 2;
    RECREATE(d->pagebuf, char, d->pagesize);
  }
  d->pagepoint = d->pagebuf + pageroffset;	/* pager fix (goofup fixed 08/21/97) */

  strncpy(d->pagebuf+d->pagetop, txt, length);
  d->pagetop += length;
  d->pagebuf[d->pagetop] = '\0';
  return;
}

void send_to_pager( const char *txt, CHAR_DATA *ch )
{
  if ( !ch )
  {
    bug( "Send_to_pager: NULL *ch" );
    return;
  }
   send_to_pager_color( txt, ch );
   return;
}

void send_to_pager_color( const char *txt, CHAR_DATA *ch )
{
  DESCRIPTOR_DATA *d;
  char *colstr;
  const char *prevstr = txt;
  char colbuf[20];
  int ln;

  if ( !ch )
  {
    bug( "Send_to_pager_color: NULL *ch" );
    return;
  }
  if ( !txt || !ch->desc )
    return;
  d = ch->desc;
  ch = d->original ? d->original : d->character;
  if ( IS_NPC(ch) || !IS_SET(ch->pcdata->flags, PCFLAG_PAGERON) )
  {
    send_to_char_color(txt, d->character);
    return;
  }
	switch (sysdata.outBytesFlag)
    {
		default:
			sysdata.outBytesOther += strlen(txt);
			break;

		case LOGBOUTCHANNEL:
			sysdata.outBytesChannel += strlen(txt);
			break;

		case LOGBOUTCOMBAT:
			sysdata.outBytesCombat += strlen(txt);
			break;

		case LOGBOUTMOVEMENT:
			sysdata.outBytesMovement += strlen(txt);
			break;

		case LOGBOUTINFORMATION:
			sysdata.outBytesInformation += strlen(txt);
			break;

		case LOGBOUTPROMPT:
			sysdata.outBytesPrompt += strlen(txt);
			break;

		case LOGBOUTINFOCHANNEL:
			sysdata.outBytesInfoChannel += strlen(txt);
			break;
	}

  /* Clear out old color stuff */
/*  make_color_sequence(NULL, NULL, NULL);*/
  while ( (colstr = strpbrk(prevstr, "&^}")) != NULL )
  {
    if ( colstr > prevstr )
      write_to_pager(d, prevstr, (colstr-prevstr));
    ln = make_color_sequence(colstr, colbuf, d);
    if ( ln < 0 )
    {
      prevstr = colstr+1;
      break;
    }
    else if ( ln > 0 )
      write_to_pager(d, colbuf, ln);
    prevstr = colstr+2;
  }
  if ( *prevstr )
    write_to_pager(d, prevstr, 0);
  return;
}

sh_int figure_color( sh_int AType, CHAR_DATA *ch )
{
  int at = AType;
  if (at >= AT_COLORBASE && at < AT_TOPCOLOR)
  {
    at -= AT_COLORBASE;
    if (IS_NPC(ch) || ch->pcdata->colorize[at] == -1)
      at = at_color_table[at].def_color;
    else
      at = ch->pcdata->colorize[at];
  }
  if (at < 0 || at > AT_WHITE+AT_BLINK)
  {
    //bug("Figure_color: color %d invalid.", at);
    at = AT_GREY;
  }
  return at;
}

void set_char_color( sh_int AType, CHAR_DATA *ch )
{
    char buf[16];
    CHAR_DATA *och;

    if ( !ch || !ch->desc )
      return;

    och = (ch->desc->original ? ch->desc->original : ch);
    if ( !IS_NPC(och) && xIS_SET(och->act, PLR_ANSI) )
    {
	AType = figure_color(AType, och);
	if ( AType == 7 )
	  strcpy( buf, "\033[0;37m" );
	else
	  sprintf(buf, "\033[0;%d;%s%dm", (AType & 8) == 8,
	        (AType > 15 ? "5;" : ""), (AType & 7)+30);
	write_to_buffer( ch->desc, buf, strlen(buf) );
    }
    return;
}

void set_pager_color( sh_int AType, CHAR_DATA *ch )
{
    char buf[16];
    CHAR_DATA *och;

    if ( !ch || !ch->desc )
      return;

    och = (ch->desc->original ? ch->desc->original : ch);
    if ( !IS_NPC(och) && xIS_SET(och->act, PLR_ANSI) )
    {
	AType = figure_color(AType, och);
	if ( AType == 7 )
	  strcpy( buf, "\033[0;37m" );
	else
	  sprintf(buf, "\033[0;%d;%s%dm", (AType & 8) == 8,
	        (AType > 15 ? "5;" : ""), (AType & 7)+30);
	send_to_pager( buf, ch );
	ch->desc->pagecolor = AType;
    }
    return;
}


/* source: EOD, by John Booth <???> */
void ch_printf(CHAR_DATA *ch, char *fmt, ...)
{
    char buf[MAX_STRING_LENGTH*2];	/* better safe than sorry */
    va_list args;

    va_start(args, fmt);
    vsprintf(buf, fmt, args);
    va_end(args);

    send_to_char(buf, ch);
}

void pager_printf(CHAR_DATA *ch, char *fmt, ...)
{
    char buf[MAX_STRING_LENGTH*2];
    va_list args;

    va_start(args, fmt);
    vsprintf(buf, fmt, args);
    va_end(args);

    send_to_pager(buf, ch);
}


/*
 * Function to strip off the "a" or "an" or "the" or "some" from an object's
 * short description for the purpose of using it in a sentence sent to
 * the owner of the object.  (Ie: an object with the short description
 * "a long dark blade" would return "long dark blade" for use in a sentence
 * like "Your long dark blade".  The object name isn't always appropriate
 * since it contains keywords that may not look proper.		-Thoric
 */
char *myobj( OBJ_DATA *obj )
{
    if ( !str_prefix("a ", obj->short_descr) )
	return obj->short_descr + 2;
    if ( !str_prefix("an ", obj->short_descr) )
	return obj->short_descr + 3;
    if ( !str_prefix("the ", obj->short_descr) )
	return obj->short_descr + 4;
    if ( !str_prefix("some ", obj->short_descr) )
	return obj->short_descr + 5;
    return obj->short_descr;
}


char *obj_short( OBJ_DATA *obj )
{
    static char buf[MAX_STRING_LENGTH];

    if ( obj->count > 1 )
    {
	sprintf( buf, "%s (%d)", obj->short_descr, obj->count );
	return buf;
    }
    return obj->short_descr;
}

/*
 * The primary output interface for formatted output.
 */
/* Major overhaul. -- Alty */

void ch_printf_color(CHAR_DATA *ch, char *fmt, ...)
{
     char buf[MAX_STRING_LENGTH*2];
     va_list args;

     va_start(args, fmt);
     vsprintf(buf, fmt, args);
     va_end(args);

     send_to_char_color(buf, ch);
}

void pager_printf_color(CHAR_DATA *ch, char *fmt, ...)
{
     char buf[MAX_STRING_LENGTH*2];
     va_list args;

     va_start(args, fmt);
     vsprintf(buf, fmt, args);
     va_end(args);

     send_to_pager_color(buf, ch);
}

#define MORPHNAME(ch)   ((ch->morph&&ch->morph->morph)? \
                         ch->morph->morph->short_desc: \
                         IS_NPC(ch) ? ch->short_descr : ch->name)
#define NAME(ch)        (IS_NPC(ch) ? ch->short_descr : ch->name)

char *act_string(const char *format, CHAR_DATA *to, CHAR_DATA *ch,
		 const void *arg1, const void *arg2, int flags)
{
  static char * const he_she  [] = { "it",  "he",  "she" };
  static char * const him_her [] = { "it",  "him", "her" };
  static char * const his_her [] = { "its", "his", "her" };
  static char * const che_she [] = { "It",  "He",  "She" };
  static char * const chim_her[] = { "It",  "Him", "Her" };
  static char * const chis_her[] = { "Its", "His", "Her" };
  static char buf[MAX_STRING_LENGTH];
  char fname[MAX_INPUT_LENGTH];
  char temp[MAX_STRING_LENGTH];
  char *point = buf;
  const char *str = format;
  const char *i;
  CHAR_DATA *vch = (CHAR_DATA *) arg2;
  OBJ_DATA *obj1 = (OBJ_DATA  *) arg1;
  OBJ_DATA *obj2 = (OBJ_DATA  *) arg2;
  bool capitalize = FALSE;

  if ( str[0] == '$' )
  	DONT_UPPER = FALSE;
  else
	DONT_UPPER = TRUE;

  while ( *str != '\0' )
  {
    capitalize = FALSE;

    if ( *str != '$' )
    {
      *point++ = *str++;
      continue;
    }
    ++str;
    if ( !arg2 && *str >= 'A' && *str <= 'Z' )
    {
      bug( "Act: missing arg2 for code %c:", *str );
      bug( format );
      i = " <@@@> ";
    }
    else
    {
      if( *str == '*' )
      {
	capitalize = TRUE;
        str++;
      }

      switch ( *str )
      {
      default:  bug( "Act: bad code %c.", *str );
		i = " <@@@> ";						break;
      case 't': i = (char *) arg1;					break;
      case 'T': i = (char *) arg2;					break;
      case 'n':
              if ( ch->morph == NULL )
                  i = (to ? PERS ( ch, to ): NAME ( ch ) );
              else if ( !IS_SET( flags, STRING_IMM ) )
                  i = (to ? MORPHPERS (ch, to) : MORPHNAME (ch));
              else
              {
                sprintf(temp, "(MORPH) %s", (to ? PERS(ch,to):NAME(ch)));
                i = temp;
              }
              break;
      case 'N':
              if ( vch->morph == NULL )
                   i = (to ? PERS ( vch, to ) : NAME( vch ) );
              else if ( !IS_SET( flags, STRING_IMM ) )
                   i = (to ? MORPHPERS (vch, to) : MORPHNAME (vch));
              else
              {
                sprintf(temp, "(MORPH) %s", (to ? PERS(vch,to):NAME(vch)));
                i = temp;
              }
              break;

      case 'e': if (ch->sex > 2 || ch->sex < 0)
		{
		  bug("act_string: player %s has sex set at %d!", ch->name,
		      ch->sex);
		  i = "it";
		}
		else
		  i = capitalize ? che_she [URANGE(0,  ch->sex, 2)] : he_she [URANGE(0, ch->sex, 2)];
		break;
      case 'E': if (vch->sex > 2 || vch->sex < 0)
		{
		  bug("act_string: player %s has sex set at %d!", vch->name,
		      vch->sex);
		  i = "it";
		}
		else
		  i = capitalize ? che_she [URANGE(0, vch->sex, 2)] : he_she[URANGE(0, vch->sex, 2)];
		break;
      case 'm': if (ch->sex > 2 || ch->sex < 0)
		{
		  bug("act_string: player %s has sex set at %d!", ch->name,
		      ch->sex);
		  i = "it";
		}
		else
		  i = capitalize ? chim_her[URANGE(0,  ch->sex, 2)] : him_her[URANGE(0, ch->sex, 2)];
		break;
      case 'M': if (vch->sex > 2 || vch->sex < 0)
		{
		  bug("act_string: player %s has sex set at %d!", vch->name,
		      vch->sex);
		  i = "it";
		}
		else
		  i = capitalize ? chim_her[URANGE(0, vch->sex, 2)] : him_her[URANGE(0, vch->sex, 2)];
		break;
      case 's': if (ch->sex > 2 || ch->sex < 0)
		{
		  bug("act_string: player %s has sex set at %d!", ch->name,
		      ch->sex);
		  i = "its";
		}
		else
		  i = capitalize ? chis_her[URANGE(0,  ch->sex, 2)] : his_her[URANGE(0, ch->sex, 2)];
		break;
      case 'S': if (vch->sex > 2 || vch->sex < 0)
		{
		  bug("act_string: player %s has sex set at %d!", vch->name,
		      vch->sex);
		  i = "its";
		}
		else
		  i = capitalize ? chis_her[URANGE(0,  vch->sex, 2)] : his_her[URANGE(0, vch->sex, 2)];
		break;
      case 'q': i = (to == ch) ? "" : "s";				break;
      case 'Q': i = (to == ch) ? "your" :
		    his_her[URANGE(0,  ch->sex, 2)];			break;
      case 'p': i = (!to || can_see_obj(to, obj1)
		  ? obj_short(obj1) : "something");			break;
      case 'P': i = (!to || can_see_obj(to, obj2)
		  ? obj_short(obj2) : "something");			break;
      case 'd':
        if ( !arg2 || ((char *) arg2)[0] == '\0' )
          i = "door";
        else
        {
          one_argument((char *) arg2, fname);
          i = fname;
        }
        break;
      }
    }
    ++str;
    while ( (*point = *i) != '\0' )
      ++point, ++i;

   /*  #0  0x80c6c62 in act_string (
    format=0x81db42e "$n has reconnected, kicking off old link.", to=0x0,
    ch=0x94fcc20, arg1=0x0, arg2=0x0, flags=2) at comm.c:2901 */
  }
  strcpy(point, "\n\r");
  if ( !DONT_UPPER )
     buf[0] = UPPER(buf[0]);

  DONT_UPPER = FALSE;

  return buf;
}
#undef NAME

void act( sh_int AType, const char *format, CHAR_DATA *ch, const void *arg1, const void *arg2, int type )
{
    char *txt;
    CHAR_DATA *to;
    CHAR_DATA *vch = (CHAR_DATA *)arg2;

    /*
     * Discard null and zero-length messages.
     */
    if ( !format || format[0] == '\0' )
	return;

    if ( !ch )
    {
	bug( "Act: null ch. (%s)", format );
	return;
    }

    if ( !ch->in_room )
      to = NULL;
    else if ( type == TO_CHAR )
      to = ch;
    else
      to = ch->in_room->first_person;

    /*
     * ACT_SECRETIVE handling
     */
    if ( IS_NPC(ch) && xIS_SET(ch->act, ACT_SECRETIVE) && type != TO_CHAR )
	return;

    if ( type == TO_VICT )
    {
	if ( !vch )
	{
	    bug( "Act: null vch with TO_VICT." );
	    bug( "%s (%s)", ch->name, format );
	    return;
	}
	if ( !vch->in_room )
	{
	    bug( "Act: vch in NULL room!" );
	    bug( "%s -> %s (%s)", ch->name, vch->name, format );
	    return;
	}
	to = vch;
/*	to = vch->in_room->first_person;*/
    }

    if ( MOBtrigger && type != TO_CHAR && type != TO_VICT && to )
    {
      OBJ_DATA *to_obj;

      txt = act_string(format, NULL, ch, arg1, arg2, STRING_IMM);
      if ( HAS_PROG(to->in_room, ACT_PROG) )
        rprog_act_trigger(txt, to->in_room, ch, (OBJ_DATA *)arg1, (void *)arg2);
      for ( to_obj = to->in_room->first_content; to_obj;
            to_obj = to_obj->next_content )
        if ( HAS_PROG(to_obj->pIndexData, ACT_PROG) )
          oprog_act_trigger(txt, to_obj, ch, (OBJ_DATA *)arg1, (void *)arg2);
    }

    /* Anyone feel like telling me the point of looping through the whole
       room when we're only sending to one char anyways..? -- Alty */
    for ( ; to; to = (type == TO_CHAR || type == TO_VICT)
                     ? NULL : to->next_in_room )
    {
	if ((!to->desc
	&& (  IS_NPC(to) && !HAS_PROG(to->pIndexData, ACT_PROG) ))
	||   !IS_AWAKE(to) )
	    continue;

	if ( type == TO_CHAR && to != ch )
	    continue;
	if ( type == TO_VICT && ( to != vch || to == ch ) )
	    continue;
	if ( type == TO_ROOM && to == ch )
	    continue;
	if ( type == TO_NOTVICT && (to == ch || to == vch) )
	    continue;
	if ( type == TO_CANSEE && ( to == ch ||
	    (!IS_IMMORTAL(to) && !IS_NPC(ch) && (xIS_SET(ch->act, PLR_WIZINVIS)
	    && (get_trust(to) < (ch->pcdata ? ch->pcdata->wizinvis : 0) ) ) ) ) )
	    continue;

        if ( IS_IMMORTAL(to) )
            txt = act_string (format, to, ch, arg1, arg2, STRING_IMM);
        else
       	    txt = act_string (format, to, ch, arg1, arg2, STRING_NONE);

	if (to->desc)
	{
        set_char_color(AType, to);
        send_to_char( txt, to );
	}
	if (MOBtrigger)
        {
          /* Note: use original string, not string with ANSI. -- Alty */
	  mprog_act_trigger( txt, to, ch, (OBJ_DATA *)arg1, (void *)arg2 );
        }
    }
    MOBtrigger = TRUE;
    return;
}

/*
char *default_fprompt( CHAR_DATA *ch )
{
  static char buf[60];

  if (ch->race == race_lookup( "android" ))
  {
    strcpy(buf, "&D&z<&wDamage &w(&c%h&w) ");
    strcat(buf, "&wArmor &w(&c%z&w) ");
    strcat(buf, "&wEnergy &w(&c%m&w) ");
    strcat(buf, "&wFocus &w(&c%f&w)&z>");
    if ( IS_NPC(ch) || IS_IMMORTAL(ch) )
      strcat(buf, " %i%R&D\n\r");
    else
      strcat(buf, " &D\n\r");
    strcat(buf, "&D&z<&cTechlevel &c(&w%x&c/&w&W%P&c)&z>&w Enemy: &w(&R%y&w)&D&w ");
  }
  else
  {
    strcpy(buf, "&D&z<&wLifeforce &w(&c%h&w) ");
    strcat(buf, "&wArmor &w(&c%z&w) ");
    strcat(buf, "&wEnergy &w(&c%m&w) ");
    strcat(buf, "&wFocus &w(&c%f&w)&z>");
    if ( IS_NPC(ch) || IS_IMMORTAL(ch) )
      strcat(buf, " %i%R&D\n\r");
    else
      strcat(buf, " &D\n\r");
    strcat(buf, "&D&z<&cPowerlevel &c(&w%x&c/&w&W%P&c)&z>&w Enemy: &w(&R%y&w)&D&w ");
  }
  return buf;
}
*/

char *default_fprompt( CHAR_DATA *ch )
{
	static char buf[60];

	if (!IS_NPC(ch))
	{
		switch (ch->pcdata->battlePromptConfig)
		{
			default:
				if (is_android(ch))
				{
					strcpy(buf, "&D<Damage(&c%h&w) En(&c%m&w) ");
					if (ch->exp < ch->pl)
						strcat(buf, "TL(%x/&Y%P&w)>");
					else if (ch->exp > ch->pl)
						strcat(buf, "TL(%x/&B%P&w)>");
					else
						strcat(buf, "TL(%x/&W%P&w)>");

					if (IS_NPC(ch) || IS_IMMORTAL(ch))
						strcat(buf, " %i%R");
				}
				else
				{
					strcpy(buf, "&D<Life(&c%h&w) Ki(&c%m&w) ");
					if (ch->exp < ch->pl)
						strcat(buf, "PL(%x/&Y%P&w)>");
					else if (ch->exp > ch->pl)
						strcat(buf, "PL(%x/&B%P&w)>");
					else
						strcat(buf, "PL(%x/&W%P&w)>");

					if (IS_NPC(ch) || IS_IMMORTAL(ch))
						strcat(buf, " %i%R");
				}
				strcat(buf, "\n\r<Enemy(&R%y&w) Focus(&C%f&w) Armor(&c%z&w)> ");
			break;
			case 1:
				if ( is_android(ch) )
				{
					strcpy(buf, "&D<Damage(&c%h&w) En(&c%m&w) ");
					if (ch->exp < ch->pl)
						strcat(buf, "TL(%x/&Y%P&w)>");
					else if (ch->exp > ch->pl)
						strcat(buf, "TL(%x/&B%P&w)>");
					else
						strcat(buf, "TL(%x/&W%P&w)>");

					if (IS_NPC(ch) || IS_IMMORTAL(ch))
						strcat(buf, " %i%R");
				}
				else
				{
					strcpy(buf, "&D<Life(&c%h&w) Ki(&c%m&w) ");
					if (ch->exp < ch->pl)
						strcat(buf, "PL(%x/&Y%P&w)>");
					else if (ch->exp > ch->pl)
						strcat(buf, "PL(%x/&B%P&w)>");
					else
						strcat(buf, "PL(%x/&W%P&w)>");

					if (IS_NPC(ch) || IS_IMMORTAL(ch))
						strcat(buf, " %i%R");
				}
				strcat(buf, "\n\r<Enemy(&R%y&w) Focus(&C%f&w)> ");
			break;
			case 2:
				if ( is_android(ch) )
				{
					strcpy(buf, "&D<Damage(&c%h&w) En(&c%m&w) ");
					if (ch->exp < ch->pl)
						strcat(buf, "TL(&Y%p&w)>");
					else if (ch->exp > ch->pl)
						strcat(buf, "TL(&B%p&w)>");
					else
						strcat(buf, "TL(&W%p&w)>");

					if (IS_NPC(ch) || IS_IMMORTAL(ch))
						strcat(buf, " %i%R");
				}
				else
				{
					strcpy(buf, "&D<Life(&c%h&w) Ki(&c%m&w) ");
					if (ch->exp < ch->pl)
						strcat(buf, "PL(&Y%p&w)>");
					else if (ch->exp > ch->pl)
						strcat(buf, "PL(&B%p&w)>");
					else
						strcat(buf, "PL(&W%p&w)>");

					if (IS_NPC(ch) || IS_IMMORTAL(ch))
						strcat(buf, " %i%R");
				}
				strcat(buf, "\n\r<Enemy(&R%y&w) Focus(&C%f&w) Armor(&c%z&w)> ");
			break;
			case 3:
				if (is_android(ch))
				{
					strcpy(buf, "&D<Damage(&c%h&w) En(&c%m&w) ");
					if (ch->exp < ch->pl)
						strcat(buf, "TL(&Y%p&w)>");
					else if (ch->exp > ch->pl)
						strcat(buf, "TL(&B%p&w)>");
					else
						strcat(buf, "TL(&W%p&w)>");

					if (IS_NPC(ch) || IS_IMMORTAL(ch))
						strcat(buf, " %i%R");
				}
				else
				{
					strcpy(buf, "&D<Life(&c%h&w) Ki(&c%m&w) ");
					if (ch->exp < ch->pl)
						strcat(buf, "PL(&Y%p&w)>");
					else if (ch->exp > ch->pl)
						strcat(buf, "PL(&B%p&w)>");
					else
						strcat(buf, "PL(&W%p&w)>");

					if (IS_NPC(ch) || IS_IMMORTAL(ch))
						strcat(buf, " %i%R");
				}
				strcat(buf, "\n\r<Enemy(&R%y&w) Focus(&C%f&w)> ");
			break;
		}
	}
	else
	{
		if (is_android(ch))
		{
			strcpy(buf, "&D<Damage(&c%h&w) En(&c%m&w) ");
			if (ch->exp < ch->pl)
				strcat(buf, "TL(%x/&Y%P&w)>");
			else if (ch->exp > ch->pl)
				strcat(buf, "TL(%x/&B%P&w)>");
			else
				strcat(buf, "TL(%x/&W%P&w)>");

			if (IS_NPC(ch) || IS_IMMORTAL(ch))
				strcat(buf, " %i%R");
		}
		else
		{
			strcpy(buf, "&D<Life(&c%h&w) Ki(&c%m&w) ");
			if (ch->exp < ch->pl)
				strcat(buf, "PL(%x/&Y%P&w)>");
			else if (ch->exp > ch->pl)
				strcat(buf, "PL(%x/&B%P&w)>");
			else
				strcat(buf, "PL(%x/&W%P&w)>");

			if (IS_NPC(ch) || IS_IMMORTAL(ch))
				strcat(buf, " %i%R");
		}
		strcat(buf, "\n\r<Enemy(&R%y&w) Focus(&C%f&w) Armor(&c%z&w)> ");
	}

	return buf;
}



char *default_prompt( CHAR_DATA *ch )
{
	static char buf[60];

/*
	if (ch->desc->psuppress_channel != 0);
	{
		if (ch->race == race_lookup("android"))
			strcpy(buf, "&D<D:%h E:%m> ");
		else
			strcpy(buf, "&D<L:%h K:%m> ");

		ch->desc->psuppress_channel = 0;
		return buf;
	}
*/
	if (!IS_NPC(ch))
	{
		switch (ch->pcdata->normalPromptConfig)
		{
			default:
				if (is_android(ch))
				{
					strcpy(buf, "&D<Damage(&c%h&w) En(&c%m&w) ");
					if (ch->exp < ch->pl)
						strcat(buf, "TL(%x/&Y%P&w)>");
					else if (ch->exp > ch->pl)
						strcat(buf, "TL(%x/&B%P&w)>");
					else
						strcat(buf, "TL(%x/&W%P&w)>");

					if (IS_NPC(ch) || IS_IMMORTAL(ch))
						strcat(buf, " %i%R");
					else
						strcat(buf, " ");
				}
				else
				{
					strcpy(buf, "&D<Life(&c%h&w) Ki(&c%m&w) ");
					if (ch->exp < ch->pl)
						strcat(buf, "PL(%x/&Y%P&w)>");
					else if (ch->exp > ch->pl)
						strcat(buf, "PL(%x/&B%P&w)>");
					else
						strcat(buf, "PL(%x/&W%P&w)>");

					if (IS_NPC(ch) || IS_IMMORTAL(ch))
						strcat(buf, " %i%R");
					else
						strcat(buf, " ");
				}
			break;
			case 1:
				if (is_android(ch))
				{
					strcpy(buf, "&D<Damage(&c%h&w) En(&c%m&w) ");
					if (ch->exp < ch->pl)
						strcat(buf, "TL(&Y%p&w)>");
					else if (ch->exp > ch->pl)
						strcat(buf, "TL(&B%p&w)>");
					else
						strcat(buf, "TL(&W%p&w)>");

					if (IS_NPC(ch) || IS_IMMORTAL(ch))
						strcat(buf, " %i%R");
					else
						strcat(buf, " ");
				}
				else
				{
					strcpy(buf, "&D<Life(&c%h&w) Ki(&c%m&w) ");
					if (ch->exp < ch->pl)
						strcat(buf, "PL(&Y%p&w)>");
					else if (ch->exp > ch->pl)
						strcat(buf, "PL(&B%p&w)>");
					else
						strcat(buf, "PL(&W%p&w)>");

					if (IS_NPC(ch) || IS_IMMORTAL(ch))
						strcat(buf, " %i%R");
					else
						strcat(buf, " ");
				}
			break;
		}
	}
	else
	{
		if (is_android(ch))
		{
			strcpy(buf, "&D<Damage(&c%h&w) En(&c%m&w) ");
			if (ch->exp < ch->pl)
				strcat(buf, "TL(%x/&Y%P&w)>");
			else if (ch->exp > ch->pl)
				strcat(buf, "TL(%x/&B%P&w)>");
			else
				strcat(buf, "TL(%x/&W%P&w)>");

			if (IS_NPC(ch) || IS_IMMORTAL(ch))
				strcat(buf, " %i%R");
		}
		else
		{
			strcpy(buf, "&D<Life(&c%h&w) Ki(&c%m&w) ");
			if (ch->exp < ch->pl)
				strcat(buf, "PL(%x/&Y%P&w)>");
			else if (ch->exp > ch->pl)
				strcat(buf, "PL(%x/&B%P&w)>");
			else
				strcat(buf, "PL(%x/&W%P&w)>");

			if (IS_NPC(ch) || IS_IMMORTAL(ch))
				strcat(buf, " %i%R");
			else
				strcat(buf, " ");
		}
	}

	return buf;
}

/*
char *default_prompt( CHAR_DATA *ch )
{
  static char buf[60];

  if (is_android(ch))
  {
  strcpy(buf, "&D&z<&wDamage &w(&c%h&w) ");
  strcat(buf, "&wArmor &w(&c%z&w) ");
  strcat(buf, "&wEnergy &w(&c%m&w)&z>");
  if ( IS_NPC(ch) || IS_IMMORTAL(ch) )
    strcat(buf, " %i%R&D\n\r");
  else
    strcat(buf, " &D\n\r");
  strcat(buf, "&D&z<&cTechlevel &c(&w%x&c/&w&W%P&c)&z>&D&w ");
  }
  else
  {
  strcpy(buf, "&D&z<&wLifeforce &w(&c%h&w) ");
  strcat(buf, "&wArmor &w(&c%z&w) ");
  strcat(buf, "&wEnergy &w(&c%m&w)&z>");
  if ( IS_NPC(ch) || IS_IMMORTAL(ch) )
    strcat(buf, " %i%R&D\n\r");
  else
    strcat(buf, " &D\n\r");
  strcat(buf, "&D&z<&cPowerlevel &c(&w%x&c/&w&W%P&c)&z>&D&w ");
  }
  return buf;
}
*/
int getcolor(char clr)
{
  static const char colors[16] = "xrgObpcwzRGYBPCW";
  int r;

  for ( r = 0; r < 16; r++ )
    if ( clr == colors[r] )
      return r;
  return -1;
}

void display_prompt( DESCRIPTOR_DATA *d )
{
  CHAR_DATA *ch = d->character;
  CHAR_DATA *och = (d->original ? d->original : d->character);
  CHAR_DATA *victim;
  bool ansi = (!IS_NPC(och) && xIS_SET(och->act, PLR_ANSI));
  const char *prompt;
  const char *helpstart = "<Type HELP START>";
  char buf[MAX_STRING_LENGTH];
  char *pbuf = buf;
  int stat, percent;
  long double stat_ld = -1;
  long double stat_ldLong = -1;

  if ( !ch )
  {
    bug( "display_prompt: NULL ch" );
    return;
  }

  if ( !IS_NPC(ch) && !IS_SET(ch->pcdata->flags, PCFLAG_HELPSTART ) )
	prompt = helpstart;
  else if ( !IS_NPC(ch) && ch->substate != SUB_NONE && ch->pcdata->subprompt
  &&   ch->pcdata->subprompt[0] != '\0' )
    prompt = ch->pcdata->subprompt;
  else if (IS_NPC (ch) || (!ch->fighting && (!ch->pcdata->prompt
                || !*ch->pcdata->prompt) ) )
    prompt = default_prompt (ch);
  else if ( ch->fighting )
  {
        if ( !ch->pcdata->fprompt || !*ch->pcdata->fprompt )
                prompt = default_fprompt ( ch );
        else
                prompt = ch->pcdata->fprompt;
  }
  else
    prompt = ch->pcdata->prompt;
  if ( ansi )
  {
    strcpy(pbuf, "\033[0;37m");
    d->prevcolor = 0x07;
    pbuf += 7;
  }
  /* Clear out old color stuff */
/*  make_color_sequence(NULL, NULL, NULL);*/
  for ( ; *prompt; prompt++ )
  {
    /*
     * '%' = prompt commands
     * Note: foreground changes will revert background to 0 (black)
     */
    if( *prompt != '%' )
    {
      *(pbuf++) = *prompt;
      continue;
    }
    ++prompt;
    if ( !*prompt )
      break;
    if ( *prompt == *(prompt-1) )
    {
      *(pbuf++) = *prompt;
      continue;
    }
    switch(*(prompt-1))
    {
    default:
      bug( "Display_prompt: bad command char '%c'.", *(prompt-1) );
      break;
    case '%':
      *pbuf = '\0';
      stat = 0x80000000;
      switch(*prompt)
      {
      case '%':
	*pbuf++ = '%';
	*pbuf = '\0';
	break;
      case 'a':
	if ( ch->level >= 10 )
	  stat = ch->alignment;
	else if ( IS_GOOD(ch) )
	  strcpy(pbuf, "good");
	else if ( IS_EVIL(ch) )
	  strcpy(pbuf, "evil");
	else
	  strcpy(pbuf, "neutral");
	break;
      case 'A':
	sprintf( pbuf, "%s%s%s", IS_AFFECTED( ch, AFF_INVISIBLE ) ? "I" : "",
				 IS_AFFECTED( ch, AFF_HIDE )      ? "H" : "",
				 IS_AFFECTED( ch, AFF_SNEAK )     ? "S" : "" );
        break;
        case 'C':  /* Tank */
	  if ( !IS_IMMORTAL( ch ) ) break;
          if ( !ch->fighting || ( victim = ch->fighting->who ) == NULL )
             strcpy( pbuf, "N/A" );
          else if(!victim->fighting||(victim = victim->fighting->who)==NULL)
             strcpy( pbuf, "N/A" );
          else {
              if ( victim->max_hit > 0 )
                    percent = (100 * victim->hit) / victim->max_hit;
              else
                    percent = -1;
                   if (percent >= 100)
                       strcpy (pbuf, "perfect health");
                   else if (percent >= 90)
                       strcpy (pbuf, "slightly scratched");
                   else if (percent >= 80)
                       strcpy (pbuf, "few bruises");
                    else if (percent >= 70)
                       strcpy (pbuf, "some cuts");
                    else if (percent >= 60)
                       strcpy (pbuf, "several wounds");
                    else if (percent >= 50)
                       strcpy (pbuf, "nasty wounds");
                    else if (percent >= 40)
                       strcpy (pbuf, "bleeding freely");
                    else if (percent >= 30)
                       strcpy (pbuf, "covered in blood");
                    else if (percent >= 20)
                       strcpy (pbuf, "leaking guts");
                    else if (percent >= 10)
                       strcpy (pbuf, "almost dead");
                    else
                       strcpy (pbuf, "DYING");
             }
          break;
        case 'c':
	  if ( !IS_IMMORTAL( ch ) ) break;
          if ( !ch->fighting || ( victim = ch->fighting->who ) == NULL )
             strcpy( pbuf, "N/A" );
          else {
              if ( victim->max_hit > 0 )
                    percent = (100 * victim->hit) / victim->max_hit;
              else
                    percent = -1;
                if (percent >= 100)
                       strcpy (pbuf, "perfect health");
                    else if (percent >= 90)
                       strcpy (pbuf, "slightly scratched");
                    else if (percent >= 80)
                       strcpy (pbuf, "few bruises");
                    else if (percent >= 70)
                       strcpy (pbuf, "some cuts");
                    else if (percent >= 60)
                       strcpy (pbuf, "several wounds");
                    else if (percent >= 50)
                       strcpy (pbuf, "nasty wounds");
                    else if (percent >= 40)
                       strcpy (pbuf, "bleeding freely");
                    else if (percent >= 30)
                       strcpy (pbuf, "covered in blood");
                    else if (percent >= 20)
                       strcpy (pbuf, "leaking guts");
                    else if (percent >= 10)
                       strcpy (pbuf, "almost dead");
                    else
                       strcpy (pbuf, "DYING");
           }
          break;
      case 'h':
	stat = ch->hit;
	break;
      case 'H':
	stat = ch->max_hit;
	break;
      case 'm':
	  stat = ch->mana;
	break;
      case 'M':
	  stat = ch->max_mana;
	break;
       case 'P':
	stat_ld = ch->pl;
	break;
       case 'p':
	stat_ldLong = ch->pl;
	break;
        case 'N': /* Tank */
	  if ( !IS_IMMORTAL(ch) ) break;
          if ( !ch->fighting || ( victim = ch->fighting->who ) == NULL )
            strcpy( pbuf, "N/A" );
          else if(!victim->fighting||(victim=victim->fighting->who)==NULL)
            strcpy( pbuf, "N/A" );
          else {
            if ( ch == victim )
                    strcpy ( pbuf, "You" );
            else if ( IS_NPC(victim) )
                    strcpy ( pbuf, victim->short_descr );
            else
                    strcpy ( pbuf, victim->name );
                pbuf[0] = UPPER( pbuf[0] );
          }
          break;
        case 'n':
	  if ( !IS_IMMORTAL(ch) ) break;
          if (!ch->fighting || (victim = ch->fighting->who) == NULL )
            strcpy( pbuf, "N/A" );
          else {
            if ( ch == victim )
                    strcpy ( pbuf, "You" );
            else if ( IS_NPC(victim) )
                    strcpy ( pbuf, victim->short_descr );
            else
                    strcpy ( pbuf, victim->name );
            pbuf[0] = UPPER( pbuf[0] );
          }
          break;
      case 'T':
        if      ( time_info.hour <  5 ) strcpy( pbuf, "night" );
        else if ( time_info.hour <  6 ) strcpy( pbuf, "dawn" );
        else if ( time_info.hour < 19 ) strcpy( pbuf, "day" );
        else if ( time_info.hour < 21 ) strcpy( pbuf, "dusk" );
        else                            strcpy( pbuf, "night" );
        break;
      case 'b':
	  stat = 0;
	break;
      case 'B':
	  stat = 0;
	break;
      case 'u':
	stat = num_descriptors;
	break;
      case 'U':
	stat = sysdata.maxplayers;
	break;
      case 'v':
	stat = ch->move;
	break;
      case 'V':
	stat = ch->max_move;
	break;
      case 'g':
	stat = ch->gold;
	break;
      case 'r':
	if ( IS_IMMORTAL(och) )
	  stat = ch->in_room->vnum;
	break;
      case 'F':
	if ( IS_IMMORTAL( och ) )
	  sprintf( pbuf, "%s", ext_flag_string( &ch->in_room->room_flags, r_flags) );
	break;
      case 'R':
	if ( xIS_SET(och->act, PLR_ROOMVNUM) )
	  sprintf(pbuf, "<#%d> ", ch->in_room->vnum);
	break;
      case 'x':
	stat_ld = ch->exp;
	break;
      case 'X':
	stat = exp_level(ch, ch->level+1) - ch->exp;
      case 'y':
          if ( !ch->fighting || ( victim = ch->fighting->who ) == NULL )
             strcpy( pbuf, "N/A" );
          else {
              	stat = victim->hit;
/*
              if ( victim->max_hit > 0 )
                    percent = (victim->hit);
              else
                    percent = -1;
                if (percent >= 100)
                       strcpy (pbuf, "|||\x1b[1;33m|||\x1b[1;32m||||");
                    else if (percent >= 90)
                       strcpy (pbuf, "|||\x1b[1;33m|||\x1b[1;32m|||\x1b[1;31m-");
                    else if (percent >= 80)
                       strcpy (pbuf, "|||\x1b[1;33m|||\x1b[1;32m||\x1b[1;31m--");
                    else if (percent >= 70)
                       strcpy (pbuf, "|||\x1b[1;33m|||\x1b[1;32m|\x1b[1;31m---");
                    else if (percent >= 60)
                       strcpy (pbuf, "|||\x1b[1;33m|||\x1b[1;31m----");
                    else if (percent >= 50)
                       strcpy (pbuf, "|||\x1b[1;33m||\x1b[1;31m-----");
                    else if (percent >= 40)
                       strcpy (pbuf, "|||\x1b[1;33m|\x1b[1;31m------");
                    else if (percent >= 30)
                       strcpy (pbuf, "|||-------");
                    else if (percent >= 20)
                       strcpy (pbuf, "||--------");
                    else if (percent >= 10)
                       strcpy (pbuf, "|---------");
                    else
                       strcpy (pbuf, "----------");
*/
           }
          break;

      case 'o':         /* display name of object on auction */
        if ( auction->item )
           strcpy( pbuf, auction->item->name );
        break;
      case 'S':
        if      ( ch->style == STYLE_BERSERK )    strcpy( pbuf, "B" );
        else if ( ch->style == STYLE_AGGRESSIVE ) strcpy( pbuf, "A" );
        else if ( ch->style == STYLE_DEFENSIVE )  strcpy( pbuf, "D" );
        else if ( ch->style == STYLE_EVASIVE )    strcpy( pbuf, "E" );
        else                                      strcpy( pbuf, "S" );
	break;
      case 'i':
	if ( (!IS_NPC(ch) && xIS_SET(ch->act, PLR_WIZINVIS)) ||
	      (IS_NPC(ch) && xIS_SET(ch->act, ACT_MOBINVIS)) )
	  sprintf(pbuf, "(Invis %d) ", (IS_NPC(ch) ? ch->mobinvis : ch->pcdata->wizinvis));
	else
	if ( IS_AFFECTED(ch, AFF_INVISIBLE) )
	  sprintf(pbuf, "(Invis) " );
	break;
      case 'I':
	stat = (IS_NPC(ch) ? (xIS_SET(ch->act, ACT_MOBINVIS) ? ch->mobinvis : 0)
	     : (xIS_SET(ch->act, PLR_WIZINVIS) ? ch->pcdata->wizinvis : 0));
	break;
	case 'z':
		stat = get_armor(ch);
		break;
	case 'Z':
		stat = get_maxarmor(ch);
		break;
	case 'f':
		stat = ch->focus;
		break;
      }


	if ( stat != 0x80000000 ) {
		if (stat >= 1000)
			sprintf(pbuf, "%s", num_punct(stat));
		else
			sprintf(pbuf, "%d", stat);
	}
	if (stat_ld != -1) {
		sprintf(pbuf, "%s", abbNumLD(stat_ld));
		stat_ld = -1;
	}
	else if (stat_ldLong != -1) {
		sprintf(pbuf, "%s", num_punct_ld(stat_ldLong));
		stat_ldLong = -1;
	}

//		sprintf(pbuf, "%s", abbNumLD(stat_ld));
//		sprintf(pbuf, "%s", num_punct_ld(stat_ld));



      pbuf += strlen(pbuf);
      break;
    }
  }
  *pbuf = '\0';
  send_to_char( buf, ch );
  return;
}

int make_color_sequence( const char *col, char *code, DESCRIPTOR_DATA *d )
{
//   CHAR_DATA *ch = (d->original ? d->original : d->character);
   const char *ctype = col;
   int ln;
//   bool ansi = ( !IS_NPC(ch) && xIS_SET( ch->act, PLR_ANSI ) );

   col++;

   if( !*col )
      ln = -1;
   else if ( *ctype != '&' && *ctype != '^' && *ctype != '}' )
   {
      bug( "colorcode: command '%c' not '&', '^' or '}'", *ctype );
      ln = -1;
   }
   else if( *col == *ctype )
   {
      code[0] = *col;
      code[1] = '\0';
      ln = 1;
   }
   else if( !d->ansi )
      ln = 0;
   else
   {
	/* Foreground text - non-blinking */
	if( *ctype == '&' )
	{
         switch( *col )
         {
            default:
		   code[0] = *ctype;
		   code[1] = *col;
		   code[2] = '\0';
		   return 2;
		case 'i': /* Italic text */
		case 'I':
		   strcpy( code, ANSI_ITALIC );
		   break;
		case 'v': /* Reverse colors */
		case 'V':
		   strcpy( code, ANSI_REVERSE );
		   break;
		case 'u': /* Underline */
		case 'U':
		   strcpy( code, ANSI_UNDERLINE );
		   break;
	      case 'd': /* Player's client default color */
		   strcpy( code, ANSI_RESET );
		   break;
	      case 'D': /* Reset to custom color for whatever is being displayed */
		   strcpy( code, ANSI_RESET ); /* Yes, this reset here is quite necessary to cancel out other things */
		   strcat( code, ANSI_GREY );
//		   strcat( code, color_str( ch->desc->pagecolor, ch ) );
		   break;
            case 'x': /* Black */
               strcpy( code, ANSI_BLACK );
               break;
            case 'O': /* Orange/Brown */
               strcpy( code, ANSI_ORANGE );
               break;
            case 'c': /* Cyan */
               strcpy( code, ANSI_CYAN );
               break;
            case 'z': /* Dark Grey */
               strcpy( code, ANSI_DGREY );
               break;
            case 'g': /* Dark Green */
               strcpy( code, ANSI_DGREEN );
               break;
            case 'G': /* Light Green */
               strcpy( code, ANSI_GREEN );
               break;
            case 'P': /* Pink/Light Purple */
               strcpy( code, ANSI_PINK );
               break;
            case 'r': /* Dark Red */
               strcpy( code, ANSI_DRED );
               break;
            case 'b': /* Dark Blue */
               strcpy( code, ANSI_DBLUE );
               break;
            case 'w': /* Grey */
               strcpy( code, ANSI_GREY );
               break;
            case 'Y': /* Yellow */
               strcpy( code, ANSI_YELLOW );
               break;
            case 'C': /* Light Blue */
               strcpy( code, ANSI_LBLUE );
               break;
            case 'p': /* Purple */
               strcpy( code, ANSI_PURPLE );
               break;
            case 'R': /* Red */
               strcpy( code, ANSI_RED );
               break;
            case 'B': /* Blue */
               strcpy( code, ANSI_BLUE );
               break;
            case 'W': /* White */
               strcpy( code, ANSI_WHITE );
               break;
         }
	}
	/* Foreground text - blinking */
	if( *ctype == '}' )
	{
         switch( *col )
         {
            default:
		   code[0] = *ctype;
		   code[1] = *col;
		   code[2] = '\0';
		   return 2;
            case 'x': /* Black */
               strcpy( code, BLINK_BLACK );
               break;
            case 'O': /* Orange/Brown */
               strcpy( code, BLINK_ORANGE );
               break;
            case 'c': /* Cyan */
               strcpy( code, BLINK_CYAN );
               break;
            case 'z': /* Dark Grey */
               strcpy( code, BLINK_DGREY );
               break;
            case 'g': /* Dark Green */
               strcpy( code, BLINK_DGREEN );
               break;
            case 'G': /* Light Green */
               strcpy( code, BLINK_GREEN );
               break;
            case 'P': /* Pink/Light Purple */
               strcpy( code, BLINK_PINK );
               break;
            case 'r': /* Dark Red */
               strcpy( code, BLINK_DRED );
               break;
            case 'b': /* Dark Blue */
               strcpy( code, BLINK_DBLUE );
               break;
            case 'w': /* Grey */
               strcpy( code, BLINK_GREY );
               break;
            case 'Y': /* Yellow */
               strcpy( code, BLINK_YELLOW );
               break;
            case 'C': /* Light Blue */
               strcpy( code, BLINK_LBLUE );
               break;
            case 'p': /* Purple */
               strcpy( code, BLINK_PURPLE );
               break;
            case 'R': /* Red */
               strcpy( code, BLINK_RED );
               break;
            case 'B': /* Blue */
               strcpy( code, BLINK_BLUE );
               break;
            case 'W': /* White */
               strcpy( code, BLINK_WHITE );
               break;
         }
	}
	/* Background color */

	if( *ctype == '^' )
	{
         switch( *col )
         {
            default:
		   code[0] = *ctype;
		   code[1] = *col;
		   code[2] = '\0';
		   return 2;
            case 'x': /* Black */
               strcpy( code, BACK_BLACK );
               break;
            case 'r': /* Dark Red */
               strcpy( code, BACK_DRED );
               break;
            case 'g': /* Dark Green */
               strcpy( code, BACK_DGREEN );
               break;
            case 'O': /* Orange/Brown */
               strcpy( code, BACK_ORANGE );
               break;
            case 'b': /* Dark Blue */
               strcpy( code, BACK_DBLUE );
               break;
            case 'p': /* Purple */
               strcpy( code, BACK_PURPLE );
               break;
            case 'c': /* Cyan */
               strcpy( code, BACK_CYAN );
               break;
            case 'w': /* Grey */
               strcpy( code, BACK_GREY );
               break;
         }
	}
      ln = strlen( code );
   }
   if ( ln <= 0 )
      *code = '\0';
   return ln;
}

void set_pager_input( DESCRIPTOR_DATA *d, char *argument )
{
  while ( isspace(*argument) )
    argument++;
  d->pagecmd = *argument;
  return;
}

bool pager_output( DESCRIPTOR_DATA *d )
{
  register char *last;
  CHAR_DATA *ch;
  int pclines;
  register int lines;
  bool ret;

  if ( !d || !d->pagepoint || d->pagecmd == -1 )
    return TRUE;
  ch = d->original ? d->original : d->character;
  pclines = UMAX(ch->pcdata->pagerlen, 5) - 1;
  switch(LOWER(d->pagecmd))
  {
  default:
    lines = 0;
    break;
  case 'b':
    lines = -1-(pclines*2);
    break;
  case 'r':
    lines = -1-pclines;
    break;
  case 'n':
    lines = 0;
    pclines = 0x7FFFFFFF;	/* As many lines as possible */
    break;
  case 'q':
    d->pagetop = 0;
    d->pagepoint = NULL;
    flush_buffer(d, TRUE);
    DISPOSE(d->pagebuf);
    d->pagesize = MAX_STRING_LENGTH;
    return TRUE;
  }
  while ( lines < 0 && d->pagepoint >= d->pagebuf )
    if ( *(--d->pagepoint) == '\n' )
      ++lines;
  if ( *d->pagepoint == '\n' && *(++d->pagepoint) == '\r' )
      ++d->pagepoint;
  if ( d->pagepoint < d->pagebuf )
    d->pagepoint = d->pagebuf;
  for ( lines = 0, last = d->pagepoint; lines < pclines; ++last )
    if ( !*last )
      break;
    else if ( *last == '\n' )
      ++lines;
  if ( *last == '\r' )
    ++last;
  if ( last != d->pagepoint )
  {
    if ( !write_to_descriptor(d->descriptor, d->pagepoint,
          (last-d->pagepoint)) )
      return FALSE;
    d->pagepoint = last;
  }
  while ( isspace(*last) )
    ++last;
  if ( !*last )
  {
    d->pagetop = 0;
    d->pagepoint = NULL;
    flush_buffer(d, TRUE);
    DISPOSE(d->pagebuf);
    d->pagesize = MAX_STRING_LENGTH;
    return TRUE;
  }
  d->pagecmd = -1;
   if( xIS_SET( ch->act, PLR_ANSI ) )
      if( write_to_descriptor( d->descriptor, ANSI_LBLUE, 0 ) == FALSE )
	   return FALSE;
  if ( (ret=write_to_descriptor(d->descriptor,
	"(C)ontinue, (N)on-stop, (R)efresh, (B)ack, (Q)uit: [C] ", 0)) == FALSE )
	return FALSE;
  if ( xIS_SET( ch->act, PLR_ANSI ) )
  {
      char buf[32];

	sprintf( buf, "%s", color_str( d->pagecolor, ch ) );
  }
  return ret;
}


#ifdef WIN32

void shutdown_mud( char *reason );

void bailout(void)
{
    echo_to_all( AT_IMMORT, "MUD shutting down by system operator NOW!!", ECHOTAR_ALL );
    shutdown_mud( "MUD shutdown by system operator" );
    log_string ("MUD shutdown by system operator");
    Sleep (5000);		/* give "echo_to_all" time to display */
    mud_down = TRUE;		/* This will cause game_loop to exit */
    service_shut_down = TRUE;	/* This will cause characters to be saved */
    fflush(stderr);
    return;
}

#endif

bool check_total_ip( DESCRIPTOR_DATA *dnew)
{
	DESCRIPTOR_DATA *d;
	CHAR_DATA *vch;
	int ip_total = 0;
	bool ch = TRUE;
	bool is_nolimit = FALSE;
	bool linkdead = FALSE;

	if (!sysdata.check_plimit)
		return FALSE;

	if (dnew->character->level >= sysdata.level_noplimit)
		return FALSE;

	for( d = first_descriptor; d; d= d->next )
	{

		if (d == dnew)
		{
			continue;
		}

		vch = d->character;

		if (!vch)
			ch = FALSE;

		if (ch)
		{
			if (!vch->desc)
				linkdead = TRUE;
			if (!linkdead && dnew->character)
			{
				if (!str_cmp(dnew->character->name, vch->name))
					return FALSE;
			}
		}

#ifdef DNS_SLAVE
		if (ch && !str_cmp( d->host, dnew->host )
			&& vch->level >= sysdata.level_noplimit)
			is_nolimit = TRUE;

		if (!linkdead && !str_cmp( d->host, dnew->host ))
			ip_total ++;

#else
		if (ch && !str_cmp( d->host, dnew->host )
			&& vch->level >= sysdata.level_noplimit)
			is_nolimit = TRUE;

		if (!linkdead && !str_cmp( d->host, dnew->host ))
			ip_total ++;

#endif

	}

	if (is_nolimit)
		return FALSE;

	if (ip_total >= sysdata.plimit)
		return TRUE;

	return FALSE;
}

char *color_str( sh_int AType, CHAR_DATA *ch )
{
   if( !ch )
   {
	bug( "color_str: NULL ch!", 0 );
	return( "" );
   }

   if( IS_NPC(ch) || !xIS_SET( ch->act, PLR_ANSI ) )
      return( "" );

   switch( AType )
   {
	case 0:  return( ANSI_BLACK );		break;
	case 1:  return( ANSI_DRED );		break;
	case 2:  return( ANSI_DGREEN );		break;
	case 3:  return( ANSI_ORANGE );		break;
	case 4:  return( ANSI_DBLUE );		break;
	case 5:  return( ANSI_PURPLE );		break;
	case 6:  return( ANSI_CYAN );		break;
	case 7:  return( ANSI_GREY );		break;
	case 8:  return( ANSI_DGREY );		break;
	case 9:  return( ANSI_RED );		break;
	case 10: return( ANSI_GREEN );		break;
	case 11: return( ANSI_YELLOW );		break;
	case 12: return( ANSI_BLUE );		break;
	case 13: return( ANSI_PINK );		break;
	case 14: return( ANSI_LBLUE );		break;
	case 15: return( ANSI_WHITE );		break;

	default: return( ANSI_RESET );	break;
   }
}

/*
 *  Remove Colour Codes - By Komarosu
 *
 *  Colour code removal function, removes color codes but wont remove
 *  double instances of chars ( &&, }}. ^^ etc.).
 *  Not sure about memleaks, but i doubt any.
 *  Modified from a C++ Class for string modification.
 *
 */

char *remove_color(char *str)
{
    /* List of chars to remove */
    char *list = "&}^";
    int ii, jj, kk;

    char *aa = (char *) strdup(str);
    for (ii = 0, jj = strlen(aa); ii < jj; ii++)
    {
        for (kk = 0; kk < 3; kk++)
        {
            if (aa[ii] == list[kk])
            {
                if (aa[ii+1] != list[kk])
                  strcpy(& aa[ii], & aa[ii+2]);
                ii += 2;
                break;
            }
        }
    }
    return aa;
}

#define NAME(ch)        (IS_NPC(ch) ? ch->short_descr : ch->name)

char *act2_string(const char *format, CHAR_DATA *to, CHAR_DATA *ch,
		 const void *arg1, const void *arg2, int flags, const void *arg3)
{
  static char * const he_she  [] = { "it",  "he",  "she" };
  static char * const him_her [] = { "it",  "him", "her" };
  static char * const his_her [] = { "its", "his", "her" };
  static char buf[MAX_STRING_LENGTH];
  char fname[MAX_INPUT_LENGTH];
  char temp[MAX_STRING_LENGTH];
  char *point = buf;
  const char *str = format;
  const char *i;
  CHAR_DATA *vch = (CHAR_DATA *) arg2;
  OBJ_DATA *obj1 = (OBJ_DATA  *) arg1;
  OBJ_DATA *obj2 = (OBJ_DATA  *) arg2;

  if ( str[0] == '$' )
  	DONT_UPPER = FALSE;

  while ( *str != '\0' )
  {
    if ( *str != '$' )
    {
      *point++ = *str++;
      continue;
    }
    ++str;
    if ( !arg2 && *str >= 'A' && *str <= 'Z' )
    {
      bug( "Act: missing arg2 for code %c:", *str );
      bug( format );
      i = " <@@@> ";
    }
    else
    {
      switch ( *str )
      {
      default:  bug( "Act: bad code %c.", *str );
		i = " <@@@> ";						break;
      case 't': i = (char *) arg1;					break;
      case 'T': i = (char *) arg3;					break;
      case 'n':
              if ( ch->morph == NULL )
                  i = (to ? PERS ( ch, to ): NAME ( ch ) );
              else if ( !IS_SET( flags, STRING_IMM ) )
                  i = (to ? MORPHPERS (ch, to) : MORPHNAME (ch));
              else
              {
                sprintf(temp, "(MORPH) %s", (to ? PERS(ch,to):NAME(ch)));
                i = temp;
              }
              break;
      case 'N':
              if ( vch->morph == NULL )
                   i = (to ? PERS ( vch, to ) : NAME( vch ) );
              else if ( !IS_SET( flags, STRING_IMM ) )
                   i = (to ? MORPHPERS (vch, to) : MORPHNAME (vch));
              else
              {
                sprintf(temp, "(MORPH) %s", (to ? PERS(vch,to):NAME(vch)));
                i = temp;
              }
              break;

      case 'e': if (ch->sex > 2 || ch->sex < 0)
		{
		  bug("act_string: player %s has sex set at %d!", ch->name,
		      ch->sex);
		  i = "it";
		}
		else
		  i = he_she [URANGE(0,  ch->sex, 2)];
		break;
      case 'E': if (vch->sex > 2 || vch->sex < 0)
		{
		  bug("act_string: player %s has sex set at %d!", vch->name,
		      vch->sex);
		  i = "it";
		}
		else
		  i = he_she [URANGE(0, vch->sex, 2)];
		break;
      case 'm': if (ch->sex > 2 || ch->sex < 0)
		{
		  bug("act_string: player %s has sex set at %d!", ch->name,
		      ch->sex);
		  i = "it";
		}
		else
		  i = him_her[URANGE(0,  ch->sex, 2)];
		break;
      case 'M': if (vch->sex > 2 || vch->sex < 0)
		{
		  bug("act_string: player %s has sex set at %d!", vch->name,
		      vch->sex);
		  i = "it";
		}
		else
		  i = him_her[URANGE(0, vch->sex, 2)];
		break;
      case 's': if (ch->sex > 2 || ch->sex < 0)
		{
		  bug("act_string: player %s has sex set at %d!", ch->name,
		      ch->sex);
		  i = "its";
		}
		else
		  i = his_her[URANGE(0,  ch->sex, 2)];
		break;
      case 'S': if (vch->sex > 2 || vch->sex < 0)
		{
		  bug("act_string: player %s has sex set at %d!", vch->name,
		      vch->sex);
		  i = "its";
		}
		else
		  i = his_her[URANGE(0, vch->sex, 2)];
		break;
      case 'q': i = (to == ch) ? "" : "s";				break;
      case 'Q': i = (to == ch) ? "your" :
		    his_her[URANGE(0,  ch->sex, 2)];			break;
      case 'p': i = (!to || can_see_obj(to, obj1)
		  ? obj_short(obj1) : "something");			break;
      case 'P': i = (!to || can_see_obj(to, obj2)
		  ? obj_short(obj2) : "something");			break;
      case 'd':
        if ( !arg2 || ((char *) arg2)[0] == '\0' )
          i = "door";
        else
        {
          one_argument((char *) arg2, fname);
          i = fname;
        }
        break;
      }
    }
    ++str;
    while ( (*point = *i) != '\0' )
      ++point, ++i;

   /*  #0  0x80c6c62 in act_string (
    format=0x81db42e "$n has reconnected, kicking off old link.", to=0x0,
    ch=0x94fcc20, arg1=0x0, arg2=0x0, flags=2) at comm.c:2901 */
  }
  strcpy(point, "\n\r");
  if ( !DONT_UPPER )
     buf[0] = UPPER(buf[0]);
  return buf;
}
#undef NAME

void act2( sh_int AType, const char *format, CHAR_DATA *ch, const void *arg1, const void *arg2, int type, const void *arg3)
{
    char *txt;
    CHAR_DATA *to;
    CHAR_DATA *vch = (CHAR_DATA *)arg2;

    /*
     * Discard null and zero-length messages.
     */
    if ( !format || format[0] == '\0' )
	return;

    if ( !ch )
    {
	bug( "Act: null ch. (%s)", format );
	return;
    }

    if ( !ch->in_room )
      to = NULL;
    else if ( type == TO_CHAR )
      to = ch;
    else
      to = ch->in_room->first_person;

    /*
     * ACT_SECRETIVE handling
     */
    if ( IS_NPC(ch) && xIS_SET(ch->act, ACT_SECRETIVE) && type != TO_CHAR )
	return;

    if ( type == TO_VICT )
    {
	if ( !vch )
	{
	    bug( "Act: null vch with TO_VICT." );
	    bug( "%s (%s)", ch->name, format );
	    return;
	}
	if ( !vch->in_room )
	{
	    bug( "Act: vch in NULL room!" );
	    bug( "%s -> %s (%s)", ch->name, vch->name, format );
	    return;
	}
	to = vch;
/*	to = vch->in_room->first_person;*/
    }

    if ( MOBtrigger && type != TO_CHAR && type != TO_VICT && to )
    {
      OBJ_DATA *to_obj;

      txt = act2_string(format, NULL, ch, arg1, arg2, STRING_IMM, arg3);
      if ( HAS_PROG(to->in_room, ACT_PROG) )
        rprog_act_trigger(txt, to->in_room, ch, (OBJ_DATA *)arg1, (void *)arg2);
      for ( to_obj = to->in_room->first_content; to_obj;
            to_obj = to_obj->next_content )
        if ( HAS_PROG(to_obj->pIndexData, ACT_PROG) )
          oprog_act_trigger(txt, to_obj, ch, (OBJ_DATA *)arg1, (void *)arg2);
    }

    /* Anyone feel like telling me the point of looping through the whole
       room when we're only sending to one char anyways..? -- Alty */
    for ( ; to; to = (type == TO_CHAR || type == TO_VICT)
                     ? NULL : to->next_in_room )
    {
	if ((!to->desc
	&& (  IS_NPC(to) && !HAS_PROG(to->pIndexData, ACT_PROG) ))
	||   !IS_AWAKE(to) )
	    continue;

	if ( type == TO_CHAR && to != ch )
	    continue;
	if ( type == TO_VICT && ( to != vch || to == ch ) )
	    continue;
	if ( type == TO_ROOM && to == ch )
	    continue;
	if ( type == TO_NOTVICT && (to == ch || to == vch) )
	    continue;
	if ( type == TO_CANSEE && ( to == ch ||
	    (!IS_IMMORTAL(to) && !IS_NPC(ch) && (xIS_SET(ch->act, PLR_WIZINVIS)
	    && (get_trust(to) < (ch->pcdata ? ch->pcdata->wizinvis : 0) ) ) ) ) )
	    continue;

        if ( IS_IMMORTAL(to) )
            txt = act2_string (format, to, ch, arg1, arg2, STRING_IMM, arg3);
        else
       	    txt = act2_string (format, to, ch, arg1, arg2, STRING_NONE, arg3);

	if (to->desc)
	{
	  if ( AType == AT_COLORIZE )
	  {
	     if ( txt[0] == '&' )
	     	send_to_char_color( txt, to );
	     else
	     {
	        set_char_color(AT_MAGIC, to );
	     	send_to_char_color( txt, to );
	     }
	  }
	  else {
	     set_char_color(AType, to);
	     send_to_char_color( txt, to );
	  }
	}
	if (MOBtrigger)
        {
          /* Note: use original string, not string with ANSI. -- Alty */
	  mprog_act_trigger( txt, to, ch, (OBJ_DATA *)arg1, (void *)arg2 );
        }
    }
    MOBtrigger = TRUE;
    return;
}


#ifdef MCCP
/*
 * Ported to SMAUG by Garil of DOTDII Mud
 * aka Jesse DeFer <dotd@dotd.com>  http://www.dotd.com
 *
 * revision 1: MCCP v1 support
 * revision 2: MCCP v2 support
 * revision 3: Correct MMCP v2 support
 * revision 4: clean up of write_to_descriptor() suggested by Noplex@CB
 *
 * See the web site below for more info.
 */

/*
 * mccp.c - support functions for mccp (the Mud Client Compression Protocol)
 *
 * see http://homepages.ihug.co.nz/~icecube/compress/ and README.Rom24-mccp
 *
 * Copyright (c) 1999, Oliver Jowett <icecube@ihug.co.nz>.
 *
 * This code may be freely distributed and used if this copyright notice is
 * retained intact.
 */

void *zlib_alloc(void *opaque, unsigned int items, unsigned int size)
{
    return calloc(items, size);
}

void zlib_free(void *opaque, void *address)
{
    DISPOSE(address);
}

bool process_compressed(DESCRIPTOR_DATA *d)
{
    int iStart = 0, nBlock, nWrite, len;

    if (!d->out_compress)
        return TRUE;

    // Try to write out some data..
    len = d->out_compress->next_out - d->out_compress_buf;

    if (len > 0)
    {
        // we have some data to write
        for (iStart = 0; iStart < len; iStart += nWrite)
        {
            nBlock = UMIN (len - iStart, 4096);
            if ((nWrite = write(d->descriptor, d->out_compress_buf + iStart, nBlock)) < 0)
            {
                if (errno == EAGAIN ||
                    errno == ENOSR)
                    break;

                return FALSE;
            }

            if (!nWrite)
                break;
        }

        if (iStart)
        {
            // We wrote "iStart" bytes
            if (iStart < len)
                memmove(d->out_compress_buf, d->out_compress_buf+iStart, len - iStart);

            d->out_compress->next_out = d->out_compress_buf + len - iStart;
        }
    }

    return TRUE;
}

char enable_compress[] =
{
    IAC, SB, TELOPT_COMPRESS, WILL, SE, 0
};
char enable_compress2[] =
{
    IAC, SB, TELOPT_COMPRESS2, IAC, SE, 0
};

bool compressStart(DESCRIPTOR_DATA *d, unsigned char telopt)
{
    z_stream *s;
	char buf[MAX_INPUT_LENGTH];

    if (d->out_compress)
        return TRUE;

		sprintf( buf, "Starting compression for descriptor %d", d->descriptor );
		log_string_plus( buf, LOG_COMM, LEVEL_SUPREME );
//    bug("Starting compression for descriptor %d", d->descriptor);

    CREATE(s, z_stream, 1);
    CREATE(d->out_compress_buf, unsigned char, COMPRESS_BUF_SIZE);

    s->next_in = NULL;
    s->avail_in = 0;

    s->next_out = d->out_compress_buf;
    s->avail_out = COMPRESS_BUF_SIZE;

    s->zalloc = zlib_alloc;
    s->zfree  = zlib_free;
    s->opaque = NULL;

    if (deflateInit(s, 9) != Z_OK)
    {
        DISPOSE(d->out_compress_buf);
        DISPOSE(s);
        return FALSE;
    }

    if (telopt == TELOPT_COMPRESS)
        write_to_descriptor(d->descriptor, enable_compress, 0);
    else if (telopt == TELOPT_COMPRESS2)
        write_to_descriptor(d->descriptor, enable_compress2, 0);
    else
        bug("compressStart: bad TELOPT passed");

    d->compressing = telopt;
    d->out_compress = s;

    return TRUE;
}

bool compressEnd(DESCRIPTOR_DATA *d)
{
    unsigned char dummy[1];
	char buf[MAX_INPUT_LENGTH];

    if (!d->out_compress)
        return TRUE;

		sprintf( buf, "Stopping compression for descriptor %d", d->descriptor );
		log_string_plus( buf, LOG_COMM, LEVEL_SUPREME );
//    bug("Stopping compression for descriptor %d", d->descriptor);

    d->out_compress->avail_in = 0;
    d->out_compress->next_in = dummy;

    if (deflate(d->out_compress, Z_FINISH) != Z_STREAM_END)
        return FALSE;

    if (!process_compressed(d)) /* try to send any residual data */
        return FALSE;

    deflateEnd(d->out_compress);
    DISPOSE(d->out_compress_buf);
    DISPOSE(d->out_compress);
    d->compressing = 0;

    return TRUE;
}

void do_compress( CHAR_DATA *ch, char *argument )
{
	char arg[MAX_STRING_LENGTH];

    if (!ch->desc) {
        send_to_char("What descriptor?!\n", ch);
        return;
    }

	argument = one_argument( argument, arg );

    if (!str_cmp(arg, "on"))
    {
	    send_to_char("Initiating compression....", ch);
	    compressStart(ch->desc, TELOPT_COMPRESS2);
	    if (!ch->desc->out_compress)
	    	send_to_char("failure\n\r", ch);
		else
			send_to_char("success\n\r", ch);
	}
    else if (!str_cmp(arg, "off"))
    {
        send_to_char("Terminating compression...", ch);
        compressEnd(ch->desc);
	    if (!ch->desc->out_compress)
	    	send_to_char("success\n\r", ch);
		else
			send_to_char("failure\n\r", ch);
    }
    else
    {
    	send_to_char("Syntax: compression <on/off>\n\r", ch);
    	send_to_char("MCCP compression is currently: ", ch);
	    if (!ch->desc->out_compress)
	    	send_to_char("Off\n\r", ch);
		else
			send_to_char("On\n\r", ch);
	}

	return;
}
#endif
