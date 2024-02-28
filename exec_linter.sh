#!/bin/bash

clear
cpplint --recursive src/ include/ sample/
retval=$?
if [ $retval -eq 0 ]
then
    echo "linter check success!!"
    retval=0
else
    echo "linter check fail!!"
    exit $retval
fi

exit $retval