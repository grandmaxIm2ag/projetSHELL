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

void commande_pipe(struct  cmdline *l){
	int pid = Fork(); int status; int i=0;
		if(pid == 0){
			while(1){
				pid = Fork();
				int desc[2];
				pipe(desc);
				if(pid == 0 && l->seq[i+1] != 0){
					i++;
				} else if(pid == 0 && l->seq[i+1] == 0){
					close(desc[0]);
					dup2(desc[1],1);
					execvp(l->seq[i][0], l->seq[i]);
					exit(0);
				}else{
					close(desc[1]);
					dup2(desc[0],0);
					execvp(l->seq[i+1][0], l->seq[i+1]);
					waitpid(pid, &status, 0);
					break;
				}
			}
			exit(0);
		}else{
			waitpid(pid, &status, 0);
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

		if(l->seq[1] == 0)
			if(l->in !=NULL || l->out != NULL)
				commande_redirection(l);
			else
				commande_simple(l);
		else
			commande_pipe(l);
	}
	exit(0);
}
