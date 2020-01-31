#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/wait.h>
#include <pwd.h>

int readword(char buf[]) {
  int ch, i;
  while(isspace(ch = getchar()) && ch != '\n');
  buf[0] = ch;
  if(ch == '\n') return 0;
  for(i = 1; !isspace(ch = getchar()); i++) buf[i] = ch;
  buf[i]=0;
  ungetc(ch, stdin);
  return 1;
}
void readargs(char* args[]) {
  char buf[100];
  int i;
  for(i = 0; readword(buf)!=0; i++)
    args[i] = strdup(buf);
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
	       b_none };
const char* const builtins[] =
  { "exit",
    "ifs",
    "cd",
    "save",
    "get",
    "status",
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
  int iflast = 0;
  int store[10] = { -1 };
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
    printf("[%s] $ ", nbuf);
    char* args[100] = { NULL };
    readargs(args);
    if(args[0]==NULL) continue;
    int savei;
    if(iferr==status) {
      switch (which_builtin(args[0])) {
      case b_exit:
	return args[1]==NULL ? 0 : atoi(args[1]);
      case b_cd:
	status = -chdir(args[1]==NULL ? pw->pw_dir : args[1]);
	break;
      case b_ifs:
	iferr = args[1]==NULL ? 0 : atoi(args[1]);
	iflast = args[2]==NULL ? 0 : atoi(args[2]);
	break;
      case b_save:
	savei = args[1]==NULL ? 0 : atoi(args[1]);
	status = savei>=0 && savei<10;
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
      case b_none:
	if(fork() != 0) {
	  waitpid(-1, &status, 0);
	} else {
	  execvp(args[0], args);
	  return 1;
	}
      }
    }
    if (iflast==0) iferr = status;
    else if(iflast<0) iflast=0;
    else iflast--;
    
    free_all_in((void**)args);
  }
}
