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
	int status; //status du job (bg, fg suspend)
};

struct job** enCours;// = malloc(sizeof(struct job)*1);
int nbEnCours = 0;
int child = 0;


/*
fonction permettant de gerer l'ajout de processus dans les jobs
*/
void ajout_job(struct cmdline *l, int pid, int status){
	/*
	Alloue ou realloue la memoire pour notre structure job
	*/
	if(nbEnCours == 0){
		enCours = malloc(sizeof(struct job*)*(nbEnCours+1));
		enCours[nbEnCours] = malloc(sizeof(struct job)*1);
	}else{
		enCours = realloc(enCours, (sizeof(struct job*)*(nbEnCours+1)));
		enCours[nbEnCours] = malloc(sizeof(struct job)*1);
	}
	/*Copie du nom de la commande du processus*/ 
	enCours[nbEnCours]->cmd = malloc(sizeof(char)*strlen(l->seq[0][0]));
	memcpy(enCours[nbEnCours]->cmd, l->seq[0][0], strlen(l->seq[0][0]));
	/*
	Copie du numéro de pid et du status
	*/
	enCours[nbEnCours]->pid = pid;
	enCours[nbEnCours]->status = status;
	nbEnCours++;
}

void free_job(struct job** jobs, int taille){
	int i;
	for(i=0; i<taille; i++){
		free(jobs[i]->cmd);
		free(jobs[i]);
	}
	free(jobs);
}

/*
Affiche les jobs en cours
*/
void affiche_job(){
		int i;
		for(i=0; i<nbEnCours; i++){
			switch(enCours[i]->status){
				case 0:
					printf("[%d] \t En cours d'execution en arriere plan \t%d \t%s\n", i+1,enCours[i]->pid,  enCours[i]->cmd);
					break;
				case 1:
					printf("[%d] \t En cours d'execution au premier plan\t%d\t %s\n", i+1,enCours[i]->pid, enCours[i]->cmd);
					break;
				default:
					printf("[%d] \t Suspendu \t%d\t %s\n", i+1, enCours[i]->pid, enCours[i]->cmd);
			}
		}
}

int getStatus(int pid){
	int k;
	for(k = 0; k<nbEnCours && enCours[k]->pid != pid; k++);
	if(k<nbEnCours)
		return enCours[k]->status;
	else
		return -1;
}

/*
Fonction permettant de supprimer de Pid pid
*/
void supprime_job(int pid){
	printf("delete %d\n",pid);
	/*
	Si il y a au moins 2 jobs, alors on modifie enCours pour qu'il n'y plus le job de Pid pid
	dans la structure enCours 
	*/
	if(nbEnCours>1){
		struct job** tmp = (struct job**)malloc(sizeof(struct job *)*(nbEnCours-1));
		int i;
		int j = 0;
		for(i=0; i<nbEnCours; i++){
			if(enCours[i]->pid != pid){
				tmp[j] = malloc(sizeof(struct job)*1);
				memcpy(tmp[j], enCours[i],sizeof(struct job));
				tmp[j]->cmd = malloc(sizeof(char)*strlen(enCours[i]->cmd));
				memcpy(tmp[j]->cmd,enCours[i]->cmd,sizeof(char)*strlen(enCours[i]->cmd));
				j++;
			}
		}
		if(j==nbEnCours-1){
			nbEnCours--;
			free_job(enCours, nbEnCours);
			enCours = malloc(sizeof(struct job *)*nbEnCours );
			memcpy(enCours,tmp,sizeof(struct job *)*nbEnCours );
		}else{
			free_job(tmp, j);
		}
	/*
	Si il n'y a qu'un seul job dans enCours
	Alors ont libere la memoire allouee pour enCours
	*/
	}else if(nbEnCours == 1){
		free_job(enCours, nbEnCours);
		nbEnCours = 0;
	}else;
}

