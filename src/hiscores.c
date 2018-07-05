/*
 *  Hiscore code
 *  Author: Cronel (cronel_kal@hotmail.com)
 *  of FrozenMUD (empire.digiunix.net 4000)
 *  nov 98
 *
 *  Permission to use and distribute this code is granted provided
 *  this header is retained and unaltered, and the distribution
 *  package contains all the original files unmodified.
 *  If you modify this code and use/distribute modified versions
 *  you must give credit to the original author(s).
 *
 */
#include <stdio.h>
#include "mud.h"

/* data */
typedef struct hiscore_entry HISCORE_ENTRY;
typedef struct hiscore HISCORE;
struct hiscore_entry
{
	char *name;
	int score;
	long double score2;

	HISCORE_ENTRY *next;
	HISCORE_ENTRY *prev;
};
struct hiscore
{
	char *keyword;
	char *desc;
	int vnum;
	sh_int max_length;
	sh_int length;
	HISCORE_ENTRY *first_entry; /* first entry is number 1, highest score */
	HISCORE_ENTRY *last_entry; /* last entry is number 10 or last, lowest */

	HISCORE		  *next;
	HISCORE		  *prev;
};

HISCORE *first_table;
HISCORE *last_table;

/*
void do_hiscoset( CHAR_DATA *ch, char *argument );
void do_hiscore( CHAR_DATA *ch, char *argument );

char *is_hiscore_obj( OBJ_DATA *obj );
void show_hiscore( char *keyword, CHAR_DATA *victim );

void adjust_hiscore( char *keyword, CHAR_DATA *ch, int score );
*/

/* local function declarations */
bool add_hiscore( char *keyword, char *name, int score );
bool add_hiscore_ld( char *keyword, char *name, long double score );
sh_int get_position( char *keyword, char *name );

void fix_table_length( HISCORE *table );
void create_hitable( char *keyword, char *desc );
bool destroy_hitable( char *keyword );
void save_hiscores( void );
void load_hiscores( void );
HISCORE *find_table( char *keyword );
HISCORE *load_hiscore_table( FILE *fp );



