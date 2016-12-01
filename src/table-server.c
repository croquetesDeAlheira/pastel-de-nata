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
#include <stdio.h>


#include "../include/inet.h"
#include "../include/table-private.h"
#include "../include/message-private.h"
#include "../include/table_skel.h"

#define ERROR -1
#define OK 0
#define CHANGE_ROUTINE 1
#define TRUE 1 // boolean true
#define FALSE 0 // boolean false
#define NCLIENTS 10 // Número de sockets (uma para listening e uma para o stdin)
#define TIMEOUT -1 // em milisegundos

//variaveis globais
int i;
int numFds = 2; //numero de fileDescriptors
struct pollfd socketsPoll[NCLIENTS]; // o array de fds
int isPrimary; // booleano a representar se eu sou primario
int isSecondaryOn; // booleano a representar se secundario estar ligado
int listening_socket; // listening socket
int stdin_fd; // socket do stdin (keyboard input - stdin)
int connsock; // sockets conectada
int result; // resultado de operacoes
int client_on = TRUE; // booleano cliente conectado
int server_on = TRUE; // booleano servidor online
struct sockaddr_in client; // struct cliente
socklen_t size_client; // size cliente
int checkPoll; // check do poll , verificar se houve algo no poll

int activeFDs = 0; //num de fds activos
int close_conn; 
int compress_list; //booleano representa se deve fazer compress da de socketsPoll


