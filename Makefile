SRC = .
OBJ = obj
WALL = -Wall
CLIENT = bin/client
OBJECT_CLIENT = $(OBJ)/client.o $(OBJ)/client_func.o
OBJECT_COMMON = $(OBJ)/common.o


.PHONY: all
all: client

client: $(OBJECT_CLIENT) $(OBJECT_COMMON)
	gcc $^ -o $(CLIENT) $(WALL)

$(OBJECT_COMMON): $(SRC)/common.h
$(OBJECT_CLIENT): $(SRC)/client.h

$(OBJ)/%.o: $(SRC)/%.c
	gcc -c $< -o $@ $(WALL)

.PHONY: clean
clean:
	rm -f $(OBJ)/*.o
	rm -f $(CLIENT)