void do_hiscoset( CHAR_DATA *ch, char *argument )
/* Hiscoset command */
{
	char cmd[ MAX_INPUT_LENGTH ];
	char kwd[ MAX_INPUT_LENGTH ];
	char set_val[ MAX_INPUT_LENGTH ];
	HISCORE *table;
	HISCORE_ENTRY *entry;
	sh_int i;
	bool cmd_found;

	if (sysdata.stall_hiscores)
	{
		pager_printf(ch, "Hiscore tables are disabled.");
		return;
	}

	if( argument[0] == '\0' )
	{
		send_to_char("Syntax: hiscoset <command> [table keyword] [field] [value]\n\r", ch );
		send_to_char("\n\r", ch );
		send_to_char("    Command being one of:\n\r", ch );
		send_to_char("    list                     Lists all hiscore tables.\n\r", ch );
		send_to_char("    show <kwd>               Shows the data for that table.\n\r", ch );
		send_to_char("    create <kwd>             Create new table with that keyword.\n\r", ch );
		send_to_char("    destroy <kwd>            Destroy table with that keyword.\n\r", ch );
		send_to_char("    set <kwd> <field> <val>  Sets data for that table.\n\r", ch );
		send_to_char("\n\r", ch );
		send_to_char("    Field being one of:\n\r", ch );
		send_to_char("    keyword <str>            Set new keyword for the table.\n\r", ch );
		send_to_char("    vnum <num>               Set an object vnum for this table.\n\r", ch );
		send_to_char("    desc <str>               Set the description.\n\r", ch );
		send_to_char("    maxlength <num>          Set Max entries in the table.\n\r", ch );
		send_to_char("    entry <name> <score>     Change the score for a player.\n\r", ch );
		send_to_char("\n\r", ch );
		send_to_char("    See HELP HISCOSET for more details.\n\r", ch );
		return;
	}

	argument = one_argument( argument, cmd );

	if( !str_cmp( cmd, "list" ) )
	{
		send_to_char( "List of hiscore tables: \n\r", ch );
		for( table = first_table ; table ; table = table->next )
		{
			ch_printf( ch, "    Table '%s' (%s), %d entries, vnum is %d\n\r",
				table->keyword, table->desc, table->length, table->vnum );
		}
		if( !first_table )
			send_to_char( "    No tables found.\n\r", ch );
		return;
	}

	argument = one_argument( argument, kwd );
	if( kwd[0] != '\0' )
		table = find_table( kwd );
	else
		table = NULL;
	cmd_found = FALSE;

	if( !str_cmp( cmd, "create" ) && (cmd_found=TRUE) && kwd[0] != '\0' )
	{
		if( table != NULL )
		{
			send_to_char( "A table with that keyword already exists.\n\r", ch );
			return;
		}
		create_hitable( kwd, "Description not set yet" );
		ch_printf( ch, "Table '%s' created.\n\r", kwd );
		save_hiscores();
		return;
	}
	else if( !str_cmp( cmd, "show" )  && (cmd_found=TRUE) && table != NULL )
	{
		pager_printf( ch, "Hiscore table, keyword '%s':\n\r", kwd );
		pager_printf( ch, "Description: %s\n\r", table->desc );
		pager_printf( ch, "Vnum: %d    MaxLength: %d Length: %d\n\r", table->vnum, table->max_length, table->length );
		for( entry = table->first_entry, i=1 ; entry ; entry = entry->next, i++ )
		{
		if (!strcmp(table->keyword, "powerlevel")
			|| !strcmp(table->keyword, "plsaiyan")
			|| !strcmp(table->keyword, "plhuman")
			|| !strcmp(table->keyword, "plhalfbreed")
			|| !strcmp(table->keyword, "plnamek")
			|| !strcmp(table->keyword, "plandroid")
			|| !strcmp(table->keyword, "plicer")
			|| !strcmp(table->keyword, "plkaio")
			|| !strcmp(table->keyword, "pldemon")
			|| !strcmp(table->keyword, "plbio-android")
			|| !strcmp(table->keyword, "januaryladder")
			|| !strcmp(table->keyword, "februaryladder")
			|| !strcmp(table->keyword, "marchladder")
			|| !strcmp(table->keyword, "aprilladder")
			|| !strcmp(table->keyword, "mayladder")
			|| !strcmp(table->keyword, "juneladder")
			|| !strcmp(table->keyword, "julyladder")
			|| !strcmp(table->keyword, "augustladder")
			|| !strcmp(table->keyword, "septemberladder")
			|| !strcmp(table->keyword, "octoberladder")
			|| !strcmp(table->keyword, "novemberladder")
			|| !strcmp(table->keyword, "decemberladder"))
			pager_printf( ch, "%d. Name: %-15.15s    Score: %s\n\r",
				i, capitalize(entry->name), num_punct_ld(entry->score2) ) ;
		else
			pager_printf( ch, "%d. Name: %-15.15s    Score: %s\n\r",
				i, capitalize(entry->name), num_punct(entry->score) ) ;
		}
		return;
	}
	else if( !str_cmp( cmd, "destroy" ) && (cmd_found=TRUE) && table != NULL )
	{
		if( destroy_hitable( kwd ) )
			ch_printf( ch, "Table '%s' destroyed.\n\r", kwd );
		else
		{
			ch_printf( ch, "ERROR: Couldn't destroy table '%s'.\n\r", kwd );
			LINK( table, first_table, last_table, next, prev );
		}
		save_hiscores();
		return;
	}
	else if( !str_cmp( cmd, "set" ) )
		cmd_found = TRUE;

	if( !cmd_found )
	{
		ch_printf( ch, "Invalid command '%s' for hiscoset.\n\r", cmd );
		return;
	}
	if( kwd[0] == '\0' )
	{
		send_to_char( "Please specify a table keyword.\n\r", ch );
		return;
	}
	if( table == NULL )
	{
		ch_printf( ch, "No such table '%s'.\n\r", kwd );
		return;
	}

	/* If it reached here, its "hiscoset set" and table is valid */
	argument = one_argument( argument, cmd );
	if( cmd[0] == '\0' )
	{
		send_to_char( "Hiscoset set what?\n\r", ch );
		return;
	}

	cmd_found = FALSE;
	if( !str_cmp( cmd, "desc" ) && (cmd_found=TRUE) && argument[0] != '\0' )
	{
		STRFREE( table->desc );
		table->desc = STRALLOC( argument );
		ch_printf( ch, "Ok. Description for table '%s' is now '%s'.\n\r",
			kwd, argument );
		save_hiscores();
		return;
	}
	argument = one_argument( argument, set_val );
	if( !str_cmp( cmd, "keyword" ) && (cmd_found=TRUE) && set_val[0] != '\0' )
	{
		STRFREE( table->keyword );
		table->keyword = STRALLOC( set_val );
		ch_printf( ch, "Ok. Keyword for table '%s' is now '%s'.\n\r",
			kwd, set_val );
		save_hiscores();
		return;
	}
	else if( !str_cmp( cmd, "vnum" ) && (cmd_found=TRUE) && set_val[0] != '\0' )
	{
		if( is_number( set_val) )
		{
			table->vnum = atoi( set_val ) ;
			ch_printf( ch, "Ok. Vnum for table '%s' is now '%d'.\n\r",
				kwd, table->vnum );
			save_hiscores();
		}
		else
			send_to_char( "Argument for set vnum must be numeric.\n\r", ch );
		return;
	}
	else if( !str_cmp( cmd, "maxlength" ) && (cmd_found=TRUE) && set_val[0] != '\0' )
	{
		if( is_number( set_val) )
		{
			table->max_length = atoi( set_val ) ;
			ch_printf( ch, "Ok. Max length for table '%s' is now '%d'.\n\r",
				kwd, table->max_length );
			fix_table_length( table );
			save_hiscores();
		}
		else
			send_to_char( "Argument for set maxlength must be numeric.\n\r", ch );
		return;
	}
	else if( !str_cmp( cmd, "entry" ) )
		cmd_found = TRUE;

	if( !cmd_found )
	{
		ch_printf( ch, "Invalid value '%s' for hiscoset set.\n\r",
			cmd );
		return;
	}
	if( set_val[0] == '\0' )
	{
		ch_printf( ch, "Hiscoset set %s to what?.\n\r", cmd );
		return;
	}

	/* at this point, its hiscoset set entry .. */
	if( argument == '\0' || !is_number( argument) )
	{
		send_to_char( "Second argument to set entry must be numberic.\n\r", ch );
		return;
	}

	ch_printf( ch, "New score for %s set to %s.\n\r", set_val, argument );

	if( add_hiscore( kwd, set_val, atoi(argument) ) )
	{
		ch_printf( ch, "New position is %d.\n\r",
			get_position( kwd, set_val ) );
	}
	else
		ch_printf( ch, "They are out of the table.\n\r" );
	save_hiscores();
}

