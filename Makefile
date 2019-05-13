target=genericTcpServer

${target}: *.cpp *.h
	g++ *.cpp -pthread -o $@

clean:
	rm -f ${target}

