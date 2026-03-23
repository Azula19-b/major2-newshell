makeproject: main.h main.c commands.c pipeline.c redirection.c signalControl.commands
	gcc main.c commands.c pipeline.c redirection.c signalControl.c -Wall -o newshell

clean:
	rm newshell