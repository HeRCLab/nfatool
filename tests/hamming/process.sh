#!/bin/bash

for i in *.anml loop; do
	/share/jbakos/data/code/nfatool/nfatool -i $i -c -f 21
done
