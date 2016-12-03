	/*
*		Grupo 12
* @author Daniel Santos 44887
* @author Luis Barros  47082
* @author Marcus Dias 44901
*/


/*
   Programa que implementa um servidor de uma tabela hash com chainning.
   Uso: table-serverAux <porta TCP> <dimensão da tabela>
   Exemplo de uso: ./table_serverAux 54321 10
*/
#include <error.h>
#include <errno.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <signal.h>
#include <stdio.h>
#include <pthread.h>


#include "../include/inet.h"
#include "../include/table-private.h"
#include "../include/message-private.h"
#include "../include/table_skel.h"
#include "../include/primary_backup-private.h"

#define ERROR -1
#define OK 0
#define CHANGE_ROUTINE 1
#define TRUE 1 // boolean true
#define FALSE 0 // boolean false
#define NCLIENTS 10 // Número de sockets (uma para listening e uma para o stdin)
#define TIMEOUT -1 // em milisegundos


//declarações
void *threaded_send_receive(void *threadID);
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
int serverAux_on = TRUE; // booleano servidor online
struct sockaddr_in client; // struct cliente
socklen_t size_client; // size cliente
int checkPoll; // check do poll , verificar se houve algo no poll
struct server_t *server; //servidor
char *portoSecundario;
char *ipSecundario;

int activeFDs = 0; //num de fds activos
int close_conn; 
int compress_list; //booleano representa se deve fazer compress da de socketsPoll


int dadosProntos;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t dados_para_enviar = PTHREAD_COND_INITIALIZER;
struct thread_params{
	struct message_t *msg;
	int threadResult; // o resultado do envio da thread
};
struct thread_params *params;

