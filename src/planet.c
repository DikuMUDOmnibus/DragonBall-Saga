

#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
/* #include <stdlib.h> */
#include <time.h>
#include "mud.h"


PLANET_DATA * first_planet;
PLANET_DATA * last_planet;

GUARD_DATA * first_guard;
GUARD_DATA * last_guard;


void	fread_planet	args( ( PLANET_DATA *planet, FILE *fp ) );
bool	load_planet_file	args( ( char *planetfile ) );
void	write_planet_list	args( ( void ) );

PLANET_DATA *get_planet( char *name )
{
    PLANET_DATA *planet;
    
    for ( planet = first_planet; planet; planet = planet->next )
       if ( !str_cmp( name, planet->name ) )
         return planet;
    return NULL;
}

void write_planet_list( )
{
    PLANET_DATA *tplanet;
    FILE *fpout;
    char filename[256];

    sprintf( filename, "%s%s", PLANET_DIR, PLANET_LIST );
    fpout = fopen( filename, "w" );
    if ( !fpout )
    {
	bug( "FATAL: cannot open planet.lst for writing!\n\r", 0 );
 	return;
    }	  
    for ( tplanet = first_planet; tplanet; tplanet = tplanet->next )
	fprintf( fpout, "%s\n", tplanet->filename );
    fprintf( fpout, "$\n" );
    fclose( fpout );
}

void save_planet( PLANET_DATA *planet )
{
    FILE *fp;
    char filename[256];
    char buf[MAX_STRING_LENGTH];

    if ( !planet )
    {
	bug( "save_planet: null planet pointer!", 0 );
	return;
    }
        
    if ( !planet->filename || planet->filename[0] == '\0' )
    {
	sprintf( buf, "save_planet: %s has no filename", planet->name );
	bug( buf, 0 );
	return;
    }
 
    sprintf( filename, "%s%s", PLANET_DIR, planet->filename );
    
    fclose( fpReserve );
    if ( ( fp = fopen( filename, "w" ) ) == NULL )
    {
    	bug( "save_planet: fopen", 0 );
    	perror( filename );
    }
    else
    {
        AREA_DATA *pArea;
        
	fprintf( fp, "#PLANET\n" );
	fprintf( fp, "Name         %s~\n",	planet->name		);
	fprintf( fp, "Filename     %s~\n",	planet->filename        );
	fprintf( fp, "BaseValue    %ld\n",	planet->base_value      );
	fprintf( fp, "Flags        %d\n",	planet->flags           );
	fprintf( fp, "PopSupport   %.0f\n",	planet->pop_support      );
	if ( planet->starsystem && planet->starsystem->name )
        	fprintf( fp, "Starsystem   %s~\n",	planet->starsystem->name);
	if ( planet->governed_by && planet->governed_by->name )
        	fprintf( fp, "GovernedBy   %s~\n",	planet->governed_by->name);
	for( pArea = planet->first_area ; pArea ; pArea = pArea->next_on_planet )
	    if (pArea->filename)
         	fprintf( fp, "Area         %s~\n",	pArea->filename  );
	fprintf( fp, "End\n\n"						);
	fprintf( fp, "#END\n"						);
    }
    fclose( fp );
    fpReserve = fopen( NULL_FILE, "r" );
    return;
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

void fread_planet( PLANET_DATA *planet, FILE *fp )
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

	case 'A':
	    if ( !str_cmp( word, "Area" ) )
	    {
	        char aName[MAX_STRING_LENGTH];
                AREA_DATA *pArea;
                	        
	     	sprintf (aName, fread_string(fp));
		for( pArea = first_area ; pArea ; pArea = pArea->next )
	          if (pArea->filename && !str_cmp(pArea->filename , aName ) )
	          {
	             pArea->planet = planet; 
	             LINK( pArea, planet->first_area, planet->last_area, next_on_planet, prev_on_planet);
	          }      
                fMatch = TRUE;
	    }
	    break;

	case 'B':
	    KEY( "BaseValue",	planet->base_value,		fread_number( fp ) );
	    break;

	case 'E':
	    if ( !str_cmp( word, "End" ) )
	    {
		if (!planet->name)
		  planet->name		= STRALLOC( "" );
		return;
	    }
	    break;

	case 'F':
	    KEY( "Filename",	planet->filename,		fread_string_nohash( fp ) );
	    KEY( "Flags",	planet->flags,		        fread_number( fp ) );
	    break;
	
	case 'G':
	    if ( !str_cmp( word, "GovernedBy" ) )
	    {
	     	planet->governed_by = get_clan ( fread_string(fp) );
                fMatch = TRUE;
	    }
	    break;
	
	case 'N':
	    KEY( "Name",	planet->name,		fread_string( fp ) );
	    break;
	
	case 'P':
	    KEY( "PopSupport",	planet->pop_support,		fread_number( fp ) );
	    break;

	case 'S':
	    if ( !str_cmp( word, "Starsystem" ) )
	    {
	     	planet->starsystem = starsystem_from_name ( fread_string(fp) );
                if (planet->starsystem)
                {
                     SPACE_DATA *starsystem = planet->starsystem;
                     
                     LINK( planet , starsystem->first_planet, starsystem->last_planet, next_in_system, prev_in_system );
                }
                fMatch = TRUE;
	    }
	    break;
	
	case 'T':
	    KEY( "Taxes",	planet->base_value,		fread_number( fp ) );
	    break;
    
	}
	
	if ( !fMatch )
	{
	    sprintf( buf, "Fread_planet: no match: %s", word );
	    bug( buf, 0 );
	}
	
    }
}

