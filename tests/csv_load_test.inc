<?php

require __DIR__ . '/../batches/csv_load.inc';

$file = __DIR__ . '/LMX.csv';
$rows = csv_load::rows($file);
$count_rows = count($rows);

for($i = 0; $i < $count_rows; $i++) {
    echo PHP_EOL . $rows[$i]['pub_id'] . PHP_EOL;
    print_r($rows[$i]);
}