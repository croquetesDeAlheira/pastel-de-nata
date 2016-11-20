####################################
#		Grupo 12                   #
# @author Daniel Santos 44887	   #
# @author Luis Barros  47082	   #
# @author Marcus Dias 44901	       #
####################################

BIN = bin
INC = include
OBJ = obj
SRC = src
CFLAGS = -I include  

#OBJFILES_DATA = $(OBJ)/data.o $(OBJ)/entry.o $(OBJ)/list.o $(OBJ)/test_data.o
#OBJFILES_ENTRY = $(OBJ)/data.o $(OBJ)/entry.o $(OBJ)/list.o $(OBJ)/test_entry.o
#OBJFILES_LIST = $(OBJ)/data.o $(OBJ)/entry.o $(OBJ)/list.o $(OBJ)/test_list.o
#OBJFILES_TABLE = $(OBJ)/data.o $(OBJ)/entry.o $(OBJ)/list.o $(OBJ)/table.o $(OBJ)/test_table.o 
#OBJFILES_MESSAGE = $(OBJ)/data.o $(OBJ)/entry.o $(OBJ)/list.o $(OBJ)/message.o $(OBJ)/test_message.o
OBJFILES_CLIENT = $(OBJ)/data.o $(OBJ)/entry.o $(OBJ)/list.o $(OBJ)/table.o $(OBJ)/message.o $(OBJ)/network_client.o $(OBJ)/table-client.o $(OBJ)/client_stub.o 
OBJFILES_SERVER = $(OBJ)/data.o $(OBJ)/entry.o $(OBJ)/list.o $(OBJ)/message.o $(OBJ)/table.o $(OBJ)/table-server.o $(OBJ)/table_skel.o

all: table-server table-client

table-client: $(OBJFILES_CLIENT)
	gcc -o table-client $(OBJFILES_CLIENT)
	
table-server: $(OBJFILES_SERVER)
	gcc -o table-server $(OBJFILES_SERVER)
	
#test_message: $(OBJFILES_MESSAGE)
#	gcc -o test_message $(OBJFILES_MESSAGE)

#test_table: $(OBJFILES_TABLE)
#	gcc -o test_table $(OBJFILES_TABLE)

#test_list: $(OBJFILES_LIST)
#	gcc -o test_list $(OBJFILES_LIST)

#test_entry: $(OBJFILES_ENTRY)
#	gcc -o test_entry $(OBJFILES_ENTRY)

#test_data: $(OBJFILES_DATA)
#	gcc -o test_data $(OBJFILES_DATA)

# Ficheiros Fase_3

$(OBJ)/client_stub.o: $(SRC)/client_stub.c $(INC)/client_stub-private.h $(INC)/message-private.h $(INC)/network_client-private.h
	gcc -c $(SRC)/client_stub.c -o $(OBJ)/client_stub.o

# Ficheiros .o Fase_2 2a parte
$(OBJ)/table-client.o: $(SRC)/table-client.c $(INC)/network_client-private.h $(INC)/client_stub-private.h
	gcc -c $(SRC)/table-client.c -o $(OBJ)/table-client.o

$(OBJ)/network_client.o: $(SRC)/network_client.c $(INC)/network_client-private.h 
	gcc -c $(SRC)/network_client.c -o $(OBJ)/network_client.o

$(OBJ)/table-server.o: $(SRC)/table-server.c $(INC)/inet.h $(INC)/table-private.h $(INC)/message-private.h
	gcc -c $(SRC)/table-server.c -o $(OBJ)/table-server.o

$(OBJ)/table_skel.o: $(SRC)/table_skel.c $(INC)/table_skel.h
	gcc -c $(SRC)/table_skel.c -o $(OBJ)/table_skel.o

# Ficheiros .o de teste da Fase 2 1a parte
#$(OBJ)/test_message.o: test_message.c $(INC)/list-private.h $(INC)/message-private.h 
#	gcc $(CFLAGS) -c test_message.c -o $(OBJ)/test_message.o

#$(OBJ)/test_table.o: test_table.c $(INC)/data.h $(INC)/entry.h $(INC)/table-private.h 
#	gcc $(CFLAGS) -c test_table.c -o $(OBJ)/test_table.o
	

# Ficheiros .o Fase_2 1a parte
$(OBJ)/message.o: $(SRC)/message.c $(INC)/message.h $(INC)/message-private.h
	gcc -c $(SRC)/message.c -o $(OBJ)/message.o

$(OBJ)/table.o: $(SRC)/table.c $(INC)/table.h $(INC)/table-private.h
	gcc -c $(SRC)/table.c -o $(OBJ)/table.o


# Ficheiros .o de teste da Fase_1
#$(OBJ)/test_list.o: test_list.c $(INC)/list-private.h
#	gcc $(CFLAGS) -c test_list.c -o $(OBJ)/test_list.o	

#$(OBJ)/test_entry.o: test_entry.c $(INC)/entry.h $(INC)/data.h
#	gcc $(CFLAGS) -c test_entry.c -o $(OBJ)/test_entry.o
	
#$(OBJ)/test_data.o: test_data.c $(INC)/data.h
#	gcc $(CF$AGS) -c test_data.c -o $(OBJ)/test_data.o


# Ficheiros .o da Fase_1 
$(OBJ)/list.o: $(SRC)/list.c $(INC)/list.h $(INC)/list-private.h $(INC)/entry.h
	gcc -c $(SRC)/list.c -o $(OBJ)/list.o
	
$(OBJ)/entry.o: $(SRC)/entry.c $(INC)/entry.h $(INC)/data.h
	gcc -c $(SRC)/entry.c -o $(OBJ)/entry.o
	
$(OBJ)/data.o: $(SRC)/data.c $(INC)/data.h
	gcc -c $(SRC)/data.c -o $(OBJ)/data.o
			
clean:
	rm -f obj/*.o
	rm -f table-server table-client
	rm -f *~
	
.PHONY: clean
