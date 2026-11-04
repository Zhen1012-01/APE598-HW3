CC ?= gcc
CFLAGS ?= -O3 -g -Werror
LDLIBS = -lm
all: sandpile.exe
sandpile.exe: sandpile.c stb_image_write.h
	$(CC) $(CFLAGS) sandpile.c -o $@ $(LDLIBS)
baseline/sandpile_baseline.exe: baseline/sandpile_baseline.c stb_image_write.h
	$(CC) $(CFLAGS) -I. $< -o $@ $(LDLIBS)
variants: baseline/sandpile_baseline.exe sandpile_batch.exe sandpile_queue.exe sandpile_forward.exe
sandpile_batch.exe: sandpile_batch_scan.c stb_image_write.h
	$(CC) $(CFLAGS) $< -o $@ $(LDLIBS)
sandpile_queue.exe: sandpile_queue.c stb_image_write.h
	$(CC) $(CFLAGS) $< -o $@ $(LDLIBS)
sandpile_forward.exe: sandpile_forward.c stb_image_write.h
	$(CC) $(CFLAGS) $< -o $@ $(LDLIBS)
test: all baseline/sandpile_baseline.exe
	python3 tests/test_correctness.py
sanitize:
	$(CC) -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer sandpile.c -o tests/sandpile_sanitize.exe $(LDLIBS)
	ASAN_OPTIONS=detect_leaks=1 tests/sandpile_sanitize.exe 201
clean:
	rm -f sandpile.exe sandpile_batch.exe sandpile_queue.exe sandpile_forward.exe baseline/*.exe tests/*.exe identity.png
.PHONY: all variants test sanitize clean
