#!/bin/sh
echo "Check proto file ../proto/example1.proto"
protoc --plugin=protoc-gen-pkt2="./protoc-gen-pkt2" --proto_path=proto --pkt2_out=tests/pkt2 proto/example1.proto
exit 0
