#ifndef _CLIENT_STUB_PRIVATE_H
#define _CLIENT_STUB_PRIVATE_H

#include "client_stub.h"


#define RETRY_TIME 2  // SECONDS
/* Remote table. A definir pelo grupo em client_stub-private.h 
 */
struct rtable_t{
	struct server_t *server;
	char *ipAddr_1;
  char *piAddr_2;
};

/*
Função que inicializa uma tabela remota
e aloca os endereços dos servidores
*/
rtable_t* main_bind_rtable(char* server1, char*server2);

#endif