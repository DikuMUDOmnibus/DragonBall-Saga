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
 *			         Hotboot support headers                          *
 ****************************************************************************/

#ifndef FCLOSE
   #define FCLOSE(fp)  fclose(fp); fp=NULL;
#endif

// #ifndef CH(d)
	#define CH(d)			((d)->original ? (d)->original : (d)->character)
// #endif

// #define HOTBOOT_FILE SYSTEM_DIR "copyover.dat" /* for hotboots */
#define HOTBOOT_FILE SYSTEM_DIR "copyover.dat" /* for hotboots */
#define EXE_FILE "../src/dbsaga"

/* warmboot code */
void hotboot_recover( void );

DECLARE_DO_FUN( do_hotboot ); /* Hotboot command - Samson 3-31-01 */
