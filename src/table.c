/*
*		Grupo 12
* @author Daniel Santos 44887
* @author Luis Barros  47082
* @author Marcus Dias 44901
*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "../include/table-private.h"

//constants
#define ERROR -1
#define OK 0

int key_hash(char *key, int l){
	//nao eh necessario, todo sition onde key hash é chamada , a verificacao ja foi feita.
  /* Verificar se key é NULL */
	//if(key == NULL){ return; }
  /* l tem valor válido? */
	//if(l < 0){ return; }

	int soma=0, keySize, ind;
	if((keySize = strlen(key)) < 6){
		for(ind = 0; ind < keySize; ind++)
			soma+=key[ind];
	}else{
		for(ind = 0; ind < 3; ind++)
			soma+=key[ind];
		
		soma = soma + key[keySize-2] + key[keySize-1];
	}

  	return soma % l;
}

/** Função para criar/inicializar uma nova tabela hash, com n  
 * linhas(n = módulo da função hash)
 */
struct table_t *table_create(int n) {
    /* n tem valor válido? */
	if (n <= 0) { return NULL; }

    /* Alocar memória para struct table_t */
	struct table_t *new_table = (struct table_t *) malloc(sizeof(struct table_t));
	if (new_table == NULL) { return NULL; } //verifica se not null
    /* Alocar memória para array de listas com n entradas 
       que ficará referenciado na struct table_t alocada.*/ 
	new_table->tabela = (struct list_t **)malloc(n * sizeof(struct list_t*));
	if (new_table->tabela == NULL) { 
		free(new_table);
		return NULL; 
	}

	// Inicializa restantes atributos
	new_table->size = n;
	new_table->nElems = 0;
  	
    /* Inicializar listas.
       Inicializar atributos da tabela.
    */
	int i;
	for (i = 0; i < n; i++) {
		struct list_t *list = list_create();
		if (list == NULL) { 
			// Tem de destruir todas as outras listas criadas anteriormente
			// Ex: erro em new_table->tabela[2] = list;
			// Tem de libertas new_table->tabela[1] e new_table->tabela[0]
			// Chama o table_destroy passando o table criado
			//table_destroy(new_table);
			table_destroy(new_table);
			return NULL; 
		}
		// Aloca um novo apontador de lista a cada linha da tabela
		new_table->tabela[i] = list;
	}
	return new_table;
}


/* Libertar toda a memória ocupada por uma tabela.
 */
void table_destroy(struct table_t *table) {
	struct list_t *list;
  	
  /* table é NULL? */
	if (table == NULL) { return; /*do nothing*/ }
	/*Libertar memória das listas.
	  Libertar memória da tabela.
	*/
	int i;
	for (i = 0; i < table->size; i++) {
		list = table->tabela[i];
		list_destroy(list);
	}
	free(table->tabela);
	// Liberta a tabela
	free(table);
}


/* Função para adicionar um par chave-valor na tabela. 
 * Os dados de entrada desta função deverão ser copiados.
 * Devolve 0 (ok) ou -1 (out of memory, outros erros)
 */
int table_put(struct table_t *table, char *key, struct data_t *value) {
  struct data_t *data;
  int result;
  // Verifica value...Restante verificado pelo table_get
  if (value == NULL) {return ERROR; }
  // Verifica se já existe na tabela um par {chave, valor}
  // Se True então não faz o put e sai
  data = table_get(table, key);
  if (data != NULL) { 
  	data_destroy(data);
  	return ERROR; 
  }

  result = insert(table, key, value);

  if (result == OK) { table->nElems++; }

  return result;
}

/* Função para substituir na tabela, o valor associado à chave key. 
 * Os dados de entrada desta função deverão ser copiados.
 * Devolve 0 (OK) ou -1 (out of memory, outros erros)
 */
int table_update(struct table_t *table, char *key, struct data_t *value) {
	struct data_t *data;
	int result;
	// Verifica value...Restante verificado pelo table_get
	if (value == NULL) { return ERROR; }
	// Verifica se ja existe na tabela um par {chave, valor}
	// Se isso for FALSE então NÂO faz update
	data = table_get(table, key);
	if (data == NULL) { return ERROR; }

	result = insert(table, key, value);
	data_destroy(data);
	return result;
}


/* Função para obter da tabela o valor associado à chave key.
 * A função deve devolver uma cópia dos dados que terão de ser libertados
 * no contexto da função que chamou table_get.
 * Devolve NULL em caso de erro.
 */
