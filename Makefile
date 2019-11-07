# NAME: Daniel Chen,    Winston Lau
# EMAIL: kim77chi@gmail.com, winstonlau99@gmail.com
# ID: 605006027, 504934155

CC = gcc
FLAGS = -Wall -Wextra

webserver: webserver.c
	$(CC) $(FLAGS) -o $(@) webserver.c
clean: 
	rm webserver