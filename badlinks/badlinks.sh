for file in $(find -L $1); do
	now=$(date +%s)
	file_modification=$(stat -c %Y $file)
	let dif="now-file_modification"
	let week=60*60*24*7 
	if [[ -L $file && $dif -gt $week && ! -f $(readlink $file) ]]
	then
		echo $file
	fi
done
