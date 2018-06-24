#!/bin/sh

rm -f website-in-a-box.tgz

rm -f manifest
touch manifest
find . -follow -name \*.py | grep -v OLD | grep -v -- "-system-image" >> manifest
find . -follow -name \*.sql | grep -v -- "-system-image" >> manifest
ls *.sh >> manifest
ls static/* | grep -v ~ >> manifest
echo "Dockerfile" >> manifest

tar -hcvzf website-in-a-box.tgz -T manifest

rm -f manifest
