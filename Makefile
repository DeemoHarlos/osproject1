kernel: kernel_files
	sudo cp kernel_files/* -rf /usr/src/linux-5.3/

main: main.c
	gcc main.c -o main

debug: main.c
	gcc main.c -o main -D DEBUG -g