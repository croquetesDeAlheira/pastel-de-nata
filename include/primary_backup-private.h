#include "primary_backup.h"


struct server_t {
  int socket;
	struct sockaddr_in *addr;
 	char *ip;
 	char *porto;
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

/*
Escreve/Atualiza no ficheiro de log ip:porto do primario,
onde ip_port é o porto:ip do novo servidor primario e
e file_name é o nome do ficheiro de log
Retorna 0 em caso de sucesso, caso contrario -1
*/
int write_log(char* ip_port, char* file_name);

/*
Le do ficheiro o ip_porto do servidor primario
Onde file_name é o nome do ficheiro de log e
ip_port_buffer é para onde vai ser copiado ip_porto do 
servidor primário atual
Retorna 0 em caso de sucesso, caso contrario, -1
*/
int read_log(char* file_name, char* ip_port_buffer);

// Apaga o ficheiro de log
// Retorna 0 em caso de sucesso, coso contrário -1.
int destroy_log(char* file_name);




