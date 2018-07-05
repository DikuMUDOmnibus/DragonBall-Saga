/*                           Marriage Code v1.3
                                  By Cid
                            eos.execnet.com:1234

Unlike my other codes this one does require the addition of another pcdata
field.  Like spouse.  If you add it spouse is a string not an integer.
So thats how it works.  Plus you'll have to modify db.c save.c to load up
the spouse string so this will work.   This snippet also includes a rings
command.  It'll create the rings for the married couple.  Which you'll have
to define in mud.h OBJ_VNUM_DIAMOND_RING and OBJ_VNUM_WEDDING_BAND to vnums
that you might have (ofcourse I created 2 rings for this then added in the
marriage code).  The rings command would be the easiest to add in cause of
just putting in 2 lines in mud.h...even though its a lot easier then the
marriage, and divorce code...but it needs the spouse field(checks to see if
they are married).   Well enjoy...btw dont forget to put somewhere in your
code to give me credits or if you just pop in this file...Keep the this header
intact.

Add do_divorce and do_marry to mud.h and tables.c - Ntanel StormBlade
*/


#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "mud.h"

void do_marry( CHAR_DATA *ch, char *argument)
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    CHAR_DATA *victim2;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );


       if ( arg1[0] == '\0' || arg2[0] == '\0' )
          {
             send_to_char("&WSyntax: marry <char1> <char2>\n\r",ch);
             return;
          }

       if ( ((victim = get_char_world(ch,arg1)) == NULL) ||
            ((victim2 = get_char_world(ch,arg2)) == NULL))
          {
          send_to_char("&WBoth characters must be playing!\n\r", ch );
          return;
          }

       if ( IS_NPC(victim) || IS_NPC(victim2))
          {
          send_to_char("&WSorry! Mobs can't get married!\n\r", ch);
          return;
          }


       if (victim->pcdata->spouse[0] == '\0' && victim2->pcdata->spouse[0] == '\0')
          {
            send_to_char("&WYou pronounce them man and wife!\n\r", ch);
            send_to_char("&WYou say the big 'I do.'\n\r", victim);
            send_to_char("&WYou say the big 'I do.'\n\r", victim2);

            act(AT_BLUE,"$n and $N are now declared married!\n\r", victim, NULL, victim2, TO_ROOM);

            victim->pcdata->spouse = str_dup(victim2->name);

            victim2->pcdata->spouse = str_dup(victim->name);

            return;
          }
        else
        {
           send_to_char("&WThey are already married!\n\r", ch);
           return;
        }


       return;
}

void do_divorce( CHAR_DATA *ch, char *argument)
   {
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    CHAR_DATA *victim2;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

       if ( arg1[0] == '\0' || arg2[0] == '\0' )
          {
          send_to_char("&WSyntax: divorce <char1> <char2>\n\r",ch);
          return;
          }
       if ( ((victim = get_char_world(ch,arg1)) == NULL) ||
            ((victim2 = get_char_world(ch,arg2)) == NULL))
          {
          send_to_char("&WBoth characters must be playing!\n\r", ch );
          return;
          }

       if ( IS_NPC(victim) || IS_NPC(victim2))
          {
          send_to_char("&WI don't think they're Married to the Mob!\n\r", ch);
          return;
          }

       if ( !str_cmp( victim->pcdata->spouse, victim2->name ) && !str_cmp( victim2->pcdata->spouse, victim->name ) )
/*       if (victim->pcdata->spouse != victim2->name || victim2->pcdata->spouse != victim->name)
          {
          send_to_char(AT_WHITE, "They aren't even married!!\n\r",ch);
          return;
          } */
       {
       send_to_char("&WYou hand them their papers.\n\r", ch);
       send_to_char("&WYour divorce is final.\n\r", victim);
       send_to_char("&WYour divorce is final.\n\r", victim2);

       act(AT_WHITE,"$n and $N swap divorce papers, they are no-longer married.",victim, NULL, victim2, TO_NOTVICT);

       DISPOSE( victim->pcdata->spouse         );
       DISPOSE( victim2->pcdata->spouse        );
       victim->pcdata->spouse = str_dup( "" );
       victim2->pcdata->spouse = str_dup( "" );
       return;
       }
       else
       {
       send_to_char("&WThey arent married!", ch);
       return;
       }
}