void do_hiscore( CHAR_DATA *ch, char *argument )
/* Hiscore command */
{
	HISCORE *table;
	bool some_table;
	OBJ_INDEX_DATA *oindex;

	if (sysdata.stall_hiscores)
	{
		pager_printf(ch, "Hiscore tables are disabled, most likely due to a bug.\n\r");
		pager_printf(ch, "Please notify an Admin of this on the 'bug' message board.\n\r");
		return;
	}

	if( argument[0] == '\0' )
	{
		sysdata.outBytesFlag = LOGBOUTINFORMATION;
		some_table = FALSE;
		send_to_char( "Hiscore tables in this game:\n\r", ch );
		for( table = first_table ; table ; table = table->next )
		{
			if( table->vnum == -1 )
			{
				ch_printf( ch, "  &W%40s&w: keyword '&W%s&w'\n\r", table->desc, table->keyword );
				some_table = TRUE;
			}
			else
			{
				oindex = get_obj_index( table->vnum );
				if( oindex != NULL )
				{
					ch_printf( ch, "  %s: in object %s\n\r",
						table->desc,
						oindex->short_descr );
					some_table = TRUE;
				}
			}
		}
		if( !some_table )
			send_to_char( "  None.\n\r", ch );
		sysdata.outBytesFlag = LOGBOUTNORM;
		return;
	}

	table = find_table( argument );
	if( table == NULL )
	{
		HISCORE_ENTRY *entry;
		int num = 0;
		bool tablefound = FALSE;

		sysdata.outBytesFlag = LOGBOUTINFORMATION;

		for( table = first_table ; table ; table = table->next )
		{
			if( table->vnum == -1 )
			{
				num = 1;
				for( entry = table->first_entry ; entry ; entry = entry->next )
				{
					if (!strcmp(entry->name, capitalize(argument)))
					{
						pager_printf_color(ch, "&w%s\n\r", table->desc);
						pager_printf_color(ch, "&W  ---------------\n\r");
						if (!strcmp(table->keyword, "powerlevel")
							|| !strcmp(table->keyword, "plsaiyan")
							|| !strcmp(table->keyword, "plhuman")
							|| !strcmp(table->keyword, "plhalfbreed")
							|| !strcmp(table->keyword, "plnamek")
							|| !strcmp(table->keyword, "plandroid")
							|| !strcmp(table->keyword, "plkaio")
                                                        || !strcmp(table->keyword, "pldemon")
							|| !strcmp(table->keyword, "plicer")
							|| !strcmp(table->keyword, "plbio-android")
							|| !strcmp(table->keyword, "januaryladder")
							|| !strcmp(table->keyword, "februaryladder")
							|| !strcmp(table->keyword, "marchladder")
							|| !strcmp(table->keyword, "aprilladder")
							|| !strcmp(table->keyword, "mayladder")
							|| !strcmp(table->keyword, "juneladder")
							|| !strcmp(table->keyword, "julyladder")
							|| !strcmp(table->keyword, "augustladder")
							|| !strcmp(table->keyword, "septemberladder")
							|| !strcmp(table->keyword, "octoberladder")
							|| !strcmp(table->keyword, "novemberladder")
							|| !strcmp(table->keyword, "decemberladder"))
							pager_printf_color(ch, "%s  #%2d) %s\n\r",
							num <= 3 ? "&R" : "&Y",
							num, entry->name);
						else
							pager_printf_color(ch, "%s  #%2d) %s %s\n\r",
							num <= 3 ? "&R" : "&Y",
							num, entry->name, num_punct(entry->score));
						tablefound = TRUE;
						pager_printf_color(ch, "\n\r");
						break;
					}
					num++;
				}
			}
		}
		if (!tablefound)
			send_to_char( "No such hiscore table.\n\r", ch );

		sysdata.outBytesFlag = LOGBOUTNORM;
		return;
	}

 	if( table->vnum != -1 && !IS_IMMORTAL(ch) )
	{
		send_to_char( "That hiscore table is attached to an object. Go see the object.\n\r", ch );
		return;
	}

	sysdata.outBytesFlag = LOGBOUTINFORMATION;
	show_hiscore( table->keyword, ch );
	sysdata.outBytesFlag = LOGBOUTNORM;
}

