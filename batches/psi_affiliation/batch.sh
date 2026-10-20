#!/bin/bash

USER="Luca"

CSV="../../data/psi_affiliation/psi_affiliation.csv"

PIDSELECT=csv_column
PIDOPTIONS="filename=$CSV|id_column=pub_id|delimiter=;|prefix=psi:"

COMMAND=change_psi_affiliation
COMMANDOPTIONS="log_ds=false"

DRYRUN=true
if [[ ${1:-} = "dryfalse" ]]
then
    DRYRUN=false
fi

echo drush -u "$USER" -r /var/www/html dora-batch --pids="$PIDSELECT" --pids_options="$PIDOPTIONS" --command="$COMMAND" --command_options="$COMMANDOPTIONS" --dry="$DRYRUN"

drush -u "$USER" -r /var/www/html dora-batch --pids="$PIDSELECT" --pids_options="$PIDOPTIONS" --command="$COMMAND" --command_options="$COMMANDOPTIONS" --dry="$DRYRUN"