void do_rings ( CHAR_DATA *ch, char *argument )
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    CHAR_DATA *victim2;
    char        buf  [ MAX_STRING_LENGTH ];
    OBJ_DATA  * ring;
    EXTRA_DESCR_DATA * ed;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );


       if ( ((victim = get_char_world(ch,arg1)) == NULL) ||
            ((victim2 = get_char_world(ch,arg2)) == NULL))
          {
          send_to_char("&WBoth characters must be playing!\n\r", ch );
          return;
          }
/*       if ((victim->pcdata->spouse != victim2->name) || (victim2->pcdata->spouse != victim->name))
       {
          send_to_char(AT_WHITE, "They arent even married!!\n\r", ch);
          return;
       } */

    switch( victim2->sex )
    {
        case SEX_FEMALE:
        {
            ring = create_object( get_obj_index( OBJ_VNUM_DIAMOND_RING ), 0 );
            switch( victim->sex )
            {
                case SEX_FEMALE:
                {
                    sprintf( buf, "This is the beautiful diamond ring given to you by your lovely\n\rwife %s at your wedding. It signifies your eternal love for eachother.\n\r",victim->name );
                    ring->description = str_dup( buf );
                    break;
                }
                case SEX_MALE:
                {
                    sprintf( buf, "This is the beautiful diamond ring given to you by your handsome\n\rhusband %s at your wedding. It signifies your eternal love for eachother.\n\r", victim->name );
                    ring->description = str_dup( buf );
                    break;
                }
                case SEX_NEUTRAL:
                default:
                {
                    sprintf( buf, "This is the beautiful diamond ring given to you by your\n\rspouse %s at your wedding. It signifies your eternal love for eachother.\n\r", victim->name );
                    ring->description = str_dup( buf );
                    break;
                }
            }
/*
            if ( !extra_descr_free )
            {
                ed = alloc_perm( sizeof( *ed ) );
            }
            else
            {
                ed = extra_descr_free;
                extra_descr_free = extra_descr_free->next;
            }
*/
            ed = SetOExtra(ring, "inscription");
//            ed->keyword = str_dup( "inscription" );
            sprintf( buf, "The inscription reads:\n\rTo my lovely wife, yours forever, %s\n\r", victim->name );
            ed->description = str_dup( buf );
//            ed->deleted = FALSE;
//            ed->next = ring->extra_descr;
//            ring->extra_descr = ed;
            break;
        }
        case SEX_MALE:
        case SEX_NEUTRAL:
        default:
        {
            ring = create_object( get_obj_index( OBJ_VNUM_WEDDING_BAND ), 0 );
            switch( victim->sex )
            {
                case SEX_FEMALE:
                {
                    sprintf( buf, "This is the ring given to you by your beautifull wife %s\n\rat your wedding. It signifies your eternal love for eachother.\n\r", victim->name );
                    ring->description = str_dup( buf );
                    break;
                }
                case SEX_MALE:
                {
                    sprintf( buf, "This is the ring given to you by your handsome husband %s\n\rat your wedding. It signifies your eternal love for eachother.\n\r", victim->name );
                    ring->description = str_dup( buf );
                    break;
                }
                case SEX_NEUTRAL:
                default:
                {
                    sprintf( buf, "This is the ring given to you by your spouse %s at\n\ryour wedding. It signifies your eternal love for eachother.\n\r", victim->name );
                    ring->description = str_dup( buf );
                    break;
                }
            }


/*
            if ( !extra_descr_free )
            {
                ed = alloc_perm( sizeof( *ed ) );
            }
            else
            {
                ed = extra_descr_free;
                extra_descr_free = extra_descr_free->next;
            }
*/

            ed = SetOExtra(ring, "inscription");
/*
            ed->keyword = str_dup( "inscription" );
            ed->deleted = FALSE;
            ed->next = ring->extra_descr;
            ring->extra_descr = ed;
*/

            switch( victim->sex )
            {
                default:
                case SEX_MALE:
                {
                    sprintf( buf, "The inscription reads:\n\rTo my handsome husband... Forever yours, %s\n\r", victim->name );
                    ed->description = str_dup( buf );
                    break;
                }
                case SEX_NEUTRAL:
                {
                    sprintf( buf,"The inscription reads:\n\rForever love, %s\n\r", victim->name );
                    ed->description = str_dup( buf );
                    break;
                }
            }
        }
    }

    obj_to_char ( ring, victim );

    return;
}

