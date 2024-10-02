gcc -o server_poll server_poll.c
gcc -o client client.c
./server_poll
telnet 127.0.0.1 8080
