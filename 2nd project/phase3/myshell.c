/* $begin shellmain */
#include "csapp.h"
#include <errno.h>
#define MAXARGS 128
#define MAXJOBS 128

int job_index;
enum { Run, Stop, Terminate, Foreground };
char *state[4] = { "run", "stop", "terminate", "foreground" };

struct Job {
	pid_t pid;
	char command[MAXBUF];
	int state;
} Job;
struct Job jobs[MAXJOBS];

/* Function prototypes */
void eval(char *cmdline);
int parseline(char *buf, char **argv);
int builtin_command(char **argv); 

void initJob();
int addJob(pid_t pid, int state, char *command);
void deleteJob(int index);
void printJob();

void sig_int_handler(int signal);
void sig_tstp_handler(int signal);
void sig_chld_handler(int signal);
void sig_quit_handler(int signal);

int main() 
{
    char cmdline[MAXLINE]; /* Command line */

	signal(SIGINT, sig_int_handler);
	signal(SIGTSTP, sig_tstp_handler);
	signal(SIGCHLD, sig_chld_handler);
	signal(SIGQUIT, sig_quit_handler);
	initJob();

    while (1) {
		/* Read */
		printf("CSE4100-SP-P2 > ");                   
		fgets(cmdline, MAXLINE, stdin); 
		if (feof(stdin)) exit(0);

		/* Evaluate */
		eval(cmdline);

		fflush(stdout);
    } 
}
/* $end shellmain */

/* $begin eval */
/* eval - Evaluate a command line */
void eval(char *cmdline) 
{
    char *argv[MAXARGS]; /* Argument list execve() */
    char buf[MAXLINE];   /* Holds modified command line */
    int bg;              /* Should the job run in bg or fg? */
    pid_t pid;           /* Process id */
	int pgid = 0;
	int flag = 0;
	sigset_t M, P;

	sigemptyset(&M);
	sigaddset(&M, SIGINT);
	sigaddset(&M, SIGCHLD);
	sigaddset(&M, SIGTSTP);

	strcpy(buf, cmdline);
	bg = parseline(buf, argv);

	if (argv[0] == NULL) return;
	else {
		int argc = 0, fd[2];
		if (pipe(fd)) putchar('!');
		Dup2(0, fd[0]);
		Dup2(1, fd[1]);

		while (argv[argc] != NULL) {
			int cur = argc, index = 0;
			char *command[MAXARGS];

			for (index = 0; argv[cur] != NULL; index++) {
				command[index] = argv[cur];
				if (*argv[cur] == '|' || *argv[cur] == '&') break;
				cur++;
			}
			command[index] = NULL;

			if (argv[cur] == NULL) { // last command
				if (!builtin_command(command)) {
					sigprocmask(SIG_BLOCK, &M, &P);

					pid_t pid;
					
					if ((pid = Fork()) == 0) {
						sigprocmask(SIG_SETMASK, &P, NULL);
						setpgid(0, pgid);
						getpgid(getpid());
						if (execvp(command[0], command) < 0) {
							printf("%s: Command not found.\n", command[0]);
							exit(0);
						}
					}

					if (!bg) {
						int id = addJob(pid, Foreground, cmdline);
						sigprocmask(SIG_SETMASK, &P, NULL);
						while (jobs[id].state == Foreground) sleep(1);
					}
					else {
						int id = addJob(pid, Run, cmdline);
						printf("Job - index : %d - pid : %d -\n", id, pid);
						sigprocmask(SIG_SETMASK, &P, NULL);
					}
				}
			}
			else { // pipe line
				if (*argv[cur] == '|') {
					if (!builtin_command(command)) {
						int fdd[2];
						pid_t pid;
					
						if (pipe(fdd)) putchar('!');

						if ((pid = Fork()) == 0) {
							setpgid(0, pgid);
							getpgid(getpid());
							close(fdd[0]);
							dup2(fdd[1], STDOUT_FILENO);
							close(fdd[1]);

							if (execvp(command[0], command) < 0) {
								printf("%s: Command not found.\n", command[0]);
								exit(0);
							}
						}

						if (!flag) {
							pgid = pid;
							flag = 1;
						}
						close(fdd[1]);
						dup2(fdd[0], STDIN_FILENO);
						close(fdd[0]);
					}
				}
				cur++;
			}
			argc = cur;
		}
		Dup2(fd[0], 0);
		close(fd[0]);
		Dup2(fd[1], 1);
		close(fd[1]);
	}
}

