/*Possiveis funcoes a usar pelo table-server.c*/

/*
Pede atualização de estado do server
Retorna 0 em caso de sucesso e -1 em caso de insucesso
Recolhe todas as chaves existentes na tabela servidor primario
Por cada chave, pede o seu valor e executa 
a operacao PUT do respetivo par {chave, valor}
na tabela do servidor secundario
*/
int update_state(struct server_t *server) {
  // Argumentos
  int index;
  char** keys;
  char* key;
  struct data_t* value;
  char* all = "!";
  struct message_t *msg_out, *msg_all_keys, *msg_get, *msg_put;

  // Inicializa mensagem
  msg_out = (struct message_t*)malloc(sizeof(struct message_t));
  // Verifica mensagem
  if (msg_out == NULL) {return ERROR;}

  // Cria a mensagem pedindo todas as keys
  msg_out->opcode = OC_GET;
  msg_out->c_type = CT_KEY;
  msg_out->content.key = strdup(all);
  // Envia a msg e recebe a resposta
  msg_all_keys = invoke(msg_out);

  // Liberta memoria
  free_message(msg_out);

  // Testa mensagem de resposta
  if (msg_all_keys == NULL) {return ERROR;}

  // Todas as chaves da tabela primario
  keys = msg_all_keys->content.keys;

  if (keys[0] == NULL) {
    free_message(msg_all_keys);
    return OK;
  }

  // Elabora ciclo
  // Corre as chaves
  // Para cada chave faz o get do valor associado
  // Para cada par {chave, valor} envia msg de PUT
  // para servidor secundario
  index = 0;
  while(keys[index] != NULL) {
    key = keys[index];
    // Por cada chave pede o respetivo valor associado
    
    // Prepara a msg GET
    msg_out = (struct message_t*)malloc(sizeof(struct message_t));
    if (msg_out == NULL) {
      free_message(msg_all_keys);
      return ERROR;
    }
    
    msg_out->opcode = OC_GET;
    msg_out->c_type = CT_KEY;
    msg_out->content.key = strdup(key);
    // Envia amsg
    msg_get = invoke(msg_out);
    //  Liberta memoria
    free_message(msg_out);
    // Testa a msg
    if (msg_get == NULL) {
      free_message(msg_all_keys);
      return ERROR;
    }

    // Obtem valor
    value = msg_get->content.data;

    // Msg com pedido PUT
    msg_out = (struct message_t*)malloc(sizeof(struct message_t));
    if (msg_out == NULL) {
      free_message(msg_get);
      free_message(msg_all_keys);
      return ERROR;
    }
    // Compoe a msg PUT
    msg_out->opcode = OC_PUT;
    msg_out->c_type = CT_ENTRY;
    msg_out->content.entry = entry_create(key, value);

    // ENVIA MSG A SERVIDOR SECUNDARIO
    // FUNCAO QUE TENTA DUAS VEZES
    msg_put = server_send_with_retry(server, msg_out);

    // Liberta memoria
    free_message(msg_out);

    // Testa msg
    if (msg_put == NULL) {
      free_message(msg_get);
      free_message(msg_all_keys);
      return ERROR;
    }

    // Testa resultado da operacao PUT
    if (msg_put->content.result == -1) {
      free_message(msg_put);
      free_message(msg_get);
      free_message(msg_all_keys);
      return ERROR;
    }

    // Liberta memoria
    free_message(msg_put);
    free_message(msg_get);

    // Atualiza index 
    index++;
  }

  // Liberta msgs que contem todas as keys
  free_message(msg_all_keys);

  // Correu tudo bem envia a confirmacao
  return OK;  
}

/*
Tenta duas vezes enviar uma mensagem
Caso seja bem sucedido retorna mensagem de resposta
Caso contrário retorna NULL
*/
struct message_t* server_send_with_retry (struct server_t *server, struct message_t *msg_out) {
  struct message_t* msg_in;

  // Testa argumentos
  if (server == NULL) {return NULL;}

  // 1st attempt
  msg_in = network_send_receive(server, msg_out);

  // Testa primeira tentativa
  if (msg_in == NULL) {
    // Retry time
    sleep(RETRY_TIME);
    // 2nd attempt
    msg_in = network_send_receive(server, msg_out);

    // Testa segunda tentativa
    if (msg_in == NULL) {return NULL;}
  }

  return msg_in;
}