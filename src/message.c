
/*
*		Grupo 12
* @author Daniel Santos 44887
* @author Luis Barros  47082
* @author Marcus Dias 44901
*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>

#include "../include/message-private.h"


//constants
const int OK = 0;
const int ERROR = -1;





int message_to_buffer(struct message_t *msg, char **msg_buf){
	printf("msg 2 buff init\n");

	/* Verificar se msg é NULL */
	if(msg == NULL){ return  ERROR; }

	/* ::::: VARIAVEIS :::::*/
	int vetorSize =(_SHORT + _SHORT); //comeca sempre 2 shorts.
	uint16_t short_value;
	uint32_t int_value;
	short keySize;
	short strSize;
	int dataSize;
	char **keys;
	int numKeys;

	/* Consoante o msg->c_type, determinar o tamanho do vetor de bytes
	que tem de ser alocado antes de serializar msg
	*/
	int type = msg->c_type;
	switch(type){
		case CT_RESULT :
			vetorSize += _INT; // + 4 bytes do result
			break;

		case CT_VALUE :
			/* datasize int */
			vetorSize += _INT; 
			/* soma o valor do datasize */
			if(msg->content.data != NULL){
				vetorSize += msg->content.data->datasize;
			}
			break;

		case CT_KEY :
			/*  key size */
			keySize = strlen(msg->content.key);
			/* 2bytes + keysize  */
			vetorSize += _SHORT + keySize;
			break;

		case CT_KEYS :
			/*  num keys  */
			vetorSize += _INT;
			keys = msg->content.keys;
			int i = 0;
			while(*(keys + i) != NULL){
				/* key size  */
				strSize = strlen(*(keys + i));
				/* 2bytes + keySize  */
				vetorSize += _SHORT + strSize;
				i++;
			}
			//we need to know numKeys later ;)
			numKeys = i;
			break;

		case CT_ENTRY :
			vetorSize += _SHORT; //keysize
			keySize = strlen(msg->content.entry->key);
			vetorSize += keySize; //Tamanho da key
			vetorSize += _INT; //tamanho data
			dataSize = msg->content.entry->value->datasize;
			vetorSize += dataSize; //tamanho da mem do data
			break;

		default:
			return ERROR; // se o type nao for nenhum dos esperados
	}



	/* Alocar quantidade de memória determinada antes 
	*msg_buf = ....
	*/
	*msg_buf = (char *)malloc(vetorSize);
	if(msg_buf == NULL){return ERROR;}
	/* Inicializar ponteiro auxiliar com o endereço da memória alocada */
	char *ptr = *msg_buf;

	/* serializa os 2 primeiros atributos
	 * opcode primeiro
	 * ct_type depois  */
	short_value = htons(msg->opcode);
	memcpy(ptr, &short_value, _SHORT);
	ptr += _SHORT;
	
	short_value = htons(msg->c_type);
	memcpy(ptr, &short_value, _SHORT);
	ptr += _SHORT;
	
	/* Consoante o conteúdo da mensagem, continuar a serialização da mesma */
	switch(type){
		case CT_RESULT :
			/* serializar result  */
			/*  result eh 1 inteiro */
			int_value = htonl(msg->content.result);
			memcpy(ptr, &int_value, _INT);
			ptr += _INT;
			break;

		case CT_VALUE :
			/* serializar value tipo data_t  */
			/* datasize , tamanho do data  */
			if(msg->content.data == NULL){
				dataSize == 0;
			}else{
				dataSize = msg->content.data->datasize;
			}
			int_value = htonl(dataSize);
			memcpy(ptr, &int_value, _INT);
			ptr += _INT;
			/*  serializar o valor do data */
			if(msg->content.data != NULL){
				memcpy(ptr, msg->content.data->data, dataSize);
				ptr += dataSize;
			}
			break;

		case CT_KEY :
			printf("inside ct key msg buf\n");
			if(msg->content.key == NULL){
				//não existe a chave
				strSize = 0;
			}else{
				strSize = strlen(msg->content.key); //supondo q retorna short
			
			}	
			short_value = htons(strSize);		
			memcpy(ptr, &short_value, _SHORT);
			ptr += _SHORT;
			memcpy(ptr, msg->content.key , strSize); //sem o \0
			ptr += strSize;
			printf("fim msg buff\n");
			break;

		case CT_KEYS :
			keys = msg->content.keys;
			int_value = htonl(numKeys);
			memcpy(ptr, &int_value, _INT);
			ptr += _INT;
			int j = 0;
			while(j < numKeys){
				strSize = strlen(*(keys + j));
				short_value = htons(strSize);
				memcpy(ptr, &short_value, _SHORT);
				ptr += _SHORT;
				memcpy(ptr, *(keys + j), strSize);
				ptr += strSize;
				j++;
			}
			break;

		case CT_ENTRY :
			strSize = strlen(msg->content.entry->key);
			short_value = htons(strSize);
			memcpy(ptr, &short_value, _SHORT);
			ptr += _SHORT;
			memcpy(ptr, msg->content.entry->key, strSize);
			ptr += strSize;
			//copy data now.
			dataSize = msg->content.entry->value->datasize;
			int_value = htonl(dataSize);
			memcpy(ptr, &int_value, _INT);
			ptr += _INT;
			memcpy(ptr, msg->content.entry->value->data, dataSize);
			ptr += dataSize;
			break;
	}

	return vetorSize;
}

