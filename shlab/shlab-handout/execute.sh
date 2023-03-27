#! /bin/bash

mytest="test"
rtest="rtest"

make "$mytest$1" > my_ans.txt
make "$rtest$1" > correct_ans.txt
sed -i '1d' my_ans.txt			#删首行，无关的行
sed -i '1d' correct_ans.txt
echo "=======================test $1======================="
diff my_ans.txt correct_ans.txt > my_result.txt
if [[ ! -s "my_result.txt" ]]			#result是否为空
then
	echo "Success."
else
	cat my_result.txt
fi
