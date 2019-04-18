target=genericTcpServer

${target}: *.c *.h
	gcc *.c -pthread -o $@

clean:
	rm -f ${target}

