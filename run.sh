for((c=1;c<30;c++))
do
Var1=$(date +%s%N)
./build/bin/client --ip 0.0.0.0 --port 50000 &
Var2=$(date +%s%N)
Var3="$((Var2-Var1))"
echo "$(($Var3/1000000))"
done
