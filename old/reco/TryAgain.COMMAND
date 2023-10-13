#!/bin/sh
echo opening webpage
open -a "Google Chrome" http://localhost:8080

echo Starting Try Again
export PYTHONPATH=:$(dirname $0)/src/
cd $(dirname $0)/src
python3 -W ignore ./reco.py


echo Done
