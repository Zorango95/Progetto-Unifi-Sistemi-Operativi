#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

	int connection = 0; /* Indica lo stato della connessione: 0 = non connesso. 1 = connesso.*/
        int myID[1];
	int Spipe; /* Dichiarazione file descriptor della pipe per l'invio di informazioni al server. */
	char pipeName[9];
	int client_pipe; /* Dichiarazione file descriptor della pipe per la ricezione di informazioni dal server. */
	int name;
/*FUNZIONE DI STAMPA DEL MENÙ DELLE OPERAZIONI*/
void stampaMenu(){

	printf("\nInserire un intero corrispondente alle seguenti istruzioni:\n\n");
	printf("1 = Connessione al server.\n");
	printf("2 = Richiesta elenco ID dei Client connessi.\n");
	printf("3 = Invio messaggio testuale ad un client o ad un gruppo di client.\n");
	printf("4 = Disconnessione dal server.\n");
	printf("5 = Chiusura del client.\n\n");


}

/*FUNZIONE DI CONNESSIONE AL SERVER*/
void connessione(int *id){
	//int attempt_count = 0; /*Contatore che tiene traccia dei tentativi effettuati per aprire la pipe con cui comunicare informazioni al server.*/
	printf("Mi sto connettendo\n");
	int info[2]; /* Dichiarazione vettore che conterrà numero dell'operazione e nome client (PID). */

	name = getpid(); /*Associazione PID al nome del client.*/

	info[0] = 1; /* Numero operazione da inviare al server.*/
	info[1] = name; /*PID da inviare al server*/

	do{
		Spipe = open("Spipe", O_WRONLY/*, O_NONBLOCK*/);  /* Apertura della pipe con cui comunicare informazioni al server. In questo punto avremmo voluto utilizzare la label O_NONBLOCK per non bloccare l'apertura della pipe ed eventualmente stampare un messaggio di avviso per notificare che il server non è raggiungibile nel caso in cui un client provi a connettersi prima che il server sia stato avviato. Ciò non è stato possibile poichè anche inserendo la label O_NONBLOCK, l'apertura della pipe risulta comunque essere bloccante. Quindi provare a connettere un client al server prima che quest'ultimo sia già avviato, porta il client ad un punto morto. L'unica recovery possibile è la combinazione di tasti Ctrl+C, che permette al client di terminare per essere poi rilanciato una volta che il server diventa operativo. Di seguito sono commentati frammenti di codice utilizzati nel caso in cui O_NONBLOCK funzionasse come desiderato.*/

		//attempt_count ++;
		//printf("%d\n", attempt_count);

	}while(Spipe == -1 /*&& attempt_count < 3*/);
	/*if(attempt_count == 3){
		printf("Il server non è momentaneamente raggiungibile.\nRiprova più tardi.\n");
	}else{*/


		write(Spipe, &info, sizeof(info)); /* Invio dell'operazione e del PID del client che si sta connettendo. */
		close(Spipe);

		sprintf(pipeName, "pipe_%d", name); /* Concatenazione della stringa "pipe_" e nome (PID) del client. Sarà il nome utilizzato per indicare la pipe dal quale il client riceverà informazioni dal server. */

		do{
			client_pipe = open(pipeName, O_RDONLY);
		}while(client_pipe == -1);

		read(client_pipe, myID, sizeof(myID)); /*Lettura dell'ID con cui il server ha identificato il client. Questo ID corrisponde all'indice del vettore mantenuto dal server, in cui i metadati del nuovo client sono stati salvati.*/

		if(myID[0] == -1){
	/* Un ID uguale a -1 indica una condizione in cui il server ha esaurito la capacità del vettore in cui memorizzare i metadati dei client connessi. Viene quindi stampato il seguente messaggio:*/

			printf("Il server è momentaneamente occupato. Riprova più tardi.\n");

		}else{
	/*Altrimenti viene imposata la variabile globale connection = 1, che indica una connessione avvenuta con successo, e stampato l'ID con cui il server ha identificato il client.*/
			connection = 1;
			printf("L'ID associato a questa sessione è: %d\n\n\n" , myID[0]);
		}

		close(client_pipe);
		unlink(pipeName);
	//} /*È la parentesi di chiusura dell' if/else commentato in precedenza.*/
}

