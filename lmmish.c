#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/wait.h>
#include <pwd.h>
#include <errno.h>

const char* const help_text =
  "This is help text for this microshell project.\n"
  "This microshell project is called\n"
  "\tLast-Minute µkro Shell\n"
  "and is the work, and submission, of\n"
  "\tMichał Krzysztof Feiler\n"
  "\t\t\t<mkf@wmi.amu.edu.pl>\n"
  "\t\t\t s444368@wmi.amu.edu.pl\n"
  "\n"
  "It expands '~' to your user's taken from passwd unless followed by a char other than '/'.\n"
  "It allows escaping a ' ' space character with a '^' caret character following each one immediately.\n"
  "Its ]exit command accepts an argument of exit code.\n"
  "Its ]cd command defaults to home directory.\n"
  "]verbosity 1 -- enables verbosity\n"
  "]verbosity 0 -- disables\n"
  "]ifs <i> <j> -- will only run commands if status is <i>, for <j> cycles of REPL. Both default to zero.\n"
  "]save <k> <v> -- will save <v> (defaults to status) to store number <k> (zero to ten, defaults to zero.\n"
  "]get <k> -- sets status to the contents of store <k>.\n"
  "]status -- prints status.\n"
  "]help -- prints this help, nothing else.\n"
  ;

/*
 * readword is a procedure for getting a word, null-delimited, into buf.
 * Spaces can be escaped bz immediately succeeding them with '^'.
 * readword (WRITES TO GIVEN char[] named buf, RETURNS int of {0,1}):
 * - first passes (getchar-s) through whitespace (isspace but non-newline)
 *   characters
 * - IF first non-whitespace is a newline WRITES buf: {0: '\n'} and RETURNS 0).
 * - OTHERWISE, WRITES each non-isspace character to buf[0...] unless it's a
 *   space followed by a '^' in which case space gets written and '^' is not
 *   counted
 * - WRITES 0 to buf[i], after the end of word, for a null-delimited string
 * - the first non-isspace character is ungetc-ed back into stdin
 * - RETURNS 1
 */
int readword(char buf[]) {
  int ch, i;
  while(isspace(ch = getchar()) && ch != '\n');
  /* WRITES first read non-whitespace character to buf[0] */
  buf[0] = ch;
  /* if it was a newline then RETURNS 0 (RESULTING buf: {0: '\n'}) */
  if(ch == '\n') return 0;
  i = 1;
  /*
   * WRITES each non-isspace character to buf[1...] unless it's a space
   * followed by a '^' in which case space gets written and '^' is not counted.
   */
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
  /* WRITES 0 to buf[i], after the end of word, for a null-delimited string */
  buf[i]=0;
  /* the first non-isspace character is ungetc-ed back into stdin */
  ungetc(ch, stdin);
  return 1;
}
/*
 * history - file that we write to, commands delimited by newline, but
 * words delimited by null
 */
FILE* history;
/*
 * readargs (WRITES TO GIVEN char*[] named args,
 *           READS GIVEN char* named home,
 *           WRITES TO SCOPE FILE* named `history`):
 * - for consecutively incrementing integer i, has `readword` write to a buffer of 100
 * - as long as `readword` returns non-zero, it WRITES TO `history` the result (buffer),
 *   then:
 *   - if a word is just a tilde (~) on its own or has leading tilde followed by a slash (/),
 *     it WRITES TO args[i] a pointer to a newly allocated string with the word with
 *     tilde swapped out for contents of `home`
 *   - otherwise it WRITES TO args[i] a pointer to a newly allocated string with the word.
 * - once `readword` returns zero, it writes a newline to `history`, and delimits args
 *   with a null pointer at args[i] ( WRITES null TO args[i] )
 */
