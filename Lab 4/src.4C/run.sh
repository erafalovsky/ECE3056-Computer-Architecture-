
######################################################################################
# This scripts runs all three traces
# You will need to uncomment the configurations that you want to run
# the results are stored in the ../results/ folder
######################################################################################



########## ---------------  C ---------------- ################



 #./sim -mode 2 -L2sizeKB 512 ../traces/lbm.mtr.gz     > ../results/B.S512K.lbm.res &
# ./sim -mode 2 -L2sizeKB 512 ../traces/bzip2.mtr.gz   > ../results/B.S512K.bzip2.res &
# ./sim -mode 2 -L2sizeKB 512 ../traces/mcf.mtr.gz     > ../results/B.S512K.mcf.res &
#
# ./sim -mode 2 -L2sizeKB 1024 ../traces/lbm.mtr.gz    > ../results/B.S1MB.lbm.res &
# ./sim -mode 2 -L2sizeKB 1024 ../traces/bzip2.mtr.gz  > ../results/B.S1MB.bzip2.res
# ./sim -mode 2 -L2sizeKB 1024 ../traces/mcf.mtr.gz    > ../results/B.S1MB.mcf.res
./sim -mode 3 -L2sizeKB 512 ../traces/bzip2.mtr.gz  > ../results/C.bzip2.res &
./sim -mode 3 -L2sizeKB 512 ../traces/lbm.mtr.gz  > ../results/C.lbm.res &
./sim -mode 3 -L2sizeKB 512 ../traces/mcf.mtr.gz  > ../results/C.mcf.res &
./sim -mode 3 -L2sizeKB 512 ../traces/libq.mtr.gz  > ../results/C.libq.res &



echo "All Done. Check the .res file in ../results directory";
