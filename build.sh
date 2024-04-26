#!/bin/bash

rm todo-cli
g++ -o todo-cli main.cpp http.cpp -lstdc++ 
