#!/bin/sh

g++ -ldl -lboost_thread test.cpp
g++ -shared -fPIC -ldl -o timehack.so timehack.cpp 

./a.out
echo
LD_PRELOAD=./timehack.so ./a.out
