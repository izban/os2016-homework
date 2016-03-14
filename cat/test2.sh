curl neerc.ifmo.ru/~os/blob.bin | ./cat > blob.bin && curl neerc.ifmo.ru/~os/blob.bin > blob2.bin && diff blob.bin blob2.bin && echo OK