void finishserverAux(int signal){
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
int make_serverAux_socket(short port){
  int socket_fd;
  int rc, on = 1;
  struct sockaddr_in serverAux;

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

  serverAux.sin_family = AF_INET;
  serverAux.sin_port = htons(port);  
  serverAux.sin_addr.s_addr = htonl(INADDR_ANY);



  if (bind(socket_fd, (struct sockaddr *) &serverAux, sizeof(serverAux)) < 0){
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
	printf("start writeall\n");
	int bufsize = len;
	while(len > 0){
		int res = write(sock, buf, len);
		if(res == 0){
			//servidor disconnected...
			return ERROR;
		}
		if(res < 0){
			if(errno == EINTR) continue;
			perror("write failed:");
			return res;
		}
		buf+= res;
		len-= res;
	}
	printf("fim writeall\n");
	return bufsize;
}
/* Função que garante a receção de len bytes através da socket sock,
   armazenando-os em buf.
*/
int read_all(int sock, char *buf, int len){
	printf("start readall\n");
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
	printf("fim readall\n");
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
		}else if( opcode == OC_UPDATE_S ){
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

		if(isSecondaryOn){
			printf("secundario online\n");
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

				params->msg = msg_pedido;
				dadosProntos = ERROR;
				printf("dadosProntos = %d\n", dadosProntos);
				pthread_cond_signal(&dados_para_enviar);
	      		pthread_mutex_unlock(&mutex);
	      		//receber resultado
	      		// void *threadRetun;pthread_join(thread, NULL);
	      		// printf(" char = %s\n", (char *) &(*threadRetun) );
	      		// int resultThread = atoi((char *)&threadRetun)
	      		while(dadosProntos != OK){}
				pthread_mutex_lock(&mutex);
				if(params->threadResult = ERROR && params->msg == NULL){
					//é pq o secundario desconectou

				}
				/*
					ENVIAR PARA O SECUNDARIO AQUI
					DEVE SER FEITO ATRAVES DE UMA THREAD
				*/
			}
		}else{
			printf("secundario offline\n");
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

int subRoutine(){
	//Codigo de acordo com as normas da IBM
	/*make a reusable listening socket*/
	/* ciclo para receber os clients conectados */
	if(isPrimary){
		printf("a espera de clientes - primario...\n");

		//inicializar mutex
		if (pthread_mutex_init(&mutex, NULL) != 0){
		    printf("\n mutex init failed\n");
			exit(ERROR);
		}
		//o primario ganha poder do mutex
		pthread_mutex_lock(&mutex);
					
		pthread_t thread;
		long id = 1234;
		int threadCreated = pthread_create(&thread, NULL, threaded_send_receive, NULL);
		if (threadCreated){
     		printf("ERROR; return code from pthread_create() is %d\n", threadCreated);
      		exit(ERROR);
  		}


      	
	}else{
		printf("a espera de clientes secundario...\n");
	}
	//call poll and check
	while(serverAux_on){ //while no cntrl c
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
								if(isPrimary){
									printf("* servidor primario\n");
								}else{
									printf("* servidor secundario\n");
								}
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
		 					}else if(result == CHANGE_ROUTINE){
		 						isPrimary = TRUE;
		 						subRoutine();
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




int serverAuxInit(char *myPort, char *listSize){
		listening_socket = make_serverAux_socket(atoi(myPort));
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
	 signal(SIGINT, finishserverAux);
	
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
		result = serverAuxInit(myPort, listSize);
		if(result == ERROR){return ERROR;}
		ipSecundario = secIP;
		portoSecundario = secPort;
		subRoutine();

	}else if(argc == 1){
		//secundario
		isPrimary = FALSE;

		char *myPort = /*argv[1]*/ "44902";
		char *listSize = /*argv[2]*/ "10";

		//inicializa servidor
		result = serverAuxInit(myPort, listSize);
		if(result == ERROR){return ERROR;}


		subRoutine();
	}else{
		//errou
		printf("\nUso do primario: ./serverAux <porta TCP> <IP secundario> <porta TCP secundario> <dimensão da tabela>\n");
		printf("	Exemplo de uso: ./table-serverAux 54321 127.0.0.1 54322 10\n");
		printf("Uso do secundario: ./serverAux <porta TCP> <dimensão da tabela>\n");
		printf("	Exemplo de uso: ./table-serverAux 54321 10\n\n");
		return ERROR;
	}

}//fim main





/********************************
**
**  METODOS UTILIZADOS PELA THREAD
**
**********************************/
void *threaded_send_receive(void *parametro){
	printf("thread started\n");

		printf("inside mutex in threaded send receive\n");
		if(!isSecondaryOn){
				if(server == NULL){
				//criar servidor primeiro
				server = linkToSecServer();
				if(server == NULL){
					server = linkToSecServer();
					if(server == NULL){
						//destruir thread
						return NULL;
					}else{isSecondaryOn = TRUE;}
				}else{isSecondaryOn = TRUE;}
			}
		}
		printf("connected to secundario\n");
		params = (struct thread_params *) malloc(sizeof(struct thread_params));
		if(params == NULL){return NULL;}
		dadosProntos = 0;
		//ciclo de espera e envio
		while(isSecondaryOn){
			printf("\n\n\n\n");
			params->threadResult = OK;


			pthread_mutex_lock(&mutex);
			while(dadosProntos == 0){
				pthread_cond_wait(&dados_para_enviar, &mutex);
			}
			//enviar para o servidor secundario
			struct message_t *msg_resposta;
			if(params->msg == NULL){
				//acabar thread
				params->threadResult = ERROR;
			}
			msg_resposta = network_send_receive(server, params->msg);
			if(msg_resposta == NULL){
				//acabar thread ocorreu um erro
				printf("secun off\n");
				isSecondaryOn = FALSE;
				params->threadResult = ERROR;
			}else{
				//acabou bem
				if(msg_resposta->c_type == CT_RESULT){
					if(msg_resposta->content.result == OK){
						params->threadResult = OK;
					}else{
						params->threadResult = ERROR;
					}
				}else{
					params->threadResult = ERROR;
				}

			}
			dadosProntos = 0;
			pthread_mutex_unlock(&mutex);
		}


	return NULL;
}

struct server_t* linkToSecServer(){
	struct server_t *serverAux = (struct server_t*)malloc(sizeof(struct server_t));
	
	/* Verificar parâmetro da função e alocação de memória */
	if(serverAux == NULL){ return NULL; }
	/* allocar a memoria do addrs dentro do serverAux*/
	serverAux->addr = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
	if(serverAux->addr == NULL){
		free(serverAux);
		return NULL;
	}

	// // Separar os elementos da string, ip : porto	
	// const char ip_port_seperator[2] = ":";
	// char *ip, *port, *p;
	// // adress_por é constante
	// p = strdup(address_port);
	// char *token = strtok(p, ip_port_seperator);
	// ip = strdup(token);
	// token = strtok(NULL, ip_port_seperator);
	// port = strdup(token);
	// free(p);
	
	//fixing inet addrs
	int inet_res = inet_pton(AF_INET, ipSecundario, &(serverAux->addr->sin_addr));
	if(inet_res == -1){
		free(serverAux->addr);
		free(serverAux);
		return NULL;
	}else if(inet_res == 0){
		printf("Endereço IP não é válido\n");
		free(serverAux->addr);
		free(serverAux);
		return NULL;
	}

	// Porto	
	serverAux->addr->sin_port = htons(atoi(portoSecundario));
	// Tipo
	serverAux->addr->sin_family = AF_INET;
	
	// Criação do socket
	int sockt;
	// Também pode ser usado o SOCK_DGRAM no tipo, UDP
	if((sockt = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		perror("Problema na criação do socket\n");
		free(serverAux->addr);
		free(serverAux);
		return NULL;
	}

	// Estabeleber ligação
	if(connect(sockt, (struct sockaddr *) (serverAux->addr), sizeof(struct sockaddr_in)) < 0){
		perror("Problema a conectar ao servidor\n");
		free(serverAux->addr);
		free(serverAux);
		return NULL;
	}

	/* Se a ligação não foi estabelecida, retornar NULL */
	serverAux->socket = sockt;
	if(portoSecundario != NULL){serverAux->porto = portoSecundario;}
	if(ipSecundario != NULL){serverAux->ip = ipSecundario;}
	isSecondaryOn = TRUE;
	return serverAux;
}


struct message_t *network_send_receive(struct server_t *server, struct message_t *msg){
	char *message_out;
	int message_size, msg_size, result;
	struct message_t *msg_resposta;

	/* Verificar parâmetros de entrada */
	if(server == NULL || msg == NULL){return NULL;}

	/* Serializar a mensagem recebida */
	message_size = message_to_buffer(msg, &message_out);

	/* Verificar se a serialização teve sucesso */
	if(message_size <= 0){return NULL;} //ocorreu algum erro

	/* Enviar ao servidor o tamanho da mensagem que será enviada
	   logo de seguida
	*/
	msg_size = htonl(message_size);
 	result = write_all(server->socket, (char *) &msg_size, _INT); //envia o size primeiro
 	printf("enviou size...\n");
	/* Verificar se o envio teve sucesso */
	if(result != _INT){return NULL;}


	/* Enviar a mensagem que foi previamente serializada */

	result = write_all(server->socket, message_out, message_size);
	printf("enviou...\n");

	/* Verificar se o envio teve sucesso */
	if(result != message_size){return NULL;} //enviar de novo?

	/* De seguida vamos receber a resposta do servidor:*/
	/*		Com a função read_all, receber num inteiro o tamanho da 
		mensagem de resposta.*/
	result = read_all(server->socket, (char *) &msg_size, _INT);
	if(result != _INT){return NULL;}
	
	message_size = ntohl(msg_size);
	free(message_out);
	
	/*	Alocar memória para receber o número de bytes da
		mensagem de resposta.*/
	message_out = (char *) malloc(message_size);
	/*		Com a função read_all, receber a mensagem de resposta. */
	
	result = read_all(server->socket, message_out, message_size);
	if(result != message_size){

		printf("erro no size 5");
		free(message_out);
		return NULL;
	}


	/* Desserializar a mensagem de resposta */
	msg_resposta = buffer_to_message(message_out, message_size);

	/* Verificar se a desserialização teve sucesso */
	if(msg_resposta == NULL){
		free(message_out);	

		printf("erro resposta");
		return NULL;
	}
	/* Libertar memória */
	free(message_out);

	return msg_resposta;
}

