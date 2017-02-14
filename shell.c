#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "csapp.h"
#include "readcmd.h"
#include <time.h>
#include <signal.h>

/*
Structure representant les jobs
*/
struct job{
	char* cmd; //Commande du processus
	int pid; //pid du processus
};

struct job** enCours;// = malloc(sizeof(struct job)*1);
int nbEnCours = 0;
int child = 0;

/*
Cette fonction permet d'executer une commande simple sans redirection et sans tube
*/
void commande_simple(struct cmdline *l){
	//Processus fils cree un processus fils qui devra executer la commande
	int pid = Fork();
	int status;
	if(pid == 0){
		/*Si on est le fils alors
			on execute la commande passe en paramettre
			on s'arrette
		*/
		execvp(l->seq[0][0], l->seq[0]);
		exit(0);
	}else{
		/*
		Le pere attend la fin de son fils
		*/
		while(waitpid(pid, &status, 0) != pid);
	}
}

/*
Cette fonction permet d'executer une commande avec redirection des flux sans pipe
*/
void commande_redirection(struct cmdline *l){
	/*
	fIn : le descripteur de fichier d'entrer, initialiser a 1, l'entree standard
	fOut : le descripteur de fichier de sortie, initialiser a 1, le sortie standard
	*/
	int fOut = 1; int fIn = 0;

	if(l->in != NULL)
		/*
		Si, ete present, '> <fic>' alors fIn prend la valeur du descripteur de <fic>
		*/
		fIn = open(l->in, O_RDONLY,0);
	if(l->out != NULL)
		fOut = open(l->out, O_WRONLY | O_CREAT, 0700);
		/*
		Si, ete present, '< <fic>' alors fOut prend la valeur du descripteur de <fic>
		*/
	int pid = Fork(); int status;
	if(pid == 0){
		if(fOut != 1){
			/*
			Si fOut diffrenrent de 1 alors
				On ferme la sortie standard
				La sortie de l'execution devient le fichier descrit par fOut
			*/
			close(1);
			dup2(fOut, 1);
		}

		/*
			Si fIn diffrenrent de i alors
				On ferme l'entree standard
				L'entree de l'execution devient le fichier descrit par fIn
				On execute
				On s'arrete
			Sinon
				On execute
				On s'arrete
			*/
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
		/*
		Le pere attend la fin d'execution du fils
		*/
		while(waitpid(pid, &status, 0)!=pid);
	}
}

