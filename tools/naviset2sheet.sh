#!/bin/bash
DATA=`cat`
IMEI=$(echo $DATA | jq -r .io.imei)
RECVTIME=$(echo $DATA | jq -r .io.recvtime)
for row in $(echo $DATA | jq -c .payload.naviset[]); do
    row=$(echo $row | jq --arg k1 'imei' --arg v1 $IMEI --arg k2 'recvtime' --arg v2 $RECVTIME '.[$k1]=$v1 | .[$k2]=$v2')
    echo ${row} | ./js2sheet -e andrei.i.ivanov@commandus.com -s 1iDg77CjmqyxWyuFXZHat946NeAEtnhL6ipKtTZzF-mo -t Naviset
done
