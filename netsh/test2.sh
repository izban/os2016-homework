#for x in $(pidof ./netsh); do kill $x; done

let port=`cat port`
#make clean
#make
#./netsh $port
sleep 0.1
echo 'ls' | socat STDIO TCP:localhost:$port
echo "echo -e 123 45 67 \n 123 456 67" | socat STDIO TCP:localhost:$port
echo "echo 1 | echo 1" | socat STDIO TCP:localhost:$port
echo "echo -e 123 45 67 \r\n 123 456 67\r\n | grep 56" | socat STDIO TCP:localhost:$port 
echo "cat /proc/cpuinfo | grep 'model name' | sed -re 's/.*: (.*)/\1/' | uniq" | socat STDIO TCP:localhost:$port
(echo "sh"; echo "echo \"hello, I am shell\""; sleep 0.1; echo "cat"; echo "3") | socat STDIO TCP:localhost:$port

