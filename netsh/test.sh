for x in $(pidof ./netsh); do kill $x; done

let port=1257
make clean
make
./netsh $port
sleep 0.1
echo 'ls' | socat STDIN TCP:localhost:$port
echo "echo -e 123 45 67 \n 123 456 67" | socat STDIN TCP:localhost:$port
echo "echo 1 | echo 1" | socat STDIN TCP:localhost:$port
#echo "echo -e 123 45 67 \r\n 123 456 67\r\n | grep 56" | socat STDIN TCP:localhost:$port 
#echo "cat /proc/cpuinfo | grep 'model name' | sed -re 's/.*: (.*)/\1/' | uniq" | socat STDIN TCP:localhost:$port
#echo sh | socat STDIN TCP:localhost:$port
#echo 'ls' | socat STDIN TCP:localhost:$port