struct data_t *table_get(struct table_t *table, char *key){
	 int index;
	 struct entry_t *entry;

	 /* Verificar valores de entrada */
	if (table == NULL || key == NULL) { return NULL; }
	/* Verifica o hash index */
	index = key_hash(key, table->size);
	/*  buscar o entry na lista  */
	entry = list_get(table->tabela[index], key);
	/*  verificar se entry no null */ 
	if (entry == NULL) { return NULL; }
	/*  return o data do entry duplicado, pois é uma copia  */
	return data_dup(entry->value);
}

/* Função para remover um par chave valor da tabela, especificado 
 * pela chave key, libertando a memória associada a esse par.
 * Devolve: 0 (OK), -1 (nenhum tuplo encontrado; outros erros)
 */
int table_del(struct table_t *table, char *key){
	int index;
	int result;

	/*  verificar valores de entrada */
	if(table == NULL || key == NULL) { return ERROR; }
	/* encontrar o hash index*/
	index = key_hash(key, table->size);

	/* ja tenho a lista, remove esta key */
	result = list_remove(table->tabela[index], key);
	// Remoção bem sucedida actualiza numero de Elmentos
	if(result == OK) { table->nElems--; }
	
	return result;
}

/* Devolve o número de elementos na tabela */
int table_size(struct table_t *table) {
	return table == NULL ? ERROR : table->nElems;
}

/* Devolve um array de char * com a cópia de todas as keys da tabela,
 * e um último elemento a NULL.
 */
char **table_get_keys(struct table_t *table) {
	char **all_keys;
	char **keys_by_line;
	struct list_t **p_tab;
	int i, count_keys, j;
	struct list_t *list;
	
	// Verifica tabela
	if (table == NULL) { return NULL; }

	// Aloca memória para a tabela de todas as chaves
	all_keys = (char **)malloc((table->nElems + 1) * sizeof(char*));
	if (all_keys == NULL) { return NULL; }

	// Percorrendo a lista e passando os valores
	p_tab = table->tabela; // Tabela hash
	count_keys = 0; // indice **char keys: no final count_keys = table->nElems
	j = 0;
	for (i = 0; i < table->size; i++) {
		// Lista a considerar
		list = p_tab[i];
		// Chaves da lista considerada
		keys_by_line = list_get_keys(list);
		if (keys_by_line == NULL) { 
			// Caso haja erro apaga
			// O que já foi feito e introduzido no all_keys
			list_free_keys(all_keys);
			return NULL; 
		}
		// Copia as chaves que existe na linha da tabela hash
		while(keys_by_line[j] != NULL) {
			all_keys[count_keys] = strdup(keys_by_line[j]);
			if (all_keys[count_keys] == NULL) {
				list_free_keys(keys_by_line);
				list_free_keys(all_keys);
				return NULL;
			}
			count_keys++;
			j++;
		}
		list_free_keys(keys_by_line);
		// Faz reset a j
		j = 0;
	}

	// Ultimo elemento de all_keys tem de vir a NULL
	all_keys[count_keys] = NULL;

	return all_keys;
}


/* Liberta a memória alocada por table_get_keys().
 */
void table_free_keys(char **keys) {
	list_free_keys(keys);
}

/*
  Função auxiliar que faz uma inserção 
  um par {chave, valor} numa determinada tabela hash
  Devolve 0 (OK) ou -1 (out of memory, outros erros) 
*/
int insert(struct table_t *table, char *key, struct data_t *value) {
	struct entry_t *entry;
 	int index;
  	struct list_t *list;
  	int result;

  /* Verificar valores de entrada */
	if (table == NULL || key == NULL || value == NULL) { return ERROR; }

  /* Criar entry com par chave/valor */
	entry = entry_create(key, value);
	//Verifica entry
	if (entry == NULL) { return ERROR; }
  /* Executar hash para determinar onde inserir a entry na tabela */
	index = key_hash(key, table->size); 
	/* Linha da tabela a considerar */
	list = table->tabela[index];
	// Resultado de ter feito add do par {chave, valor}
	// na lista list
	result = list_add(list, entry);
	// Destroi o entry criado porque independentemente do resultado,
	// já nao vai ser usado nem preciso
	entry_destroy(entry);
  /* Devolve o resultado de list_add */
	return result;
}
