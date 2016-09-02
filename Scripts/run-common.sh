# (C) 2016 University of Bristol. See License.txt


run_player() {
    port=$((RANDOM%10000+10000))
    >&2 echo Port $port
    bin=$1
    shift
    if test $bin = Player-Online.x; then
	params="$* -pn $port -h localhost"
    else
	params="$port localhost $*"
    fi
    if test $bin = Player-KeyGen.x -a ! -e Player-Data/Params-Data; then
	./Setup.x $players $size 40
    fi
    >&2 echo Parameters $params
    $SPDZROOT/Server.x $players $port &
    rem=$(($players - 2))
    for i in $(seq 0 $rem); do
      echo "trying with player $i"
      $prefix $SPDZROOT/$bin $i $params 2>&1 | tee $SPDZROOT/logs/$i &
    done
    last_player=$(($players - 1))
    $prefix $SPDZROOT/$bin $last_player $params > $SPDZROOT/logs/$last_player 2>&1 || return 1
}

killall Player-Online.x Server.x
sleep 0.5

#mkdir /dev/shm/Player-Data

players=${PLAYERS:-2}

#. Scripts/setup.sh

mkdir logs
