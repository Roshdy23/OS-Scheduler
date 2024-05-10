build:
	gcc `pkg-config --cflags gtk+-3.0` -o GUI.out GUI.c `pkg-config --libs gtk+-3.0`
	gcc `pkg-config --cflags gtk+-3.0` -o image_generator.out image_generator.c `pkg-config --libs gtk+-3.0`
	gcc process_generator.c -o process_generator.out -lm
	gcc clk.c -o clk.out -lm
	gcc scheduler.c -o scheduler.out -lm
	gcc process.c -o process.out -lm
	gcc test_generator.c -o test_generator.out -lm

clean:
	rm -f *.out  processes.txt

all: clean build

run:
	./GUI.out
