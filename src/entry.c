/*
*		Grupo 12
* @author Daniel Santos 44887
* @author Luis Barros  47082
* @author Marcus Dias 44901
*/

#include <stdlib.h>
#include <string.h>

#include "../include/data.h"
#include "../include/entry.h"

/* Função que cria um novo par {chave, valor} (isto é, que inicializa
 * a estrutura e aloca a memória necessária).
 */
struct entry_t *entry_create(char *key, struct data_t *data) {
  // Verifica consistencia de apontador key
  // verifica consistencia do apontador data
  if (key == NULL || data == NULL)
    return NULL;
  // Criar novo apontador para memória dinamica
  struct entry_t *p = (struct entry_t *)malloc(sizeof(struct entry_t));
  // Verifica apontador p
  if (p == NULL)
    return NULL;
  // Reservar memoria dinamica para receber a key
  // Copiar key
  p->key = strdup(key);
  if (p->key == NULL) {
    free(p);
    return NULL;
  }
  // Copia data
  p->value = data_dup(data);
  if (p->value == NULL)
    free(p);

  return p;
}

/* Função que destrói um par {chave-valor} e liberta toda a memória.
 */
void entry_destroy(struct entry_t *entry) {
  if (entry != NULL) {
    free(entry->key);
    data_destroy(entry->value);
    free(entry);
  }
}

/* Função que duplica um par {chave, valor}.
 */
struct entry_t *entry_dup(struct entry_t *entry) {
  // Verifica apontador entry
  if (entry == NULL)
    return NULL;
  // Cria um novo entry_t
  // com os atributos copiados de entry
  return entry_create(entry->key, entry->value);  
}
