## Format of the files 'wator.conf' and 'planet.dat'

### wator.conf
This file must be formatted in the following way:

	sd x 
	sb y
	fb z

'sd x' means that if a shark doesn't eat any fish for x consecutive chronons, it will die.
'sb y' means that a shark will reproduce every 'y' chronons (if it's alive, of course).
'fb z' means that a fish will reproduce every 'z' chronons (again, only if it's alive).

Check the files 'wator.conf.1' and 'wator.conf.2' for an example.


### planet.dat 
This file must be formatted in the following way:
	
	nrows
	ncols
	X X ... X
 	X X ... X
 	. .     .
 	. .     .
 	. .     .
 	X X ... X

'nrows' is the number of rows of the matrix that represents the toroidal planet, while 'ncols' represents the number of columns. Each element 'X' of the matrix must be either 'W' (water), 'S' (shark') or 'F' (fish). 

Check the files 'planet1.dat', 'planet2.dat' and 'planet3.dat' for an example.