/* If first arg is a builtin command, run it and return true */
int builtin_command(char **argv) 
{
    if (strcmp(argv[0], "quit") == 0) exit(0); /* quit command */
	if (strcmp(argv[0], "exit") == 0) exit(0); /* quit command */
	if (strcmp(argv[0], "cd") == 0) {
		char path[MAXBUF];
		pid_t pid;
		int channel;

		if (*argv[1] == '~') {
			strcpy(path, getenv("HOME"));
			strcat(path, argv[1] + sizeof(char));
		}
		else strcpy(path, argv[1]);

		channel = chdir(path);
		if (channel != 0) printf("cd: %s: No such file or directory\n", argv[1]);

		return 1;
	}
	//if (strcmp(argv[0], "ls") == 0) { /* print list of the files in dir */
	/*	DIR *dir;
		struct dirent *entry;
		char *directory = ".";
		if (argv[1] != NULL) strcpy(directory, argv[1]);

		dir = Opendir(directory);
		if (dir == NULL) {
			printf("ls error\n");
			return 1;
		}

		while ((entry = Readdir(dir)) != NULL) {
			if (entry->d_name[0] != '.') printf("%s ", entry->d_name);
		}
		printf("\n");
		Closedir(dir);
		return 1;
	}*/
	if (strcmp(argv[0], "mkdir") == 0) { /* create new dir */
		if (argv[1] == NULL) {
			printf("mkdir error\n");
			return 1;
		}
		if (mkdir(argv[1], 0755) != 0) { // 0755 is permission
			printf("mkdir error\n");
			return 1;
		}
		return 1;
	}
	if (strcmp(argv[0], "rmdir") == 0) { /* delete dir */
		if (argv[1] == NULL) {
			printf("rmdir error\n");
			return 1;
		}
		if (rmdir(argv[1]) != 0) {
			printf("rmdir error\n");
			return 1;
		}
		return 1;
	}
	if (strcmp(argv[0], "touch") == 0) { /* create new file */
		if (argv[1] == NULL) {
			printf("touch error\n");
			return 1;
		}

		FILE *file = Fopen(argv[1], "a");
		if (file == NULL) {
			printf("touch error\n");
			return 1;
		}

		Fclose(file);
		return 1;
	}
	if (strcmp(argv[0], "cat") == 0) { /* create new file || print file */
		if (argv[1] == NULL) {
			printf("cat error\n");
			return 1;
		}
		
		if (strcmp(argv[1], ">") == 0) {
			if (argv[2] == NULL) {
				printf("cat error\n");
				return 1;
			}

			FILE *file = Fopen(argv[2], "a");
			if (file == NULL) {
				printf("cat error\n");
				return 1;
			}
			Fclose(file);
			return 1;
		}
		else {
			int i = 1;
			while (1) {
				if (argv[i] == NULL) return 1;

				FILE *file = Fopen(argv[i], "r");
				if (file == NULL) {
					printf("cat error\n");
					return 1;
				}
				else {
					char *line;
					while ((line = fgetc(file)) != EOF) putchar(line);
					Fclose(file);
				}
				i++;
			}
		}
		return 1;
	}
	if (strcmp(argv[0], "rm") == 0) { /* delete file */
		if (argv[1] == NULL) {
			printf("rm error\n");
			return 1;
		}

		if (remove(argv[1]) != 0) {
			printf("rm error\n");
			return 1;
		}

		return 1;
	}
	if (strcmp(argv[0], "echo") == 0) { /* print argv || print file */
		if (argv[1] == NULL) {
			printf("echo error\n");
			return 1;
		}

		FILE *file = NULL;
		int i = 1, flag = 0;
		while (argv[i] != NULL) {
			if (strcmp(argv[i], ">") == 0) {
				file = Fopen(argv[i + 1], "w");
			
				if (file == NULL) {
					printf("echo write error\n");
					return 1;
				}
				flag = 1;
				// printf("%d\n", i);
				break;
			}
			else if (strcmp(argv[i], ">>") == 0) {
				file = Fopen(argv[i + 1], "a");

				if (file == NULL) {
					printf("echo write error\n");
					 return 1;
				}
				flag = 1;
				break;
			}
			i++;
		}

		if (flag) {
			i = 1;
			while (strcmp(argv[i], ">") != 0 && strcmp(argv[i], ">>") != 0) {
				fprintf(file, "%s", argv[i]);
				if (argv[i + 1] != NULL) fprintf(file, " ");
				i++;
			}
			fprintf(file, "\n");
			Fclose(file);
		}
		else {
			//FILE *file = fopen(argv[1], "r");
			//if (file == NULL) {
				int i = 1;
				while (argv[i] != NULL) {
					printf("%s", argv[i]);
					if (argv[i + 1] != NULL) printf(" ");
					i++;
				}
				printf("\n");
			//}
			/*else {
				char *line;
				while ((line = fgetc(file)) != EOF) putchar(line);
				Fclose(file);
			}*/
		}
		return 1;
	}
	if (strcmp(argv[0], "&") == 0) return 1; /* Ignore singleton & */
	/* Job Command */
	if (strcmp(argv[0], "jobs") == 0) {
		printJob();
		return 1;
	}

	if (strcmp(argv[0], "bg") == 0) {
		if (argv[1] == NULL) printf("Need more argument\n");
		else {
			int idx = atoi(argv[1]);
			if (jobs[idx].pid == -1) printf("Job doesn't exist\n");
			else if (jobs[idx].state == Run) {
				printf("bash: bg: job %d already in background\n", idx);
			}
			else {
				pid_t pid = jobs[idx].pid;
				pid_t wpid;
				int S;
				kill(pid, SIGCONT);
				if ((wpid = waitpid(pid, &S, WCONTINUED)) < 0) unix_error("waitpid error");
				jobs[idx].state = Run;
				printf("[bg] Job - index : %d - pid : %d - cmd : %s\n", idx, jobs[idx].pid, jobs[idx].command);
			}
		}
		return 1;
	}

	if (strcmp(argv[0], "fg") == 0) {
		if (argv[1] == NULL) printf("need more argument\n");
		else {
			int idx = atoi(argv[1]);
			if (jobs[idx].pid == -1) printf("Job doesn't exist\n");
			else {
				pid_t pid = jobs[idx].pid, wpid;
				char C[MAXBUF];
				strcpy(C, jobs[idx].command);
				int S;

				if (jobs[idx].state == Stop) {
					kill(pid, SIGCONT);
					if ((wpid = waitpid(pid, &S, WCONTINUED)) < 0) unix_error("waitpid error");
				}

				printf("[fg] JOb - index: %d - pid : %d - cmd : %s\n", idx, jobs[idx].pid, jobs[idx].command);
				jobs[idx].state = Foreground;
				while (jobs[idx].state == Foreground) sleep(1);
			}
		}
		return 1;
	}

	if (strcmp(argv[0], "kill") == 0) {
		if (argv[1] == NULL) printf("need more argument\n");
		else {
			char *num = argv[1];
			if (num[0] == '%') num++;

			int idx = atoi(num);
			//printf("%d\n", idx);
			if (idx < 0 || idx > MAXJOBS || jobs[idx].pid == -1) printf("bash: kill: (%d): no such job\n", idx);
			else {
				printf("[kill] Job - index : %d - pid : %d - cmd : %s\n", idx, jobs[idx].pid, jobs[idx].command);
				kill(-getpgid(jobs[idx].pid), SIGKILL);
				deleteJob(idx);
			}
		}
		return 1;
	}
	/*****/

	return 0; /* Not a builtin command */
}
/* $end eval */

