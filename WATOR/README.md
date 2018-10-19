
## Instructions (English)

To generate executable files, open the terminal and head to the 'src' directory, then run:

	make
	
	
To adjust the settings of the simulation, modify the files 'wator.conf' (which contains the parameters of the simulation) and 'planet.dat' (which contains the planet's configuration) contained in the 'DATA' directory. 
The 'DATA' directory also contains additional files such as 'wator.conf.1', 'wator.conf.2', 'planet1.dat', 'planet2.dat' and 'planet3.dat', whose purpose is to exemplify the correct formatting of the files 'wator.conf' and 'planet.dat' so that the program works properly.
	
	
With the terminal opened in the directory 'src', you can run the program using the following command:

	./wator planet_file [-n nwork] [-v chronon] [-f dumpfile]
	
('planet_file' is any file having the same formatting as 'planet1.dat', 'planet2.dat' and 'planet3.dat')


To run the script, execute:

	./watorscript planet_file [-f] [-s] [--help]
	
Additional informations on how to use the script can be obtained using the option '--help'



## Istruzioni (Italiano)

Per generare gli eseguibili dei programmi, posizionarsi con il terminale sulla directory 'src' ed eseguire:

	make
	
Per cambiare le impostazioni della simulazione, modificare i file 'wator.conf' (che contiene i parametri 
della simulazione) e 'planet.dat' (che contiene la configurazione del pianeta) contenuti
nella directory 'DATA'. Quest'ultima contiene anche i file 'wator.conf.1', 'wator.conf.2', 'planet1.dat',
'planet2.dat' e 'planet3.dat', che danno un'idea di come devono essere formattati i file 
'wator.conf' e 'planet.dat' affinché il programma funzioni correttamente.

Per eseguire il programma lanciare, sempre dalla directory 'src', il comando:

	./wator planet.dat [-n nwork] [-v chronon] [-f dumpfile]
	
(Al posto di planet.dat può essere specificato un qualsiasi file con lo stesso formato)

Per lanciare lo script, eseguire:

	./watorscript planet_file [-f] [-s] [--help]
	
Ulteriori informazioni sull'utilizzo dello script si possono avere specificando l'opzione '--help'

