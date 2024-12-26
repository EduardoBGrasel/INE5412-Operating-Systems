GXX=g++

simplefs: shell.o fs.o disk.o
	$(GXX) shell.o fs.o disk.o -o simplefs -lsfml-graphics -lsfml-window -lsfml-system

shell.o: shell.cc
	$(GXX) -Wall shell.cc -c -o shell.o -g

fs.o: fs.cc fs.h
	$(GXX) -Wall fs.cc -c -o fs.o -g

disk.o: disk.cc disk.h
	$(GXX) -Wall disk.cc -c -o disk.o -g

clean:
	rm -f simplefs shell.o fs.o disk.o