char *is_hiscore_obj( OBJ_DATA *obj )
/* If the given object is marked as a hiscore table object, it returns
 * a pointer to the keyword for that table; otherwise returns NULL */
{
	HISCORE *table;

	for( table = first_table ; table ; table = table->next )
	{
		if( obj->pIndexData->vnum == table->vnum )
			return table->keyword;
	}
	return NULL;
}

/*
(@)                                                 (@)
| |-------------------------------------------------| |
| |                                                 | |
| |      Greatest Pkillers of Frozen Wasteland      | |
| |                                                 | |
| |      1. Darklord ..........|.......... 1000     | |
| |      2. Pepto .............|............ 50     | |
| |     10. 12345678901234567890123456789012345     | |
| |     1234                                        | |
| |-------------------------------------------------| |
(@)                                                 (@)
   width =
01234567890123456789012345678901234567890123456789012345678901234567890123456789
0         1         2         3         4         5         6         7
3 + 5 + text + 5 + 3 = 16 + text
text = 4 + 20 + 15 = 39
39 + 16 = 49+6 = 55
*/
/* this is width of text only,
 * add 13 to get actual width */

void show_hiscore( char *keyword, CHAR_DATA *ch )
/* Shows a hiscore table to a player in a (more or less) nicely formatted
 * way. */
{
	HISCORE *table;
	HISCORE_ENTRY *entry;
	sh_int num, len;
	char buf[ MAX_STRING_LENGTH ];
	bool odd;

	table = find_table( keyword );
	if( table == NULL )
	{
		bug( "show_hiscore: no such table '%s'", keyword );
		return;
	}

	pager_printf_color( ch, "\n\r&g(&r@&g)                                                 (&r@&g)&w\n\r" );
	pager_printf_color( ch, "&g| |-------------------------------------------------| |&w\n\r" );
	pager_printf_color( ch, "&g| |                                                 | |&w\n\r" );

	sprintf( buf, "%s", table->desc );
	len = strlen( buf );
	len = (39 - len);
	odd = (len%2) == 1;
	len /= 2;
	if( len < 0 )
		len = 0;
	for( num=0; num<len ; num++ )
		buf[num] = ' ';
	buf[num] = '\0';
	pager_printf_color( ch, "&g| |     &G%s%s%s%s     &g| |&w\n\r",
		buf, table->desc,
		odd ? " " : "", buf );

	pager_printf_color( ch, "&g| |                                                 | |&w\n\r" );

	num = 1;
	for( entry = table->first_entry ; entry ; entry = entry->next )
	{
		if (!strcmp(table->keyword, "powerlevel")
			|| !strcmp(table->keyword, "plsaiyan")
			|| !strcmp(table->keyword, "plhuman")
			|| !strcmp(table->keyword, "plhalfbreed")
			|| !strcmp(table->keyword, "plnamek")
			|| !strcmp(table->keyword, "plandroid")
			|| !strcmp(table->keyword, "plicer")
                        || !strcmp(table->keyword, "plkaio")
                        || !strcmp(table->keyword, "pldemon")
			|| !strcmp(table->keyword, "plbio-android")
			|| !strcmp(table->keyword, "januaryladder")
			|| !strcmp(table->keyword, "februaryladder")
			|| !strcmp(table->keyword, "marchladder")
			|| !strcmp(table->keyword, "aprilladder")
			|| !strcmp(table->keyword, "mayladder")
			|| !strcmp(table->keyword, "juneladder")
			|| !strcmp(table->keyword, "julyladder")
			|| !strcmp(table->keyword, "augustladder")
			|| !strcmp(table->keyword, "septemberladder")
			|| !strcmp(table->keyword, "octoberladder")
			|| !strcmp(table->keyword, "novemberladder")
			|| !strcmp(table->keyword, "decemberladder"))
		pager_printf_color( ch, "&g| |               %s%2d. %-20.20s          &g| |\n\r",
			num <= 3 ? "&R" : num <= 10 ? "&Y" : num <= 25 ? "&w" : "&p",
			num, capitalize(entry->name) );
		else
		pager_printf_color( ch, "&g| |     %s%2d. %-20.20s%15s     &g| |\n\r",
			num <= 3 ? "&R" : num <= 10 ? "&Y" : num <= 25 ? "&w" : "&p",
			num, capitalize(entry->name), num_punct(entry->score) );
		num++;
	}

	pager_printf_color( ch, "&g| |                                                 | |&w\n\r" );
	pager_printf_color( ch, "&g| |-------------------------------------------------| |&w\n\r" );
	pager_printf_color( ch, "&g(&r@&g)                                                 (&r@&g)&w\n\r" );
}

