#!/bin/bash
for i in vsl_programs/*.vsl
do
	filename=$(basename $i)
	extension=${filename##*.}
	filename=${filename%.*}
	echo $filename
	bin/vslc < vsl_programs/$filename.vsl 2> test
	diff vsl_programs/$filename.tree test
done
