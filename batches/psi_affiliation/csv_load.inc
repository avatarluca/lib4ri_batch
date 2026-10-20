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

    /**
     * Load csv file with PID and author ID distinction (field names "pub_id" and "psi_author_id").
     *
     * @param string $file      Path to CSV file.
     * @param string $delimiter Field separator (semicolon as default).
     * @return array            List of rows in a 2D array with keys PID and author ID.
     */
    public static function by_pub_id($file, $delimiter = ';') {
        $grouped = array();

        foreach(self::rows($file, $delimiter) as $row) {
            if(!isset($row['pub_id']) || !isset($row['psi_author_id'])) continue;

            $pub_id = trim((string) $row['pub_id']);
            $author_id = trim((string) $row['psi_author_id']);

            $colon = strrpos($pub_id, ':');
            if($colon !== false) $pub_id = substr($pub_id, $colon + 1);

            if($pub_id === '' || $author_id === '') continue;

            $grouped[$pub_id][$author_id] = $row;
        }

        return $grouped;
    }
}


