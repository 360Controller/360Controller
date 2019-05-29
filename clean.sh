#!/bin/bash
for i in Install360Controller; do
    rm -rf $i/build
done
rm -rf build
echo "Deleted build products."
