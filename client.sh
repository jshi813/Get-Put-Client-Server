#!/bin/bash
if [ $1 = "get" ]; then
  ./clientGet $2 $3
elif [ $1 = "put" ]; then
  ./clientPut $2 $3
else
  echo "Wrong command $1"
fi;
