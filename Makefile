NAME		=	ircserv
CC			=	c++
FLAGS		=	-Wall -Wextra -Werror -std=c++98
INCLUDE		=	-I ./include/
SRC			=	./src/main.cpp ./src/Autentication.cpp ./src/Channel.cpp ./src/Client.cpp ./src/Commands.cpp ./src/Server.cpp
OBJ 		=  $(SRC:.cpp=.o)

all: $(NAME)

.cpp.o:
	$(CC) $(FLAGS) -c $< -o $@

$(NAME): $(OBJ) 
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(INCLUDE)

$(OBJ): %.o: %.cpp
	$(CC) $(CFLAGS) -o $@ -c $< $(INCLUDE)
	
clean:
	rm -rf $(OBJ)	
	
fclean: clean
	rm -rf 	$(NAME)

re: fclean all

.PHONY: all clean fclean re
