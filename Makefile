target=gTcps

${target}: *.cpp *.h
	#g++ *.cpp -pthread -o $@
	mips-openwrt-linux-g++ *.cpp -pthread -o $@

clean:
	rm -f ${target}

