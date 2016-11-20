- Quando fazemos del key, costuma aparecer o erro
*** Error in `./table-client': double free or corruption (fasttop): 0x094cd0a8 ***
- Quando fazemos del key, sem que esta existe, duas vezes seguida então o erro em cima aparece
- Exemplo: se fizermos del a, update a, sem que este exista o erro em cima volta a aparecer

- O table client não está a reconhecer o size e quit

- Sequência de códigos
	put a a
	del a
	put a a
Erro: *** Error in `./table-client': malloc(): memory corruption (fast): 0x080c0069 ***
  