/*
Fonction permettant de mettre a jours le status des jobs
*/
void maj_job(int pid, int action){
	if(nbEnCours == 0)
		printf("Aucun job en cours\n");
	else{
		int k;
		switch(action){
			case 0: //bg
				/*
				Si la commande bg a ete utilisee alors
					le processus doit s'executer au second plan
					on envoie doonc un signal SIGCONT pour reveiller le prcessus
					Le processus n'attend pas la fin du processus pour reprendre la main
				*/
				for(k = 0; k<nbEnCours && enCours[k]->pid != pid; k++);
				if(k<nbEnCours){
					printf("processus %d mis au second plan\n", pid);
					kill(pid, SIGCONT);
					enCours[k]->status = action;
				}
				else
					printf("Il n'y pas de processus dont le pid est : %d\n",pid);
				break;
			case 1: //fg
				/*
				Si la commande fg a ete utilisee alors
					le processus doit s'executer au premier plan
					on envoie doonc un signal SIGCONT pour reveiller le prcessus
					Le processus pere attend pas la fin du processus pour reprendre la main
				*/
				for(k = 0; k<nbEnCours && enCours[k]->pid != pid; k++);
				if(k<nbEnCours){
					child = pid;
					printf("processus %d mis au premier plan\n", pid);
					kill(pid, SIGCONT);
					enCours[k]->status = action;
					affiche_job();
					waitpid(pid, NULL, WUNTRACED);
					if(getStatus(pid) != 2 && getStatus(pid) != -1)
						supprime_job(pid);
				}	
				else
					printf("Il n'y pas de processus dont le pid est : %d\n",pid);
				break;
			case 2: //stop
			/*
				Si la commande stop a ete utilisee alors
					le processus doit etre suspendu
					on envoie doonc un signal SIGSTOP pour suspendre le prcessus
				*/
				for(k = 0; k<nbEnCours && enCours[k]->pid != pid; k++);
				if(k<nbEnCours){
					printf("processus %d Suspendu\n", pid);
					kill(pid, SIGSTOP);
					enCours[k]->status = action;
				}				
				else
					printf("Il n'y pas de processus dont le pid est : %d\n",pid);
				break;
			default:
				printf("Action impossible\n");
		}
	}
}
void maj_job2(int idx, int action){
	if(nbEnCours == 0)
		printf("Aucun job en cours\n");
	else
		if(idx <=0 || idx > nbEnCours)
			printf("Action impossible, l'indice du pid doit etre en 1 et %d\n",nbEnCours);
		else{
			int pid = enCours[idx-1]->pid;
			maj_job(pid, action);
		}
}	


void stop(int sig){
	/*
	Si le pere recoit un sigint alors
		il stoppe son fils
	*/
	printf("\nFini %d\n", child);
	supprime_job(child);
	kill(child, SIGKILL);
}

void suspend(int sig){
	/*
	Si le pere recoit un sigtstp alors
		il suspend son fils
	*/
	maj_job(child, 2);
	return;

}
void zombi(int sig)
{
	/*
	Attend la fin des processus zombis
	*/
  	waitpid(-1, NULL, WNOHANG|WUNTRACED);
}

/*
Cette fonction permet d'executer une commande simple sans redirection et sans tube
*/
void commande_simple(struct cmdline *l){
	//Processus pere cree un processus fils qui devra executer la commande
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
		waitpid(pid, &status, 0);
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
		if(l->bg){
				ajout_job(l, pid, 0);
			}else{
				child = pid;
				Signal(SIGINT, stop);	
				Signal(SIGTSTP, suspend);
				ajout_job(l, pid, 1);
				waitpid(pid, &status, WUNTRACED);
				if(getStatus(pid) != 2 && getStatus(pid)!=-1){
					supprime_job(pid);
				}
			}
	}
}

