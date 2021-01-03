build:
	gcc process_generator.c -o process_generator.out
	gcc clk.c -o clk.out
	gcc scheduler.c -o scheduler.out
	gcc process.c -o process.out
	gcc test_generator.c -o test_generator.out

clean:
	rm -f *.out log.txt pref.txt  # processes.txt

all: clean build

run:
	./process_generator.out

kill:
	ipcrm -a
	killall -e clk.out
	killall -e scheduler.out
	killall -e process_generator.out
	killall -e process.out
	killall -e make