void finishServer(int signal){
    //close dos sockets
    for (i = 0; i < numFds; i++){
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
	int changeRoutine = FALSE;

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


	/* caso seja secundario, antes de meter na tabela devemos
		alterar o opcode da msg para um que a tabela perceba
		caso o opcode não seja o esperado de um servidor
		foi enviado por um cliente, logo devemos mudar de rotina*/
	if(!isPrimary){//ANTES DO INVOKE

		int opcode = msg_pedido->opcode;
		//verificar & mudar code
		if( opcode == OC_DEL_S){
			msg_pedido->opcode = OC_DEL;
		}else if(opcode == OC_UPDATE_S ){
			msg_pedido->opcode = OC_UPDATE;
		}else if( opcode == OC_PUT_S ){
			msg_pedido->opcode = OC_PUT;
		}

		//caso não tenha mudado, veio de um cliente
		if(opcode == msg_pedido->opcode){
			//mudar rotina 


			printf("siga mudar rotina \n");
			changeRoutine = TRUE;
		}//se nao mudou, simplesmente continua...
	}
	/* Processar a mensagem */
	msg_resposta = invoke(msg_pedido);

	if(msg_resposta == NULL){ // erro no invoke
		return ERROR;
	}

	/* verificar se somos primario ou secundario
	caso seja primario , verificar se opcode
	faz alteracoes na tabela,
	se sim, enviar para o secundario */
	if(isPrimary){ //DEPOIS DO INVOKE
		//ja temos o opcode
		int opcode = msg_pedido->opcode;
		//verificar & se for algum, mudar o opcode da mensage
		if( opcode == OC_DEL ){
			msg_pedido->opcode = OC_DEL_S;
		}else if(opcode == OC_UPDATE ){
			msg_pedido->opcode = OC_UPDATE_S;
		}else if( opcode == OC_PUT ){
			msg_pedido->opcode = OC_PUT_S;
		}

		//caso tenha mudado, enviar para o secundario
		if(opcode != msg_pedido->opcode){
			//enviar
			printf("enviar para o secundario\n");
			/*
				ENVIAR PARA O SECUNDARIO AQUI
				DEVE SER FEITO ATRAVES DE UMA THREAD
			*/
		}
	}

	/* Serializar a mensagem recebida */
	message_size = message_to_buffer(msg_resposta, &message_resposta);
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
	if(changeRoutine){

		printf("send resposta\n");
		return CHANGE_ROUTINE;
	}else{
		return OK;
	}
}

int subRoutinePrimary(){
	//Codigo de acordo com as normas da IBM
	/*make a reusable listening socket*/
	/* ciclo para receber os clients conectados */
	printf("a espera de clientes - primario...\n");
	//call poll and check
	while(server_on){ //while no cntrl c
		while((checkPoll = poll(socketsPoll, numFds, TIMEOUT)) >= 0){

			//verifica se nao houve evento em nenhum socket
			if(checkPoll == 0){
				perror("timeout expired on poll()");
				continue;
			}else {
				/* então existe pelo menos 1 poll active, QUAL???? loops ;) */
				for(i = 0; i < numFds; i++){
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
          				socketsPoll[numFds].fd = connsock;
          				socketsPoll[numFds].events = POLLIN;
          				numFds++;
						printf("cliente conectado\n");
     			
     					//fim do if do listening
     				}else{
						/* não é o listening....então deve ser outro...
							etapa 4, o outro agora pode ser o stdin */
						if(socketsPoll[i].fd == stdin_fd){
							/*
							fgets(buffer, 10, socketsPoll[i].fd);
							*/
							char buffer;
							char *print = "print";
							gets(&buffer);
							// read word "print" return 0 if equals
							int equals = strcmp(print, &buffer);
							printf("string = %s , equals = %d\n", &buffer , equals);
							if(equals == 0){
								struct message_t *msg_resposta;							
								struct message_t *msg_pedido = (struct message_t *)
										malloc(sizeof(struct message_t));
								if(msg_pedido == NULL){
									perror("Problema na criação da mensagem de pedido\n");
								}
								// codes
								msg_pedido->opcode = OC_GET;
								msg_pedido->c_type = CT_KEY;
								// Skel vai verificar se content.key == !
								msg_pedido->content.key = "!";
								msg_resposta = invoke(msg_pedido);
								if(msg_resposta == NULL){
									perror("Problema na mensagem de resposta\n");
								}								
								printf("********************************\n");
								
								printf("* servidor primario\n");
								if(msg_resposta->content.keys[0] != NULL){ 
									int i = 0;
									while(msg_resposta->content.keys[i] != NULL){
										printf("* key[%d]: %s\n", i, msg_resposta->content.keys[i]);
										i++;
									}
								}else{
									printf("* tabela vazia\n");
								}
								printf("*\n********************************\n");						
							}	
							
						
						}else{
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
									for (i = 0; i < numFds; i++){
										if (socketsPoll[i].fd == -1){
				    						for(j = i; j < numFds; j++){
				        						socketsPoll[j].fd = socketsPoll[j+1].fd;
				      						}
				    						numFds--;
				    					}
									}
								}
		 					}	
						}//Fim do else de outros fd's

	   				}//fim da ligacao cliente-servidor
     			}//fim do else
			}//fim do for numFds
		}
			//se a lista tiver fragmentada, devemos comprimir 
	}//fim do for polls
	return OK;
}
int subRoutineSecondary(){
	//Codigo de acordo com as normas da IBM
	/*make a reusable listening socket*/
	/* ciclo para receber os clients conectados */
	printf("a espera de clientes secundario...\n");
	//call poll and check
	while(server_on){ //while no cntrl c
		while((checkPoll = poll(socketsPoll, numFds, TIMEOUT)) >= 0){
			//verifica se nao houve evento em nenhum socket
			if(checkPoll == 0){
				perror("timeout expired on poll()");
				continue;
			}else {
				/* então existe pelo menos 1 poll active, QUAL???? loops ;) */
				for(i = 0; i < numFds; i++){
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

						printf("cliente conectado\n");

          				//VAMOS TRATAR O PEDIDO AQUI, 1 DE CADA VEZ . NO SECUNDARIO
          				close_conn = FALSE;
          				int primary_server_on = TRUE;
          				int result = network_receive_send(connsock);
          				if(result == CHANGE_ROUTINE){
          					isPrimary = TRUE;
          					subRoutinePrimary();
          				}

     			
     					//fim do if do listening
     				}else{
						/* não é o listening....então deve ser outro...
							etapa 4, o outro agora pode ser o stdin */
						if(socketsPoll[i].fd == stdin_fd){
							/*
							fgets(buffer, 10, socketsPoll[i].fd);
							*/
							char buffer;
							char *print = "print";
							gets(&buffer);
							// read word "print" return 0 if equals
							int equals = strcmp(print, &buffer);
							printf("string = %s , equals = %d\n", &buffer , equals);
							if(equals == 0){
								struct message_t *msg_resposta;							
								struct message_t *msg_pedido = (struct message_t *)
										malloc(sizeof(struct message_t));
								if(msg_pedido == NULL){
									perror("Problema na criação da mensagem de pedido\n");
								}
								// codes
								msg_pedido->opcode = OC_GET;
								msg_pedido->c_type = CT_KEY;
								// Skel vai verificar se content.key == !
								msg_pedido->content.key = "!";
								msg_resposta = invoke(msg_pedido);
								if(msg_resposta == NULL){
									perror("Problema na mensagem de resposta\n");
								}								
								printf("********************************\n");
								
								printf("* servidor secundario\n");
								if(msg_resposta->content.keys[0] != NULL){ 
									int i = 0;
									while(msg_resposta->content.keys[i] != NULL){
										printf("* key[%d]: %s\n", i, msg_resposta->content.keys[i]);
										i++;
									}
								}else{
									printf("* tabela vazia\n");
								}
								printf("*\n********************************\n");						
							}	
						
						}else{
							/*
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
									for (i = 0; i < numFds; i++){
										if (socketsPoll[i].fd == -1){
				    						for(j = i; j < numFds; j++){
				        						socketsPoll[j].fd = socketsPoll[j+1].fd;
				      						}
				    						numFds--;
				    					}
									}
								}
		 					}	
		 					*/
						}//Fim do else de outros fd's

	   				}//fim da ligacao cliente-servidor
     			}//fim do else
			}//fim do for numFds
		}
			//se a lista tiver fragmentada, devemos comprimir 
	}//fim do for polls
	return OK;
}