void adjust_hiscore( char *keyword, CHAR_DATA *ch, int score )
/* Adjusts the hiscore for that character in that table
 * and sends message to the character and to the whole mud
 * if the player entered the table */
{
	char buf[ MAX_STRING_LENGTH ];
	sh_int pos, old_pos;
	HISCORE *table;

	if (IS_IMMORTAL(ch))
		score = 0;
	if (xIS_SET(ch->act, PLR_NO_HISCORE))
		score = 0;
	if (IS_HC(ch) && str_cmp(keyword, "bounty"))
		return;
	if (score == 0)
		return;

	old_pos = get_position( keyword, ch->name);
	add_hiscore( keyword, ch->name, score );
	pos = get_position( keyword, ch->name );
	table = find_table( keyword );
	if( pos != -1 && pos > old_pos && table != NULL )
	{
		ch_printf( ch, "You have reached position %d in the table of '%s'.\n\r",
			pos, table->desc );
		if (pos <= 10)
		{
		sprintf( buf, "%s has reached position %d in the table of '%s'",
			ch->name, pos, table->desc );
		do_info(ch, buf);
		}
	}
}

void adjust_hiscore_ld( char *keyword, CHAR_DATA *ch, long double score )
/* Adjusts the hiscore for that character in that table
 * and sends message to the character and to the whole mud
 * if the player entered the table */
{
	char buf[ MAX_STRING_LENGTH ];
	sh_int pos, old_pos;
	HISCORE *table;

	if (IS_IMMORTAL(ch))
		score = 0;
	if (xIS_SET(ch->act, PLR_NO_HISCORE))
		score = 0;
	if (IS_HC(ch))
		return;
	if (score == 0)
		return;

	old_pos = get_position( keyword, ch->name);
	add_hiscore_ld( keyword, ch->name, score );
	pos = get_position( keyword, ch->name );
	table = find_table( keyword );
	if( pos != -1 && pos > old_pos && table != NULL )
	{
		ch_printf( ch, "You have reached position %d in the table of '%s'.\n\r",
			pos, table->desc );
		/* replace for info channel */
		if (pos <= 10)
		{
		sprintf( buf, "%s has reached position %d in the table of '%s'",
			ch->name, pos, table->desc );
		do_info(ch, buf);
		}
	}
}

