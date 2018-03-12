#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>


/* Dichiarazione della struttura che caratterizzerà il client all'interno del server. Questa struttura presenta rispettivamente: bit di presenza, PID del client connesso, nome associato alla pipe del client. */
struct client{
	int p; /*bit di presenza*/
	int pid;/*process id del client*/
	char client_name[9];/*nome identificativo della pipe su cui il server può scrivere al client*/
}client;	

struct client client_vector[10]; /*Vettore delle strutture associate ad ogni client connesso.*/
int fdsc; /* Dichiarazione del file descriptor della pipe da cui il server leggerà istruzioni e contenuti inviati dal client.*/


/*FUNZIONE SIGHANDLER PER LA GESTIONE DEL CRASH DA PARTE DEL SERVER*/
void get_interrupt(){
	int index = 0; /*Indice per lo scorrimento sul vettore di strutture.*/
	while(index < 10){ /*Cliclo sul vettore per avvisare tutti i client connessi della chiusura forzata da parte del server.*/
		if(client_vector[index].p == 1){
			kill(client_vector[index].pid, SIGUSR2);
			index ++;
		}else{
			index ++;
		}
	}exit(1);
}	


/*FUNZIONE DI REGISTRAZIONE E GESTIONE DEI METADATI DEL CLIENT*/
int new_client(char *name, int pid){ 
	int client_recorded;
	int index;
	client_recorded = 0; /*Viene trattato come un booleano per la guardia del while. Posto a 1 rompe la guardia e assicura che il salvataggio dei metadati è andato a buon fine. Altrimenti arriveremo alla fine del vettore (pieno).*/

	index = 0; /*Indice di scorrimento sul vettore.*/

	while(client_recorded == 0 && index < 10){
		if(client_vector[index].p != 1){	

			int i;
			client_vector[index].p = 1; /*Setto bit di presenza a 1. Indica che tale indice è ocupato dai metadati del client*/
			client_vector[index].pid = pid; /*Salvo il PID del client. Verrà utilizzato per inviare segnali in caso di ricezione/invio messaggi*/
			
			for(i = 0; i < 9; i++){ /*Ciclo sulla del nome per il salvataggio nella struttura.*/
				client_vector[index].client_name[i] = name[i];
				
			}
			client_recorded = 1;

		}else{
			index ++;
		}
		if(index == 10){

			return -1;
		}
	}
	return index; /*Ritorno l'indice del vettore in cui sono stati salvati i metadati riguardanti il nuovo client. Questo indice corrisponderà all'ID associato al client. Nel caso in cui il vettore sia pieno, ritorna -1.*/
}

/*FUNZIONE DI STAMPA DEI CLIENT CONNESSI AL SERVER*/

/*Funzione di stampa del vettore dei client connessi. Ogni locazione del vettore è rappresentata da ID|bit di presenza|. Tale coppia può essere: ID|1| in caso di presenza del client, ID|2| in caso di locazione precedentemente utilizzata ma attualmente diponibile, NULL altrimenti. Viene richiamata prima di ogni operazione richiesta dai client. Ha il solo scopo di dare una rappresentazione grafica dello stato del server.*/

void print_client_vector(){
	int i;
	for(i = 0; i < 10; i++){
		if(client_vector[i].p == 1 | client_vector[i].p == 2){
			printf("%d|%d|-->", i, client_vector[i].p);
		}else{
			printf("NULL-->");
		}
	}printf("Fine lista.\n\n");

}

/*FUNZIONE GESTIONE NUOVE CONNESSIONE*/
void new_connection(int *p){
	
	char pipeName[9]; /*Dichiaro vettore di caratteri che conterrà il nome della pipe relativa al client in connessione.*/
	int ID[1];

	int pipe_XXXX; /* Dichiarazione file descriptor per la sudetta pipe.*/


	int name = p[1]; /* Contiene nome inviato dal client PID*/

	printf("Un nuovo client vuole connettersi.\nIl suo PID è: %d.\n",name);	/*Stampa l'arrivo di una nuova connessione. Ha il solo scopo di informare lo stato di esecuzione in cui è il server.*/

	sprintf(pipeName, "pipe_%d", name); /* Concatenazione stringa "pipe_" e numero PID. Verrà utilizzata per dare nome univoco alla pipe di comunicazione specifica per il client. */

	ID [0] = new_client(pipeName, name); /*Richiamiamo la funzione di registrazione del nuovo client al server. Questa funzione tornerà l'ID da associare al client (corrispondente alla posizione del vettore che avrà occupato), o -1 nel caso in cui il vettore di client sia pieno.*/

	mknod(pipeName,S_IFIFO, 0); 

	chmod(pipeName, 0660); 
	

	do{
		pipe_XXXX = open(pipeName, O_WRONLY); 
	}while(pipe_XXXX == -1);
	
	write(pipe_XXXX, &ID, sizeof(ID)); /*Comunica al client (sulla sua relativa pipe) l'ID che gli è stato assegnato. Può essere -1 in caso di saturazione del server.*/
	close(pipe_XXXX);

	printf("L'ID ad esso associato è: %d.\n", ID[0]);/*Stampa risultato della connessione. Ha il solo scopo di informare sull'esito della connessione dal lato server.*/
	

}

