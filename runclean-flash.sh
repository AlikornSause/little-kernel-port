#!/bin/bash
make clean PROJECT=k61v1_64_bsp
make k61v1_64_bsp -j$(nproc) PROJECT=k61v1_64_bsp

mtk w lk_a build-k61v1_64_bsp/lk.img
mtk w lk_b build-k61v1_64_bsp/lk.img
