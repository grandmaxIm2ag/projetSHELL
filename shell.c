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

void commande_simple(struct cmdline *l){
	int pid = Fork();
	int status;
	if(pid == 0){
		execvp(l->seq[0][0], l->seq[0]);
		exit(0);
	}
	else{
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
int main(int argc, char * argv[])
{
	while (1){
		struct cmdline *l;
		int i, j;

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

		//commande1_final(l);
		commande_bg(l);
	}
	exit(0);
}
