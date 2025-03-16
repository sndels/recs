#!/usr/bin/env sh

./scripts/ninja.sh $1 $2
err=$?

cd build

if [ "$err" -eq "0" ]; then
./$2
err=$?
fi

cd ..