bool load_planet_file( char *planetfile )
{
    char filename[256];
    PLANET_DATA *planet;
    FILE *fp;
    bool found;

    CREATE( planet, PLANET_DATA, 1 );
    
    planet->governed_by = NULL;
    planet->next_in_system = NULL;
    planet->prev_in_system = NULL;
    planet->starsystem = NULL ;
    planet->first_area = NULL;
    planet->last_area = NULL;
    planet->first_guard = NULL;
    planet->last_guard = NULL;
    
    found = FALSE;
    sprintf( filename, "%s%s", PLANET_DIR, planetfile );

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
		bug( "Load_planet_file: # not found.", 0 );
		break;
	    }

	    word = fread_word( fp );
	    if ( !str_cmp( word, "PLANET"	) )
	    {
	    	fread_planet( planet, fp );
	    	break;
	    }
	    else
	    if ( !str_cmp( word, "END"	) )
	        break;
	    else
	    {
		char buf[MAX_STRING_LENGTH];

		sprintf( buf, "Load_planet_file: bad section: %s.", word );
		bug( buf, 0 );
		break;
	    }
	}
	fclose( fp );
    }

    if ( !found )
      DISPOSE( planet );
    else
      LINK( planet, first_planet, last_planet, next, prev );

    return found;
}

void load_planets( )
{
    FILE *fpList;
    char *filename;
    char planetlist[256];
    char buf[MAX_STRING_LENGTH];
    
    first_planet	= NULL;
    last_planet	= NULL;

    log_string( "Loading planets..." );

    sprintf( planetlist, "%s%s", PLANET_DIR, PLANET_LIST );
    fclose( fpReserve );
    if ( ( fpList = fopen( planetlist, "r" ) ) == NULL )
    {
	perror( planetlist );
	exit( 1 );
    }

    for ( ; ; )
    {
	filename = feof( fpList ) ? "$" : fread_word( fpList );
	log_string( filename );
	if ( filename[0] == '$' )
	  break;

	if ( !load_planet_file( filename ) )
	{
	  sprintf( buf, "Cannot load planet file: %s", filename );
	  bug( buf, 0 );
	}
    }
    fclose( fpList );
    log_string(" Done planets " );  
    fpReserve = fopen( NULL_FILE, "r" );
    return;
}

void do_setplanet( CHAR_DATA *ch, char *argument )
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    PLANET_DATA *planet;

    if ( IS_NPC( ch ) )
    {
	send_to_pager_color( "Huh?\n\r", ch );
	return;
    }

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( arg1[0] == '\0' )
    {
	send_to_pager_color( "Usage: setplanet <planet> <field> [value]\n\r", ch );
	send_to_pager_color( "\n\rField being one of:\n\r", ch );
	send_to_pager_color( " base_value flags\n\r", ch ); 
	send_to_pager_color( " name filename starsystem governed_by\n\r", ch );
	return;
    }

    planet = get_planet( arg1 );
    if ( !planet )
    {
	send_to_pager_color( "No such planet.\n\r", ch );
	return;
    }


    if ( !strcmp( arg2, "name" ) )
    {
	STRFREE( planet->name );
	planet->name = STRALLOC( argument );
	send_to_pager_color( "Done.\n\r", ch );
	save_planet( planet );
	return;
    }

    if ( !strcmp( arg2, "governed_by" ) )
    {
        CLAN_DATA *clan;
        clan = get_clan( argument );
        if ( clan )
        { 
           planet->governed_by = clan;
           send_to_pager_color( "Done.\n\r", ch ); 
       	   save_planet( planet );
        }
        else
           send_to_pager_color( "No such clan.\n\r", ch ); 
	return;
    }

    if ( !strcmp( arg2, "starsystem" ) )
    {
        SPACE_DATA *starsystem;
        
        if ((starsystem=planet->starsystem) != NULL)
          UNLINK(planet, starsystem->first_planet, starsystem->last_planet, next_in_system, prev_in_system);
	if ( (planet->starsystem = starsystem_from_name(argument)) )
        {
           starsystem = planet->starsystem;
           LINK(planet, starsystem->first_planet, starsystem->last_planet, next_in_system, prev_in_system);	
           send_to_pager_color( "Done.\n\r", ch );
	}
	else 
	       	send_to_pager_color( "No such starsystem.\n\r", ch );
	save_planet( planet );
	return;
    }

    if ( !strcmp( arg2, "filename" ) )
    {
	if ( planet->filename )
		DISPOSE( planet->filename );
	planet->filename = str_dup( argument );
	send_to_pager_color( "Done.\n\r", ch );
	save_planet( planet );
	write_planet_list( );
	return;
    }

    if ( !strcmp( arg2, "base_value" ) )
    {
	planet->base_value = atoi( argument );
	send_to_pager_color( "Done.\n\r", ch );
	save_planet( planet );
	return;
    }

    if ( !strcmp( arg2, "flags" ) )
    {
        char farg[MAX_INPUT_LENGTH];
        
        argument = one_argument( argument, farg); 
        
        if ( farg[0] == '\0' )
        {
           send_to_pager_color( "Possible flags: nocapture\n\r", ch );
           return;
        }
        
        for ( ; farg[0] != '\0' ; argument = one_argument( argument, farg) )
        {
            if ( !str_cmp( farg, "nocapture" ) )
               TOGGLE_BIT( planet->flags, PLANET_NOCAPTURE );
            else
               pager_printf_color( ch , "No such flag: %s\n\r" , farg );
	}
	send_to_pager_color( "Done.\n\r", ch );
	save_planet( planet );
	return;
    }

    do_setplanet( ch, "" );
    return;
}

