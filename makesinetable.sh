#!/bin/bash
for i in $(seq 0 1023); do echo $i; done \
    | gawk '{if ((NR-1) % 8) printf ","; else printf "\n db "; printf "."int(127+128*sin(($1/1024)*2*3.14159265)+0.5) +0;  }'
echo ""