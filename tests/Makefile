CFLAGS = -std=c11 -Wall -Wextra -Werror -I../include -I../bootloader
test: test_modbus_core test_sha256
	./test_modbus_core
	./test_sha256
test_modbus_core: test_modbus_core.c ../src/modbus_core.c
	$(CC) $(CFLAGS) -o $@ $^
test_sha256: test_sha256.c ../bootloader/sha256.c
	$(CC) $(CFLAGS) -o $@ $^
clean:
	rm -f test_modbus_core test_sha256
