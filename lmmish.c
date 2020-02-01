#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/wait.h>
#include <pwd.h>

const char* const help_text =
  "This is help text for this microshell project.\n"
  "This microshell project is called\n"
  "\tLast-Minute µkro Shell\n"
  "and is the work, and submission, of\n"
  "\tMichał Krzysztof Feiler\n"
  "\t\t\t<mkf@wmi.amu.edu.pl>\n"
  "\t\t\t s444368@wmi.amu.edu.pl\n"
  "\n"
  "It expands '~' to your home taken from passwd unless followed by a char other than '/'.\n"
  "It allows escaping a ' ' space character with a '^' caret character following each one immediately.\n"
  "Its ]exit command accepts an argument of exit code.\n"
  "Its ]cd command defaults to home directory.\n"
  "]verbosity 1 -- enables verbosity\n"
  "]verbosity 0 -- disables\n"
  "]iflast <i> <j> -- will only run commands if status is <i>, for <j> cycles of REPL. Both default to zero.\n"
  "]save <k> <v> -- will save <v> (defaults to status) to store number <k> (zero to ten, defaults to zero.\n"
  "]get <k> -- sets status to the contents of store <k>.\n"
  "]status -- prints status.\n"
  "]help -- prints this help, nothing else.\n"
  ;
  
int readword(char buf[]) {
  int ch, i;
  while(isspace(ch = getchar()) && ch != '\n');
  buf[0] = ch;
  if(ch == '\n') return 0;
  i = 1;
  while(1) {
    if(isspace(ch=getchar())) {
      if(ch==' ') {
	ch = getchar();
	if(ch=='^') {
	  buf[i++] = ' ';
	  continue;
	} else {
	  ungetc(ch, stdin);
	  ch = ' ';
	}
      }
      break;
    };
    buf[i++] = ch;
  }
  buf[i]=0;
  ungetc(ch, stdin);
  return 1;
}
const char* someoneshome(const char* whose) {
  return getpwnam(whose)->pw_dir;
}
void readargs(char* args[], char* home) {
  char buf[100];
  int i;
  for(i = 0; readword(buf)!=0; i++) {
    if(buf[0]=='~') {
      int j, k, l;
      switch(buf[1]) {
      case 0:
	args[i] = strdup(home);
	break;
      case '/':
	args[i] = malloc(200);
	for(j=0, k=0, l=1;j<199 && buf[l]!=0;j++)
	  if(home[k]!=0)
	    args[i][j] = home[k++];
	  else
	    args[i][j] = buf[l++];
	args[i][199] = 0;
	break;
	/*default:
	  args[i] = strdup(buf); break;*/
      default:
	args[i] = malloc(200);
	int u;
	char bef;
	for(u=2; u<100 && buf[u]!='/' && buf[u]!=0; u++);
	bef = buf[u];
	buf[u] = 0;
	home = someoneshome(buf);
	buf[u] = bef;
	for(j=0, k=0, l=u-1; j<199 && buf[l]!=0;j++)
	  if(home[k]!=0)
	    args[i][j] = home[k++];
	  else
	    args[i][j] = buf[l++];
	args[i][199] = 0;
	break;
      }
    } else
      args[i] = strdup(buf);
  }
  args[i] = NULL;
}
void free_all_in(void* t[]) {
  int i;
  for(i = 0; t[i]!=NULL; i++)
    free(t[i]);
}
enum builtin { b_exit,
	       b_ifs,
	       b_cd,
	       b_save,
	       b_get,
	       b_status,
	       b_verbosity,
	       b_help,
	       b_none };
const char* const builtins[] =
  { "exit",
    "ifs",
    "cd",
    "save",
    "get",
    "status",
    "verbosity",
    "help",
  };
