target=genericTcpServer

${target}: *.c *.h
	gcc *.c -std=c99 -pthread -o $@

clean:
	rm -f ${target}

