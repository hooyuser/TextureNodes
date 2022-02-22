HEADER=source/gui/all_node_headers.h
echo "#pragma once" > $HEADER
for file in source/gui/nodes/*.h
do
    echo "#include \"nodes/${file##*/}\"" >> $HEADER
done