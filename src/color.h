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
 *                          Color Code Header Information                   *
 ****************************************************************************/

#ifndef	MSL
   #define MSL MAX_STRING_LENGTH
#endif

#ifndef	MIL
   #define MIL MAX_INPUT_LENGTH
#endif

void set_char_color( sh_int	AType, CHAR_DATA *ch );
void set_pager_color( sh_int AType,	CHAR_DATA *ch );
char *color_str( sh_int	AType, CHAR_DATA *ch );

/* These are the ANSI codes	for	foreground text	colors */
#define	ANSI_BLACK		"\e[0;30m"
#define	ANSI_DRED		"\e[0;31m"
#define	ANSI_DGREEN		"\e[0;32m"
#define	ANSI_ORANGE		"\e[0;33m"
#define	ANSI_DBLUE		"\e[0;34m"
#define	ANSI_PURPLE		"\e[0;35m"
#define	ANSI_CYAN		"\e[0;36m"
#define	ANSI_GREY		"\e[0;37m"
#define	ANSI_DGREY		"\e[1;30m"
#define	ANSI_RED		"\e[1;31m"
#define	ANSI_GREEN		"\e[1;32m"
#define	ANSI_YELLOW		"\e[1;33m"
#define	ANSI_BLUE		"\e[1;34m"
#define	ANSI_PINK		"\e[1;35m"
#define	ANSI_LBLUE		"\e[1;36m"
#define	ANSI_WHITE		"\e[1;37m"
#define	ANSI_RESET		"\e[0m"

/* These are the ANSI codes	for	blinking foreground	text colors	*/
#define	BLINK_BLACK		"\e[0;5;30m"
#define	BLINK_DRED		"\e[0;5;31m"
#define	BLINK_DGREEN	"\e[0;5;32m"
#define	BLINK_ORANGE	"\e[0;5;33m"
#define	BLINK_DBLUE		"\e[0;5;34m"
#define	BLINK_PURPLE	"\e[0;5;35m"
#define	BLINK_CYAN		"\e[0;5;36m"
#define	BLINK_GREY		"\e[0;5;37m"
#define	BLINK_DGREY		"\e[1;5;30m"
#define	BLINK_RED		"\e[1;5;31m"
#define	BLINK_GREEN		"\e[1;5;32m"
#define	BLINK_YELLOW	"\e[1;5;33m"
#define	BLINK_BLUE		"\e[1;5;34m"
#define	BLINK_PINK		"\e[1;5;35m"
#define	BLINK_LBLUE		"\e[1;5;36m"
#define	BLINK_WHITE		"\e[1;5;37m"

/* These are the ANSI codes	for	background colors */
#define	BACK_BLACK		"\e[40m"
#define	BACK_DRED		"\e[41m"
#define	BACK_DGREEN		"\e[42m"
#define	BACK_ORANGE		"\e[43m"
#define	BACK_DBLUE		"\e[44m"
#define	BACK_PURPLE		"\e[45m"
#define	BACK_CYAN		"\e[46m"
#define	BACK_GREY		"\e[47m"

/* Other miscelaneous ANSI tags	that can be	used */
#define	ANSI_UNDERLINE	"\e[4m"	/* Underline text */
#define	ANSI_ITALIC		"\e[6m"	/* Italic text */
#define	ANSI_REVERSE	"\e[7m"	/* Reverse colors */