bool add_hiscore( char *keyword, char *name, int score )
/* Sets the score for 'name'. If name is already on the table, the old
 * score is discarded and the new one takes effect.
 * Returns TRUE if as a result of this call, name was added to the
 * table or remained there. */
{
	HISCORE *table;
	HISCORE_ENTRY *entry, *new_entry;
	sh_int i;
	bool added;

	if (sysdata.stall_hiscores)
		return FALSE;

	table = find_table( keyword );
	if( table == NULL )
	{
		bug( "add_hiscore: table '%s' not found", keyword );
		sysdata.stall_hiscores = TRUE;
		do_ainfo(NULL, "&RError in hiscore tables.  Hiscore tables are disabled.");
		return FALSE;
	}

	for( entry = table->first_entry ; entry ; entry = entry->next )
	{
		if( !str_cmp( entry->name, name ) )
		{
			UNLINK( entry, table->first_entry, table->last_entry, next, prev );
			STRFREE( entry->name );
			DISPOSE( entry );
			table->length--;
			break;
		}
	}

	added = FALSE;
	new_entry = NULL;
	for( i=1, entry = table->first_entry ; i<= table->max_length ; entry = entry->next, i++ )
	{
		if( !entry )
		/* there are empty slots at end of list, add there */
		{
			CREATE( new_entry, HISCORE_ENTRY, 1 );
			new_entry->name = STRALLOC( name );
			new_entry->score = score;
			LINK( new_entry, table->first_entry, table->last_entry, next, prev );
			table->length++;
			added = TRUE;
			break;
		}
		else if( score > entry->score )
		{
			CREATE( new_entry, HISCORE_ENTRY, 1 );
			new_entry->name = STRALLOC( name );
			new_entry->score = score;
			INSERT( new_entry, entry, table->first_entry, next, prev );
			table->length++;
			added = TRUE;
			break;
		}
	}

	fix_table_length( table );

	if( added )
		save_hiscores ();

	return added;
}

bool add_hiscore_ld( char *keyword, char *name, long double score )
/* Sets the score for 'name'. If name is already on the table, the old
 * score is discarded and the new one takes effect.
 * Returns TRUE if as a result of this call, name was added to the
 * table or remained there. */
{
	HISCORE *table;
	HISCORE_ENTRY *entry, *new_entry;
	sh_int i;
	bool added;

	if (sysdata.stall_hiscores)
		return FALSE;

	table = find_table( keyword );
	if( table == NULL )
	{
		bug( "add_hiscore: table '%s' not found", keyword );
		sysdata.stall_hiscores = TRUE;
		do_ainfo(NULL, "&RError in hiscore tables.  Hiscore tables are disabled.");
		return FALSE;
	}

	for( entry = table->first_entry ; entry ; entry = entry->next )
	{
		if( !str_cmp( entry->name, name ) )
		{
			UNLINK( entry, table->first_entry, table->last_entry, next, prev );
			STRFREE( entry->name );
			DISPOSE( entry );
			table->length--;
			break;
		}
	}

	added = FALSE;
	new_entry = NULL;
	for( i=1, entry = table->first_entry ; i<= table->max_length ; entry = entry->next, i++ )
	{
		if( !entry )
		/* there are empty slots at end of list, add there */
		{
			CREATE( new_entry, HISCORE_ENTRY, 1 );
			new_entry->name = STRALLOC( name );
			new_entry->score2 = score;
			LINK( new_entry, table->first_entry, table->last_entry, next, prev );
			table->length++;
			added = TRUE;
			break;
		}
		else if( score > entry->score2 )
		{
			CREATE( new_entry, HISCORE_ENTRY, 1 );
			new_entry->name = STRALLOC( name );
			new_entry->score2 = score;
			INSERT( new_entry, entry, table->first_entry, next, prev );
			table->length++;
			added = TRUE;
			break;
		}
	}

	fix_table_length( table );

	if( added )
		save_hiscores ();

	return added;
}


void fix_table_length( HISCORE *table )
/* Chops entries at the end of the table if the table
 * has exceeded its maximum length */
{
	HISCORE_ENTRY *entry;

	while( table->length > table->max_length )
	{
		table->length--;
		entry = table->last_entry;
		UNLINK( entry, table->first_entry, table->last_entry, next, prev );
		STRFREE( entry->name );
		DISPOSE( entry );
	}
}

sh_int get_position( char *keyword, char *name )
/* Returns the position of 'name' within the table of that keyword, or
 * -1 if 'name' is not in that table */
{
	sh_int i;
	HISCORE *table;
	HISCORE_ENTRY *entry;

	table = find_table( keyword );
	if( !table )
		return -1;

	i = 1;
	for( entry = table->first_entry ; entry ; entry = entry->next )
	{
		if( !str_cmp( entry->name, name) )
			return i;
		i++;
	}

	return -1;
}

HISCORE *find_table( char *keyword )
/* Returns a pointer to the table for the given keyword
 * or NULL if there's no such table */
{
	HISCORE *table;

	for( table = first_table ; table ; table = table->next )
	{
		if( !str_cmp( table->keyword, keyword ) )
			return table;
	}
	return NULL;
}