enum builtin which_builtin(char* what) {
  for(int i = 0; i<b_none; i++)
    if(strcmp(what, builtins[i])==0)
      return i;
  return b_none;
}
void shortentilde(char* in, char* what) {
  int i;
  for(i = 0; what[i]!=0 && in[i]!=0; i++) {
    if(what[i]!=in[i]) return;
  }
  if(in[i]==0) {
    if(what[i]==0) {
      in[0]='~';
      in[1]=0;
    }
  } else {
    in[0]='~';
    i--;
    for(int j=1; j<1000-i; j++)
      in[j]=in[j+i];
  };
}
int main() {
  int status = -1;
  int iferr = -1;
  int iflast = -1;
  int store[10] = { -1 };
  int verbosity = 0;
  int color = 2;
  register uid_t uid = geteuid();
  register struct passwd *pw = getpwuid(uid);
  char *userrunning = pw ? pw->pw_name : NULL;
  while(1) {
    char buf[1000] = { 0 };
    char *cbuf = getcwd(buf, 900);
    char nbuf[1000] = { 0 };
    shortentilde(buf, pw->pw_dir);
    if(cbuf==NULL) snprintf(nbuf, 1000, "ooh… %s", buf);
    else {
      if(userrunning && buf[0]=='~' && buf[1]==0) /*strcmp(buf, "~" // pw->pw_dir)==0*/
	snprintf(nbuf, 1000, "at home");
      else if(strcmp("/root", buf)==0)
	snprintf(nbuf, 1000, "visiting root's");
      else if(userrunning)
	snprintf(nbuf, 1000, "%s in %s", userrunning, buf);
      else
	snprintf(nbuf, 1000, "someone in %s", buf);
    } 
    printf("\033" "[1;3%dm" "[%s]" "\033" "[0m" " : ", color, nbuf);
    color = 2;
    char* args[100] = { NULL };
    readargs(args, pw->pw_dir);
    if(verbosity) {
      putchar('[');
      int iver = 0;
      while(args[iver]!=NULL)
	printf("«%s» ", args[iver++]); 
      putchar('\n');
    }
    if(args[0]==NULL) continue;
    int savei;
    if(iflast==-1 || iferr==status) {
      switch (which_builtin(args[0])) {
      case b_exit:
	if(args[1]==NULL) exit(0);
	if(args[2]!=NULL)
	  printf("don't use that many arguments next time, they got discarded anyways beyond the first one\n");
	exit(atoi(args[1]));
      case b_cd:
	if(args[2]!=NULL) {
	  printf("too many arguments, we change directory to just one, 't would be lovely otherwise\n");
	  break;
	}
	status = -chdir(args[1]==NULL ? pw->pw_dir : args[1]);
	if(status)
	  printf("so this is supposed to be a nice notif that the directory change failed\n");
	break;
      case b_ifs:
	iferr = 0; iflast = 0;
        for(savei = 1; savei < 4 && args[savei]!=NULL; savei++);
	switch(savei) {
	case 4:
	  printf("This is a warning, the arguments beyond those two numbers are discarded\n");
	case 3:
	  iflast = atoi(args[2]);
	case 2:
	  iferr = atoi(args[1]);
	}
	break;
      case b_save:
	savei = args[1]==NULL ? 0 : atoi(args[1]);
	status = savei<0 || savei>=10;
	if(status==0)
	  store[savei] = args[2]==NULL ? status : atoi(args[2]);
	break;
      case b_get:
	savei = args[1]==NULL ? 0 : atoi(args[1]);
	if(savei>=0 && savei<10)
	  status = store[savei];
	break;
      case b_status:
	printf("%d\n", status);
	break;
      case b_verbosity:
	if(args[1]==NULL)
	  status = 5;
	else {
	  savei = atoi(args[1]);
	  switch(savei) {
	  case 0:
	  case 1:
	    verbosity = savei;
	    status = 0;
	    break;
	  default:
	    status = 2;
	  }
	}
	break;
      case b_help:
	puts(help_text);
	break;
      case b_none:
	if(fork() != 0) {
	  savei = status;
	  waitpid(-1, &status, 0);
	  if(status==256) {
	    printf("Uh, the command seems to not be findable, since it was apparently unfound...\n");
	    status = savei;
	    color = 4;
	  }
	} else {
	  execvp(args[0], args);
	  return 1;
	}
      }
    } else if(verbosity) {
      printf("ifs is active and inhibited this action\n");
    }
    if (iflast==0) iferr = status;
    else if(iflast<0) iflast=-1;
    else iflast--;

    if(color!=4) color = status ? 1 : 2;
    
    free_all_in((void**)args);
  }
}