/*
Cette fonction permet d'executer une suite d'instruction pipee sans redirection de flus d'entrees/sorties
*/
void commande_pipe(struct cmdline *l){
	int tailleSeq; int tmp;
	//On calcul le nb de commande pipee
	for(tailleSeq=0; l->seq[tailleSeq+1]!=0; tailleSeq++);
	int pid = Fork(); int status; int i=0; int desc[2];
	if(pid == 0){
		/*
		Le premier fils creer pipe qu'il partageras avec son futur fil
		Il ferme son entrer standard et recupere comme entree la sortie du pipe
		Attend la fin de son fils
		s'execute
		s'arrete
		*/
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
			/*
			Tant qu'il reste des commandes suivantes
				Si nous sommes la derniere commande
					On ferme sa sortie standard et recupere comme sortie l'entree du pipe partage avec le pere
					on cree un pipe
					On cree un processus fils
					Si on est le pere
						On ferme  son entrer standard et recupere comme entree la sortie du pipe
						on attend la fin de son fils
						On s'execute
						On s'arrete
				Sinon
					On ferme sa sortie standard et recupere comme sortie l'entree du pipe partage avec le pere
					on cree un pipe
					On s'execute
					On s'arrete
			*/
			for(i = 1; i<=tailleSeq; i++){
				if(i+1 <= tailleSeq){
					dup2(desc[1], 1);
					close(desc[0]);
					pipe(desc);
					pid = Fork();
					if(pid != 0){
						dup2(desc[0], 0);
						close(desc[1]);
						while(waitpid(pid, &status, 0) != pid);
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
		}
	}else{
		// le processus pere attend la fin de tous ses descendants.
		while(waitpid(pid, &status, 0) != pid);
	}
}

/*
Cette fontion permet d'executer une sequence de commandes pipees avec redirection de flus d'entrees/sorties 
*/
void commande1_final(struct cmdline *l){
	if(l->seq[1]!=0){
		/*
		SI nous avons une sequences de commande pipe alors
			On gere les les flux d'entree/sortie comme pour la fonction commande_redirection
			La suite est gerer de la meme facon que dans fonction commande_pipe
		*/
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
			while(waitpid(pid, &status, 0) != pid);
		}
	}else{
		/*Si nous n'avons pas de commande pipees alors
			nous appelons la fonction commande_redirection
		*/ 
		commande_redirection(l);
	}
}

/*
Cette fonction permet de gerer les commandes avec &
*/
void commande_bg(struct cmdline *l){
	/*
	Si la commande est suivi d'un & alors
		Alors la commande est en second plan et le pere n'attend pas la fin de l'execution de son fils
	SInon
		Le pere attend la fin de l'execution de son fils
	*/ 
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
/*
Si le pere recoit un sigint alors
	il stoppe son fils
*/
void stop(int sig){
	printf("Fini\n");
	kill(child, SIGKILL);
}
/*
Si le pere recoit un sigtstp alors
	il suspend son fils
*/
void suspend(int sig){
	printf("Suspendu\n");
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
		//Le pere attend un dignal tant que son n'a pas fini son execution
	}
}


void zombi(int sig)
{
    pid_t pid;

    if ((pid = waitpid(-1, NULL, WNOHANG|WUNTRACED)) < 0)
        unix_error("waitpid error");
    return;
}
void commande_zombi(struct cmdline *l){
	Signal(SIGCHLD, zombi);
	int pid = Fork();
	if(pid == 0){
		execvp(l->seq[0][0], l->seq[0]);
		exit(0);
	}
}

void free_job(struct job** jobs, int taille){
	int i;
	for(i=0; i<taille; i++){
		free(jobs[i]->cmd);
		free(jobs[i]);
	}
	free(jobs);
}
void affiche_job(){
	int i;
	for(i=0; i<nbEnCours; i++){
		printf("[%d] \t %d %s\n", i+1, enCours[i]->pid, enCours[i]->cmd);
	}
}
void maj_job(int pid, int action){
	switch(action){
		case 0: //bg
			printf("processus %d mis au second plan\n", pid);
			break;
		case 1: //fg
			printf("processus %d mis au premier plan\n", pid);
			break;
		case 2: //stop
			printf("processus %d Suspendu\n", pid);
			kill(pid, SIGSTOP);
			break;
		default:
			printf("Action impossible\n");
	}
}
void maj_job2(int idx, int action){
	int pid = enCours[idx-1]->pid;
	maj_job(pid, action);
}

void ajout_job(struct cmdline *l, int pid){
	if(nbEnCours == 0){
		enCours = malloc(sizeof(struct job*)*(nbEnCours+1));
		enCours[nbEnCours] = malloc(sizeof(struct job)*1);
	}else{
		enCours = realloc(enCours, (sizeof(struct job*)*(nbEnCours+1)));
		enCours[nbEnCours] = malloc(sizeof(struct job)*1);
	}
	enCours[nbEnCours]->cmd = malloc(sizeof(char)*strlen(l->seq[0][0]));
	memcpy(enCours[nbEnCours]->cmd, l->seq[0][0], strlen(l->seq[0][0]));
	enCours[nbEnCours]->pid = pid;
	nbEnCours++;
}

void commande_job(struct cmdline *l){
	int pid, status;

	pid = Fork();

	if(pid == 0){
		execvp(l->seq[0][0], l->seq[0]);
		exit(0);
	}else{
		if(l->bg){
			ajout_job(l, pid);
		}
		else{
			while(waitpid(pid, &status, 0) != pid);
		}
	}
}

void commande_plan(struct cmdline *l){
	if((strcmp(l->seq[0][0], "bg") == 0) || (strcmp(l->seq[0][0], "fg") == 0) || (strcmp(l->seq[0][0], "stop") == 0)){
		int pid = Fork();
		if(pid==0){
			if(l->seq[0][1][0] == '%'){
				char tmp[strlen(l->seq[0][1])];
				memcpy(tmp, l->seq[0][1]+1, 4);
				int idx = atoi(tmp);
				if(strcmp(l->seq[0][0], "bg") == 0){
					maj_job2(idx, 0);
				}else if(strcmp(l->seq[0][0], "fg") == 0){
					maj_job2(idx, 1);
				}else{
					maj_job2(idx, 2);
				}
			}else{
				int pid = atoi(l->seq[0][1]);
				printf("%d, %s\n",pid, l->seq[0][0]);
				if(strcmp(l->seq[0][0], "bg") == 0){
					maj_job(pid, 0);
				}else if(strcmp(l->seq[0][0], "fg") == 0){
					maj_job(pid, 1);
				}else{
					printf("coucou\n");
					maj_job(pid, 2);
				}
			}
		}else{
			int stat;
			while(waitpid(pid,&stat, 0)!=pid);
		}
	}else
		commande_job(l);
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
		//Si aucune commande n'est entree
		if(l->seq[0] == 0);
		//Si la commande exit a ete entree 
		else if(strcmp("exit", l->seq[0][0]) == 0)
			exit(0);
		//Si la commande jobs a ete entree
		else if(strcmp("jobs", l->seq[0][0]) == 0){
			affiche_job();
		} else{
			commande_plan(l);
			//commande1_final(l);
		}
	}
	exit(0);
}