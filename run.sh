#!/bin/bash
make 

gol 10000 ./inputs/1k.pbm ./1k_out.pbm
diff  ./outputs/1k_verify_out.pbm ./1k_out.pbm > diff.out