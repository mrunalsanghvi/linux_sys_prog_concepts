for((c=1;c<150;c++))
do
#Var1=$(date +%s%N)
#./build/bin/client --ip 0.0.0.0 --port 50000 &
time ruby src/client.rb 3 >/dev/null &
#echo $Var
#Var2=$(date +%s%N)
#Var3="$((Var2-Var1))"
#echo "$(($Var3/1000000))"
done