/* $begin parseline */
/* parseline - Parse the command line and build the argv array */
int parseline(char *buf, char **argv) 
{
    char *delim;         /* Points to first space delimiter */
    int argc;            /* Number of args */
    int bg;              /* Background job? */

	char *end = buf + strlen(buf) - 1; /*****/

    buf[strlen(buf)-1] = ' ';  /* Replace trailing '\n' with space */
    while (*buf && (*buf == ' ')) /* Ignore leading spaces */
		buf++;

    /* Build the argv list */
    argc = 0;
    while ((delim = strchr(buf, ' '))) {
		/*****/
		if (*buf == '\"') {
			*buf++ = '\0';
			argv[argc++] = buf;
			while (buf != end && *buf != '\"') buf++;
			if (buf != end) *buf++ = '\0';
		}
		else {
			*delim = '\0';
			while (delim != buf) { // replace '|', '&' to '\0' and append to argv
				if (strncmp("|", buf, 1) == 0) {
					argv[argc++] = "|";
					*buf = '\0';
					buf++;
				}
				else if (strncmp("&", buf, 1) == 0) {
					argv[argc++] = "&";
					*buf = '\0';
					buf++;
				}

				if (delim != buf) { // command after '|', '&'
					argv[argc++] = buf;
					while (delim != buf && '&' != *buf && '|' != *buf) buf++;
				}
			}
		}
		/*****/

		buf = delim + 1;
		while (*buf && (*buf == ' ')) /* Ignore spaces */
    		buf++;
    }
    argv[argc] = NULL;
    
    if (argc == 0)  /* Ignore blank line */
	return 1;

    /* Should the job run in the background? */
    if ((bg = (*argv[argc-1] == '&')) != 0)
	argv[--argc] = NULL;

    return bg;
}
/* $end parseline */