/*FUNZIONE DISCONNESSIONE DAL SERVER*/
void connection_off(int *ID){
	int free_id = ID[1];
	client_vector[free_id].p = 2; /*Settiamo il bit di presenza a 2. Indica che l'indice è stato utilizzato ma è tornato disponibile per il salvataggio di nuovi metadati.*/
}


/*FUNZIONE DI INVIO DEGLI ID CLIENT CONNESSI AL SERVER*/
void send_client_list(int *ID){
	int count = 0; /*Contatore dei client attualmente connessi.*/
	char SINGLE_CLIENT[8]; /*Dichiarazione vettore stringa in cui salveremo gli 8 caratteri corrispondenti a "ID:|%d|".*/
	int i = 0; /*Indice di scorrimento sul vettore.*/
	int client_id = ID[1]; 


	while(i < 10){ /*Cliclo sul vettore per il conteggio dei client attualmente connessi e registrati.*/
		if(client_vector[i].p == 1){
			count ++;
			i ++;
		}else{
			i ++;
		}
	}
	
	int dim[1];
	dim[0] = sizeof(SINGLE_CLIENT)*count; /*Calcolo lunghezza del vettore che conterrà la stringa "ID:|%d|" per ogni client connesso. Tale vettore avrà dimensione pari alla lunghezza della stringa "ID:|%d|" (8 caratteri), moltiplicata per il numero dei client connessi.*/


	char CLIENT_LIST[dim[0] + 1]; /*Dichiarazione del sudetto vettore. Viene aggiunto 1 alla sua dimensione, in modo da poter inserire il cattere di fine stringa '\0'*/
	
	i= 0; /*L'indice sull'array viene reimpostato a zero per ricominciare il ciclo sul vettore. Questa volta verrà rianalizzato l'intero vettore per poter concatenare tutte le stringhe "ID:|%d|".*/
	
	int j; /*Indice del for che ciclerà ricorsivamente la stringa "ID:|%d|" per andarla a copiare nel vettore finale da inviare al client*/
	int index = 0;

	
	printf("Il client con ID %d richiede la lista dei client connessi.\n", client_id);
	while(i < 10){
		if(client_vector[i].p ==1){
			
			sprintf(SINGLE_CLIENT, "ID:|%d|.\n", i);

			for(j = 0; j < 8; j++){ 

				CLIENT_LIST[8*index + j] = SINGLE_CLIENT[j]; /*Viene calcolato l'indice in cui andare a scrivere il prissimo carattere.*/
			
			}

			index ++;
			i ++;
			CLIENT_LIST[dim[0]] = '\0'; /*Inserimento del carattere di fine stringa in fondo al vettore.*/
		}else{
			i++;
		}
	}


	int pipe_XXXX; /* File descriptor pipe relativa al client.*/
	

	mknod(client_vector[client_id].client_name,S_IFIFO, 0); 
	
	chmod(client_vector[client_id].client_name, 0660); 
	
	
	do{

		pipe_XXXX = open(client_vector[client_id].client_name, O_WRONLY); 

	}while(pipe_XXXX == -1);
	
	write(pipe_XXXX, &dim, sizeof(dim)); /*Invio informazione riguardante la dimensione della stringa che sarà inviata al passo successivo. In questo modo il client potrà creare a sua volta un vettore stringa su cui effettuare la read().*/

	write(pipe_XXXX, &CLIENT_LIST, sizeof(CLIENT_LIST)); /*Invio della stringa contente gli ID connessi.*/
	
	close(pipe_XXXX);
	
	
}

