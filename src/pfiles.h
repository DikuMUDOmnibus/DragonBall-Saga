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
 *			   Header info for pfile cleanup code 	                *
 ****************************************************************************/

/* Used to interact with other snippets */
#define PFILECODE 

#ifndef FCLOSE
  #define FCLOSE(fp)  fclose(fp); fp=NULL;
#endif

extern  time_t pfile_time;  
extern  HOUR_MIN_SEC * set_pfile_time; 
extern  struct  tm *new_pfile_time;
extern  time_t new_pfile_time_t;
extern  sh_int num_pfiles;

void check_pfiles	args( ( time_t reset ) );
void init_pfile_scan_time	args( ( void ) );

void save_timedata	args( ( void ) );
void fread_timedata args( ( FILE *fp ) );
bool load_timedata	args( ( void ) );

DECLARE_DO_FUN( do_pfiles );
