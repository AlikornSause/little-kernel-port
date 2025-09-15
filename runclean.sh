#!/bin/bash
make clean PROJECT=qinf21pro
make qinf21pro -j$(nproc) PROJECT=qinf21pro