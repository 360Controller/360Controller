#!/bin/bash
for i in Install360Controller; do
    rm -rf $i/build
done
rm -rf build
rm -rf Install360Controller/*.dmg
echo "Deleted build products."
