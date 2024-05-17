build: encode decode

test: encode decode
	./encode test.wav test.brainwire
	./decode test.brainwire test.copy.wav
	diff test.wav test.copy.wav

eval: encode decode
	./eval.sh

encode: encode.c wav.c wav.h
	gcc -Wall -std=c17 -pedantic -g -o encode encode.c wav.c

decode: decode.c wav.c wav.h
	gcc -Wall -std=c17 -pedantic -g -o decode decode.c wav.c

clean:
	mv test.wav temp.wav
	rm -f encode decode *.o test.*
	rm -rf data *.dSYM
	mv temp.wav test.wav
