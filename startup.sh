#!/usr/bin/bash
pkill -9 python
source venv/bin/activate 
export FLASK_APP=app.py
nohup python3 app.py > log.txt 2>&1 &