/*FUNZIONE PER LA DISCONNESSIONE DAL SERVER*/
void connection_off(){
	int info[2]; /* Dichiarazione vettore che conterrà numero dell'operazione e nome client (PID). */
	info[0] = 4; /*Numero operazione da inviare al server.*/
	info[1] = myID[0]; /*ID con cui il server potrà identificare quale client si sta disconnettendo.*/
	do{
		Spipe = open("Spipe", O_WRONLY);

	}while(Spipe == -1);

	write(Spipe, &info, sizeof(info)); /*Invio del numero dell'operazione e dell'ID del client che si sta disconnettendo.*/
	connection = 0; /*Viene quindi impostata la variabile globale connection = 0 per indicare che il client non è connesso al server.*/
	close(Spipe);

}


/*FUNZIONE SIGHANDLER LA LETTURA DEI MESSAGGI*/
void read_message(){
	int sender_id[1]; /*Dichiarazione del vetore che conterrà l'ID del client mittente.*/

	do{
		client_pipe = open(pipeName, O_RDONLY);
	}while(client_pipe == -1);

	read(client_pipe, sender_id, sizeof(sender_id)); /*Lettura dell'ID corrispondente al client mittente.*/

	printf("\nHAI UN NUOVO MESSAGGIO.\n\nIl client con ID %d scrive:\n", sender_id[0]); /*Stampa l'avvenuta ricezione del messaggio, e l'ID del client mittente.*/

	int message_dim[1]; /*Dichiarazione vettore che conterrà la dimensione del messaggio. Utilizzato in seguito per dichiarare il vettore in cui salvare il messaggio.
*/
	read(client_pipe, message_dim, sizeof(message_dim)); /*Lettura della dimensione del messaggio.*/

	char message[message_dim[0] + 1]; /*Dichiarazione del vettore che conterrà il messaggio.*/

	read(client_pipe, message, sizeof(message)); /*Lettura/ricezione del messaggio.*/

	message[message_dim[0]] = '\0';

	printf("\n[%s]\n\n", message); /*Stampa il messaggio ricevuto.*/

	stampaMenu(); /*Richiamiamo infine la funzione di stama del menù. */

	unlink(pipeName);
	close(client_pipe);

}

/*FUNZIONE SIGHANDLER PER LA CHIUSURA FORZATA Ctrl-C*/
void get_interrupt(){
	printf("\n");
	if(connection == 1){ /*SIGINT potrebbe essere sollevato a prescindere dalla connesione del client al server.*/
		connection_off(); /*In presenza di connessione viene richiamata la funzione per la disconnessione, così da informare il server della chiusura da parte del client. Quindi si esce dal programma*/

		printf("Chiusura del client.\n");
		exit(0);
	}else{

		printf("Chiusura del client.\n");
		exit(0);
	}
}


/*FUNZIONE PER LA RICHIESTA DEGLI ID CONNESSI*/
void require_client_list(){
	int ans_dim[1], info[2]; /*Dichirazione vettore che conterrà la dimensione della lista degli ID. Dichiarazione vettore che conterrà numero dell'operazione e nome client (PID). */

	info[0] = 2; /*Numero dell'operazione da inviare al server*/
	info[1] = myID[0];/*Invio dell'identificatvo.*/
	do{
		Spipe = open("Spipe", O_WRONLY);
	}while(Spipe == -1);
	write(Spipe, &info, sizeof(info));/*Invio dei parametri al server.*/
	close(Spipe);

	do{
		client_pipe = open(pipeName, O_RDONLY);/*Apertura in lettura della pipe in cui il server ha inviato la lista degli ID connessi.*/
	}while(client_pipe == -1);

	read(client_pipe, ans_dim, sizeof(ans_dim));/*Ricezione dimensione della lista.*/

	char ans[ans_dim[0] + 1];
	read(client_pipe, ans, sizeof(ans));/*Ricezione della lista ID.*/
	ans[ans_dim[0]] = '\0';
	printf("Gli id connessi sono:\n%s\n", ans);
	close(client_pipe);
	unlink(pipeName);
}

