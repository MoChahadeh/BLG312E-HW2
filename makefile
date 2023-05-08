multi-threading: multi-threading.out
	gcc -o multi-threading.out ./multi-threading/main.c -pthread
	./multi-threading.out

multi-processing: multi-processing.out
	gcc -o multi-processing.out ./multi-processing/main.c
	./multi-processing.out

multi-threading.out: multi-threading/main.c
	gcc -o multi-threading.out ./multi-threading/main.c -pthread

multi-processing.out: multi-processing/main.c
	gcc -o multi-processing.out ./multi-processing/main.c -pthread

clean:
	rm -f *.out