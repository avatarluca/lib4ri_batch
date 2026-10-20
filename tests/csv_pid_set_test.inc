<?php

require_once __DIR__ . '/../includes/pid_set.inc';
require_once __DIR__ . '/../includes/csv_pid_set.inc';

function check_result($condition, $message) {
    if(!$condition) {
        fwrite(STDERR, "FAIL: " . $message . PHP_EOL);
        exit(1);
    }
}

$csv = __DIR__ . '/LMX.csv';

$selector = new csv_column(array(
    'filename' => $csv,
    'id_column' => 'pub_id',
    'delimiter' => ';',
    'prefix' => 'psi:',
));

echo "PID amount: " . $selector->get_pids_count() . PHP_EOL;
echo "PIDs:\n";
echo $selector->get_pids_list() . PHP_EOL;

check_result(
    $selector->get_pids_count() === 31,
    'Expectde 31 unique pub PIDs'
);

$rows = $selector->get_rows_for_pid('psi:7843');

check_result(
    count($rows) === 2,
    'Pub 7843 should have two author rows'
);

$author_ids = array();

foreach($rows as $row) {
    $author_ids[] = trim($row['psi_author_id']);
}

sort($author_ids);

check_result(
    $author_ids === array('2139', '4220'),
    'Pub 7843 shd contain auth 2139 and 4220'
);

$pids = array();

for($i = 0; $i < $selector->get_pids_count(); $i++) {
    $pids[] = $selector->get_pid();
}

check_result(
    count(array_unique($pids)) === 31,
    'PID list contains duplicates'
);

check_result(
    in_array('psi:7843', $pids, true),
    'PID psi:7843 is missing'
);

echo "All tests passed." . PHP_EOL;