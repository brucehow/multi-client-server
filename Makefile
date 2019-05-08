# Makefile for the CITS3002 2019 project running on the Mac
#
# Name: Bruce How (22242664)

PROGNAME = server
C99 = cc -std=c99 -Werror -Wall -pedantic
DEPENDENCIES = clients.c connection.c game.c memory.c server.c

install: $(DEPENDENCIES)
	@$(C99) -o $(PROGNAME) $(DEPENDENCIES)
	@echo "make: 'server' successfully built."

clean: $(DEPENDENCIES)
	@rm $(PROGNAME)
	@echo "make: 'server' has been removed."
