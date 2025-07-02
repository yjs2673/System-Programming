/* $begin shellmain */
#include "csapp.h"
#include <errno.h>
#define MAXARGS 128

/* Function prototypes */
void eval(char *cmdline);
int parseline(char *buf, char **argv);
int builtin_command(char **argv); 

int main() 
{
    char cmdline[MAXLINE]; /* Command line */

    while (1) {
		/* Read */
		printf("CSE4100-SP-P2 > ");                   
		fgets(cmdline, MAXLINE, stdin); 
		if (feof(stdin)) exit(0);

		/* Evaluate */
		eval(cmdline);
    } 
}
/* $end shellmain */

/*****/
void multiple(char **argv, int argc) {
	int fd[2];
	if (pipe(fd)) putchar("error");

	Dup2(0, fd[0]);
	Dup2(1, fd[1]);

	while (argv[argc] != NULL) { /* execute multiple commands */
		int idx = 0, curr = argc;
		char *command[MAXARGS];

		for (idx = 0; argv[curr] != NULL; idx++, curr++) {
			command[idx] = argv[curr];
			if (*argv[curr] == '|' || *argv[curr] == '&') break;
		}
		command[idx] = NULL;

		if (argv[curr] == NULL) {
			if (!builtin_command(command)) {
				pid_t pid;
				if ((pid = Fork()) == 0) {
					if (execvp(command[0], command) < 0) {
						printf("%s: Command not found.\n", command[0]);
						exit(0);
					}
				}
				int status;
				if (Waitpid(pid, &status, 0) < 0) unix_error("waitpid error\n");
			}
		}
		else {
			if ('|' == *argv[curr++]) {
				if (!builtin_command(command)) {
					pid_t pid;
					int fd2[2];
					if (pipe(fd2)) putchar("error");

					if ((pid = Fork()) == 0) {
						close(fd2[0]);
						Dup2(fd2[1], STDOUT_FILENO);
						close(fd2[1]);

						if (execvp(command[0], command) < 0) {
							printf("%s: Command not found.\n", command[0]);
							exit(0);
						}
					}

					int status;
					pid_t wpid = Waitpid(pid, &status, 0);
					close(fd2[1]);
					Dup2(fd2[0], STDIN_FILENO);
					close(fd2[0]);
				}
			}
		}
		argc = curr;
	}

	dup2(fd[0], 0);
	close(fd[0]);
	dup2(fd[1], 1);
	close(fd[1]);
}
/*****/

/* $begin eval */
/* eval - Evaluate a command line */
void eval(char *cmdline) 
{
    char *argv[MAXARGS]; /* Argument list execve() */
    char buf[MAXLINE];   /* Holds modified command line */
    int bg;              /* Should the job run in bg or fg? */
    pid_t pid;           /* Process id */
	int argc = 0; /*****/

    strcpy(buf, cmdline);
    bg = parseline(buf, argv); 
    if (argv[0] == NULL) return; /* Ignore empty lines */
	else multiple(argv, argc); /* Execute multiple commands in Shell */

    return;
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


