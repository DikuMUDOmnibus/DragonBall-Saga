//Chris Busch
//(c) 1995

//dont link this object into your mud!!!!
#include <stdio.h>
#include "chatmain.hpp"
#include "eliza.hpp"
extern eliza chatter;

void byby()
{
  puts(eliza_title);
  puts(eliza_version);
  puts("syntax:  chat filename");
}


char s[MAXSIZE+10];

int main(int argc,char **args)
{
  chatter.reducespaces("");
  randomize();
  if(argc==2)
    {
      if(!chatter.loaddata(args[1]))
	{
	  printf("Cannot load '%s' as a data file.\n",args[1]);
	  byby();
	  return 0;
	}
    }
  else
    {
      printf("need data file: ");
      fgets(s,MAXSIZE,stdin);
      if(!chatter.loaddata(chatter.trim(s)))
	{
	  printf("Cannot load '%s' as a data file.\n",s);
	  byby();
	  return 0;
	}

    }

  char talker[MAXSIZE],yourname[MAXSIZE];
  printf("Enter who you are talking to:");
  fgets(talker, MAXSIZE, stdin);
  printf("Enter your name:");
  fgets(yourname, MAXSIZE, stdin);
  while(fgets(s,80,stdin)!=NULL)
    puts(chatter.process(talker,s,yourname));
}

