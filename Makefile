CC = gcc
CFLAGS = -Wall -O2 -g

SRC_DIR = ./src
OBJ_DIR = ./obj
INCLUDE = -I./include

OBJS += $(OBJ_DIR)/utils.o
OBJS += $(OBJ_DIR)/list.o 
OBJS += $(OBJ_DIR)/queue.o 
OBJS += $(OBJ_DIR)/network.o 
OBJS += $(OBJ_DIR)/timer.o 
OBJS += $(OBJ_DIR)/packet.o
OBJS += $(OBJ_DIR)/connection.o 
OBJS += $(OBJ_DIR)/connection_list.o 
OBJS += $(OBJ_DIR)/worker_queue.o


all: $(OBJS) server client

# utils
$(OBJ_DIR)/utils.o: $(SRC_DIR)/utils.c
	$(CC) $(CFLAGS) -c $(SRC_DIR)/utils.c -o $(OBJ_DIR)/utils.o

### data structure ###

# list
$(OBJ_DIR)/list.o: $(SRC_DIR)/list.c
	$(CC) $(CFLAGS) -c $(SRC_DIR)/list.c -o $(OBJ_DIR)/list.o

# queue
$(OBJ_DIR)/queue.o: $(SRC_DIR)/queue.c
	$(CC) $(CFLAGS) -c $(SRC_DIR)/queue.c -o $(OBJ_DIR)/queue.o


### utils ###

# network
$(OBJ_DIR)/network.o: $(SRC_DIR)/network.c
	$(CC) $(CFLAGS) -c $(SRC_DIR)/network.c -o $(OBJ_DIR)/network.o

# timer
$(OBJ_DIR)/timer.o: $(SRC_DIR)/timer.c
	$(CC) $(CFLAGS) -c $(SRC_DIR)/timer.c -o $(OBJ_DIR)/timer.o

# packet
$(OBJ_DIR)/packet.o: $(SRC_DIR)/packet.c
	$(CC) $(CFLAGS) -c $(SRC_DIR)/packet.c -o $(OBJ_DIR)/packet.o

# connection
$(OBJ_DIR)/connection.o: $(SRC_DIR)/connection.c
	$(CC) $(CFLAGS) -c $(SRC_DIR)/connection.c -o $(OBJ_DIR)/connection.o

# connection_list
$(OBJ_DIR)/connection_list.o: $(SRC_DIR)/connection_list.c
	$(CC) $(CFLAGS) -c $(SRC_DIR)/connection_list.c -o $(OBJ_DIR)/connection_list.o

# worker_queue
$(OBJ_DIR)/worker_queue.o: $(SRC_DIR)/worker_queue.c
	$(CC) $(CFLAGS) -c $(SRC_DIR)/worker_queue.c -o $(OBJ_DIR)/worker_queue.o


### server & clietn ###
server: $(SRC_DIR)/server.c
	$(CC) $(CFLAGS) -o server $(SRC_DIR)/server.c $(OBJS)

client: $(SRC_DIR)/client.c
	$(CC) $(CFLAGS) -o client $(SRC_DIR)/client.c $(OBJS)

clean:
	rm -rf $(OBJS)
	rm -rf server
	rm -rf client