<?php

/**
 * Generic PID selector based on a CSV column.
 *
 * Options:
 *   filename  = CSV file path
 *   id_column = column containing the publication ID
 *   delimiter = CSV delimiter (default: ;)
 *   prefix    = optional prefix for generated PIDs (e.g. ir:)
 */
class csv_column extends get_pids {
    protected $rows_by_id = array();

    protected function get_pid_set() {
        $this->rows_by_id = array();

        $filename = isset($this->options['filename']) ? $this->options['filename'] : '';
        if($filename === '' || !is_readable($filename)) return array();

        $delimiter = isset($this->options['delimiter']) ? $this->options['delimiter'] : ';';
        $id_column = isset($this->options['id_column']) ? trim($this->options['id_column']) : 'pub_id';
        $prefix = isset($this->options['prefix']) ? $this->options['prefix'] : '';
        $handle = fopen($filename, 'r');

        if(!$handle) return array();

        $header = fgetcsv($handle, 0, $delimiter);

        if(!$header) {
            fclose($handle);
            return array();
        }

        $header[0] = preg_replace('/^\xEF\xBB\xBF/', '', $header[0]); // https://stackoverflow.com/questions/10290849/how-to-remove-multiple-utf-8-bom-sequences
        $header = array_map('trim', $header);

        if(!in_array($id_column, $header, true)) {
            fclose($handle);
            return array();
        }

        $pids = array();
        $seen = array();

        while(($values = fgetcsv($handle, 0, $delimiter)) !== false) {
            if(count($values) !== count($header)) continue;

            $row = array_combine($header, $values);
            if($row === false) continue;

            $raw_id = trim((string) $row[$id_column]);
            if($raw_id === '') continue;

            $key = $this->normalize_id($raw_id, $prefix);
            if($key === '') continue;

            if(!isset($this->rows_by_id[$key])) $this->rows_by_id[$key] = array();
            $this->rows_by_id[$key][] = $row;

            $pid = ($prefix !== '') ? $prefix . $key : $raw_id; // for example 12345 => psi:12345

            if(!isset($seen[$pid])) {
                $seen[$pid] = true;
                $pids[] = $pid;
            }
        }

        fclose($handle);

        return array_reverse($pids);
    }

    public function get_rows_for_pid($pid) {
        $prefix = isset($this->options['prefix']) ? $this->options['prefix'] : '';
        $key = $this->normalize_id($pid, $prefix);
        return isset($this->rows_by_id[$key]) ? $this->rows_by_id[$key]: array();
    }

    private function normalize_id($value, $prefix = '') {
        $value = trim((string) $value);

        if($prefix !== '' && strpos($value, $prefix) === 0) $value = substr($value, strlen($prefix));

        return trim($value);
    }
}