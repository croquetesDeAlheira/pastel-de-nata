/*
*	Grupo 12
* @author Daniel Santos 44887
* @author Luis Barros  47082
* @author Marcus Dias 44901
*/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "../include/client_stub-private.h"
#include "../include/codes.h"
#include "../include/message-private.h"
#include "../include/network_client-private.h"
#include "../include/table.h"


/**
*	:::: faz o pendido e se der erro tenta novamente depois do tempo de retry :::
*
*/
struct message_t *network_with_retry(struct rtable_t *table, struct message_t *msg_pedido){
	struct message_t *msg_resposta;
	msg_resposta = network_send_receive(table->server, msg_pedido);
	if(msg_resposta == NULL){
		perror("Problema com a mensagem de resposta, tentar novamente..\n");
		sleep(RETRY_TIME);

		if(network_close(table->server) < 0){
			perror("Problema ao terminar a associação entre cliente e tabela remota\n");
		}
		table->server = network_connect(table->ipAddr);
		if(table->server == NULL){
			perror("Problema na conecção\n");
			return NULL;
		}
		msg_resposta = network_send_receive(table->server, msg_pedido);
		if(msg_resposta == NULL){
			perror("sem resposta do servidor\n");
		}
	}

	//retorna resposta seja null ou nao
	return msg_resposta;

}



/* Função para estabelecer uma associação entre o cliente e uma tabela
 * remota num servidor.
 * address_port é uma string no formato <hostname>:<port>.
 * retorna NULL em caso de erro .
 */
struct rtable_t *rtable_bind(const char *address_port){

	if(address_port == NULL){
		perror("Problema com o address_port\n");
		return NULL;
	}

	struct rtable_t *rtable = (struct rtable_t *)malloc(sizeof(struct rtable_t));
	if(rtable == NULL){
		perror("Problema na criação da tabela remota\n");
		return NULL;
	}	
	
	rtable->server = network_connect(address_port);
	if(rtable->server == NULL){
		perror("Problema na conecção\n");
		return NULL;
	}
	rtable->ipAddr = strdup(address_port);

	return rtable;	
}

/* Termina a associação entre o cliente e a tabela remota, e liberta
 * toda a memória local. 
 * Retorna 0 se tudo correr bem e -1 em caso de erro.
 */
int rtable_unbind(struct rtable_t *rtable){
	if(network_close(rtable->server) < 0){
		perror("Problema ao terminar a associação entre cliente e tabela remota\n");
		return ERROR;
	}	
	free(rtable);
	return OK;
}

/* Função para adicionar um par chave valor na tabela remota.
 * Devolve 0 (ok) ou -1 (problemas).
 */
int rtable_put(struct rtable_t *rtable, char *key, struct data_t *value){
	struct message_t *msg_resposta, *msg_pedido;
	// Verificação se a rtable, key e value são válidos
	if(rtable == NULL || key == NULL || value == NULL){
		perror("Problema com rtable/key/value\n");
		return ERROR;
	}
	// Criação e alocação da mensagem de pedido
	msg_pedido = (struct message_t *)malloc(sizeof(struct message_t));
	if(msg_pedido == NULL){
		perror("Problema na criação da mensagem de pedido\n");
		return ERROR;
	}
	// Mensagem a enviar
	msg_pedido->opcode = OC_PUT;
	msg_pedido->c_type = CT_ENTRY;
	msg_pedido->content.entry = entry_create(key, value);
	// Receber a mensagem de resposta
	msg_resposta = network_with_retry(rtable, msg_pedido);
	//msg_resposta = network_send_receive(rtable->server, msg_pedido);
	if(msg_resposta == NULL){
		//perror("Problema com a mensagem de resposta\n");
		return ERROR;
	}
	// Mensagem de pedido já não é necessária
	//free_message(msg_pedido);
	return msg_resposta->content.result;
}

/* Função para substituir na tabela remota, o valor associado à chave key.
 * Devolve 0 (OK) ou -1 em caso de erros.
 */
int rtable_update(struct rtable_t *rtable, char *key, struct data_t *value){
	struct message_t *msg_resposta, *msg_pedido;
	// Verificação se a rtable, key e value são válidos
	if(rtable == NULL || key == NULL || value == NULL){
		perror("Problema com rtable/key/value\n");
		return ERROR;
	}
	// Criação e alocação da mensagem de pedido
	msg_pedido = (struct message_t *)malloc(sizeof(struct message_t));
	if(msg_pedido == NULL){
		perror("Problema na criação da mensagem de pedido\n");
		return ERROR;
	}
	// Mensagem a enviar
	msg_pedido->opcode = OC_UPDATE;
	msg_pedido->c_type = CT_ENTRY;
	msg_pedido->content.entry = entry_create(key, value);
	// Receber a mensagem de resposta
	msg_resposta = network_with_retry(rtable, msg_pedido);
	//msg_resposta = network_send_receive(rtable->server, msg_pedido);
	if(msg_resposta == NULL){
	//	perror("Problema com a mensagem de resposta\n");
		return ERROR;
	} 
	// Mensagem de pedido já não é necessária
	//free_message(msg_pedido);
	return msg_resposta->content.result;	
}

/* Função para obter da tabela remota o valor associado à chave key.
 * Devolve NULL em caso de erro.
 */
