#!/bin/bash
make clean PROJECT=qinf21pro
make qinf21pro -j$(nproc) PROJECT=qinf21pro

mtk w lk_a build-qinf21pro/lk.img
mtk w lk_b build-qinf21pro/lk.img
