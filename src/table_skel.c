
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "../include/table_skel.h"
#include "../include/table_skel-private.h"
#include "../include/message-private.h"
#include "../include/table.h"
#include "../include/data.h"
#include "../include/entry.h"



#define ERROR -1
#define OK 0

//espero que isto funcione 
struct table_t *tabela;

/* Inicia o skeleton da tabela.
 * O main() do servidor deve chamar esta função antes de poder usar a
 * função invoke(). O parâmetro n_lists define o número de listas a
 * serem usadas pela tabela mantida no servidor.
 * Retorna 0 (OK) ou -1 (erro, por exemplo OUT OF MEMORY)
 */
int table_skel_init(int n_lists){
	tabela = table_create(n_lists);
	if(tabela == NULL){
		return ERROR;
	}else{
		return OK;
	}

}

/* Libertar toda a memória e recursos alocados pela função anterior.
 */
int table_skel_destroy(){
	table_destroy(tabela);
	return OK;
}

/* Executa uma operação (indicada pelo opcode na msg_in) e retorna o
 * resultado numa mensagem de resposta ou NULL em caso de erro.
 */
struct message_t *invoke(struct message_t *msg_pedido){
		//mensagem para devolver a resposta
	struct message_t *msg_resposta = (struct message_t*)malloc(sizeof(struct message_t));
	
	if(msg_resposta == NULL){
		return NULL;
	}
	/* Verificar parâmetros de entrada */
	if(msg_pedido == NULL || tabela == NULL){
		return NULL;
	}
	
	/* Verificar opcode e c_type na mensagem de pedido */
	short opcode = msg_pedido->opcode;
	short c_type =msg_pedido->c_type;	
	char *all = "!";
	/* Aplicar operação na tabela */
	// opcode de resposta tem que ser opcode + 1
	char** all_keys;
	switch(opcode){
		case OC_SIZE:
			msg_resposta->opcode = OC_SIZE_R;
			msg_resposta->c_type = CT_RESULT;
			msg_resposta->content.result = table_size(tabela);
			break;
		case OC_DEL:
			msg_resposta->opcode = OC_DEL_R;
			msg_resposta->c_type = CT_RESULT;
			msg_resposta->content.result = table_del(tabela, msg_pedido->content.key);
			break;
		case OC_UPDATE:
			msg_resposta->opcode = OC_UPDATE_R;
			msg_resposta->c_type = CT_RESULT;
			msg_resposta->content.result = table_update(tabela, msg_pedido->content.entry->key, msg_pedido->content.entry->value);
			break;
		case OC_GET:
			// c_type CT_VALUE se a chave não existe
			// c_type CT_KEYS
			// c_type se for ! table_get_keys
			// c_type se for key table_get
			msg_resposta->opcode = OC_GET_R;
			if(strcmp(msg_pedido->content.key,all) == 0){
				// Get de todas as chaves
				msg_resposta->c_type = CT_KEYS;
				msg_resposta->content.keys = table_get_keys(tabela);
			}else if(table_get(tabela, msg_pedido->content.key) == NULL){
				//struct data_t *dataRet = data_create(0);
				msg_resposta->c_type = CT_VALUE;
				msg_resposta->content.data = NULL;
			}else{
				msg_resposta->c_type = CT_VALUE;
				msg_resposta->content.data = table_get(tabela, msg_pedido->content.key);
			}
			break;			
		case OC_PUT:
			msg_resposta->opcode = OC_PUT_R;
			msg_resposta->c_type = CT_RESULT;
			msg_resposta->content.result = table_put(tabela, msg_pedido->content.entry->key, msg_pedido->content.entry->value);
			break;
		
		default:	
			printf("opcode nao e´ valido, opcode = %i\n", opcode);
	}
	/* Preparar mensagem de resposta */
	//printf("opcode = %i, c_type = %i\n", msg_resposta->opcode, msg_resposta->c_type);	
	return msg_resposta;

}