/*FUNZIONE GESTIONE RICEZIONE E INVIO MESSAGGI*/
void manage_msg(int *ID){
	int vector_index;
	int client_id[1]; /*Conterrà l'ID del client che vuole inviare il messaggio.*/
	client_id[0] = ID[1];
	
	int list_dim[1]; /*Conterrà la dimensione da assegnare al vettore in cui salvare la lista degli ID destinatari al momento della read().*/
	int message_dim[1]; /*Conterrà la dimensione del vettore in cui salvare il messaggio al momento della riead().*/
	
	printf("Il client con ID %d vuole inviare un messaggio.\n", client_id[0]);


	read(fdsc, list_dim, sizeof(list_dim)); /*Lettura della dimensione della lista delgi ID destinatari che il server sta per ricevere.*/
	
	
	char id_list[list_dim[0]]; /*Dichiarazione del vettore che conterrà la lista degli ID connessi. Tale li sta sarà della forma ID<spazio>ID<spazio>...ID<\n>*/
	
	read(fdsc, id_list, sizeof(id_list)); /*Lettura della lista degli ID destinatari.*/
	
	id_list[list_dim[0] - 1] = '\0'; 
	
	printf("La lista degli ID destinatari è:\n%s\n", id_list); /*Stampa della lista dal lato Server. Utile per monitorarne l'integrità.*/

	read(fdsc, message_dim, sizeof(message_dim)); /*Lettura della dimensione del messaggio che il server sta per ricevere.*/


	char message[message_dim[0]]; /*Dichiarazione del vettore in cui verrà salvato il messaggio.*/

	read(fdsc, message, sizeof(message)); /*Lettura del messaggio.*/
	message[message_dim[0] - 1] = '\0';

	printf("Il messaggio inviato dall'utente è:\n%s\n", message); /*Stampa del messaggio dal lato Server. Utile per monitorarne l'integrità.*/
	
	/*Poichè gli ID sono salvati in un array di caratteri, nel codice che segue viene effettuata la conversione da ACSII ad intero.*/
	int decade_count; /*contatore delle decine in caso di numeri a doppia cifra.*/
	int dest_id;	/*Conterrà l'ID del client destinatario.*/
	vector_index = 0; /*Indice per lo scorrimento su vettore.*/

	while(vector_index < list_dim[0] - 1){
		
		decade_count = 0;
		dest_id = 0;
		int pipe_XXXX;
		/*Nel while che segue si utilizzano gli spazi per parsare la stringa degli ID destinatari.*/
		while(id_list[vector_index]  != ' ' && vector_index < list_dim[0] - 1){	
			
			decade_count =  decade_count * 10;
			dest_id = dest_id * decade_count + (id_list[vector_index] - 48);
			
			vector_index ++;
			decade_count++;
		}

		if(dest_id > sizeof(client_vector) || client_vector[dest_id].p != 1){ /*Controllo validità degli ID destinatari. Tale controllo si avvale della dimensione del vettore e dei bit di controllo.*/
			printf("ID %d non valido.\n", dest_id); /*Stampa dal lato server il risultato della funzione.*/

		}else{
						
			printf("il PID del client destinatario è %d\n", client_vector[dest_id].pid);/*Stampa Il PID del client destinatario*/
			
			close(fdsc);

			kill(client_vector[dest_id].pid, SIGUSR1);/*Invio del segnale SIGUSR1 al client destiatario. Questo segnale permetterà a tale client di aprire la pipe ad esso dedicata e leggere il messaggio che vi sarà inviato. */
			
			mknod(client_vector[dest_id].client_name, S_IFIFO, 0);  /* Creazioe della pipe su cui il server invierà infomrazioni al client. */

			chmod(client_vector[dest_id].client_name, 0660);
			
			do{

			pipe_XXXX = open(client_vector[dest_id].client_name, O_WRONLY); /* Apertura Pipe Client-Server specifica in scrittura */

			}while(pipe_XXXX == -1);

			write(pipe_XXXX, client_id, sizeof(client_id)); /*Invio al destinatario ID del mittente.*/
			write(pipe_XXXX, message_dim, sizeof(message_dim)); /*Invio al destinatario  dimensione del messaggio per la creazione del vettore sul quale effetture la read.*/
			write(pipe_XXXX, message, sizeof(message)); /*Invio al destinatario del messaggio.*/
			close(pipe_XXXX);
			printf("Il client con ID %d ha ricevuto il messaggio.\n", dest_id); /*Stampa dal lato server il risultato della funzione.*/

	}
	vector_index ++;
	
	}
	

}

void main(){

	signal(SIGINT, get_interrupt);	
	
	while(1){
	close(fdsc);
	unlink("Spipe");
	print_client_vector(); /*Stampa iniziale dello stato dei client connessi al server*/


	mknod("Spipe", S_IFIFO, 0); /* Creazione della pipe in sola scrittura per i client e in sola lettura per il server.*/

	chmod("Spipe", 0660); /* Assegnamento dei permessi sulla pipe*/


	do{
		fdsc = open("Spipe", O_RDONLY); 

	}while(fdsc == -1);

	
	int infoclient[2]; /* Vettore che conterrà le informazioni inviate dal client, quali numero del comando da eseguire e nome( PROCESS ID DEL CLIENT ) */
	
		read (fdsc, infoclient, sizeof(infoclient)); /* Lettura informazioni*/

		switch(infoclient[0]){ 
			
			case 1 :new_connection(infoclient);
				break;
			case 2: send_client_list(infoclient);
				break;
			case 3: manage_msg(infoclient);
				break;
			case 4: printf("Il client con ID %d si sta disconettendo.\n\n", infoclient[1]);
				connection_off(infoclient);
				break;

		}
	}
}