void readargs(char* args[], char* home) {
  char buf[100];
  int i;
  for(i = 0; readword(buf)!=0; i++) {
    fprintf(history, "%s", buf);
    if(buf[0]=='~') {
      switch(buf[1]) {
      case 0:
	args[i] = strdup(home);
	break;
      case '/':
	args[i] = malloc(200);
	int j, k, l;
	for(j=0, k=0, l=1;j<199 && buf[l]!=0;j++)
	  if(home[k]!=0)
	    args[i][j] = home[k++];
	  else
	    args[i][j] = buf[l++];
	args[i][199] = 0;
	break;
      default:
	args[i] = strdup(buf);
      }
    } else
      args[i] = strdup(buf);
    fputc(0, history);
  }
  fputc('\n', history);
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

int forkresult;
void sighandler(int signalnumber) {
  if(forkresult==0) {
    printf("\nreceived signal %d %s, exiting with zero\n",
	   signalnumber, strsignal(signalnumber));
    exit(0);
  }
  printf("\nkilling our process with the received signal %d %s\n",
	 signalnumber, strsignal(signalnumber));
  int result = kill(forkresult, signalnumber);
  printf("done the killing, %s.\n", result==64 ?
	 "it tried killing more then one" :
	 (result==0 ? "successfully" :
	  (result==1 ? "to a failure" : "but that did a weird thing")));
}
char* concat(const char *s1, const char *s2)
{
    const size_t l1 = strlen(s1);
    const size_t l2 = strlen(s2);
    char *result = malloc(l1+l2+1);
    memcpy(result, s1, l1);
    memcpy(result+l1, s2, l2+1);
    return result;
}
int main() {
  int status = 0;
  int iferr = -1;
  int iflast = -1;
  int store[10] = { -1 };
  int verbosity = 0;
  int color = 2;
  uid_t uid = geteuid();
  struct passwd *pw = getpwuid(uid);
  char *userrunning = pw ? pw->pw_name : NULL;
  history = fopen(concat(pw->pw_dir,"/history_lmmish.dat"), "a+");
  while(1) {
    forkresult = 0;
    signal(SIGINT, sighandler);
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
    int whereargsnull;
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
	for(whereargsnull = 1; whereargsnull < 4 && args[whereargsnull]!=NULL; whereargsnull++);
	int to_save = status;
	savei = 0;
	switch(whereargsnull) {
	case 4:
	  puts("This is a warning, the arguments beyond those two numbers are discarded.");
	case 3:
	  to_save = atoi(args[2]);
	case 2:
	  savei = atoi(args[1]);
	case 1:
	  if(savei>=0 && savei<10)
	    store[savei] = to_save;
	}
	break;
      case b_get:
	for(whereargsnull = 1; whereargsnull < 3 && args[whereargsnull]!=NULL; whereargsnull++);
	savei = 0;
	switch(whereargsnull) {
	case 3:
	  puts("This is a warning, the arguments beyond the one number are discarded.");
	case 2:
	  savei = atoi(args[1]);
	}
	if(savei>=0 && savei<10)
	  status = store[savei];
	break;
      case b_status:
	if(args[1]!=NULL) puts("This command does not set status or compare it or anything, don't give it arguments.");
	printf("%d\n", status);
	break;
      case b_verbosity:
	if(args[1]==NULL) {
	  status = 5;
	  puts("Verbostity must be explicitly set to zero or one.");
	} else if(args[2]==NULL){
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
	} else {
	  status = 6;
	  puts("Too many arguments, give just one zero or one to set the verbosity level to!");
	}
	break;
      case b_help:
	puts(help_text);
	for(savei = 1; savei<100 && args[savei]!=NULL;
	    printf("I did not provide any specific help for %s.\n", args[savei++]));
	if(--savei)
	  printf("You asked for %d specific topics, the command that just has one general text.\n", savei);
	break;
      case b_none:
	if((forkresult = fork()) != 0) {
	  if(forkresult==-1)
	    printf("failed to fork. error %d. Says, «%s».\n", errno, strerror(errno));
	  /*savei = status;*/
	  savei = waitpid(-1, &status, 0);
	  if(savei==-1)
	    printf("waitpid failed, error %d. Says, «%s».\n", errno, strerror(errno));
	  else {
	    if(WIFEXITED(status))
	      status = WEXITSTATUS(status);
	    else if(WIFSIGNALED(status)) {
	      int signalnumber = WTERMSIG(status);
	      printf("an uncaught signal %d, which is %s.\n", signalnumber, strsignal(signalnumber));
	    }
	  }
	} else {
	  int res = execvp(args[0], args);
	  if (res==-1) {
	    printf("%s: ...error... (%d...): %d... ", args[0], res, errno);
	    char* errtext = strerror(errno);
	    int back = 0;
	    int spacji = 1;
	    while(errtext[back] != 0){
	      if(errtext[back]==' ' && (spacji++)%2==0) {
		putchar('.'); putchar('.'); putchar('.');
	      }
	      putchar(errtext[back++]);
	    }
	    putchar('.');
	    putchar('.');
	    putchar('.');
	    putchar('\n');
	  }
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
