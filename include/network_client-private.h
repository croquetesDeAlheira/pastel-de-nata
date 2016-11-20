#ifndef _NETWORK_CLIENT_PRIVATE_H
#define _NETWORK_CLIENT_PRIVATE_H

#include "inet.h"
#include "network_client.h"

struct server_t{
	/* Atributos importantes para interagir com o servidor, */
	/* tanto antes da ligação estabelecida, como depois.    */
	int socket;
	struct sockaddr_in *addr;
};

// Estrutura que contem codigo do comando e a string que a representa
struct commands_t{char *key; int val; };

/* Função que garante o envio de len bytes armazenados em buf,
   através da socket sock.
*/
int write_all(int sock, char *buf, int len);

/* Função que garante a receção de len bytes através da socket sock,
   armazenando-os em buf.
*/
int read_all(int sock, char *buf, int len);

// Funcao que me dá o valor de um comando
// correspondente a uma string para usar no switch
int keyfromstring(char *key);


#endif
