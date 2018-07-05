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

#define FINGERCODE /* Do not remove, used to interact with other snippets! - Samson 10-18-98 */

#ifndef FCLOSE
   #define FCLOSE(fp)  fclose(fp); fp=NULL;
#endif

#ifndef GET_TIME_PLAYED
   #define GET_TIME_PLAYED(ch)     (((ch)->played + (current_time - (ch)->logon)) / 3600)
#endif

typedef struct	wizinfo_data	WIZINFO_DATA;

extern WIZINFO_DATA *	first_wizinfo;
extern WIZINFO_DATA *   last_wizinfo;

struct wizinfo_data
{
    WIZINFO_DATA *	next;
    WIZINFO_DATA *	prev;
    char *	name;
    char *  email;
    int	icq;
    sh_int	level;
};

DECLARE_DO_FUN( do_icq_number );  /* User can enter icq# for finger - Samson 1-4-99 */
DECLARE_DO_FUN( do_email	);  /* User can enter email addy for finger - Samson 4-18-98 */
DECLARE_DO_FUN( do_finger     );  /* Finger command - Samson 4-6-98 */
DECLARE_DO_FUN( do_wizinfo    );  /* Wizinfo command - Samson 6-6-99 */
DECLARE_DO_FUN( do_privacy	);  /* Privacy flag toggle - Samson 6-11-99 */
void build_wizinfo( bool bootup );
