#!/bin/sh
cd ../proto
echo "Check proto files"
protoc --plugin=protoc-gen-pkt2="proto-pkt2" --proto_path=../proto --pkt2_out=pkt2 ../proto/example1.proto
echo OK
exit 0