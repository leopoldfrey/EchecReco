#!/bin/sh
echo opening webpage
open -a "Google Chrome" http://localhost:8080

echo Starting Transcripteur
export PYTHONPATH=:$(dirname $0)/src/
cd $(dirname $0)/src
python3 -W ignore ./reco-simple.py


echo Done
