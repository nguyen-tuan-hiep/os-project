CC = gcc
CFLAGS = -Wall -lrt  # Thêm -lrt để sử dụng các hàm IPC

all: sender receiver

sender: sender.c
	$(CC) $(CFLAGS) -o sender sender.c

receiver: receiver.c
	$(CC) $(CFLAGS) -o receiver receiver.c

clean:
	rm -f sender receiver
