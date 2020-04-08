
# Compilatore
CC= gcc
# Flag di compilazione
CFLAGS = -Wall -pedantic -g -std=c99

.PHONY: clean cleanall lib

# Librerie 
# Directory in cui si trovano le librerie
LIBDIR = ../lib
# Opzione di linking
LIBS = -L $(LIBDIR)

# Nome libreria progetto
LIBNAME1 = libWator.a

# Oggetti libreria $(LIBNAME1)
objects1 = watorlib.o util.o rules.o planet.o cells.o jobsrepo.o

DATA=planet.dat
CONF=wator.conf

setup:
	make clean
	make lib
	make wator
	make visualizer
	-rm -fr $(DATA) $(CONF) 
	cp DATA/$(DATA) .
	cp DATA/$(CONF) .
	make clean

# Creazione libreria 
lib:  $(objects1)
	-rm  -f $(LIBNAME1)
	-rm  -f $(LIBDIR)/$(LIBNAME1)
	ar -r $(LIBNAME1) $(objects1)
	cp $(LIBNAME1) $(LIBDIR)

# Make rule per gli altri .o del primo frammento
watorlib.o : wator.c wator.h
	$(CC) $(CFLAGS) -c $< -o $@

util.o : util.c util.h
	$(CC) $(CFLAGS) -c $< -o $@

rules.o : rules.c
	$(CC) $(CFLAGS) -c $< -o $@

planet.o : planet.c
	$(CC) $(CFLAGS) -c $< -o $@

cells.o : cells.c
	$(CC) $(CFLAGS) -c $< -o $@

# Target visualizer e wator
visualizer : visualizer.o
	$(CC) -pthread $^ -o $@ $(LIBS) -lWator

wator : wator.o
	$(CC) -pthread $^ -lm -o $@ $(LIBS) -lWator

# make rule per gli altri .o del secondo/terzo frammento
wator.o : wator_process.c
	$(CC) $(CFLAGS) -c -pthread $< -o $@

visualizer.o : visualizer.c
	$(CC) $(CFLAGS) -c $< -o $@

jobsrepo.o : jobsrepo.c
	$(CC) $(CFLAGS) -c $< -o $@

# Ripulisce  l'ambiente 
clean:
	find . -name "*.o" -print -exec rm {} \;
	find . -name "*.a" -print -exec rm {} \; 

# cancella i file temporanei e tutto quello che devo ricreare/copiare
cleanall: clean
	\rm -f wator visualizer 
	\rm -f wator_worker_?* visualizer_dump out.wator out*.check planet?.check __wator.log wator.check planet?.dat
	\rm -f *~
	\rm -f $(LIBDIR)/$(LIBNAME1) $(LIBNAME1)

