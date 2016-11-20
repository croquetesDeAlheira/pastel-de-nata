	/*
*		Grupo 12
* @author Daniel Santos 44887
* @author Luis Barros  47082
* @author Marcus Dias 44901
*/


/*
   Programa que implementa um servidor de uma tabela hash com chainning.
   Uso: table-server <porta TCP> <dimensão da tabela>
   Exemplo de uso: ./table_server 54321 10
*/
#include <error.h>
#include <errno.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <signal.h>


#include "../include/inet.h"
#include "../include/table-private.h"
#include "../include/message-private.h"
#include "../include/table_skel.h"

#define ERROR -1
#define OK 0
#define TRUE 1 // boolean true
#define FALSE 0 // boolean false
#define NCLIENTS 4 // Número de sockets (uma para listening)
#define TIMEOUT -1 // em milisegundos

//variaveis globais
int i;
int numFDs = 1;
struct pollfd socketsPoll[NCLIENTS];


void finishServer(int signal){
    //close dos sockets
    for (i = 0; i < numFDs; i++){
    	if(socketsPoll[i].fd >= 0){
     		close(socketsPoll[i].fd);
     	}
 	}
	table_skel_destroy();
	printf("\n :::: -> SERVIDOR ENCERRADO <- :::: \n");
	exit(0);
}







/* Função para preparar uma socket de receção de pedidos de ligação.
*/
int make_server_socket(short port){
  int socket_fd;
  int rc, on = 1;
  struct sockaddr_in server;

  if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
    perror("Erro ao criar socket");
    return -1;
  }

  //make it reusable
  rc = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on));
  if(rc < 0 ){
  	perror("erro no setsockopt");
  	close(socket_fd);
  	return ERROR;
  }

  server.sin_family = AF_INET;
  server.sin_port = htons(port);  
  server.sin_addr.s_addr = htonl(INADDR_ANY);



  if (bind(socket_fd, (struct sockaddr *) &server, sizeof(server)) < 0){
      perror("Erro ao fazer bind");
      close(socket_fd);
      return -1;
  }

  //o segundo argumento talvez nao deva ser 0, para poder aceitar varios FD's
  if (listen(socket_fd, 0) < 0){
      perror("Erro ao executar listen");
      close(socket_fd);
      return -1;
  }
  return socket_fd;
}

/* Função que garante o envio de len bytes armazenados em buf,
   através da socket sock.
*/
int write_all(int sock, char *buf, int len){
	int bufsize = len;
	while(len > 0){
		int res = write(sock, buf, len);
		if(res < 0){
			if(errno == EINTR) continue;
			perror("write failed:");
			return res;
		}
		buf+= res;
		len-= res;
	}
	return bufsize;
}

/* Função que garante a receção de len bytes através da socket sock,
   armazenando-os em buf.
*/
int read_all(int sock, char *buf, int len){
	int bufsize = len;
	while(len > 0){
		int res = read(sock, buf, len);
		if(res == 0){
			//client disconnected...
			return ERROR;
		}
		if(res < 0){
			if(errno == EINTR) continue;
			perror("read failed:");
			return res;
		}
		buf+= res;
		len-= res;
	}
	return bufsize;
}



/* Função "inversa" da função network_send_receive usada no table-client.
   Neste caso a função implementa um ciclo receive/send:

	Recebe um pedido;
	Aplica o pedido na tabela;
	Envia a resposta.
*/
int network_receive_send(int sockfd){
	char *message_resposta, *message_pedido;
	int msg_length;
	int message_size, msg_size, result;
	struct message_t *msg_pedido, *msg_resposta;

	/* Com a função read_all, receber num inteiro o tamanho da 
	   mensagem de pedido que será recebida de seguida.*/
	result = read_all(sockfd, (char *) &msg_size, _INT);
	/* Verificar se a receção teve sucesso */
	if(result != _INT || result == ERROR){return ERROR;}



	/* Alocar memória para receber o número de bytes da
	   mensagem de pedido. */
	message_size = ntohl(msg_size);
	message_pedido = (char *) malloc(message_size);

	/* Com a função read_all, receber a mensagem de resposta. */
	result = read_all(sockfd, message_pedido, message_size);

	/* Verificar se a receção teve sucesso */
	if(result != message_size){return ERROR;}
	/* Desserializar a mensagem do pedido */
	msg_pedido = buffer_to_message(message_pedido, message_size);

	/* Verificar se a desserialização teve sucesso */
	if(msg_pedido == NULL){return ERROR;}

	/* Processar a mensagem */
	msg_resposta = invoke(msg_pedido);

	if(msg_resposta == NULL){ // erro no invoke
		return ERROR;
	}

	/* Serializar a mensagem recebida */
	printf("serializar agora\n");
	message_size = message_to_buffer(msg_resposta, &message_resposta);
	printf("size = %d\n",  message_size);
	/* Verificar se a serialização teve sucesso */
	if(message_resposta <= OK){return ERROR;}
	/* Enviar ao cliente o tamanho da mensagem que será enviada
	   logo de seguida
	*/
	msg_size = htonl(message_size);
	result = write_all(sockfd, (char *) &msg_size, _INT);
	/* Verificar se o envio teve sucesso */
	if(result != _INT){return ERROR;}

	/* Enviar a mensagem que foi previamente serializada */
	result = write_all(sockfd, message_resposta, message_size);

	/* Verificar se o envio teve sucesso */
	if(result != message_size){return ERROR;}
	/* Libertar memória */

	free(message_resposta);
	free(message_pedido);
	free(msg_resposta);
	free(msg_pedido);
	return OK;
}