/* Job Function */
void initJob() {
	for (int i = 0; i < MAXJOBS; i++) jobs[i].pid = -1;
	job_index = 1;
}

int addJob(pid_t pid, int state, char *command) {
	if (job_index == MAXJOBS) {
		printf("jobs are full!\n");
		return -1;
	}

	int id = job_index++;
	jobs[id].pid = pid;
	strcpy(jobs[id].command, command);
	jobs[id].state = state;

	return id;
}

void deleteJob(int index) {
	if (job_index == 0) {
		printf("jobs aren't exist!\n");
		return;
	}

	jobs[index].pid = -1;
	jobs[index].command[0] = '\0';
	jobs[index].state = -1;

	while (0 < job_index && -1 == jobs[job_index].pid) job_index = job_index - 1;
	job_index = job_index + 1;
}

void printJob() {
	for (int i = 0; i < MAXJOBS; i++) {
		if (jobs[i].pid != -1) {
			printf("index : %-d	state : %-10s	command : %-10s", i, state[jobs[i].state], jobs[i].command);
		}
	}
}

/* Handler Function */
void sig_chld_handler(int signal) {
	sigset_t M, P;
	pid_t pid;
	int S;

	Sigfillset(&M);
	while ((pid = waitpid(-1, &S, WNOHANG | WUNTRACED)) > 0) {
		Sigprocmask(SIG_BLOCK, &M, &P);
		int id;
		for (id = 1; id < job_index; id++) if (jobs[id].pid == pid) break;

		if (job_index != id) {
			if (WIFEXITED(S)) deleteJob(id);
			else if (WIFSIGNALED(S)) {
				printf("JOB - index : %d - pid : %d - terminated\n", id, jobs[id].pid);
				deleteJob(id);
			}
			else if (WIFSTOPPED(S)) {
				jobs[id].state = Stop;
				printf("JOB - index : %d - pid : %d - stopped\n", id, jobs[id].pid);
			}
			else unix_error("Waitpid error\n");

			Sigprocmask(SIG_SETMASK, &P, NULL);
		}
	}
}

void sig_int_handler(int signal) {
	sigset_t M, P;

	sigfillset(&M);
	Sigprocmask(SIG_BLOCK, &M, &P);

	pid_t pid = 0;
	for (int i = 0; i < job_index; i++) {
		if (jobs[i].state == Foreground) {
			pid = jobs[i].pid;
			break;
		}
	}
	if (pid > 0) kill(-getpgid(pid), SIGINT);
	Sigprocmask(SIG_SETMASK, &P, NULL);
}

void sig_tstp_handler(int signal) {
	sigset_t M, P;

	sigfillset(&M);
	Sigprocmask(SIG_BLOCK, &M, &P);

    pid_t pid = 0;
	for (int i = 0; i < job_index; i++) {
		if (jobs[i].state == Foreground) {
			pid = i;
			break;
		}
	}

	if (pid > 0) kill(-getpgid(pid), SIGTSTP);
	Sigprocmask(SIG_SETMASK, &P, NULL);
}

void sig_quit_handler(int signal) {
	Sio_puts("quit the program\n");
	exit(1);
}