void create_hitable( char *keyword, char *desc )
/* Creates a new hiscore table with the given keyword and description */
{
	HISCORE *new_table;

	CREATE(new_table, HISCORE, 1 );

	new_table->keyword = STRALLOC( keyword );
	new_table->desc = STRALLOC( desc );
	new_table->vnum = -1;
	new_table->max_length = 10;
	new_table->length = 0;
	new_table->first_entry = NULL;
	new_table->last_entry = NULL;
	LINK( new_table, first_table, last_table, next, prev );
}

bool destroy_hitable( char *keyword )
/* Destroyes a given hiscore table. Returns FALSE if error */
{
	HISCORE *table;
	HISCORE_ENTRY *entry, *nentry;

	table = find_table( keyword );
	if( !table )
		return FALSE;

	UNLINK( table, first_table, last_table, next, prev );
	STRFREE( table->keyword );
	STRFREE( table->desc );
	for( entry = table->first_entry ; entry ; entry = nentry )
	{
		nentry = entry->next;
		UNLINK( entry, table->first_entry, table->last_entry, next, prev );
		STRFREE( entry->name );
		DISPOSE( entry );
	}
	DISPOSE( table );
	return TRUE;
}

void save_hiscores( void )
/* Saves all hiscore tables */
{
	FILE *fp;
	char filename[ MAX_INPUT_LENGTH ];
	HISCORE *table;
	HISCORE_ENTRY *entry;

	sprintf( filename, "%shiscores.dat", SYSTEM_DIR );

	fclose( fpReserve );
	fp = fopen( filename, "w" );
	if( fp == NULL )
	{
		bug( "save_hiscores: fopen" );
		return;
	}

	fprintf( fp, "#HISCORES\n" );

	for( table = first_table ; table ; table = table->next )
	{
		fprintf( fp, "#TABLE\n" );
		fprintf( fp, "Keyword    %s\n", table->keyword );
		fprintf( fp, "Desc       %s~\n", table->desc );
		fprintf( fp, "Vnum       %d\n", table->vnum );
		fprintf( fp, "MaxLength  %d\n", table->max_length );
		for( entry = table->first_entry ; entry ; entry = entry->next )
		{
			if (entry->score == 0 && entry->score2 == 0)
				continue;

			if (!strcmp(table->keyword, "powerlevel")
				|| !strcmp(table->keyword, "plsaiyan")
				|| !strcmp(table->keyword, "plhuman")
				|| !strcmp(table->keyword, "plhalfbreed")
				|| !strcmp(table->keyword, "plnamek")
				|| !strcmp(table->keyword, "plandroid")
				|| !strcmp(table->keyword, "plicer")
                                || !strcmp(table->keyword, "plkaio")
                                || !strcmp(table->keyword, "pldemon")
				|| !strcmp(table->keyword, "plbio-android")
				|| !strcmp(table->keyword, "januaryladder")
				|| !strcmp(table->keyword, "februaryladder")
				|| !strcmp(table->keyword, "marchladder")
				|| !strcmp(table->keyword, "aprilladder")
				|| !strcmp(table->keyword, "mayladder")
				|| !strcmp(table->keyword, "juneladder")
				|| !strcmp(table->keyword, "julyladder")
				|| !strcmp(table->keyword, "augustladder")
				|| !strcmp(table->keyword, "septemberladder")
				|| !strcmp(table->keyword, "octoberladder")
				|| !strcmp(table->keyword, "novemberladder")
				|| !strcmp(table->keyword, "decemberladder"))
				fprintf( fp, "Entry2     %s %.0Lf\n", entry->name, entry->score2);
			else
				fprintf( fp, "Entry      %s %d\n", entry->name, entry->score);
		}
		fprintf( fp, "End\n\n" );
	}

	fprintf( fp, "#END\n" );

	fclose( fp );
	fpReserve = fopen( NULL_FILE, "r" );
}

void load_hiscores( void )
/* Loads all hiscore tables */
{
	char filename[MAX_INPUT_LENGTH];
	FILE *fp;
	HISCORE *new_table;

	sprintf(filename, "%shiscores.dat", SYSTEM_DIR);

	fp = fopen(filename, "r");
	if( fp == NULL)
		return;

	for(;;)
	{
		char letter;
		char *word;

		letter = fread_letter(fp);

		if(letter != '#')
		{
			bug("load_hiscores: # not found");
			return;
		}

		word = fread_word(fp);

		if(!str_cmp(word, "HISCORES"))
			;
		else if(!str_cmp(word, "END"))
			break;
		else if(!str_cmp(word, "TABLE"))
		{
			new_table = load_hiscore_table( fp );
			if( !new_table )
				continue;
			LINK( new_table, first_table, last_table, next, prev );
		}
		else
		{
			bug("load_hiscores: unknown field");
			break;
		}
	}

	fclose( fp );
	return;
}

