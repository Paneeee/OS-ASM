cd source_code
rm -r result
mkdir result
make sched
make mem
make os
./os sched_0 > result/sched_0
./os sched_1 > result/sched_1
./mem input/proc/m0 > result/mem_0
./mem input/proc/m1 > result/mem_1
./os os_0 > result/os_0
./os os_1 > result/os_1
