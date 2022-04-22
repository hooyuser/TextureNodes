HEADER=source/gui/all_node_headers.h
echo "#pragma once" > $HEADER
for file in source/gui/nodes/*.h
do
    echo "#include \"nodes/${file##*/}\""
    echo "#include \"nodes/${file##*/}\"" >> $HEADER
done
echo
read -n 1 -s -r -p "Press any key to continue"