struct message_t *buffer_to_message(char *msg_buf, int msg_size){
	/* Verificar se msg_buf é NULL */
	/* msg_size tem tamanho mínimo ? */
	if(msg_buf == NULL || msg_size < 6){return NULL;}

	//variaveis uteis
	uint16_t short_aux;
	uint32_t int_aux;
	short keySize;
	short strSize;
	int dataSize;
	int numKeys;
	void *dataBuff;
	struct data_t *value;

	/* Alocar memória para uma struct message_t */
	struct message_t *msg = (struct message_t *)malloc(sizeof(struct message_t));
	if(msg == NULL){return NULL; }
	/* Recuperar o opcode e c_type */
	memcpy(&short_aux, msg_buf, _SHORT);
	msg->opcode = ntohs(short_aux);
	msg_buf += _SHORT;;

	memcpy(&short_aux, msg_buf, _SHORT);
	msg->c_type = ntohs(short_aux);
	msg_buf += _SHORT;

	/* A mesma coisa que em cima mas de forma compacta, ao estilo C! 
	msg->opcode = ntohs(*(short *) msg_buf++);
	msg->c_type = ntohs(*(short *) ++msg_buf);
	msg_buf += _SHORT;
	*/
	/* O opcode e c_type são válidos? */
	if(opIsValid(msg->opcode) && ctIsValid(msg->c_type)){
		/* Consoante o c_type, continuar a recuperação da mensagem original */
		switch(msg->c_type){
			case CT_RESULT :
				memcpy(&int_aux, msg_buf, _INT);
				msg->content.result = ntohl(int_aux);
				msg_buf += _INT;
				break;
			case CT_VALUE :
				memcpy(&int_aux, msg_buf, _INT);
				dataSize = ntohl(int_aux);
				msg_buf += _INT;
				dataBuff = (void *)malloc(dataSize);
				if(dataBuff == NULL){ return NULL; }
				memcpy(dataBuff, msg_buf, dataSize);
				msg_buf += dataSize;
				value = data_create2(dataSize, dataBuff);
				free(dataBuff);
				if(value == NULL){return NULL; }
				msg->content.data = value;
				break;
			case CT_KEY :
				memcpy(&short_aux, msg_buf, _SHORT);
				strSize = ntohs(short_aux);
				msg_buf += _SHORT;
				//verify if 0
				printf("inside key\n");
				if(strSize == 0){
					msg->content.key = NULL;
					msg_buf += 1;
				}else{
					msg->content.key = (char *)malloc(strSize + 1);
					memcpy(msg->content.key, msg_buf, strSize);
					msg->content.key[strSize] = '\0';
					msg_buf += strSize;
				}
				printf("sai key\n");
				break;
			case CT_KEYS :
				memcpy(&int_aux, msg_buf, _INT);
				numKeys = ntohl(int_aux);
				msg_buf += _INT;
				char **keys;
				if(numKeys == 0){
					keys = (char **)malloc(1);
					*(keys) == NULL;
				}else{
					keys = (char **)malloc( (sizeof(char *) * numKeys ) + 1);
					int i = 0;
					char *aux;
					for (i = 0; i < numKeys; i++){
						memcpy(&short_aux, msg_buf, _SHORT);
						strSize = htons(short_aux);
						msg_buf += _SHORT;
						*(keys + i) = (char *)malloc(strSize +1);
						aux = (char *)malloc(strSize + 1);
						if(aux == NULL){return NULL;}
						memcpy(aux, msg_buf, strSize);
						aux[strSize] = '\0';
						strcpy(*(keys + i), aux);
						free(aux);
						msg_buf += strSize;
					}
					*(keys + i) = NULL;
				}
				msg->content.keys = keys;
				break;
			
			case CT_ENTRY :
				memcpy(&short_aux, msg_buf, _SHORT);
				strSize = ntohs(short_aux);
				msg_buf += _SHORT;
				char *key = (char *)malloc(strSize + 1);
				if(key == NULL){return NULL; }
				memcpy(key, msg_buf, strSize);
				key[strSize] = '\0';
				msg_buf += strSize;
				memcpy(&int_aux, msg_buf, _INT);
				dataSize = ntohl(int_aux);
				msg_buf += _INT;
				dataBuff = malloc(dataSize);
				memcpy(dataBuff, msg_buf, dataSize);
				value = data_create2(dataSize, dataBuff);
				if(value == NULL){return NULL; }
				struct entry_t *entry = entry_create(key, value);
				if(entry == NULL){return NULL; }
				data_destroy(value);
				free(dataBuff);
				free(key);
				msg->content.entry = entry;
				break;
			
			default :
				return 0;

		}
	}else{
		printf("dentro else\n");
		free(msg);
		return NULL;
	}

	

