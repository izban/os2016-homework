port=`cat port`

for x in $(pidof ./netsh); do kill $x; done
for x in $(lsof -n -i | grep ":$port" | perl -pe "s/\b(\w+)\b\W*\b(\w+)\b.*/\2/"); do kill $x; done
sleep 0.1

make clean
make
./netsh $port
sleep 0.1
#echo 'ls' | socat STDIO TCP:localhost:$port
#echo "echo -e 123 45 67 \n 123 456 67" | socat STDIO TCP:localhost:$port
#echo "echo 1 | echo 1" | socat STDIO TCP:localhost:$port
#echo "echo -e 123 45 67 \r\n 123 456 67\r\n | grep 56" | socat STDIO TCP:localhost:$port 
#echo "cat /proc/cpuinfo | grep 'model name' | sed -re 's/.*: (.*)/\1/' | uniq" | socat STDIO TCP:localhost:$port
##echo sh | socat STDIO TCP:localhost:$port
