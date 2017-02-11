/*
	A faire :
		Verifier que l'on peut entrer deux fois "entrer" sans bug
		Ajouter une commande pour sortir du shell

		github : Xaalan
*/ 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "csapp.h"
#include "readcmd.h"
#include <time.h>
#include <signal.h>

struct job{
	char* cmd;
	int pid;
};

struct job** enCours;// = malloc(sizeof(struct job)*1);
int nbEnCours = 0;
int child = 0;

void commande_simple(struct cmdline *l){
	int pid = Fork();
	int status;
	if(pid == 0){
		execvp(l->seq[0][0], l->seq[0]);
		exit(0);
	}else{
		waitpid(pid, &status, 0);
	}
}

void commande_redirection(struct cmdline *l){
	int fOut = 1; int fIn = 0;

	if(l->in != NULL)
		fIn = open(l->in, O_RDONLY,0);
	if(l->out != NULL)
		fOut = open(l->out, O_WRONLY | O_CREAT, 0700) ;

	int pid = Fork(); int status;
	if(pid == 0){
		if(fOut != 1){
			close(1);
			dup2(fOut, 1);
		}
		if(fIn == 0){
			execvp(l->seq[0][0], l->seq[0]);
			exit(0);
		}else{
			close(0);
			dup2(fIn, 0);
			execvp(l->seq[0][0], l->seq[0]);
			exit(0);
		}
		
	}
	else{
		waitpid(pid, &status, 0);
	}
}

void commande_pipe(struct cmdline *l){
	int tailleSeq; int tmp;
	for(tailleSeq=0; l->seq[tailleSeq+1]!=0; tailleSeq++);
	int pid = Fork(); int status; int i=0; int desc[2];
	if(pid == 0){
		pipe(desc);
		pid=Fork();
		if(pid != 0){
			dup2(desc[0], 0);
			close(desc[1]);
			while( (tmp = waitpid(pid, &status, WNOHANG|WUNTRACED)) != pid);
			execvp(l->seq[1][0], l->seq[tailleSeq]);
			close(desc[0]);
			exit(0);
		}else{
			for(i = 1; i<=tailleSeq; i++){
				if(i+1 <= tailleSeq){
					dup2(desc[1], 1);
					close(desc[0]);
					pipe(desc);
					pid = Fork();
					if(pid != 0){
						dup2(desc[0], 0);
						close(desc[1]);
						waitpid(pid, &status, 0);
						execvp(l->seq[tailleSeq - i][0], l->seq[tailleSeq-1]);
						exit(0);
					}
				} else{
					dup2(desc[1], 1);
					close(desc[0]);
					execvp(l->seq[0][0], l->seq[0]);
					exit(0);
				}
			}		
					dup2(desc[1], 1);
					close(desc[0]);
					printf("test2\n\n");
					execvp(l->seq[0][0], l->seq[0]);
					close(desc[1]);
					exit(0);
		}
	}else{
		waitpid(pid, &status, 0);
	}
}


void commande1_final(struct cmdline *l){
	if(l->seq[1]!=0){

		int fOut = 1; int fIn = 0;
			if(l->in != NULL)
			fIn = open(l->in, O_RDONLY,0);
		if(l->out != NULL)
			fOut = open(l->out, O_WRONLY | O_CREAT, 0700) ;

		int tailleSeq; int tmp;
		for(tailleSeq=0; l->seq[tailleSeq+1]!=0; tailleSeq++);

		int pid = Fork(); int status; int i=0; int desc[2];
		if(pid == 0){
			pipe(desc);
			pid=Fork();
			if(pid != 0){
				dup2(fOut, 1);
				dup2(desc[0], 0);
				close(desc[1]);
				while( (tmp = waitpid(pid, &status, WNOHANG|WUNTRACED)) != pid);
				execvp(l->seq[1][0], l->seq[tailleSeq]);
				close(desc[0]);
				exit(0);
			}else{
				for(i = 1; i<=tailleSeq; i++){
					if(i+1 <= tailleSeq){
						dup2(desc[1], 1);
						close(desc[0]);
						pipe(desc);
						pid = Fork();
						if(pid != 0){
							dup2(desc[0], 0);
							close(desc[1]);
							waitpid(pid, &status, 0);
							execvp(l->seq[tailleSeq - i][0], l->seq[tailleSeq-1]);
							exit(0);
						}
					} else{
						dup2(fIn, 0);
						dup2(desc[1], 1);
						close(desc[0]);
						execvp(l->seq[0][0], l->seq[0]);
						close(desc[1]);
						exit(0);
					}
				}		
			}
		}else{
			waitpid(pid, &status, 0);
		}
	}else{
		commande_redirection(l);
	}
}

void commande_bg(struct cmdline *l){
	int pid, status;

	pid = Fork();

	if(pid == 0){
		execvp(l->seq[0][0], l->seq[0]);
		exit(0);
	}else{
		if(l->bg);
		else{
			waitpid(pid, &status, 0);
		}
	}
}

void stop(int sig){
	kill(child, SIGKILL);
}
void suspend(int sig){
	kill(child, SIGSTOP);
}
void commande_signaux(struct cmdline *l){

	int pid = Fork(); int status;

	Signal(SIGINT, stop);	
	Signal(SIGTSTP, suspend);

	if(pid == 0){
		execvp(l->seq[0][0], l->seq[0]);
		exit(0);
	}else{
		child = pid;
		Signal(SIGINT, stop);
		Signal(SIGTSTP, suspend);
		while (waitpid(pid, &status, 0) != pid);
	}
}

void zombi(int sig)
{
    pid_t pid;

    if ((pid = waitpid(-1, NULL, WNOHANG|WUNTRACED)) < 0)
        unix_error("waitpid error");
    printf("Handler reaped child %d\n", (int)pid);
    return;
}

void commande_zombi(struct cmdline *l){
	Signal( SIGCHLD, zombi);
	int pid = Fork();
	int status;
	if(pid == 0){
		execvp(l->seq[0][0], l->seq[0]);
		exit(0);
	}
}

void commande_job(struct cmdline *l){
	int pid, status;

	pid = Fork();

	if(pid == 0){
		execvp(l->seq[0][0], l->seq[0]);
		printf("coucou");
		exit(0);
	}else{
		if(l->bg){
			if(nbEnCours == 0){
				enCours = malloc(sizeof(struct job*)*(nbEnCours+1));
			}else{
				enCours = realloc(enCours, nbEnCours+1);
			}
			enCours[nbEnCours]->cmd = malloc(sizeof(char)*strlen(l->seq[0][0]));
			enCours[nbEnCours]->cmd = l->seq[0][0];
			//enCours[nbEnCours]->pid = getpid();
			nbEnCours++;
		}
		else{
			waitpid(pid, &status, 0);
		}
	}
}

int main(int argc, char * argv[])
{
	while (1){
		struct cmdline *l;

		printf("shell> ");
		l = readcmd();

		/* If input stream closed, normal termination */
		if (!l) {
			printf("exit\n");
			exit(0);
		}


		if (l->err) {
			/* Syntax error, read another command */
			printf("error: %s\n", l->err);
			continue;
		}

		if(strcmp("exit", l->seq[0][0]) == 0)
			exit(0);
		else if(strcmp("jobs", l->seq[0][0]) == 0){
			printf("%d\n",nbEnCours);
			int i;
			for(i=0; i<nbEnCours; i++){
				printf("[%d] \t status %s\n", i+1, enCours[i]->cmd);
			}
		}
		//commande1_final(l);
		//commande_signaux(l);
		commande_job(l);
	}
	exit(0);
}