HISCORE *load_hiscore_table( FILE *fp )
/* Loads one hiscore table from the file and returns it */
{
	char *word;
	HISCORE *new_table;
	HISCORE_ENTRY *new_entry;
	sh_int entry_count;

	CREATE( new_table, HISCORE, 1 );
	entry_count = 0;
	for(;;)
	{
		word = fread_word( fp );
		if( !str_cmp( word, "End" ))
			break;
		else if( !str_cmp( word, "Keyword" ))
			new_table->keyword = STRALLOC( fread_word(fp) );
		else if( !str_cmp( word, "Desc" ))
			new_table->desc = fread_string( fp ) ;
		else if( !str_cmp( word, "Vnum" ))
			new_table->vnum = fread_number( fp );
		else if( !str_cmp( word, "MaxLength" ))
			new_table->max_length = fread_number( fp );
		else if( !str_cmp( word, "Entry" ))
		{
			CREATE( new_entry, HISCORE_ENTRY, 1 );
			new_entry->name = STRALLOC( fread_word(fp) );
			new_entry->score = fread_number( fp );
			/* LINK adds at the end of the table. */
			LINK( new_entry, new_table->first_entry, new_table->last_entry, next, prev );
			entry_count++;
		}
		else if( !str_cmp( word, "Entry2" ))
		{
			CREATE( new_entry, HISCORE_ENTRY, 1 );
			new_entry->name = STRALLOC( fread_word(fp) );
			new_entry->score2 = fread_number_ld( fp );
			/* LINK adds at the end of the table. */
			LINK( new_entry, new_table->first_entry, new_table->last_entry, next, prev );
			entry_count++;
		}
		else
		{
			bug("load_hiscore_table: unknown field");
			break;
		}
	}
	new_table->length = entry_count;
	if( entry_count > new_table->max_length )
	{
		bug( "load_hiscore_table: extra entries in table. fixed max_length" );
		new_table->max_length = entry_count;
	}

	if( new_table->keyword == NULL )
		new_table->keyword = STRALLOC( "error" );
	if( new_table->desc == NULL )
		new_table->desc = STRALLOC( "Error: no description" );

	return new_table;
}

void ladderTableClear(int mon)
{
	HISCORE *table;
//	HISCORE_ENTRY *entry;
	/* new month, so wipe old ladder data for this month */
	switch (mon)
	{
		default:
		case 0:
			table = find_table( "januaryladder");
			break;
		case 1:
			table = find_table( "februaryladder");
			break;
		case 2:
			table = find_table( "marchladder");
			break;
		case 3:
			table = find_table( "aprilladder");
			break;
		case 4:
			table = find_table( "mayladder");
			break;
		case 5:
			table = find_table( "juneladder");
			break;
		case 6:
			table = find_table( "julyladder");
			break;
		case 7:
			table = find_table( "augustladder");
			break;
		case 8:
			table = find_table( "septemberladder");
			break;
		case 9:
			table = find_table( "octoberladder");
			break;
		case 10:
			table = find_table( "novemberladder");
			break;
		case 11:
			table = find_table( "decemberladder");
			break;
	}

/*
	if (table->length > 0)
	{
	for( entry = table->first_entry ; entry ; entry = entry->next )
	{
		UNLINK( entry, table->first_entry, table->last_entry, next, prev );
		STRFREE( entry->name );
		DISPOSE( entry );
		table->length--;
	}
	if (table->length != 0)
		bug("Error while reseting hiscore table: length was no 0");
	}
*/
	return;
}

void do_clearhiscore( CHAR_DATA *ch, char *argument )
{
    char kwd[ MAX_INPUT_LENGTH ];
    HISCORE *table;
    HISCORE_ENTRY *entry;
    HISCORE_ENTRY *entry_next;
    int count = 0;

    if( IS_NPC(ch) )
	return;

    if( !IS_IMMORTAL(ch) || ch->level < 63 )
    {
	ch_printf(ch,"Huh?");
	return;
    }

    argument = one_argument(argument,kwd);

    if( (table = find_table( kwd )) == NULL )
    {
	ch_printf(ch,"No such hiscore.\n\r");
	return;
    }
    for( entry = table->first_entry; entry; entry = entry_next )
    {
	entry_next = entry->next;
	UNLINK( entry, table->first_entry, table->last_entry, next, prev );
	DISPOSE( entry );
	count++;
    }
    if( count > 0 )
	ch_printf(ch,"%d entries cleared.\n\r",count);
    else
	ch_printf(ch,"There are no entries in that board.\n\r");

    return;
}

/*eof*/
