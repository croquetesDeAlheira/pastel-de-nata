 #include "primary_backup.h"

 struct server_t {
 	int socket;
	struct sockaddr_in *addr;
 	char *ip_addr1;
 };

  /*Função usada para um servidor avisar o servidor "server" de que
 já acordou. Retorna 0 em caso de sucesso, -1 em caso de insucesso*/
 int hello(struct server_t *server);

 /*Pede atualização de estado do server
 Retorna 0 em caso de sucesso e -1 em caso de insucesso*/
 int update_state(struct server_t *server);

 void *threaded_send_receive(void *threadID);

 struct server_t * linkToSecServer();

 struct message_t *network_send_receive(struct server_t *server, struct message_t *msg);
