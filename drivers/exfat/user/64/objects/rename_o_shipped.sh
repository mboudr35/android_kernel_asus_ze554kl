list=`ls *.o`
for i in $list
do
	mv $i "$i""_shipped"
done
