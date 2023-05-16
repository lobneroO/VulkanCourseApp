#!/bin/bash

echo "COMPILING VERTEX SHADER"
glslc shader.vert -o vert.spv

echo ""
echo "COMPILING FRAGMENT SHADER"
glslc shader.frag -o frag.spv

# wait for user input before closing
read