/*FUNZIONE DI INVIO DEI MESSAGGI*/
void send_msg(char *id_list, char *message){
	int list_dim[1], message_dim[1], info[2]; /* Dichiarazione vettore che conterrà numero dell'operazione e nome client (PID). Dichiarazione vettore che conterrà dimensione del messaggio. Dichiarazione del vettore che conterrà i parametri da inviare al server.*/
	info[0] = 3;
	info[1] = myID[0];/*Settaggio dei parametri da inviare al sever.*/

	do{
		Spipe = open("Spipe", O_WRONLY);  /* Apertura della pipe per l'invio dei parametri.*/
	}while(Spipe == -1);

	write(Spipe, info, sizeof(info));/*Invio dei parametri al server.*/

	list_dim[0] = strlen(id_list);

	write(Spipe, list_dim, sizeof(list_dim));/*Invio lunghezza del vettore su cui salvare la lista degli ID destinatari.*/

	write(Spipe, id_list, strlen(id_list));/*Invio lista degli ID destinatari.*/

	message_dim[0] = strlen(message);
	message[message_dim[0]] = '\0';

	write(Spipe, message_dim, sizeof(message_dim));/*Invio lunghezza del vettore su cui salvare il messaggio.*/

	write(Spipe, message, strlen(message));/*Invio del messaggio.*/

	close(Spipe);
}

/*FUNZIONE SIGHANDLER PER LA GESTIONE DI CRASH DA PARTE DEL SERVER*/
void manage_server_crash(){
/*In corrispondenza del segnale SIGUSR2 viene notificato il crash del server ed eseguita la chiusura del client.*/
	printf("Il server è stato chiuso forzatamente e la connessione è stata interrotta.\nChiusura del client.\nPRIMA DI TENTARE UNA NUOVA CONNESSIONE ASSICURARSI CHE IL SERVER SIA OPERATIVO\n");
	exit(0);

}

void main(){
	int comando = 0;

	signal(SIGINT, get_interrupt);/**/
	signal(SIGUSR1, read_message);
	signal(SIGUSR2, manage_server_crash);

	while(1){


		if(connection == 0){
			do{

				stampaMenu();
				scanf("%d", &comando);/*Lettura del comando da eseguire.*/
				printf("\n");
				if(comando == 1){
					printf("Esecuzione comando 1.");
					connessione(myID);
				}else if(comando == 5){
					printf("Esecuzione comando 5.\n");
					printf("Esco dal programma.\n");
					exit(0);
				}else{
					printf("È RICHIESTA UNA CONNESSIONE AL SERVER.\n\n");
				}

			}while(connection != 1);



		}else{
			do{

				stampaMenu();
				scanf("%d", &comando);
				printf("\n");

					switch(comando){


						case 1 : printf("Esecuzione comando 1.\n");
							 printf("Connessione eseguita.\n");
							 break;
						case 2 : printf("Esecuzione comando 2.\n");
							 require_client_list();/*Richimamo funzione per la richiesta degli ID connessi.*/
							 break;
						case 3 : printf("esecuzione comando 3.\n");

							 char id_list[32], message[2048];	/*Sono rispettivamente il vettore per la memorizzazione degli ID destinatari e del messaggio. Per la lista degli ID è stato scelto di inserirli in un vettore di caratteri per poter utilizzare gli spazi come indici per il parsing.*/

							 printf("Inserire gli ID destinatari del messaggio.\n");
							 getchar();
							 fgets(id_list, 32, stdin);	/*Immissione destinatari del messaggio*/

							 printf("Inserire corpo del messaggio.\n");
							 fgets(message, 2048, stdin);	/*Immissione corpo del messaggio*/

							 send_msg(id_list, message);
							 break;
						case 4 : printf("Esecuzione comando 4.\n");
							 connection_off();/*Richiamo funzione per la disconnessione*/
							 printf("Mi disconnetto dal server.\n\n");
							 break;
						case 5 : printf("Esecuzione comando 5.\n");
							 printf("Esco dal programma.\n");
							 connection_off();/*Richiamo funzione per la disconnessione*/
							 exit(0);
			 			default : printf("Inserimento errato. Riprova.\n\n");
					}		 main();


			}while(connection == 1);
		}
	}
}
