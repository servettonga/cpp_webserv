NAME = webserv

SRCS = $(shell find ./srcs -type f -name *.cpp)
OBJS = $(SRCS:.cpp=.o)

CXX = clang++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98

all: $(NAME)

%.o: %.cpp
	@$(CXX) $(CXXFLAGS) -c $< -o $@
	@printf "\033[0;33mGenerating %-38.38s\r" $@

$(NAME): $(OBJS)
	@printf "\n\033[0;32mCompiling $(NAME)...\033[0m\n"
	@$(CXX) $(CXXFLAGS) -o $(NAME) $(OBJS)
	@printf "\033[0;32mDone!\033[0m\n"

clean:
	@rm -f $(OBJS)

fclean: clean
	@rm -f $(NAME)

re: fclean all

run: all
	@printf "\033[0;34mRunning $(NAME)...\033[0m\n"
	./$(NAME)

test-init:
	@chmod +x ./tests/test_server.sh

test: webserv test-init
	@printf "\033[0;34mRunning server tests...\033[0m\n"
	@./$(NAME) & \
	SERVER_PID=$$!; \
	sleep 1; \
	./tests/test_server.sh; \
	TEST_STATUS=$$?; \
	kill $$SERVER_PID; \
	exit $$TEST_STATUS

.PHONY: all clean fclean re test test-init

.SECONDARY: $(OBJS)