int serverInit(char *myPort, char *listSize){
		listening_socket = make_server_socket(atoi(myPort));
		//check if done right
		if(listening_socket < 0){return -1;}
		
		/* initialize table */
		if(table_skel_init(atoi(listSize)) == ERROR){ 
			close(listening_socket); 
			return ERROR;
		}
		//inicializa todos os clientes com 0
		memset(socketsPoll, 0 , sizeof(socketsPoll));
		//o primeiro elem deve ser o listening
		socketsPoll[0].fd = listening_socket;
		socketsPoll[0].events = POLLIN;

		//o segundo elem deve ser o stdin (para capturar o "print")
		stdin_fd = STDIN_FILENO;
		socketsPoll[1].fd = stdin_fd;
		socketsPoll[1].events = POLLIN;
		return OK;
}
int main(int argc, char **argv){
	// caso seja pressionado o ctrl+c
	 signal(SIGINT, finishServer);
	
	/* o numero de argumentos eh diferente entre secundario e primario 
		primario = programa + seuPorto + ipSecundario + portoSecundario + listSize
		secundario = programa + seuPorto + listSize*/
	if(argc == 2){
		//primario deve ser 5 depois
		isPrimary = TRUE;

		char *myPort = /*argv[1]*/ "44901";
		char *secIP = /*argv[2]*/ "127.0.0.1";
		char *secPort = /*argv[3]*/ "44902";
		char *listSize = /*argv[4]*/ "10";

		//inicializa servidor
		result = serverInit(myPort, listSize);
		if(result == ERROR){return ERROR;}

		subRoutinePrimary();

	}else if(argc == 1){
		//secundario
		isPrimary = FALSE;

		char *myPort = /*argv[1]*/ "44902";
		char *listSize = /*argv[2]*/ "10";

		//inicializa servidor
		result = serverInit(myPort, listSize);
		if(result == ERROR){return ERROR;}

		subRoutineSecondary();
	}else{
		//errou
		printf("\nUso do primario: ./server <porta TCP> <IP secundario> <porta TCP secundario> <dimensão da tabela>\n");
		printf("	Exemplo de uso: ./table-server 54321 127.0.0.1 54322 10\n");
		printf("Uso do secundario: ./server <porta TCP> <dimensão da tabela>\n");
		printf("	Exemplo de uso: ./table-server 54321 10\n\n");
		return ERROR;
	}

}//fim main
