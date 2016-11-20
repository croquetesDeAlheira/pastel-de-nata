/*
*	Grupo 12
* @author Daniel Santos 44887
* @author Luis Barros  47082
* @author Marcus Dias 44901
*/


#include "../include/network_client-private.h"
#include "../include/inet.h"


#include <stdlib.h>
#include <errno.h>
#include <netinet/in.h>



#define ERROR -1
#define OK 0



struct server_t *network_connect(const char *address_port){

	struct server_t *server = (struct server_t*)malloc(sizeof(struct server_t));
	
	/* Verificar parâmetro da função e alocação de memória */
	if(address_port == NULL){ return NULL; }
	if(server == NULL){ return NULL; }
	/* allocar a memoria do addrs dentro do server*/
	server->addr = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
	if(server->addr == NULL){
		free(server);
		return NULL;
	}

	// Separar os elementos da string, ip : porto	
	const char ip_port_seperator[2] = ":";
	char *ip, *port, *p;
	// adress_por é constante
	p = strdup(address_port);
	char *token = strtok(p, ip_port_seperator);
	ip = strdup(token);
	token = strtok(NULL, ip_port_seperator);
	port = strdup(token);
	free(p);
	
	//fixing inet addrs
	int inet_res = inet_pton(AF_INET, ip, &(server->addr->sin_addr));
	if(inet_res == -1){
		free(server->addr);
		free(server);
		free(port);
		free(ip);
		return NULL;
	}else if(inet_res == 0){
		printf("Endereço IP não é válido\n");
		free(server->addr);
		free(server);
		free(port);
		free(ip);
		return NULL;
	}

	// Porto	
	server->addr->sin_port = htons(atoi(port));
	// Tipo
	server->addr->sin_family = AF_INET;
	// liberta o port
	free(port);
	free(ip);

	
	// Criação do socket
	int sockt;
	// Também pode ser usado o SOCK_DGRAM no tipo, UDP
	if((sockt = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		perror("Problema na criação do socket\n");
		free(server->addr);
		free(server);
		return NULL;
	}

	// Estabeleber ligação
	if(connect(sockt, (struct sockaddr *) (server->addr), sizeof(struct sockaddr_in)) < 0){
		perror("Problema a conectar ao servidor\n");
		free(server->addr);
		free(server);
		return NULL;
	}

	/* Se a ligação não foi estabelecida, retornar NULL */
	server->socket = sockt;
	return server;
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
	/* Verificar se o envio teve sucesso */
	if(result != _INT){return NULL;}


	/* Enviar a mensagem que foi previamente serializada */
	result = write_all(server->socket, message_out, message_size);

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
		free(message_out);
		return NULL;
	}


	/* Desserializar a mensagem de resposta */
	msg_resposta = buffer_to_message(message_out, message_size);

	/* Verificar se a desserialização teve sucesso */
	if(msg_resposta == NULL){
		free(message_out);	
		return NULL;
	}
	/* Libertar memória */
	free(message_out);

	return msg_resposta;
}

int network_close(struct server_t *server){
	/* Verificar parâmetros de entrada */
	if(server == NULL){ return ERROR;}
	/* Terminar ligação ao servidor */
	close(server->socket);
	/* Libertar memória */
	free(server->addr);
	free(server);

	return OK;
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
