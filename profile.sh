#!/bin/bash
rm callgrind.out*
rm callgrind.anno*
valgrind --tool=callgrind --instr-atstart=no bin/yugen
callgrind_annotate callgrind.out.* > callgrind.anno
callgrind_annotate --inclusive=yes callgrind.out.* > callgrind.anno.inc
callgrind_annotate --auto=yes callgrind.out.* > callgrind.anno.src
callgrind_annotate --tree=both callgrind.out.* > callgrind.anno.tree
callgrind_annotate --inclusive=yes --auto=yes callgrind.out.* > callgrind.anno.inc.src
callgrind_annotate --inclusive=yes --tree=both callgrind.out.* > callgrind.anno.inc.tree
callgrind_annotate --auto=yes --tree=both callgrind.out.* > callgrind.anno.src.tree
callgrind_annotate --inclusive=yes --auto=yes --tree=both callgrind.out.* > callgrind.anno.inc.src.tree
