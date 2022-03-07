#!/bin/bash
# M: n_hosts 1 <= M <= 10
M=0
# N: n_players 8 <= N <= 12
N=0
# L: lucky_number 0 <= L <= 1000
L=0
# ./chiffon.sh -m 2 -n 9 -l 1000

# get the flags from bash command
while getopts "m:n:l:" flag; do
    case $flag in
    m) M=$OPTARG ;;
    n) N=$OPTARG ;;
    l) L=$OPTARG ;;
    esac
done

# 1. Prepare FIFO files for all M hosts
for i in $(seq 0 $M); do
    # if the tmp file does not exist, create one
    [ -p "fifo_$i.tmp" ] || mkfifo "fifo_$i.tmp"
    cmd="exec $(($i + 3))<> fifo_$i.tmp"
    eval $cmd
done

# 2. Generate C^N_8 combinations of N players and assign to hosts via FIFO.
# n_Cmb: number of combinations
n_Cmb=0
# Cmblist: the combination list array
Cmblist=()

function genCmb() {
    # 1: idx 2: num
    local i
    if [ $1 -eq 8 ]; then
        Cmblist[$n_Cmb]="${tmpCmb[0]} ${tmpCmb[1]} ${tmpCmb[2]} ${tmpCmb[3]} ${tmpCmb[4]} ${tmpCmb[5]} ${tmpCmb[6]} ${tmpCmb[7]}"
        ((n_Cmb++))
        return
    fi
    for ((i = $2; i <= $N; i++)); do
        tmpCmb[$1]=$i
        genCmb $(($1 + 1)) $(($i + 1))
    done
}

genCmb 0 1
rd_Cmb=0
wr_Cmb=0
PIDs=()

# init the hosts
for ((i = 1; i <= $M; i++)); do
    # ./host -m [host_id] -d [depth] -l [lucky_number]
    ./host -m $i -d 0 -l $L &
    PIDs[$i]=$!
done

# send data to each available host:
while [ $wr_Cmb -lt $M ] && [ $wr_Cmb -lt $n_Cmb ]; do
    # sleep 1
    echo ${Cmblist[$wr_Cmb]} >"fifo_$(($wr_Cmb + 1)).tmp"
    # echo "send to $(($wr_Cmb + 1))... "                   # TODO debug
    # echo "M: " $M                                         # TODO debug
    # echo ${Cmblist[$wr_Cmb]}                              # TODO debug
    ((wr_Cmb++))
done

player_ids=(0 1 2 3 4 5 6 7 8 9 10 11 12)
player_pts=(0 0 0 0 0 0 0 0 0 0 0 0 0)
player_rnks=(0 0 0 0 0 0 0 0 0 0 0 0 0)

# echo "Cmb" $i_Cmb $n_Cmb

# read from host and write to host
# echo $(( $finished && $i_Cmb < $n_Cmb ))

while [ $rd_Cmb -lt $n_Cmb ]; do
    # send data to each available host:
    # sleep 1                      # TODO debug
    # echo "sub_Cmb" $wr_Cmb $rd_Cmb $n_Cmb # TODO debug
    # read data
    line_cnt=0
    host_id=0
    for line_cnt in $(seq 0 8); do
        if [ $line_cnt -eq 0 ]; then
            # echo # TODO debug
            read host_id <"fifo_0.tmp"
            # echo "host: " $host_id # TODO debug
        else
            read player_id player_pt <"fifo_0.tmp"
            # player_pts[0]=0 # TODO debug
            ((player_pts[player_id] += player_pt))
            # echo " id: " $player_id # TODO debug
            # echo "  pt: " $player_pt # TODO debug
            # echo "  player_pt: " ${player_pts[$player_id]} # TODO debug
        fi
    done

    # write data
    if [ $wr_Cmb -lt $n_Cmb ]; then
        echo ${Cmblist[wr_Cmb]} >"fifo_$host_id.tmp"
        # echo ${Cmblist[wr_Cmb]} # TODO debug
        ((wr_Cmb++))
    fi
    ((rd_Cmb++))
done

# echo "step 2:" $wr_Cmb $rd_Cmb $n_Cmb #TODO debug

# 3. send all the operation if end of execution
for i in $(seq 1 $M); do
    echo "-1 -1 -1 -1 -1 -1 -1 -1" >"fifo_$i.tmp"
done

#4. sort the ans and print it (id ascending)
for ((i = N; i >= 2; i--)); do
    for ((j = 1; j < i; j++)); do
        if [ ${player_pts[j]} -lt ${player_pts[$((j + 1))]} ]; then
            tmp=${player_pts[j]}
            player_pts[$j]=${player_pts[$((j+1))]}
            player_pts[$((j+1))]=$tmp
            tmp=${player_ids[j]}
            player_ids[$j]=${player_ids[$((j+1))]}
            player_ids[$((j+1))]=$tmp
        fi
    done
done

# TODO debug
# echo "score:"
# for i in $(seq 1 $N); do
#     echo ${player_ids[i]} ${player_pts[i]}
# done
# echo ""

last_rnk=1
for i in $(seq 1 $N); do
    if [ ${player_pts[$i]} -lt ${player_pts[$last_rnk]} ]; then
        last_rnk=$i
    fi
    player_rnks[${player_ids[i]}]=$last_rnk
done

# print the rank
for i in $(seq 1 $N); do
    echo $i ${player_rnks[i]}
done

# 5.remove fifo
for i in $(seq 0 $M); do
    rm "fifo_$i.tmp"
done

# 6. wait for all forked process and exit
for i in $(seq 1 $M); do
    # echo "wait..." $i ${PIDs[$i]}
    wait ${PIDs[$i]}
    # echo "suc"
done