/*
Cette fonction permet d'executer une suite d'instruction pipee sans redirection de flus d'entrees/sorties
*/
void commande_pipe(struct cmdline *l, int out, int in){
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
			dup2(out, 1);
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
						waitpid(pid, &status, 0);
						execvp(l->seq[tailleSeq - i][0], l->seq[tailleSeq-1]);
						exit(0);
					}
				} else{
					dup2(in, 0);
					dup2(desc[1], 1);
					close(desc[0]);
					execvp(l->seq[0][0], l->seq[0]);
					exit(0);
				}
			}		
		}
	}else{
		// le processus pere attend la fin de tous ses descendants.
		child = pid;
		if(l->bg){
			ajout_job(l, pid, 0);
		}else{
			ajout_job(l, pid, 1);
			while(waitpid(pid, &status, WUNTRACED) != pid);
			supprime_job(pid);
		}

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
		commande_pipe(l,fOut, fIn );
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
Cette fonction permet de gerer les sinaux SIGINT et SIGSTSTP
*/
void commande_signaux(struct cmdline *l){

	int pid = Fork();
	int status;
	Signal(SIGINT, stop);	
	Signal(SIGTSTP, suspend);

	if(pid == 0){
		execvp(l->seq[0][0], l->seq[0]);
		exit(0);
	}else{
		child = pid;
		while(waitpid(pid, &status, WUNTRACED) != pid);
		//Le pere attend un signal tant que son fils n'a pas fini son execution
	}
}

void commande_zombi(struct cmdline *l){
	Signal(SIGCHLD, zombi);
	int pid = Fork();
	if(pid == 0){
		execvp(l->seq[0][0], l->seq[0]);
		exit(0);
	}
}

/*
Fonction permettant de gerer l'ajout et l'affichage'n de job
*/
void commande_job(struct cmdline *l){
	int pid, status;

	pid = Fork();
	Signal(SIGINT, stop);	
	Signal(SIGTSTP, suspend);
	if(pid == 0){
		execvp(l->seq[0][0], l->seq[0]);
		exit(0);
	}else{
		if(l->bg){
			ajout_job(l, pid, 0);
		}
		else{
			child = pid;
			ajout_job(l, pid, 1);
			while(waitpid(pid, &status, WUNTRACED) != pid);
			supprime_job(pid);
		}
	}
}


/*
Fonction permettant de gerer les fonctions en premier/second plan ou suspendu, et les jobs.
*/
void commande(struct cmdline *l){
	/*
	Attente de signaux
	*/
	Signal(SIGCHLD, zombi);
	Signal(SIGINT, stop);	
	Signal(SIGTSTP, suspend);
	/*
	Permet de gerer les commandes fg, bg, stop.
	C'est le preocessus pere qui s'occupe de mettre a jour les jobs
	*/
	if((strcmp(l->seq[0][0], "bg") == 0) || (strcmp(l->seq[0][0], "fg") == 0) || (strcmp(l->seq[0][0], "stop") == 0)){
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
			if(strcmp(l->seq[0][0], "bg") == 0){
				maj_job(pid, 0);
			}else if(strcmp(l->seq[0][0], "fg") == 0){
				maj_job(pid, 1);
			}else{
				maj_job(pid, 2);
			}
		}
	}else if(l->seq[1]!=0){
		/*
		Si nous avons une sequences de commande pipe alors
		On utilise la fonction commande_pipe modifié pour erer l'ajout/suppression de job
		*/

		int fOut = 1; int fIn = 0;
			if(l->in != NULL)
			fIn = open(l->in, O_RDONLY,0);
		if(l->out != NULL)
			fOut = open(l->out, O_WRONLY | O_CREAT, 0700) ;

		commande_pipe(l,fOut, fIn );

	}
	else{
		/*Si nous n'avons pas de commande pipees alors
			nous appelons la fonction commande_redirection modifié pour pouvoir gerer l'jout/suppression de jobs
		*/ 
		commande_redirection(l);
	}
}

int main(int argc, char * argv[])
{
	while (1){
		Signal(SIGINT, stop);	
		Signal(SIGTSTP, suspend);
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
		else if(strcmp("exit", l->seq[0][0]) == 0){
			free_job(enCours, nbEnCours);
			exit(0);
		}
		//Si la commande jobs a ete entree
		else if(strcmp("jobs", l->seq[0][0]) == 0){
			affiche_job();
		} else{
			commande(l);
		}
	}
	exit(0);
}