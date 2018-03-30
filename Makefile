build:
	gcc -lnsl server.c -o server
	gcc -lnsl client.c -o client
clean:
	rm -f client server
	rm -f *.log


