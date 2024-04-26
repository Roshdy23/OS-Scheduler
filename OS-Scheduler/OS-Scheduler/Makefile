build:
	gcc `pkg-config --cflags gtk+-3.0` -o GUI.out GUI.c `pkg-config --libs gtk+-3.0`
	gcc `pkg-config --cflags gtk+-3.0` -o image_generator.out image_generator.c `pkg-config --libs gtk+-3.0`
	gcc process_generator.c -o process_generator.out
	gcc clk.c -o clk.out
	gcc scheduler.c -o scheduler.out 
	gcc process.c -o process.out
	gcc test_generator.c -o test_generator.out

clean:
	rm -f *.out  processes.txt

all: clean build

run:
	./GUI.out
