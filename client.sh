#!/bin/bash
if [ $1 = "get" ]; then
  ./get $2 $3
elif [ $1 = "put" ]; then
  ./put $2 $3
else
  echo "Wrong command $1"
fi;
