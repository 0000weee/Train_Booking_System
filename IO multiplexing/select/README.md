gcc -o server_select server_select.c
gcc -o client client.c
./server_select
telnet 127.0.0.1 8080