void do_showplanet( CHAR_DATA *ch, char *argument )
{   
    PLANET_DATA *planet;

    if ( IS_NPC( ch ) )
    {
	send_to_pager_color( "Huh?\n\r", ch );
	return;
    }

    if ( argument[0] == '\0' )
    {
	send_to_pager_color( "Usage: showplanet <planet>\n\r", ch );
	return;
    }

    planet = get_planet( argument );
    if ( !planet )
    {
	send_to_pager_color( "No such planet.\n\r", ch );
	return;
    }

    pager_printf_color( ch, "%s\n\rFilename: %s\n\r",
    			planet->name,
    			planet->filename);
    return;
}

void do_makeplanet( CHAR_DATA *ch, char *argument )
{
    char filename[256];
    PLANET_DATA *planet;
    bool found;

    if ( !argument || argument[0] == '\0' )
    {
	send_to_pager_color( "Usage: makeplanet <planet name>\n\r", ch );
	return;
    }

    found = FALSE;
    sprintf( filename, "%s%s", PLANET_DIR, strlower(argument) );

    CREATE( planet, PLANET_DATA, 1 );
    LINK( planet, first_planet, last_planet, next, prev );
    planet->governed_by = NULL;
    planet->next_in_system = NULL;
    planet->prev_in_system = NULL;
    planet->starsystem = NULL ;
    planet->first_area = NULL;
    planet->last_area = NULL;
    planet->first_guard = NULL;
    planet->last_guard = NULL;
    planet->name		= STRALLOC( argument );
    planet->flags               = 0;
}

void do_planets( CHAR_DATA *ch, char *argument )
{
    PLANET_DATA *planet;
    int count = 0;
    AREA_DATA   *area;

    set_char_color( AT_WHITE, ch );
    for ( planet = first_planet; planet; planet = planet->next )
    {
        pager_printf_color( ch, "&WPlanet: &G%-15s   &WGoverned By: &G%s %s\n\r", 
                   planet->name ,
                   planet->governed_by ? planet->governed_by->name : "",
                   IS_SET(planet->flags, PLANET_NOCAPTURE ) ? "(permanent)" : "" );
        pager_printf_color( ch, "&WValue: &G%-10ld&W/&G%-10d   ", 
                   get_taxes(planet) , planet->base_value);
        pager_printf_color( ch, "&WPopulation: &G%-5d   &W Pop Support: &G%.1f\n\r", 
                   planet->population , planet->pop_support );
        if ( IS_IMMORTAL(ch) )
        {
          pager_printf_color( ch, "&WAreas: &G");
          for ( area = planet->first_area ; area ; area = area->next_on_planet )
             pager_printf_color( ch , "%s,  ", area->filename );
          pager_printf_color( ch, "\n\r" );
        }         
        pager_printf_color( ch, "\n\r" );
        
        count++;
    }

    if ( !count )
    {
	set_char_color( AT_BLOOD, ch);
        send_to_pager_color( "There are no planets currently formed.\n\r", ch );
    }
    
}

long get_taxes( PLANET_DATA *planet )
{
      long gain;
      
      gain = planet->base_value;
      gain += planet->base_value*planet->pop_support/100;
      gain += UMAX(0, planet->pop_support/10 * planet->population);
      
      return gain;
}
