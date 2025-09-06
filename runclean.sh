#!/bin/bash
make clean PROJECT=k61v1_64_bsp
make k61v1_64_bsp -j$(nproc) PROJECT=k61v1_64_bsp

