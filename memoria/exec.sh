#!/bin/bash
DIRECTORY=obj
if [ ! -d "$DIRECTORY" ]; then
    mkdir $DIRECTORY
fi

#!/bin/bash
S_DIRECTORY=../shared/obj
if [ ! -d "$S_DIRECTORY" ]; then
    mkdir $S_DIRECTORY
fi

LOG_DIRECTORY=cfg
if [ ! -d "$LOG_DIRECTORY" ]; then
    mkdir $LOG_DIRECTORY
fi

FILE=memoria.out
make $FILE
if test -f "./$FILE"; then
    ./$FILE
fi