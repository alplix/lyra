# Bagimsiz test:   make standalone
# BOINC surumu:    make boinc BOINC_DIR=/yol/boinc   (libboinc_api + libboinc gerekir)
CC=cc
CFLAGS=-O3 -march=native -I/opt/homebrew/include
LDFLAGS=-L/opt/homebrew/lib -lgmp -lpthread -lpthread
standalone: symm27.c
	$(CC) $(CFLAGS) symm27.c -o symm27 $(LDFLAGS)
boinc: symm27.c
	$(CC) -O3 -DUSE_BOINC -I$(BOINC_DIR)/api -I$(BOINC_DIR)/lib -c symm27.c -o symm27.o
	c++ symm27.o -o symm27_boinc -L$(BOINC_DIR)/api -L$(BOINC_DIR)/lib -lboinc_api -lboinc -lgmp -lpthread
gpu: symm27_gpu.c symm27.c
	cc -O3 -march=native -I/opt/homebrew/include symm27_gpu.c -o symm27_gpu -L/opt/homebrew/lib -lgmp -lpthread -framework OpenCL
