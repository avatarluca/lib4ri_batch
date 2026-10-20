<?php

/**
 * CSV loader which reads a CSV file and returns each data row as an
 * assoc array using the header cells as keys.
 */
class csv_load {
    /**
     * @param string $file      Path to CSV file.
     * @param string $delimiter Field separator (semicolon as default).
     * @return array            List of rows.
     */
    public static function rows($file, $delimiter = ';') {
        static $cache = array();
        if(isset($cache[$file])) return $cache[$file];

        $rows = array();
        $handle = fopen($file, 'r');
        if(!$handle) return $cache[$file] = $rows;

        $header = fgetcsv($handle, 0, $delimiter); // https://www.php.net/manual/en/function.fgetcsv.php
        if(!$header) {
            fclose($handle);
            return $cache[$file] = $rows;
        }
        $header[0] = preg_replace('/^\xEF\xBB\xBF/', '', $header[0]); // https://stackoverflow.com/questions/10290849/how-to-remove-multiple-utf-8-bom-sequences

        while(($row = fgetcsv($handle, 0, $delimiter)) !== false) $rows[] = array_combine($header, $row);

        fclose($handle);

        return $cache[$file] = $rows;
    }
}


