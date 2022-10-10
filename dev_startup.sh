#!/usr/bin/bash
pkill -9 python
source venv/bin/activate 
export FLASK_APP=app.py
export FLASK_ENV=development
flask run
