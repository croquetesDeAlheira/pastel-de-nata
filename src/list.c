/*
*		Grupo 12
* @author Daniel Santos 44887
* @author Luis Barros  47082
* @author Marcus Dias 44901
*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "../include/entry.h"
#include "../include/list.h"
#include "../include/list-private.h"

//constants
#define OK 0
#define ERROR -1
#define EQUALS 0
#define NOTHING_CHANGED 1

/* Cria uma nova lista. Em caso de erro, retorna NULL.
 */
struct list_t *list_create() {

  struct list_t *list = (struct list_t *)malloc(sizeof(struct list_t));
  // Verifica se foi criada
  if(list == NULL){
    return NULL;
  }
  // Inicializar os atributos
  list->head = NULL;
  list->size = 0;
  return list;
}

/* Elimina uma lista, libertando *toda* a memoria utilizada pela
 * lista.
 */
void list_destroy(struct list_t *list){
  if (list != NULL) {
    struct node_t *currentNode = list->head;
    struct node_t *nextNode = NULL;
    // Free nodes memory and links
    while(currentNode != NULL){
      // Save reference for next
      nextNode = currentNode->next;
      //delete node
      node_destroy(currentNode);
      list->size--;
      // Update currentNode
      currentNode = nextNode;
    }
    free(list); 
  }
}


/* Adiciona uma entry na lista. Como a lista deve ser ordenada, 
 * a nova entry deve ser colocada no local correto.
 * Retorna 0 (OK) ou -1 (erro)
 */
int list_add(struct list_t *list, struct entry_t *entry) {
  // Verifica parametros
  if (entry == NULL || list == NULL) {
    return ERROR;
  }
  // Verifica se a cabeça da lista está vazia,
  // se sim, será o primeiro par {chave, valor}
  if (list->head == NULL) {
    list->head = node_create(entry);
    if (list->head == NULL) {
      return ERROR;
    }
    list->size++;
    return OK;
  }
  // já existem pares, novo par terá que ser ordenado
  // Posiciona o node na ordem certa da lista
  // Novo node
  struct node_t *newNode = node_create(entry);
  if(newNode == NULL){
    return ERROR;
  }

  // Ordena a lista com o novo node
  int sortResult = sortList(list, newNode);

  if (sortResult == OK) 
    list->size++;

  return OK;
}

/* Elimina da lista um elemento com a chave key. 
 * Retorna 0 (OK) ou -1 (erro)
 */
int list_remove(struct list_t *list, char* key) {
  struct node_t *currentNode;
  struct node_t *toDelete;
  char *listKey;

  // Verifica parametros
  if (list == NULL || key == NULL) {
    return ERROR;
  }

  // Verifica o size
  if (list->size < 1) {
    return ERROR;
  }
    
  // Verifica a cabeça
  currentNode = list->head;
  listKey = currentNode->value->key;

  if (strcmp(listKey, key) == EQUALS) {
    list->head = currentNode->next;
    // Liberta a memoria 
    node_destroy(currentNode);
    // Actualiza tamanho da lista
    list->size--;

    return OK;
  }

  // Percorre a lista
  while (currentNode->next != NULL) {
    listKey = currentNode->next->value->key;
    if (strcmp(listKey, key) == EQUALS) {
      toDelete = currentNode->next;
      currentNode->next = toDelete->next;
      node_destroy(toDelete);
      list->size--;
      return OK;
    } 
    // Continua a percorrer a lista
    currentNode = currentNode->next;
  }
  // Percorreu toda a lista e nao encontrou
  // o par {chave, valor} a remover
  return ERROR;
}


/* Obtem um elemento da lista que corresponda à chave key. 
 * Retorna a referência do elemento na lista (ou seja, uma alteração
 * implica alterar o elemento na lista). 
 */
struct entry_t *list_get(struct list_t *list, char *key) {
  struct node_t *currentNode;
  char *listKey;

  if (list == NULL || key == NULL) {
    return NULL;
  }

  currentNode = list->head;

  while (currentNode != NULL) {
    listKey = currentNode->value->key;
    if (strcmp(listKey, key) == EQUALS) {
      return currentNode->value;
    }
    // Continua à procura
    currentNode = currentNode->next;
  }
  // Nao encontrou nenhum elemento 
  return NULL;
}

/* Retorna o tamanho (numero de elementos) da lista 
 * Retorna -1 em caso de erro.  */
int list_size(struct list_t *list) {
  if (list == NULL) {
    return ERROR;
  }
  return list->size;
}

/* Devolve um array de char * com a cópia de todas as keys da 
 * tabela, e um último elemento a NULL.
 */
