let port=`cat port`
(echo "cat"; echo "stdin") | socat STDIO TCP:localhost:$port
(echo -n "l"; sleep 2; echo "s") | socat STDIO TCP:localhost:$port &
(echo -n "ca"; sleep 1; echo "t"; echo "123") | socat STDIO TCP:localhost:$port &
