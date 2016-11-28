#ifndef _CLIENT_STUB_PRIVATE_H
#define _CLIENT_STUB_PRIVATE_H

#include "client_stub.h"


#define RETRY_TIME 2  // SECONDS
/* Remote table. A definir pelo grupo em client_stub-private.h 
 */
struct rtable_t{
	struct server_t *server;
	char *ip_addr1;
  char *ip_addr2;
  int primario;
};

/*
Função que inicializa uma tabela remota
e aloca os endereços dos servidores
*/
struct rtable_t *main_bind_rtable(const char* server1,const char*server2);

#endif