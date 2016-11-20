#ifndef _TABLE_PRIVATE_H
#define _TABLE_PRIVATE_H

#include "table.h"

struct table_t{
	struct list_t **tabela; /* continuar definição */;
	int size; /* Dimensão da tabela com dados*/
	int nElems; /* Dimensão da tabela com e sem dados*/
};

int key_hash(char *key, int l);

/*
  Função auxiliar que faz uma inserção 
  um par {chave, valor} numa determinada tabela hash
  Devolve 0 (OK) ou -1 (out of memory, outros erros) 
*/
int insert(struct table_t *table, char *key, struct data_t *value);

#endif

