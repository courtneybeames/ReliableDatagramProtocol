.phony all:
all: rdps rdpr

rdps: rdps.c
	gcc -o rdps rdps.c helperMethods.c

rdpr: rdpr.c
	gcc -o rdpr rdpr.c helperMethods.c

.PHONY clean:
clean:
	-rm rdps rdpr -f
