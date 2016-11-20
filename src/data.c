/*
*		Grupo 12
* @author Daniel Santos 44887
* @author Luis Barros  47082
* @author Marcus Dias 44901
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/data.h"

/* Função que cria um novo elemento de dados data_t e reserva a memória
 * necessária, especificada pelo parâmetro size 
 */
struct data_t *data_create(int size) {
  // Verififica consistencia do parametro size
  if (size <= 0)
    return NULL;
  // Apontador data_t de memória dinamica
  struct data_t *p = (struct data_t*)malloc(sizeof(struct data_t));
  // Verifica a criação do apontador
  if (p == NULL)
    return NULL;
  // Alocar memória para o atributo p->data
  p->data = malloc(size);
  // Verifica a criação do apontador
  if (p->data == NULL) {
    free(p);
    return NULL;
  }
  //Inicializando atributo size
  p->datasize = size;
  return p;
}

/* Função idêntica à anterior, mas que inicializa os dados de acordo com
 * o parâmetro data.
 */
struct data_t *data_create2(int size, void *data) {
  // Verifica argumento size
  // Verifica apontador data
  if (size <= 0 || data == NULL)
    return NULL;
  // Apontador data_t de memória dinamica
  struct data_t *p = data_create(size);
  // Verifica a criação do apontador
  if (p == NULL) {
    return NULL;
  }
  // Copia o centeudo 
  memcpy(p->data, data, size);
  return p;
}

/* Função que destrói um bloco de dados e liberta toda a memória.
 */
void data_destroy(struct data_t *data) {
  // Libertando espaço na memoria dinamica
  if (data != NULL) {
    free(data->data);
    free(data);
  }

}

/* Função que duplica uma estrutura data_t.
 */
struct data_t *data_dup(struct data_t *data) {
  if (data == NULL || data->data == NULL)
    return NULL;
  // Tamanho do atributo datasize
  int size = data->datasize;
  // Apontador data_t de memoria dinamica
  // Usa data_create2 para devolver um aponatador
  // para memoria dinamica já com os dados copiados
  struct data_t *p = data_create2(size, data->data);

  return p;
}
