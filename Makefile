NAME = webserv

SRCS = $(shell find ./srcs -type f -name *.cpp)
OBJS = $(SRCS:.cpp=.o)

CXX = clang++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(NAME) $(OBJS)

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all

run: all
	./$(NAME)

.PHONY: all clean fclean re

.SECONDARY: $(OBJS)