char **list_get_keys(struct list_t *list) {
  // Variáveis
  int size;
  char **keyList;
  struct node_t *currentNode;
  int i;
  char *key;
  // Verifica lista
  if (list == NULL) {
    return NULL;
  }
    
  // Reserva memória dinamica para guargar apontadores de char
  // um para cada par {chave, valor}
  size = list->size;
  keyList = (char **) malloc(sizeof(char**) * (size + 1));
  // Verifica apontador
  if (keyList == NULL) {
    return NULL;
  }

  currentNode = list->head;

  // Ao mesmo tempo que percorre a lista
  // cada apontador de keyList vai reservar memóoria
  // e guardar o conteudo de cada par {chave, valor}
  for(i = 0; i < size; i++) {
    key = currentNode->value->key;
    // Reserva memória e copia conteudo
    keyList[i] = strdup(key);
    if (keyList[i] == NULL) {
      return NULL;
    }
      
    // Actualiza par {chave, valor}
    currentNode = currentNode->next;
  }

  // Ultimo elemento a NULL, 
  // como é dito no cabeçalho da função
  keyList[size] = NULL;
  return keyList;
}

/* Liberta a memoria reservada por list_get_keys.
 */
void list_free_keys(char **keys) {  
  int index;
  if (keys != NULL) {
    index = 0;
    while (keys[index] != NULL) {
    free(keys[index]);
    index++;
    }
    free(keys);
  }
}

/* Destroi uma estrutura node_t
 * libertando a memória necessária 
 */
void node_destroy(struct node_t *toDelete){
  // Verifica o apontador
  if (toDelete != NULL) {
    entry_destroy(toDelete->value);
    free(toDelete);
  }
}

/*
 * Cria uma estrutura a partir do
 * parametro passado
 * em caso de erro retorna NULL
 */
struct node_t *node_create(struct entry_t *value) {
  // Verifica os valores 
  if (value == NULL) {
    return NULL;
  }
  // Criar apontador 
  struct node_t *p = (struct node_t *)malloc(sizeof(struct node_t));
  if (p == NULL) {
    return NULL;
  }

  // Atributos do apontador
  p->value = entry_dup(value);
  if (p->value == NULL) {
    free(p);
    return NULL;
  }
  p->next = NULL;
  return p;
}

/*
 * Adiciona o novo par {chave, valor} à lista ordenadamente
 * Caso a chave já exista substitui o valor
 */
int sortList(struct list_t *list, struct node_t *newNode) {

  // Variávies
  int compareResult;
  struct node_t *currentNode = list->head;
  struct node_t *aux;

  // Verifica se a nova chave é maior que a do primeiro node
  compareResult = compare(newNode, currentNode);
  if (compareResult > 0) {
    // Novo par será a cabeça
    //new node aponta para head
    newNode->next = list->head;
    //list head aponta para newNode
    list->head = newNode;
    //size atualiza no add para so fazer um size++
    return OK;
  } 

  if (compareResult == 0) {
    //Caso seja igual
    aux = list->head;
    //faz update da cabeça
    newNode->next = list->head->next;
    list->head = newNode;
    //lisberta a cabeca antiga;
    node_destroy(aux);
    return NOTHING_CHANGED;
  } //se for menor entramos no while

  // Percorre a lista ordem decrescente
  while (currentNode->next != NULL) {
    //checka o next para termos ref para o next e para este
    compareResult = compare(newNode, currentNode->next);
    // Verifica se a chave é menor que o next
    if (compareResult < 0) {
      currentNode = currentNode->next;
    } else if (compareResult == 0) {
      //entao substitui
      aux = currentNode->next;
      newNode->next = currentNode->next->next;
      currentNode->next = newNode;
      //fazer free no node que ta solto.
      node_destroy(aux);
      return NOTHING_CHANGED;
    } else { 
      // Significa que o novo par tem de se situar entre ambos
      //new node aponta para o next pq eh maior do que este
      newNode->next = currentNode->next;
      //o current aponta para o new node, pq eh maior do q este
      currentNode->next = newNode;
      return OK;
    }
  }
  // Sai do while: Significa que o elemento é o ultimo
  currentNode->next = newNode;
  return OK;
}

int compare(const void *l1 , const void *l2) {
  struct node_t *node1 = (struct node_t *)l1;
  struct node_t *node2 = (struct node_t *)l2;

  //if Return value < 0 then it indicates node1 is less than node2.
  //if Return value > 0 then it indicates node1 is greater than node2.
  //if Return value = 0 then it indicates node1 is equals to node 2.
  return strcmp(node1->value->key, node2->value->key); 
}