int main(int argc, char **argv){
	//remover nao usados depois...
	int listening_socket;
	int connsock;
	int result;
	int client_on = TRUE;
	int server_on = TRUE;
	struct sockaddr_in client;
	socklen_t size_client;
	int checkPoll;
	
	int activeFDs = 0;
	int close_conn;
	int compress_list;

	// caso seja pressionado o ctrl+c
	 signal(SIGINT, finishServer);
	
	// Nota: 3 argumentos. O nome do programa conta!
	if (argc != 3){
		printf("Uso: ./server <porta TCP> <dimensão da tabela>\n");
		printf("Exemplo de uso: ./table-server 54321 10\n");
		return ERROR;
	}

	//Codigo de acordo com as normas da IBM
	/*make a reusable listening socket*/
	listening_socket = make_server_socket(atoi(argv[1]));
	//check if done right
	if(listening_socket < 0){return -1;}
	

	/* initialize table */
	if(table_skel_init(atoi(argv[2])) == ERROR){ 
		close(listening_socket); 
		return ERROR;
	}



	//inicializa todos os clientes com 0
	memset(socketsPoll, 0 , sizeof(socketsPoll));
	//o primeiro elem deve ser o listening
	socketsPoll[0].fd = listening_socket;
	socketsPoll[0].events = POLLIN;

	/* ciclo para receber os clients conectados */

	printf("a espera de clientes...\n");
	//call poll and check
	while(server_on){ //while no cntrl c
		while((checkPoll = poll(socketsPoll, numFDs, TIMEOUT)) >= 0){

			//verifica se nao houve evento em nenhum socket
			if(checkPoll == 0){
				perror("timeout expired on poll()");
				continue;
			}else {

				/* então existe pelo menos 1 poll active, QUAL???? loops ;) */
				for(i = 0; i < numFDs; i++){
					//procura...0 nao houve return events
					if(socketsPoll[i].revents == 0){continue;}

					//se houve temos de ver se foi POLLIN
					if(socketsPoll[i].revents != POLLIN){
     					printf("  Error! revents = %d\n", socketsPoll[i].revents);
       					break;
     				}


     				//se for POLLIN pode ser no listening_socket ou noutro qualquer...
     				if(socketsPoll[i].fd == listening_socket){
     					//quer dizer que temos de aceitar todas as ligações com a nossa socket listening
						size_client = sizeof(struct sockaddr_in);
     					connsock = accept(listening_socket, (struct sockaddr *) &client, &size_client);
     					if (connsock < 0){
           					if (errno != EWOULDBLOCK){
              					perror("  accept() failed");
           			 		}
           			 		break;
          				}
          				socketsPoll[numFDs].fd = connsock;
          				socketsPoll[numFDs].events = POLLIN;
          				numFDs++;
						printf("cliente conectado\n");
     			
     					//fim do if do listening
     				}else{/* não é o listening....então deve ser outro...*/
     					close_conn = FALSE;
     					client_on = TRUE;
     					printf("cliente fez pedido\n");
     					//while(client_on){
     						//receive data
     					int result = network_receive_send(socketsPoll[i].fd);
     					if(result < 0){ 
     						//ou mal recebida ou o cliente desconectou
     						// -> close connection
     						printf("cliente desconectou\n");
     						 //fecha o fileDescriptor
     						close(socketsPoll[i].fd);
     						//set fd -1
          					socketsPoll[i].fd = -1;
          					compress_list = TRUE;
							int j;
							if (compress_list){
    							compress_list = FALSE;
    							for (i = 0; i < numFDs; i++){
    								if (socketsPoll[i].fd == -1){
    		    						for(j = i; j < numFDs; j++){
    		        						socketsPoll[j].fd = socketsPoll[j+1].fd;
    		      						}
    		    						numFDs--;
    		    					}
    							}
    						}
     					}	
	   				}//fim da ligacao cliente-servidor
     			}//fim do else
			}//fim do for numFDs
		}
			//se a lista tiver fragmentada, devemos comprimir 
	}//fim do for polls




}

