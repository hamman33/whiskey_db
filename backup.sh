#!/usr/bin/bash
cp log.txt "./backups/log_$(date +"%Y-%m-%d %T").txt"
cp whiskey.db "./backups/whiskey_$(date +"%Y-%m-%d %T").db"