struct data_t *rtable_get(struct rtable_t *table, char *key){
	struct message_t *msg_resposta, *msg_pedido;
	// Verificação se a rtable e key são válidos
	if(table == NULL || key == NULL){
		perror("Problema com rtable/key\n");
		return NULL;
	}
	// Criação e alocação da mensagem de pedido
	msg_pedido = (struct message_t *)malloc(sizeof(struct message_t));
	if(msg_pedido == NULL){
		perror("Problema na criação da mensagem de pedido\n");
		return NULL;
	}
	// Mensagem a enviar
	msg_pedido->opcode = OC_GET;
	msg_pedido->c_type = CT_KEY;
	msg_pedido->content.key = key;
	// Receber a mensagem de resposta
	msg_resposta = network_with_retry(table, msg_pedido);
	//msg_resposta = network_send_receive(table->server, msg_pedido);
	if(msg_resposta == NULL){
	//	perror("Problema com a mensagem de resposta\n");
		return NULL;
	}
	//Se data vem a NULL é porque não existem keys
	if(msg_resposta->content.data == NULL){
		char *noKeys = "Não existem chaves";
		return data_create2(strlen(noKeys), noKeys);
	}
	// Mensagem de pedido já não é necessária
	//free(msg_pedido);
	return msg_resposta->content.data;
}

/* Função para remover um par chave valor da tabela remota, especificado 
 * pela chave key.
 * Devolve: 0 (OK) ou -1 em caso de erros.
 */
int rtable_del(struct rtable_t *table, char *key){
	struct message_t *msg_resposta, *msg_pedido;
	// Verificação se a rtable e key são válidos
	if(table == NULL || key == NULL){
		perror("Problema com rtable/key\n");
		return ERROR;
	}
	// Criação e alocação da mensagem de pedido
	msg_pedido = (struct message_t *)malloc(sizeof(struct message_t));
	if(msg_pedido == NULL){
		perror("Problema na criação da mensagem de pedido\n");
		return ERROR;
	}
	// Mensagem a enviar
	msg_pedido->opcode = OC_DEL;
	msg_pedido->c_type = CT_KEY;
	msg_pedido->content.key = key;
	// Receber a mensagem de resposta
	msg_resposta = network_with_retry(table, msg_pedido);
	//msg_resposta = network_send_receive(table->server, msg_pedido);
	if(msg_resposta == NULL){
	//	perror("Problema com a mensagem de resposta\n");
		return ERROR;
	} 
	// Mensagem de pedido já não é necessária
	//free_message(msg_pedido);
	return msg_resposta->content.result;	

}

/* Devolve número de pares chave/valor na tabela remota.
 */
int rtable_size(struct rtable_t *rtable){
	struct message_t *msg_resposta, *msg_pedido;
	// Verificar se rtable é válido
	if(rtable == NULL){
		perror("Problema com rtable\n");
		return ERROR;
	}
	// Criação e alocação da mensagem de pedido
	msg_pedido = (struct message_t *)malloc(sizeof(struct message_t));
	if(msg_pedido == NULL){
		perror("Problema na criação da mensagem de pedido\n");
		return ERROR;
	}
	// Mensagem a enviar
	msg_pedido->opcode = OC_SIZE;
	msg_pedido->c_type = CT_RESULT;
	// Receber a mensagem de resposta
	msg_resposta = network_with_retry(rtable, msg_pedido);
	//msg_resposta = network_send_receive(rtable->server, msg_pedido);
	if(msg_resposta == NULL){
	//	perror("Problema com a mensagem de resposta\n");
		return ERROR;
	} 
	// Mensagem de pedido já não é necessária
	//free_message(msg_pedido);
	return msg_resposta->content.result;	
}

/* Devolve um array de char * com a cópia de todas as keys da
 * tabela remota, e um último elemento a NULL.
 */
char **rtable_get_keys(struct rtable_t *rtable){
	struct message_t *msg_resposta, *msg_pedido;
	char *all = "!";
	// Verificar se rtable é válido
	if(rtable == NULL){
		perror("Problema com rtable\n");
		return NULL;
	}
	// Criação e alocação da mensagem de pedido
	msg_pedido = (struct message_t *)malloc(sizeof(struct message_t));
	if(msg_pedido == NULL){
		perror("Problema na criação da mensagem de pedido\n");
		return NULL;
	}
	// Mensagem a enviar
	msg_pedido->opcode = OC_GET;
	msg_pedido->c_type = CT_KEY;
	// Servidor vai verificar se content.key == !
	msg_pedido->content.key = all;
	// Receber a mensagem de resposta
	msg_resposta = network_with_retry(rtable, msg_pedido);
	//msg_resposta = network_send_receive(rtable->server, msg_pedido);
	if(msg_resposta == NULL){
	//	perror("Problema com a mensagem de resposta\n");
		return NULL;
	} 
	// Mensagem de pedido já não é necessária
	/////////////////////////////////////
	// Este free message dá problemas... Tentar perceber porquê
	/////////////////////////////////////
	//free_message(msg_pedido);
	return msg_resposta->content.keys;	
}

/* Liberta a memória alocada por table_get_keys().
 */
void rtable_free_keys(char **keys){
	table_free_keys(keys);
}