	return msg;
}

void free_message(struct message_t *msg){
	/* Verificar se msg é NULL */
		if(msg == NULL){ return; }

	/* Se msg->c_type for:
	VALOR, libertar msg->content.data
	ENTRY, libertar msg->content.entry_create
	CHAVES, libertar msg->content.keys
	CHAVE, libertar msg->content.key
	*/
	int type = msg->c_type;
	switch(type){
		case CT_RESULT :
			//empty 
			break;
		case CT_VALUE:
			data_destroy(msg->content.data);
			break;
		case CT_KEY:
			free(msg->content.key);
			break;
		case CT_KEYS:
			list_free_keys(msg->content.keys);
			break;
		case CT_ENTRY:
			entry_destroy(msg->content.entry);
			break;
	}
	free(msg);
	/* libertar msg */
}


int opIsValid(short opcode){
	switch(opcode){
		case OC_SIZE :
			return 1;
			break;
		case OC_DEL :
			return 1;
			break;
		case OC_UPDATE :
			return 1;
			break;
		case OC_GET :
			return 1;
			break;
		case OC_PUT :
			return 1;
			break;
		case OC_SIZE + 1 :
			return 1;
			break;
		case OC_DEL + 1 :
			return 1;
			break;
		case OC_UPDATE + 1 :
			return 1;
			break;
		case OC_GET + 1 :
			return 1;
			break;
		case OC_PUT + 1 :
			return 1;
			break;
		default :
			return 0;
	}
}

int ctIsValid(short c_type){
	switch(c_type){
		case CT_RESULT :
			return 1;
			break;
		case CT_VALUE :
			return 1;
			break;
		case CT_KEY :
			return 1;
			break;
		case CT_KEYS :
			return 1;
			break;
		case CT_ENTRY :
			return 1;
			break;
		default :
			return 0;

	}
}

